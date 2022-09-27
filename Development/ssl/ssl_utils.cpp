#include "ssl/ssl_utils.h"

#include <boost/algorithm/string.hpp> // for boost::split
#include <openssl/err.h>
#include <openssl/pem.h>

namespace ssl
{
	namespace experimental
	{
        typedef std::unique_ptr<ASN1_TIME, decltype(&ASN1_STRING_free)> ASN1_TIME_ptr;

        namespace details
        {
            // get common name from subject
            std::string common_name(X509* x509)
            {
                auto subject_name = X509_get_subject_name(x509);
                if (!subject_name)
                {
                    throw ssl_exception("failed to get subject: X509_get_subject_name failure: " + last_openssl_error());
                }
                auto name = X509_NAME_oneline(subject_name, NULL, 0);
                std::string subject(name);
                OPENSSL_free(name);

                // exmaple subject format
                // e.g. subject=/DC=AMWA Workshop/CN=GBDEVWIND-8GGX.workshop.nmos.tv
                const std::string common_name_prefix{ "CN=" };
                std::vector<std::string> tokens;
                boost::split(tokens, subject, boost::is_any_of("/"));
                auto found_common_name_token = std::find_if(tokens.begin(), tokens.end(), [&common_name_prefix](const std::string& token)
                    {
                        return std::string::npos != token.find(common_name_prefix);
                    });
                if (tokens.end() != found_common_name_token)
                {
                    return found_common_name_token->substr(common_name_prefix.length());
                }
                return "";
            }

            // get issuer name from issuer
            std::string issuer_name(X509* x509)
            {
                auto issuer_name = X509_get_issuer_name(x509);
                if (!issuer_name)
                {
                    throw ssl_exception("failed to get issuer: X509_get_issuer_name failure: " + last_openssl_error());
                }
                auto name = X509_NAME_oneline(issuer_name, NULL, 0);
                std::string issuer(name);
                OPENSSL_free(name);

                // e.g. issuer=/C=GB/ST=England/O=NMOS Testing Ltd/CN=ica.workshop.nmos.tv
                const std::string common_name_prefix{ "CN=" };
                std::vector<std::string> tokens;
                boost::split(tokens, issuer, boost::is_any_of("/"));
                auto found_common_name_token = std::find_if(tokens.begin(), tokens.end(), [&common_name_prefix](const std::string& token)
                    {
                        return std::string::npos != token.find(common_name_prefix);
                    });
                if (tokens.end() != found_common_name_token)
                {
                    return found_common_name_token->substr(common_name_prefix.length());
                }
                return "";
            }

            // get subject alternative names
            std::vector<std::string> subject_alt_names(X509* x509)
            {
                std::vector<std::string> sans;
                GENERAL_NAMES_ptr subject_alt_names((GENERAL_NAMES*)X509_get_ext_d2i(x509, NID_subject_alt_name, NULL, NULL), &GENERAL_NAMES_free);
                for (auto idx = 0; idx < sk_GENERAL_NAME_num(subject_alt_names.get()); idx++)
                {
                    auto gen = sk_GENERAL_NAME_value(subject_alt_names.get(), idx);
                    if (gen->type == GEN_URI || gen->type == GEN_DNS || gen->type == GEN_EMAIL)
                    {
                        auto asn1_str = gen->d.uniformResourceIdentifier;
#if (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
                        auto san = std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(asn1_str)), ASN1_STRING_length(asn1_str));
#else
                        auto san = std::string(reinterpret_cast<char*>(ASN1_STRING_data(asn1_str)), ASN1_STRING_length(asn1_str));
#endif
                        sans.push_back(san);
                    }
                    else
                    {
                        // hmm, not supporting other type of subject alt name
                    }
                }
                return sans;
            }

            // create POSIX
            // see https://stackoverflow.com/questions/10975542/asn1-time-to-time-t-conversion/47015958#47015958
            time_t posix_time(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t min, uint32_t sec)
            {
                if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || min > 59 || sec > 60)
                {
                    return -1;
                }

                // days upto months for non-leap years
                static const int32_t month_day[13] = { -1, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
                year -= 1900;
                // number of Februaries since 1900
                const auto year_for_leap = (month > 2) ? year + 1 : year;

                return sec + min * 60 + hour * 3600 + (month_day[month] + day - 1) * 86400 +
                    (year - 70) * 31536000 + ((year_for_leap - 69) / 4) * 86400 -
                    ((year_for_leap - 1) / 100) * 86400 + ((year_for_leap + 299) / 400) * 86400;
            }

            // convert ANS.1 to POSIX
            // see https://stackoverflow.com/questions/10975542/asn1-time-to-time-t-conversion/47015958#47015958
            time_t ASN1_TIME_to_posix_time(const ASN1_TIME* time)
            {
                if (!time) { return -1; }
                auto s = (const char*)time->data;
                if (!s) { return -1; }

                auto two_digits_to_uint = [&]() // nested function: gcc extension
                {
                    unsigned n = 10 * (*s++ - '0');
                    return n + (*s++ - '0');
                };

                uint32_t year;
                switch (time->type)
                {
                    // https://www.rfc-editor.org/rfc/rfc5280#section-4.1.2.5.1
                case V_ASN1_UTCTIME: // YYMMDDHHMMSSZ
                    year = two_digits_to_uint();
                    year += year < 50 ? 2000 : 1900;
                    break;
                    // https://www.rfc-editor.org/rfc/rfc5280#section-4.1.2.5.2
                case V_ASN1_GENERALIZEDTIME: // YYYYMMDDHHMMSSZ
                    year = 100 * two_digits_to_uint();
                    year += two_digits_to_uint();
                    break;
                default:
                    return -1; // error
                }
                auto month = two_digits_to_uint();
                auto day = two_digits_to_uint();
                auto hour = two_digits_to_uint();
                auto min = two_digits_to_uint();
                auto sec = two_digits_to_uint();
                if (*s != 'Z') { return -1; }
                // 99991231235959Z rfc 5280
                if (year == 9999 && month == 12 && day == 31 && hour == 23 && min == 59 && sec == 59) { return -1; }
                return posix_time(year, month, day, hour, min, sec);
            }
        }

        // get last openssl error
        std::string last_openssl_error()
        {
            char buffer[1024] = { 0 };
            ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
            return buffer;
        }

        // get certificate information, such as expire date, it is represented as the number of seconds from 1970-01-01T0:0:0Z as measured in UTC
        cert_info cert_information(const std::string& cert_data)
        {
            BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
            if ((size_t)BIO_write(bio.get(), cert_data.data(), (int)cert_data.size()) != cert_data.size())
            {
                throw ssl_exception("failed to load cert to bio: BIO_write failure: " + last_openssl_error());
            }

            X509_ptr x509(PEM_read_bio_X509_AUX(bio.get(), NULL, NULL, NULL), &X509_free);
            if (!x509)
            {
                throw ssl_exception("failed to load cert: PEM_read_bio_X509_AUX failure: " + last_openssl_error());
            }

            auto sans = details::subject_alt_names(x509.get());

            auto _common_name = details::common_name(x509.get());
            if (_common_name.empty())
            {
                throw ssl_exception("missing Common Name");
            }

            auto _issuer_name = details::issuer_name(x509.get());
            if (_issuer_name.empty())
            {
                throw ssl_exception("missing Issuer Common Name");
            }

            // X509_get_notAfter returns the time that the cert expires, in Abstract Syntax Notation
            // According to the openssl documentation, the returned value is an internal pointer which MUST NOT be freed
#if (OPENSSL_VERSION_NUMBER >= 0x1010000fL)
            auto not_before = X509_get0_notBefore(x509.get());
#else
            auto not_before = X509_get_notBefore(x509.get());
#endif
            if (!not_before)
            {
                throw ssl_exception("failed to get notBefore: X509_get0_notBefore failure: " + last_openssl_error());
            }
#if (OPENSSL_VERSION_NUMBER >= 0x1010000fL)
            auto not_after = X509_get0_notAfter(x509.get());
#else
            auto not_after = X509_get_notAfter(x509.get());
#endif
            if (!not_after)
            {
                throw ssl_exception("failed to get notAfter: X509_get0_notAfter failure: " + last_openssl_error());
            }
            time_t not_before_time;
            time_t not_after_time;
#if (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
            tm not_before_tm;
            if (!ASN1_TIME_to_tm(not_before, &not_before_tm))
            {
                throw ssl_exception("failed to convert notBefore ASN1_TIME to tm: ASN1_TIME_to_tm failure: " + last_openssl_error());
            }
            not_before_time = mktime(&not_before_tm);
            tm not_after_tm;
            if (!ASN1_TIME_to_tm(not_after, &not_after_tm))
            {
                throw ssl_exception("failed to convert not_after ASN1_TIME to tm: ASN1_TIME_to_tm failure: " + last_openssl_error());
            }
            not_after_time = mktime(&not_after_tm);
#elif (OPENSSL_VERSION_NUMBER >= 0x1000200fL)
            // Construct another ASN1_TIME for the unix epoch, get the difference
            // between them and use that to calculate a unix timestamp representing
            // when the cert expires
            ASN1_TIME_ptr epoch(ASN1_TIME_new(), &ASN1_STRING_free);
            ASN1_TIME_set_string(epoch.get(), "700101000000Z");
            int days{ 0 };
            int seconds{ 0 };

            if (!ASN1_TIME_diff(&days, &seconds, epoch.get(), not_before))
            {
                throw est_exception("failed to get the days and seconds value of not_before: ASN1_TIME_diff failure: " + last_openssl_error());
            }
            not_before_time = (days * 24 * 60 * 60) + seconds;
            if (!ASN1_TIME_diff(&days, &seconds, epoch.get(), not_after))
            {
                throw est_exception("failed to get the days and seconds value of not_after: ASN1_TIME_diff failure: " + last_openssl_error());
            }
            not_after_time = (days * 24 * 60 * 60) + seconds;
#else
            not_before_time = details::ASN1_TIME_to_posix_time(not_before);
            not_after_time = details::ASN1_TIME_to_posix_time(not_after);
#endif
            return{ _common_name, _issuer_name, not_before_time, not_after_time, sans };
        }

        // split certificate chain to list
        std::vector<std::string> split_certificate_chain(const std::string& cert_data)
        {
            std::vector<std::string> certs;
            const std::string begin_delimiter{ "-----BEGIN CERTIFICATE-----" };
            const std::string end_delimiter{ "-----END CERTIFICATE-----" };
            size_t start = 0;
            size_t end = 0;
            do
            {
                start = cert_data.find(begin_delimiter, start);
                end = cert_data.find(end_delimiter, start);

                if (std::string::npos != start && std::string::npos != end)
                {
                    certs.push_back(cert_data.substr(start, end - start + end_delimiter.length()));
                    start = end + end_delimiter.length();
                }

            } while (std::string::npos != start && std::string::npos != end);

            return certs;
        }

        // calculate the number of seconds to expire with the given ratio
        int certificate_expiry_from_now(const std::string& cert_data, double ratio)
        {
            const auto cert_info = cert_information(cert_data);
            const auto now = time(NULL);
            const auto from_now = difftime(cert_info.not_after, now);
            return (int)(from_now > 0 ? from_now * ratio : 0);
        }
	}
}

#include "ssl/ssl_utils.h"

#include <boost/algorithm/string.hpp> // for boost::split
#include <openssl/err.h>
#include <openssl/pem.h>

namespace ssl
{
    namespace experimental
    {
        namespace details
        {
            // get the text value from the X509 Name with the given Number identifier (NID)
            std::string get_text_by_NID(X509_NAME* x509_name, int nid)
            {
                const auto len = X509_NAME_get_text_by_NID(x509_name, nid, NULL, 0);
                if (0 < len)
                {
                    const auto buffer_size = len + 1;
                    std::vector<char> buffer(buffer_size);
                    if (len == X509_NAME_get_text_by_NID(x509_name, nid, buffer.data(), buffer_size))
                    {
                        return std::string(buffer.data(), len);
                    }
                }
                return "";
            }

            // get the Subject Alternative Name(s) from certificate
            std::vector<std::string> get_subject_alt_names(X509* x509)
            {
                std::vector<std::string> subject_alternative_names;
                GENERAL_NAMES_ptr subject_alt_names((GENERAL_NAMES*)X509_get_ext_d2i(x509, NID_subject_alt_name, NULL, NULL), &GENERAL_NAMES_free);
                for (auto idx = 0; idx < sk_GENERAL_NAME_num(subject_alt_names.get()); idx++)
                {
                    auto gen = sk_GENERAL_NAME_value(subject_alt_names.get(), idx);
                    if (gen->type == GEN_DNS)
                    {
                        auto asn1_str = gen->d.dNSName;
#if (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
                        auto san = std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(asn1_str)), ASN1_STRING_length(asn1_str));
#else
                        auto san = std::string(reinterpret_cast<char*>(ASN1_STRING_data(asn1_str)), ASN1_STRING_length(asn1_str));
#endif
                        subject_alternative_names.push_back(san);
                    }
                    else
                    {
                        // hmm, not supporting other type of subject alt name
                    }
                }
                return subject_alternative_names;
            }

            // convert ASN.1 time to POSIX (UTC)
            time_t ASN1_TIME_to_time_t(const ASN1_TIME* time)
            {
                if (!time)
                {
                    throw ssl_exception("failed to convert ASN1_TIME to UTC: invalid ASN1_TIME");
                }

                tm tm;

#if (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
                if (!ASN1_TIME_to_tm(time, &tm))
                {
                    throw ssl_exception("failed to convert ASN1_TIME to tm: ASN1_TIME_to_tm failure: " + last_openssl_error());
                }
#else
                auto s = time->data;
                if (!s)
                {
                    throw ssl_exception("failed to convert ASN1_TIME to UTC: invalid ASN1_TIME, no ANS1 data");
                }

                auto two_digits_to_uint = [&]()
                {
                    uint32_t n = 10 * (*s++ - '0');
                    return n + (*s++ - '0');
                };

                switch (time->type)
                {
                // see https://tools.ietf.org/html/rfc5280#section-4.1.2.5.1
                case V_ASN1_UTCTIME: // YYMMDDHHMMSSZ
                    tm.tm_year = two_digits_to_uint();
                    tm.tm_year += tm.tm_year < 50 ? 2000 : 1900;
                    tm.tm_year -= 1900;
                    break;
                // https://tools.ietf.org/html/rfc5280#section-4.1.2.5.2
                case V_ASN1_GENERALIZEDTIME: // YYYYMMDDHHMMSSZ
                    tm.tm_year = 100 * two_digits_to_uint();
                    tm.tm_year += two_digits_to_uint();
                    tm.tm_year -= 1900;
                    break;
                default:
                    throw ssl_exception("failed to convert ASN1_TIME to UTC: invalid ASN.1 time type");
                }
                tm.tm_mon = two_digits_to_uint() - 1;
                tm.tm_mday = two_digits_to_uint();
                tm.tm_hour = two_digits_to_uint();
                tm.tm_min = two_digits_to_uint();
                tm.tm_sec = two_digits_to_uint();
                if (*s != 'Z') { throw ssl_exception("failed to convert ASN1_TIME to UTC: invalid ASN.1 time format"); }
                tm.tm_isdst = 0;
#endif

                return mktime(&tm);
            }
        }

        // get last openssl error
        std::string last_openssl_error()
        {
            char buffer[1024] = { 0 };
            ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
            return buffer;
        }

        // get certificate information, such as subject, issuer and validity
        certificate_info get_certificate_info(const std::string& certificate)
        {
            BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
            if (!bio)
            {
                throw ssl_exception("failed to load certificate while creating BIO memory: BIO_new failure: " + last_openssl_error());
            }

            if ((size_t)BIO_write(bio.get(), certificate.data(), (int)certificate.size()) != certificate.size())
            {
                throw ssl_exception("failed to load certificate to bio: BIO_write failure: " + last_openssl_error());
            }

            X509_ptr x509(PEM_read_bio_X509_AUX(bio.get(), NULL, NULL, NULL), &X509_free);
            if (!x509)
            {
                throw ssl_exception("failed to load certificate bio to X509: PEM_read_bio_X509_AUX failure: " + last_openssl_error());
            }

            auto subject_alternative_names = details::get_subject_alt_names(x509.get());

            auto subject_common_name = details::get_text_by_NID(X509_get_subject_name(x509.get()), NID_commonName);
            if (subject_common_name.empty())
            {
                throw ssl_exception("missing Subject Common Name");
            }

            auto issuer_common_name = details::get_text_by_NID(X509_get_issuer_name(x509.get()), NID_commonName);
            if (issuer_common_name.empty())
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

            auto not_before_time = details::ASN1_TIME_to_time_t(not_before);
            auto not_after_time = details::ASN1_TIME_to_time_t(not_after);

            return{ subject_common_name, issuer_common_name, not_before_time, not_after_time, subject_alternative_names };
        }

        // split certificate chain to a list of certificates
        std::vector<std::string> split_certificate_chain(const std::string& certificate_chain)
        {
            std::vector<std::string> certificates;
            const std::string begin_delimiter{ "-----BEGIN CERTIFICATE-----" };
            const std::string end_delimiter{ "-----END CERTIFICATE-----" };
            size_t start = 0;
            size_t end = 0;
            do
            {
                start = certificate_chain.find(begin_delimiter, start);
                end = certificate_chain.find(end_delimiter, start);

                if (std::string::npos != start && std::string::npos != end)
                {
                    certificates.push_back(certificate_chain.substr(start, end - start + end_delimiter.length()));
                    start = end + end_delimiter.length();
                }

            } while (std::string::npos != start && std::string::npos != end);

            return certificates;
        }

        // calculate the number of seconds until expiry of the specified certificate
        // 0 is returned if certificate has already expired
        double certificate_expiry_from_now(const std::string& certificate)
        {
            const auto certificate_info = get_certificate_info(certificate);
            const auto now = time(NULL);
            const auto from_now = difftime(certificate_info.not_after, now);
            return (std::max)(0.0, from_now);
        }
    }
}

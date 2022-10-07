#ifndef SSL_SSL_UTILS_H
#define SSL_SSL_UTILS_H

#include <memory>
#include <openssl/x509v3.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace ssl
{
    namespace experimental
    {
        struct ssl_exception : std::runtime_error
        {
            ssl_exception(const std::string& message) : std::runtime_error(message) {}
        };

        typedef std::unique_ptr<BIO, decltype(&BIO_free)> BIO_ptr;
        typedef std::unique_ptr<X509, decltype(&X509_free)> X509_ptr;
        typedef std::unique_ptr<GENERAL_NAMES, decltype(&GENERAL_NAMES_free)> GENERAL_NAMES_ptr;
        typedef std::unique_ptr<ASN1_TIME, decltype(&ASN1_STRING_free)> ASN1_TIME_ptr;

        // get last openssl error
        std::string last_openssl_error();

        struct cert_info
        {
            std::string common_name;
            std::string issuer_name;
            time_t not_before;
            time_t not_after;
            std::vector<std::string> sans;
        };
        // get certificate information, such as expire date, it is represented as the number of seconds from 1970-01-01T0:0:0Z as measured in UTC
        cert_info cert_information(const std::string& cert_data);

        // split certificate chain to list of certificates
        std::vector<std::string> split_certificate_chain(const std::string& cert_data);

        // calculate the number of seconds to expire with the given ratio
        int certificate_expiry_from_now(const std::string& cert_data, double ratio = 1.0);
    }
}

#endif

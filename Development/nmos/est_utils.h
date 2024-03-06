#ifndef NMOS_EST_UTILS_H
#define NMOS_EST_UTILS_H

#include <stdexcept>
#include <string>
#include <vector>

namespace nmos
{
    namespace experimental
    {
        struct est_exception : std::runtime_error
        {
            est_exception(const std::string& message) : std::runtime_error(message) {}
        };

		namespace details
		{
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

            // split certificate chain to list
            std::vector<std::string> split_certificate_chain(const std::string& certificate_data);

            // calculate the number of seconds to expire with the given ratio
            int certificate_expiry_from_now(const std::string& cert_data, double ratio = 1.0);

            std::string make_pem_from_pkcs7(const std::string& pkcs7);

            typedef std::pair<std::string, std::string> csr;  // csr represented private key pem and csr pem

            csr make_rsa_csr(const std::string& common_name, const std::string& country, const std::string& state, const std::string& city, const std::string& organization, const std::string& organizational_unit, const std::string& email_address, const std::string& private_key_data = {}, const std::string& password = {});

            csr make_ecdsa_csr(const std::string& common_name, const std::string& country, const std::string& state, const std::string& city, const std::string& organization, const std::string& organizational_unit, const std::string& email_address, const std::string& private_key_data = {}, const std::string& password = {});

            std::vector<std::string> x509_crl_urls(const std::string& cert_data);

            bool is_revoked_by_crl(const std::string& certificate_file, const std::string& certificate_issuer_file, const std::string& crl_data);
		}
    }
}

#endif

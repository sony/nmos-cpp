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
            std::string make_pem_from_pkcs7(const std::string& pkcs7);

            typedef std::pair<std::string, std::string> csr;  // csr represented private key pem and csr pem

            csr make_rsa_csr(const std::string& common_name, const std::string& country, const std::string& state, const std::string& city, const std::string& organization, const std::string& organizational_unit, const std::string& email_address, const std::string& private_key_data = {}, const std::string& password = {});

            csr make_ecdsa_csr(const std::string& common_name, const std::string& country, const std::string& state, const std::string& city, const std::string& organization, const std::string& organizational_unit, const std::string& email_address, const std::string& private_key_data = {}, const std::string& password = {});

            std::vector<std::string> x509_crl_urls(const std::string& certificate);

            bool is_revoked_by_crl(const std::string& certificate, const std::string& cert_issuer_data, const std::string& crl_data);
		}
    }
}

#endif

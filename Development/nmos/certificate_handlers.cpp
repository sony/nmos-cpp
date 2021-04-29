#include "nmos/certificate_handlers.h"

#include "cpprest/basic_utils.h"
#include "nmos/slog.h"

namespace nmos
{
    // implement callback to load TLS keys and certificate chains
    load_tls_handler make_load_tls_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        // load the TLS keys and certificate chains from files
        const auto tls_list = nmos::experimental::fields::tls(settings);

        return[&, tls_list]()
        {
            auto tls_data = std::vector<nmos::tls>();

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load TLS keys and certificate chains";

            if (0 == tls_list.size())
            {
                throw std::runtime_error("Missing TLS settings");
            }

            for (const auto& tls : tls_list.as_array())
            {
                if (!tls.has_field(nmos::experimental::fields::type) || !tls.has_field(nmos::experimental::fields::private_key_file) || !tls.has_field(nmos::experimental::fields::certificate_chain_file))
                {
                    throw std::runtime_error("Missing TLS type/private key file/certificate chain file");
                }
                const auto type = nmos::experimental::fields::type(tls);
                const auto private_key_file = nmos::experimental::fields::private_key_file(tls);
                const auto certificate_chain_file = nmos::experimental::fields::certificate_chain_file(tls);

                std::stringstream pkey;
                if (private_key_file.empty())
                {
                    throw std::runtime_error("Missing private key file");
                }
                else
                {
                    std::ifstream pkey_file(private_key_file);
                    pkey << pkey_file.rdbuf();
                }

                std::stringstream cert;
                if (certificate_chain_file.empty())
                {
                    throw std::runtime_error("Missing certificate chain file");
                }
                else
                {
                    std::ifstream cert_chain_file(certificate_chain_file);
                    cert << cert_chain_file.rdbuf();
                }

                tls_data.push_back({ type == nmos::tls_types::ECDSA.name ? nmos::tls_types::ECDSA : nmos::tls_types::RSA, utility::s2us(pkey.str()), utility::s2us(cert.str()) });
            }
            return tls_data;
        };
    }

    // implement callback to load Diffie-Hellman parameters for ephemeral key exchange support
    nmos::load_dh_param_handler make_load_dh_param_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        // load the DH parameters from file for the caller
        const auto dh_param_file = nmos::experimental::fields::dh_param_file(settings);

        return[&, dh_param_file]()
        {
            utility::string_t dh_param_data;

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load DH parameters";

            if (dh_param_file.empty())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing DH parameters file";
            }
            else
            {
                std::ifstream dh_file(dh_param_file);
                std::stringstream dh_param;
                dh_param << dh_file.rdbuf();
                dh_param_data = utility::s2us(dh_param.str());
            }
            return dh_param_data;
        };
    }

    // implement callback to load certification authorities
    nmos::load_cacerts_handler make_load_cacerts_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        // load the certification authorities from file for the caller
        const auto ca_certificate_file = nmos::experimental::fields::ca_certificate_file(settings);

        return [&, ca_certificate_file]()
        {
            utility::string_t cacerts_data;

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load certification authorities";

            if (ca_certificate_file.empty())
            {
                throw std::runtime_error("Missing certification authorities file");
            }
            else
            {
                std::ifstream ca_file(ca_certificate_file);
                std::stringstream cacerts;
                cacerts << ca_file.rdbuf();
                cacerts_data = utility::s2us(cacerts.str());
            }
            return cacerts_data;
        };
    }
}

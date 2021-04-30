#include "nmos/certificate_handlers.h"

#include "cpprest/basic_utils.h"
#include "nmos/certificate_settings.h"
#include "nmos/slog.h"

namespace nmos
{
    // implement callback to load certification authorities
    load_ca_certificates_handler make_load_ca_certificates_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        // load the certification authorities from file for the caller
        const auto ca_certificate_file = nmos::experimental::fields::ca_certificate_file(settings);

        return [&, ca_certificate_file]()
        {
            utility::string_t data;

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load certification authorities";

            if (ca_certificate_file.empty())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing certification authorities file";
            }
            else
            {
                std::ifstream ca_file(ca_certificate_file);
                std::stringstream cacerts;
                cacerts << ca_file.rdbuf();
                data = utility::s2us(cacerts.str());
            }
            return data;
        };
    }

    // implement callback to load server certificate keys and certificate chains
    load_server_certificate_chains_handler make_load_server_certificate_chains_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        // load the server keys and certificate chains from files
        const auto server_certificate_chains = nmos::experimental::fields::server_certificate_chains(settings);

        return[&, server_certificate_chains]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load server certificate keys and certificate chains";

            auto data = std::vector<nmos::server_certificate_chain>();

            if (0 == server_certificate_chains.size())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing server certificate chains settings";
            }

            for (const auto& server_certificate_chain : server_certificate_chains.as_array())
            {
                if (!server_certificate_chain.has_field(nmos::experimental::fields::key_algorithm)
                    || !server_certificate_chain.has_field(nmos::experimental::fields::private_key_file)
                    || !server_certificate_chain.has_field(nmos::experimental::fields::certificate_chain_file))
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing TLS type/private key file/certificate chain file";
                }
                const auto key_algorithm = nmos::experimental::fields::key_algorithm(server_certificate_chain);
                const auto private_key_file = nmos::experimental::fields::private_key_file(server_certificate_chain);
                const auto certificate_chain_file = nmos::experimental::fields::certificate_chain_file(server_certificate_chain);

                std::stringstream pkey;
                if (private_key_file.empty())
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing private key file";
                }
                else
                {
                    std::ifstream pkey_file(private_key_file);
                    pkey << pkey_file.rdbuf();
                }

                std::stringstream cert_chain;
                if (certificate_chain_file.empty())
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing certificate chain file";
                }
                else
                {
                    std::ifstream cert_chain_file(certificate_chain_file);
                    cert_chain << cert_chain_file.rdbuf();
                }

                data.push_back(nmos::server_certificate_chain(nmos::key_algorithms::ECDSA.name == key_algorithm ? nmos::key_algorithms::ECDSA : nmos::key_algorithms::RSA, utility::s2us(pkey.str()), utility::s2us(cert_chain.str())));
            }
            return data;
        };
    }

    // implement callback to load Diffie-Hellman parameters for ephemeral key exchange support
    load_dh_param_handler make_load_dh_param_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        // load the DH parameters from file for the caller
        const auto dh_param_file = nmos::experimental::fields::dh_param_file(settings);

        return[&, dh_param_file]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load DH parameters";

            utility::string_t data;

            if (dh_param_file.empty())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing DH parameters file";
            }
            else
            {
                std::ifstream dh_file(dh_param_file);
                std::stringstream dh_param;
                dh_param << dh_file.rdbuf();
                data = utility::s2us(dh_param.str());
            }
            return data;
        };
    }
}

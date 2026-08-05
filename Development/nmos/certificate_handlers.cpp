#include "nmos/certificate_handlers.h"

#include "cpprest/basic_utils.h"
#include "nmos/certificate_settings.h"
#include "nmos/slog.h"

namespace nmos
{
    // construct callback to load certification authorities from file based on settings, see nmos/certificate_settings.h
    load_ca_certificates_handler make_load_ca_certificates_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        const auto ca_certificate_file = nmos::experimental::fields::ca_certificate_file(settings);

        return [&, ca_certificate_file]()
        {
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Load certification authorities";

            if (ca_certificate_file.empty())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing certification authorities file";
            }
            else
            {
                utility::ifstream_t ca_file(ca_certificate_file);
                utility::ostringstream_t cacerts;
                cacerts << ca_file.rdbuf();
                return cacerts.str();
            }
            return utility::string_t{};
        };
    }

    // construct callback to load client certificate from files based on settings, see nmos/certificate_settings.h
    load_client_certificate_handler make_load_client_certificate_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        // load the client private key and certificate chain from files
        const auto client_certificate = nmos::experimental::fields::client_certificate(settings);

        return [&, client_certificate]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load client private key and certificate chain";

            const auto private_key_file = nmos::experimental::fields::private_key_file(client_certificate);
            const auto certificate_chain_file = nmos::experimental::fields::certificate_chain_file(client_certificate);

            utility::stringstream_t pkey;
            if (private_key_file.empty())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing client private key file";
            }
            else
            {
                utility::ifstream_t pkey_file(private_key_file);
                pkey << pkey_file.rdbuf();
            }

            utility::stringstream_t cert_chain;
            if (certificate_chain_file.empty())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing client certificate chain file";
            }
            else
            {
                utility::ifstream_t cert_chain_file(certificate_chain_file);
                cert_chain << cert_chain_file.rdbuf();
            }

            return (nmos::certificate(pkey.str(), cert_chain.str()));
        };
    }

    // construct callback to load server certificates from files based on settings, see nmos/certificate_settings.h
    load_server_certificates_handler make_load_server_certificates_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        // load the server private keys and certificate chains from files
        auto server_certificates = nmos::experimental::fields::server_certificates(settings);
        if (0 == server_certificates.size())
        {
            // (deprecated, replaced by server_certificates)
            const auto private_key_files = nmos::experimental::fields::private_key_files(settings);
            const auto certificate_chain_files = nmos::experimental::fields::certificate_chain_files(settings);

            const auto size = (std::min)(private_key_files.size(), certificate_chain_files.size());
            for (size_t i = 0; i < size; ++i)
            {
                web::json::push_back(server_certificates,
                    web::json::value_of({
                        { nmos::experimental::fields::private_key_file, private_key_files.at(i) },
                        { nmos::experimental::fields::certificate_chain_file, certificate_chain_files.at(i) }
                    })
                );
            }
        }

        return [&, server_certificates]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load server private keys and certificate chains";

            auto data = std::vector<nmos::certificate>();

            if (0 == server_certificates.size())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing server certificates";
            }

            for (const auto& server_certificate : server_certificates.as_array())
            {
                const auto key_algorithm = nmos::experimental::fields::key_algorithm(server_certificate);
                const auto private_key_file = nmos::experimental::fields::private_key_file(server_certificate);
                const auto certificate_chain_file = nmos::experimental::fields::certificate_chain_file(server_certificate);

                utility::ostringstream_t pkey;
                if (private_key_file.empty())
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing server private key file";
                }
                else
                {
                    utility::ifstream_t pkey_file(private_key_file);
                    pkey << pkey_file.rdbuf();
                }

                utility::ostringstream_t cert_chain;
                if (certificate_chain_file.empty())
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing server certificate chain file";
                }
                else
                {
                    utility::ifstream_t cert_chain_file(certificate_chain_file);
                    cert_chain << cert_chain_file.rdbuf();
                }

                data.push_back(nmos::certificate(nmos::key_algorithm{ key_algorithm }, pkey.str(), cert_chain.str()));
            }
            return data;
        };
    }

    // construct callback to load Diffie-Hellman parameters for ephemeral key exchange support from file based on settings, see nmos/certificate_settings.h
    load_dh_param_handler make_load_dh_param_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        const auto dh_param_file = nmos::experimental::fields::dh_param_file(settings);

        return[&, dh_param_file]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load DH parameters";

            if (dh_param_file.empty())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing DH parameters file";
            }
            else
            {
                utility::ifstream_t dh_file(dh_param_file);
                utility::ostringstream_t dh_param;
                dh_param << dh_file.rdbuf();
                return dh_param.str();
            }
            return utility::string_t{};
        };
    }

    // construct callback to load RSA private keys from file based on settings, see nmos/certificate_settings.h
    // required for OAuth client which is using Private Key JWT as the requested authentication method for the token endpoint
    load_rsa_private_keys_handler make_load_rsa_private_keys_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        // load the server private keys from files
        auto server_certificates = nmos::experimental::fields::server_certificates(settings);
        if (0 == server_certificates.size())
        {
            // (deprecated, replaced by server_certificates)
            const auto private_key_files = nmos::experimental::fields::private_key_files(settings);

            for (const auto& private_key_file : private_key_files.as_array())
            {
                web::json::push_back(server_certificates,
                    web::json::value_of({
                        { nmos::experimental::fields::private_key_file, private_key_file },
                        { nmos::experimental::fields::certificate_chain_file, {} }
                    })
                );
            }
        }

        return [&, server_certificates]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load server private keys";

            auto data = std::vector<nmos::certificate>();
            auto private_keys = std::vector<utility::string_t>();
            if (0 == server_certificates.size())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing server certificates";
            }

            for (const auto& server_certificate : server_certificates.as_array())
            {
                const auto key_algorithm = nmos::experimental::fields::key_algorithm(server_certificate);
                const auto private_key_file = nmos::experimental::fields::private_key_file(server_certificate);

                utility::ostringstream_t pkey;
                if (private_key_file.empty())
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing private key file";
                }
                else
                {
                    if (key_algorithm.empty() || key_algorithms::RSA.name == key_algorithm)
                    {
                        utility::ifstream_t pkey_file(private_key_file);
                        pkey << pkey_file.rdbuf();
                        private_keys.push_back(pkey.str());
                    }
                }
            }
            return private_keys;
        };
    }

    // construct callback to save certification authorities to file based on settings, see nmos/certificate_settings.h
    save_ca_certificates_handler make_save_ca_certificates_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        const auto ca_certificate_file = nmos::experimental::fields::ca_certificate_file(settings);

        return [&, ca_certificate_file](const utility::string_t& cacert)
        {
            // The Root CA is in pem format
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Save certification authorities file";

            utility::ofstream_t file(ca_certificate_file, std::ios::out | std::ios::trunc);
            if (file.is_open())
            {
                file << cacert;
                file.close();
            }
        };
    }

    // construct callback to save server certificates to files based on settings, see nmos/certificate_settings.h
    save_server_certificate_handler make_save_server_certificate_handler(const nmos::key_algorithm& key_algorithm, const nmos::settings& settings, slog::base_gate& gate)
    {
        const auto server_certificates = nmos::experimental::fields::server_certificates(settings);
        if (0 == server_certificates.size())
        {
            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing server certificates";
            return nullptr;
        }

        const auto server_certificates_value = server_certificates.as_array();

        auto found = std::find_if(server_certificates_value.begin(), server_certificates_value.end(), [&key_algorithm](const web::json::value& server_certificate)
        {
            return nmos::experimental::fields::key_algorithm(server_certificate) == key_algorithm.name;
        });
        if (server_certificates_value.end() == found)
        {
            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing " << utility::us2s(key_algorithm.name) << " server certificate";
            return nullptr;
        }
        const auto server_certificate = *found;

        return [&, key_algorithm, server_certificate](const nmos::certificate& certificate)
        {
            // The key and the certificate are in pem format

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Save server " << utility::us2s(key_algorithm.name) << " private keys and certificate chains";

            const auto private_key_file = nmos::experimental::fields::private_key_file(server_certificate);
            const auto certificate_chain_file = nmos::experimental::fields::certificate_chain_file(server_certificate);

            utility::stringstream_t pkey;
            if (private_key_file.empty())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing server private key file";
            }
            else
            {
                utility::ofstream_t key_file(private_key_file, std::ios::out | std::ios::trunc);
                if (key_file.is_open())
                {
                    key_file << certificate.private_key;
                    key_file.close();
                }
            }

            std::stringstream cert_chain;
            if (certificate_chain_file.empty())
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing server certificate chain file";
            }
            else
            {
                utility::ofstream_t cert_chain_file(certificate_chain_file, std::ios::out | std::ios::trunc);
                if (cert_chain_file.is_open())
                {
                    cert_chain_file << certificate.certificate_chain;
                    cert_chain_file.close();
                }
            }
        };
    }

    // construct callback to save ECDSA server certificate to file based on settings, see nmos/certificate_settings.h
    save_server_certificate_handler make_save_ecdsa_server_certificate_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        return make_save_server_certificate_handler(nmos::key_algorithms::ECDSA, settings, gate);
    }

    // construct callback to save RSA server certificate to file based on settings, see nmos/certificate_settings.h
    save_server_certificate_handler make_save_rsa_server_certificate_handler(const nmos::settings& settings, slog::base_gate& gate)
    {
        return make_save_server_certificate_handler(nmos::key_algorithms::RSA, settings, gate);
    }
}

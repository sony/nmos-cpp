#include "nmos/server_utils.h"

#include <algorithm>
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
#include "boost/asio/ssl/set_cipher_list.hpp"
#include "boost/asio/ssl/use_tmp_ecdh.hpp"
#endif
#include "cpprest/basic_utils.h"
#include "nmos/certificate_handlers.h"
#include "cpprest/details/system_error.h"
#include "cpprest/http_listener.h"
#include "cpprest/ws_listener.h"
#include "nmos/slog.h"
#include "nmos/ssl_context_options.h"

#include "nmos/certificate_settings.h"

// Utility types, constants and functions for implementing NMOS REST API servers
namespace nmos
{
    namespace details
    {
///*
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        template <typename ExceptionType>
        inline std::function<void(boost::asio::ssl::context&)> make_listener_ssl_context_callback(const nmos::settings& settings, load_server_certificates_handler load_server_certificates, load_dh_param_handler load_dh_param, slog::base_gate& gate)
        {
            if (!load_server_certificates)
            {
                load_server_certificates = make_load_server_certificates_handler(settings, gate);
            }

            if (!load_dh_param)
            {
                load_dh_param = make_load_dh_param_handler(settings, gate);
            }

            return [load_server_certificates, load_dh_param](boost::asio::ssl::context& ctx)
            {
                try
                {
                    ctx.set_options(nmos::details::ssl_context_options);

                    const auto server_certificates = load_server_certificates();

                    if (server_certificates.empty())
                    {
                        throw ExceptionType({}, "Missing server certificates");
                    }

                    for (const auto& server_certificate : server_certificates)
                    {
                        const auto key = utility::us2s(server_certificate.private_key);
                        if (0 == key.size())
                        {
                            throw ExceptionType({}, "Missing private key");
                        }
                        const auto cert_chain = utility::us2s(server_certificate.certificate_chain);
                        if (0 == cert_chain.size())
                        {
                            throw ExceptionType({}, "Missing certificate chain");
                        }
                        ctx.use_private_key(boost::asio::buffer(key.data(), key.size()), boost::asio::ssl::context_base::pem);
                        ctx.use_certificate_chain(boost::asio::buffer(cert_chain.data(), cert_chain.size()));

                        const auto key_algorithm = server_certificate.key_algorithm;
                        if (key_algorithm.name.empty() || key_algorithm == key_algorithms::ECDSA)
                        {
                            // certificates may not have ECDH parameters, so ignore errors...
                            boost::system::error_code ec;
                            use_tmp_ecdh(ctx, boost::asio::buffer(cert_chain.data(), cert_chain.size()), ec);
                        }
                    }

                    set_cipher_list(ctx, nmos::details::ssl_cipher_list);

                    const auto dh_param = utility::us2s(load_dh_param());
                    if (dh_param.size())
                    {
                        ctx.use_tmp_dh(boost::asio::buffer(dh_param.data(), dh_param.size()));
                    }
                }
                catch (const boost::system::system_error& e)
                {
                    throw web::details::from_boost_system_system_error<ExceptionType>(e);
                }
            };
        }
#endif
//*/

/*
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        template <typename ExceptionType>
        inline std::function<void(boost::asio::ssl::context&)> make_listener_ssl_context_callback(const nmos::settings& settings, load_server_certificates_handler load_server_certificates, load_dh_param_handler load_dh_param, slog::base_gate& gate)
        {
            //const auto& private_key_files = nmos::experimental::fields::private_key_files(settings);
            //const auto& certificate_chain_files = nmos::experimental::fields::certificate_chain_files(settings);

            auto private_key_files = web::json::value::array();
            auto certificate_chain_files = web::json::value::array();
            auto server_certificates = nmos::experimental::fields::server_certificates(settings);
            for (const auto& server_certificate : server_certificates.as_array())
            {
                const auto key_algorithm = nmos::experimental::fields::key_algorithm(server_certificate);
                const auto private_key_file = nmos::experimental::fields::private_key_file(server_certificate);
                const auto certificate_chain_file = nmos::experimental::fields::certificate_chain_file(server_certificate);

                push_back(private_key_files, private_key_file);
                push_back(certificate_chain_files, certificate_chain_file);
            }

            const auto& dh_param_file = utility::us2s(nmos::experimental::fields::dh_param_file(settings));
            return [private_key_files, certificate_chain_files, dh_param_file](boost::asio::ssl::context& ctx)
            {
                try
                {
                    ctx.set_options(nmos::details::ssl_context_options);

                    if (private_key_files.size() == 0)
                    {
                        throw ExceptionType({}, "Missing private key file");
                    }
                    for (const auto& private_key_file : private_key_files.as_array())
                    {
                        ctx.use_private_key_file(utility::us2s(private_key_file.as_string()), boost::asio::ssl::context::pem);
                    }

                    if (certificate_chain_files.size() == 0)
                    {
                        throw ExceptionType({}, "Missing certificate chain file");
                    }
                    for (const auto& certificate_chain_file : certificate_chain_files.as_array())
                    {
                        ctx.use_certificate_chain_file(utility::us2s(certificate_chain_file.as_string()));
                        // any one of the certificates may have ECDH parameters, so ignore errors...
                        boost::system::error_code ec;
                        use_tmp_ecdh_file(ctx, utility::us2s(certificate_chain_file.as_string()), ec);
                    }

                    set_cipher_list(ctx, nmos::details::ssl_cipher_list);

                    if (!dh_param_file.empty()) ctx.use_tmp_dh_file(dh_param_file);
                }
                catch (const boost::system::system_error& e)
                {
                    throw web::details::from_boost_system_system_error<ExceptionType>(e);
                }
            };
        }
#endif
*/
    }

    // construct listener config based on settings
    web::http::experimental::listener::http_listener_config make_http_listener_config(const nmos::settings& settings, load_server_certificates_handler load_server_certificates, load_dh_param_handler load_dh_param, slog::base_gate& gate)
    {
        web::http::experimental::listener::http_listener_config config;
        config.set_backlog(nmos::fields::listen_backlog(settings));
#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
        // hmm, hostport_listener::on_accept(...) in http_server_asio.cpp
        // only expects boost::system::system_error to be thrown, so for now
        // don't use web::http::http_exception
        config.set_ssl_context_callback(details::make_listener_ssl_context_callback<boost::system::system_error>(settings, load_server_certificates, load_dh_param, gate));
#endif

        return config;
    }

    // construct listener config based on settings
    web::websockets::experimental::listener::websocket_listener_config make_websocket_listener_config(const nmos::settings& settings, load_server_certificates_handler load_server_certificates, load_dh_param_handler load_dh_param, slog::base_gate& gate)
    {
        web::websockets::experimental::listener::websocket_listener_config config;
        config.set_backlog(nmos::fields::listen_backlog(settings));
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
        config.set_ssl_context_callback(details::make_listener_ssl_context_callback<web::websockets::websocket_exception>(settings, load_server_certificates, load_dh_param, gate));
#endif

        return config;
    }

    namespace experimental
    {
        // map the configured client port to the server port on which to listen
        int server_port(int client_port, const nmos::settings& settings)
        {
            const auto& port_map = nmos::experimental::fields::proxy_map(settings).as_array();
            const auto found = std::find_if(port_map.begin(), port_map.end(), [&](const web::json::value& m)
            {
                return client_port == m.at(U("client_port")).as_integer();
            });
            return port_map.end() != found ? found->at(U("server_port")).as_integer() : client_port;
        }
    }
}

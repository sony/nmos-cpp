#include "nmos/server_utils.h"

#include <algorithm>
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
#include "boost/asio/ssl/set_cipher_list.hpp"
#include "boost/asio/ssl/use_tmp_ecdh.hpp"
#endif
#include "cpprest/basic_utils.h"
#include "nmos/certificate_handlers.h"
#include "nmos/certificate_settings.h"
#include "cpprest/details/system_error.h"
#include "cpprest/http_listener.h"
#include "cpprest/ws_listener.h"
#include "nmos/slog.h"
#include "nmos/ssl_context_options.h"

// Utility types, constants and functions for implementing NMOS REST API servers
namespace nmos
{
    namespace details
    {
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        load_server_certificate_chains_handler make_default_server_certificate_chains_handler(const nmos::settings& settings, slog::base_gate& gate)
        {
            const auto private_key_files = nmos::experimental::fields::private_key_files(settings);
            const auto certificate_chain_files = nmos::experimental::fields::certificate_chain_files(settings);

            return [&, private_key_files, certificate_chain_files]()
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Load server certificate keys and certificate chains";

                auto data = std::vector<nmos::server_certificate_chain>();

                auto size = std::min(private_key_files.size(), certificate_chain_files.size());

                if (0 == private_key_files.size())
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing private key file";
                }

                if (0 == certificate_chain_files.size())
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing certificate chain file";
                }

                auto pkey_files = private_key_files.as_array();
                auto cert_chain_files = certificate_chain_files.as_array();
                for (size_t idx = 0; idx < size; idx++)
                {
                    std::ifstream pkey_file(pkey_files[idx].as_string());
                    std::stringstream pkey;
                    pkey << pkey_file.rdbuf();

                    std::ifstream cert_chain_file(cert_chain_files[idx].as_string());
                    std::stringstream cert_chain;
                    cert_chain << cert_chain_file.rdbuf();

                    data.push_back(nmos::server_certificate_chain(nmos::key_algorithms::unspecified, utility::s2us(pkey.str()), utility::s2us(cert_chain.str())));
                }
                return data;
            };
        }

        load_dh_param_handler make_default_load_dh_param_handler(const nmos::settings& settings, slog::base_gate& gate)
        {
            return make_load_dh_param_handler(settings, gate);
        }

        template <typename ExceptionType>
        inline std::function<void(boost::asio::ssl::context&)> make_listener_ssl_context_callback(const nmos::settings& settings, load_server_certificate_chains_handler load_server_certificate_chains, load_dh_param_handler load_dh_param, slog::base_gate& gate)
        {
            if (!load_server_certificate_chains)
            {
                load_server_certificate_chains = make_default_server_certificate_chains_handler(settings, gate);
            }

            if (!load_dh_param)
            {
                load_dh_param = make_default_load_dh_param_handler(settings, gate);
            }

            return [load_server_certificate_chains, load_dh_param](boost::asio::ssl::context& ctx)
            {
                try
                {
                    ctx.set_options(nmos::details::ssl_context_options);

                    const auto& server_certificate_chains = load_server_certificate_chains();

                    if (server_certificate_chains.empty())
                    {
                        throw ExceptionType({}, "Missing server certificate chains");
                    }

                    for (const auto& server_certificate_chain : server_certificate_chains)
                    {
                        const auto key = utility::us2s(std::get < 1 >(server_certificate_chain));
                        if (0 == key.size())
                        {
                            throw ExceptionType({}, "Missing private key");
                        }
                        const auto cert_chain = utility::us2s(std::get < 2 >(server_certificate_chain));
                        if (0 == cert_chain.size())
                        {
                            throw ExceptionType({}, "Missing server certificate chain");
                        }
                        ctx.use_private_key(boost::asio::buffer(key.data(), key.size()), boost::asio::ssl::context_base::pem);
                        ctx.use_certificate_chain(boost::asio::buffer(cert_chain.data(), cert_chain.size()));

                        const auto key_algorithm = std::get < 0 >(server_certificate_chain);
                        if (key_algorithm == key_algorithms::unspecified || key_algorithm == key_algorithms::ECDSA)
                        {
                            // certificates may have ECDH parameters, so ignore errors...
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
    }

    // construct listener config based on settings
    web::http::experimental::listener::http_listener_config make_http_listener_config(const nmos::settings& settings, load_server_certificate_chains_handler load_server_certificate_chains, load_dh_param_handler load_dh_param, slog::base_gate& gate)
    {
        web::http::experimental::listener::http_listener_config config;
        config.set_backlog(nmos::fields::listen_backlog(settings));
#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
        // hmm, hostport_listener::on_accept(...) in http_server_asio.cpp
        // only expects boost::system::system_error to be thrown, so for now
        // don't use web::http::http_exception
        config.set_ssl_context_callback(details::make_listener_ssl_context_callback<boost::system::system_error>(settings, load_server_certificate_chains, load_dh_param, gate));
#endif

        return config;
    }

    // construct listener config based on settings
    web::websockets::experimental::listener::websocket_listener_config make_websocket_listener_config(const nmos::settings& settings, load_server_certificate_chains_handler load_server_certificate_chains, load_dh_param_handler load_dh_param, slog::base_gate& gate)
    {
        web::websockets::experimental::listener::websocket_listener_config config;
        config.set_backlog(nmos::fields::listen_backlog(settings));
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
        config.set_ssl_context_callback(details::make_listener_ssl_context_callback<web::websockets::websocket_exception>(settings, load_server_certificate_chains, load_dh_param, gate));
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

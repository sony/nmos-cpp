#include "nmos/server_utils.h"

#include <algorithm>
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
#include "boost/asio/ssl/set_cipher_list.hpp"
#include "boost/asio/ssl/use_tmp_ecdh.hpp"
#endif
#include "cpprest/basic_utils.h"
#include "cpprest/details/system_error.h"
#include "cpprest/http_listener.h"
#include "cpprest/ws_listener.h"
#include "nmos/ssl_context_options.h"

// Utility types, constants and functions for implementing NMOS REST API servers
namespace nmos
{
    namespace details
    {
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        template <typename ExceptionType>
        inline std::function<void(boost::asio::ssl::context&)> make_listener_ssl_context_callback(const nmos::settings& settings)
        {
            const auto& private_key_files = nmos::experimental::fields::private_key_files(settings);
            const auto& certificate_chain_files = nmos::experimental::fields::certificate_chain_files(settings);
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
    }

    // construct listener config based on settings
    web::http::experimental::listener::http_listener_config make_http_listener_config(const nmos::settings& settings)
    {
        web::http::experimental::listener::http_listener_config config;
        config.set_backlog(nmos::fields::listen_backlog(settings));
#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
        // hmm, hostport_listener::on_accept(...) in http_server_asio.cpp
        // only expects boost::system::system_error to be thrown, so for now
        // don't use web::http::http_exception
        config.set_ssl_context_callback(details::make_listener_ssl_context_callback<boost::system::system_error>(settings));
#endif

        return config;
    }

    // construct listener config based on settings
    web::websockets::experimental::listener::websocket_listener_config make_websocket_listener_config(const nmos::settings& settings)
    {
        web::websockets::experimental::listener::websocket_listener_config config;
        config.set_backlog(nmos::fields::listen_backlog(settings));
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
        config.set_ssl_context_callback(details::make_listener_ssl_context_callback<web::websockets::websocket_exception>(settings));
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

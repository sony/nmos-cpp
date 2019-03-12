#include "nmos/server_utils.h"

#include <algorithm>
#include "cpprest/basic_utils.h"
#include "cpprest/http_listener.h"
#include "cpprest/ws_listener.h"
#include "nmos/ssl_context_options.h"

// Utility types, constants and functions for implementing NMOS REST API servers
namespace nmos
{
    // construct listener config based on settings
    web::http::experimental::listener::http_listener_config make_http_listener_config(const nmos::settings& settings)
    {
        web::http::experimental::listener::http_listener_config config;
        config.set_backlog(nmos::fields::listen_backlog(settings));
#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
        const auto private_key_file = utility::us2s(nmos::experimental::fields::private_key_file(settings));
        const auto certificate_chain_file = utility::us2s(nmos::experimental::fields::certificate_chain_file(settings));
        config.set_ssl_context_callback([private_key_file, certificate_chain_file](boost::asio::ssl::context& ctx)
        {
            try
            {
                ctx.set_options(nmos::details::ssl_context_options);
                ctx.use_private_key_file(private_key_file, boost::asio::ssl::context::pem);
                ctx.use_certificate_chain_file(certificate_chain_file);
            }
            catch (const boost::system::system_error& e)
            {
                throw web::http::http_exception(e.code(), e.what());
            }
        });
#endif

        return config;
    }

    // construct listener config based on settings
    web::websockets::experimental::listener::websocket_listener_config make_websocket_listener_config(const nmos::settings& settings)
    {
        web::websockets::experimental::listener::websocket_listener_config config;
        config.set_backlog(nmos::fields::listen_backlog(settings));
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
        const auto private_key_file = utility::us2s(nmos::experimental::fields::private_key_file(settings));
        const auto certificate_chain_file = utility::us2s(nmos::experimental::fields::certificate_chain_file(settings));
        config.set_ssl_context_callback([private_key_file, certificate_chain_file](boost::asio::ssl::context& ctx)
        {
            try
            {
                ctx.set_options(nmos::details::ssl_context_options);
                ctx.use_private_key_file(private_key_file, boost::asio::ssl::context::pem);
                ctx.use_certificate_chain_file(certificate_chain_file);
            }
            catch (const boost::system::system_error& e)
            {
                throw web::websockets::experimental::listener::websocket_exception(e.code(), e.what());
            }
        });
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

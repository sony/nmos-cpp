#include "nmos/server_utils.h"

#include <algorithm>
#include "cpprest/http_listener.h"
#include "cpprest/ws_listener.h"

// Utility types, constants and functions for implementing NMOS REST API servers
namespace nmos
{
    // construct listener config based on settings
    web::http::experimental::listener::http_listener_config make_http_listener_config(const nmos::settings& settings)
    {
        web::http::experimental::listener::http_listener_config config;
        config.set_backlog(nmos::fields::listen_backlog(settings));

        return config;
    }

    // construct listener config based on settings
    web::websockets::experimental::listener::websocket_listener_config make_websocket_listener_config(const nmos::settings& settings)
    {
        web::websockets::experimental::listener::websocket_listener_config config;

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

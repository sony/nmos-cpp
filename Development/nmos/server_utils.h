#ifndef NMOS_SERVER_UTILS_H
#define NMOS_SERVER_UTILS_H

#include "cpprest/http_listener.h" // forward declaration of web::http::experimental::listener::http_listener_config
#include "cpprest/ws_listener.h" // forward declaration of web::websockets::experimental::listener::websocket_listener_config
#include "nmos/settings.h"

// Utility types, constants and functions for implementing NMOS REST API servers
namespace nmos
{
    // construct listener config based on settings
    web::http::experimental::listener::http_listener_config make_http_listener_config(const nmos::settings& settings);

    // construct listener config based on settings
    web::websockets::experimental::listener::websocket_listener_config make_websocket_listener_config(const nmos::settings& settings);

    namespace experimental
    {
        // map the configured client port to the server port on which to listen
        int server_port(int client_port, const nmos::settings& settings);
    }
}

#endif

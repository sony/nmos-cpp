#ifndef NMOS_SERVER_UTILS_H
#define NMOS_SERVER_UTILS_H

#include "cpprest/http_listener.h" // forward declaration of web::http::experimental::listener::http_listener_config
#include "cpprest/ws_listener.h" // forward declaration of web::websockets::experimental::listener::websocket_listener_config
#include "nmos/certificate_handlers.h"
#include "nmos/settings.h"

namespace slog { class base_gate; }

// Utility types, constants and functions for implementing NMOS REST API servers
namespace nmos
{
    // construct listener config based on settings
    web::http::experimental::listener::http_listener_config make_http_listener_config(const nmos::settings& settings, load_server_certificates_handler load_server_certificates, load_dh_param_handler load_dh_param, slog::base_gate& gate);

    // construct listener config based on settings
    web::websockets::experimental::listener::websocket_listener_config make_websocket_listener_config(const nmos::settings& settings, load_server_certificates_handler load_server_certificates, load_dh_param_handler load_dh_param, slog::base_gate& gate);

    namespace experimental
    {
        // map the configured client port to the server port on which to listen
        int server_port(int client_port, const nmos::settings& settings);
    }
}

#endif

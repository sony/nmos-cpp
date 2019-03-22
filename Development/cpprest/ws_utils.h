#ifndef CPPREST_WS_UTILS_H
#define CPPREST_WS_UTILS_H

#if !defined(CPPREST_EXCLUDE_WEBSOCKETS)

#include "pplx/pplx_utils.h"
#include "cpprest/uri_schemes.h"

// Utility types, constants and functions for WebSocket
namespace web
{
    namespace websockets
    {
        namespace experimental
        {
            namespace listener
            {
                class websocket_listener;

                // RAII helper for websocket_listener sessions
                typedef pplx::open_close_guard<websocket_listener> websocket_listener_guard;

                // platform-specific wildcard address to accept connections for any address
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
                const utility::string_t host_wildcard{ _XPLATSTR("0.0.0.0") };
#else
                const utility::string_t host_wildcard{ _XPLATSTR("*") }; // "weak wildcard"
#endif

                // make an address to be used to accept WebSocket or Secure WebSocket connections for the specified address and port
                inline web::uri make_listener_uri(bool secure, const utility::string_t& host_address, int port)
                {
                    return web::uri_builder().set_scheme(web::ws_scheme(secure)).set_host(host_address).set_port(port).to_uri();
                }

                // make an address to be used to accept WebSocket connections for the specified address and port
                inline web::uri make_listener_uri(const utility::string_t& host_address, int port)
                {
                    return make_listener_uri(false, host_wildcard, port);
                }

                // make an address to be used to accept WebSocket connections for the specified port for any address
                inline web::uri make_listener_uri(int port)
                {
                    return make_listener_uri(host_wildcard, port);
                }
            }
        }
    }
}

#endif

#endif

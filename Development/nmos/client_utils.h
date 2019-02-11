#ifndef NMOS_CLIENT_UTILS_H
#define NMOS_CLIENT_UTILS_H

#include "nmos/settings.h"

namespace web
{
    namespace http { namespace client { class http_client_config; } }
    namespace websockets { namespace client { class websocket_client_config; } }
}

// Utility types, constants and functions for implementing NMOS REST API clients
namespace nmos
{
    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted, e.g. request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings);

    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted
    web::websockets::client::websocket_client_config make_websocket_client_config(const nmos::settings& settings);
}

#endif

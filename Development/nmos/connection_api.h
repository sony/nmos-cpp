#ifndef NMOS_CONNECTION_API_H
#define NMOS_CONNECTION_API_H

#include "cpprest/api_router.h"
#include "nmos/mutex.h"
#include "nmos/resources.h"

namespace slog
{
    class base_gate;
}

// Connection API implementation
// See https://github.com/AMWA-TV/nmos-device-connection-management/blob/master/APIs/ConnectionAPI.raml
namespace nmos
{
    web::http::experimental::listener::api_router make_connection_api(nmos::resources& resources, nmos::mutex& mutex, slog::base_gate& gate);
}

#endif

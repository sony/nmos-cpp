#ifndef NMOS_CONNECTION_API_H
#define NMOS_CONNECTION_API_H

#include "cpprest/api_router.h"
#include "nmos/mutex.h"
#include "nmos/resources.h"
#include "nmos/model.h"

namespace slog
{
    class base_gate;
}

// Connection API implementation
// See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml
namespace nmos
{
    web::http::experimental::listener::api_router make_connection_api(nmos::model& model, nmos::mutex& mutex, nmos::condition_variable& condition, slog::base_gate& gate);
}

#endif

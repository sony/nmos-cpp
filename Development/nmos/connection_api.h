#ifndef NMOS_CONNECTION_API_H
#define NMOS_CONNECTION_API_H

#include <functional>
#include "cpprest/api_router.h"
#include "nmos/mutex.h"
#include "nmos/model.h"

namespace slog
{
    class base_gate;
}

// Connection API implementation
// See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml
namespace nmos
{
    typedef std::function<void(const nmos::type& type, const nmos::id& id)> activate_function;

    web::http::experimental::listener::api_router make_connection_api(nmos::model& model, nmos::mutex& mutex, nmos::condition_variable& condition, nmos::activate_function activate, slog::base_gate& gate);
}

#endif

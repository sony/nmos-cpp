#ifndef NMOS_SYSTEM_API_H
#define NMOS_SYSTEM_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// System API implementation
// See https://specs.amwa.tv/is-09/releases/v1.0.0/APIs/SystemAPI.html
namespace nmos
{
    struct registry_model;

    web::http::experimental::listener::api_router make_system_api(nmos::registry_model& model, slog::base_gate& gate);
}

#endif

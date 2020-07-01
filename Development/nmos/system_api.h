#ifndef NMOS_SYSTEM_API_H
#define NMOS_SYSTEM_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// System API implementation
// See https://github.com/AMWA-TV/nmos-system/blob/v1.0/APIs/SystemAPI.raml
namespace nmos
{
    struct registry_model;

    web::http::experimental::listener::api_router make_system_api(nmos::registry_model& model, slog::base_gate& gate);
}

#endif

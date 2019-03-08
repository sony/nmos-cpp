#ifndef NMOS_SYSTEM_API_H
#define NMOS_SYSTEM_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// System API implementation
// See JT-NM TR-1001-1:2018 Annex A
namespace nmos
{
    struct registry_model;

    web::http::experimental::listener::api_router make_system_api(nmos::registry_model& model, slog::base_gate& gate);
}

#endif

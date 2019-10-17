#ifndef NMOS_SCHEMAS_API_H
#define NMOS_SCHEMAS_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// This is an experimental extension to expose the embedded JSON Schemas for the NMOS specifications
namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_schemas_api(slog::base_gate& gate);
    }
}

#endif

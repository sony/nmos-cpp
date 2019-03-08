#ifndef NMOS_LOGGING_API_H
#define NMOS_LOGGING_API_H

#include "cpprest/api_router.h"
#include "nmos/log_model.h"

namespace slog
{
    class base_gate;
}

// This is an experimental extension to expose logging via a REST API
namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_logging_api(nmos::experimental::log_model& model, slog::base_gate& gate);
    }
}

#endif

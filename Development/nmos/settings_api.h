#ifndef NMOS_SETTINGS_API_H
#define NMOS_SETTINGS_API_H

#include <atomic>
#include "cpprest/api_router.h"
#include "nmos/slog.h" // for slog::base_gate and slog::severity, etc.

// This is an experimental extension to expose configuration settings via a REST API
namespace nmos
{
    struct base_model;

    namespace experimental
    {
        web::http::experimental::listener::api_router make_settings_api(nmos::base_model& model, std::atomic<slog::severity>& logging_level, slog::base_gate& gate);
    }
}

#endif

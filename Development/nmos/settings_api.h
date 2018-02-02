#ifndef NMOS_SETTINGS_API_H
#define NMOS_SETTINGS_API_H

#include <atomic>
#include "cpprest/api_router.h"
#include "nmos/mutex.h"
#include "nmos/settings.h"
#include "nmos/slog.h" // for slog::base_gate and slog::severity, etc.

// This is an experimental extension to expose configuration settings via a REST API
namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_settings_api(nmos::settings& settings, nmos::mutex& mutex, std::atomic<slog::severity>& logging_level, slog::base_gate& gate);
    }
}

#endif

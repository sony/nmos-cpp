#ifndef NMOS_SETTINGS_API_H
#define NMOS_SETTINGS_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// This is an experimental extension to expose configuration settings via a REST API
namespace nmos
{
    struct base_model;

    namespace experimental
    {
        struct log_model;

        web::http::experimental::listener::api_router make_settings_api(nmos::base_model& model, nmos::experimental::log_model& log_model, slog::base_gate& gate);
    }
}

#endif

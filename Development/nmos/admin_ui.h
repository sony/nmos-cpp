#ifndef NMOS_ADMIN_UI_H
#define NMOS_ADMIN_UI_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// This is an experimental extension to expose a basic NMOS client and administration web-app for an NMOS server (registry)
namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_admin_ui(const utility::string_t& filesystem_root, slog::base_gate& gate);
    }
}

#endif

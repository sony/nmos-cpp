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
        // (could be moved to nmos/api_utils.h or its own file?)
        web::http::experimental::listener::api_router make_api_sub_route(const utility::string_t& sub_route, web::http::experimental::listener::api_router sub_router, slog::base_gate& gate);

        web::http::experimental::listener::api_router make_admin_ui(const utility::string_t& filesystem_root, slog::base_gate& gate);
    }
}

#endif

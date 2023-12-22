#ifndef NMOS_REGISTRATION_API_H
#define NMOS_REGISTRATION_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// Registration API implementation
// See https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/RegistrationAPI.html
namespace nmos
{
    struct registry_model;

    void erase_expired_resources_thread(nmos::registry_model& model, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_registration_api(nmos::registry_model& model, web::http::experimental::listener::route_handler validate_authorization, slog::base_gate& gate);

    inline web::http::experimental::listener::api_router make_registration_api(nmos::registry_model& model, slog::base_gate& gate)
    {
        return make_registration_api(model, {}, gate);
    }
}

#endif

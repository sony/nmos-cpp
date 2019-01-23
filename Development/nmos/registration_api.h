#ifndef NMOS_REGISTRATION_API_H
#define NMOS_REGISTRATION_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// Registration API implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/RegistrationAPI.raml
namespace nmos
{
    struct registry_model;

    void erase_expired_resources_thread(nmos::registry_model& model, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_registration_api(nmos::registry_model& model, slog::base_gate& gate);
}

#endif

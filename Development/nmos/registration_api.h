#ifndef NMOS_REGISTRATION_API_H
#define NMOS_REGISTRATION_API_H

#include "cpprest/api_router.h"
#include "nmos/mutex.h"

namespace slog
{
    class base_gate;
}

// Registration API implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/RegistrationAPI.raml
namespace nmos
{
    struct model;

    void erase_expired_resources_thread(nmos::model& model, const bool& shutdown, nmos::mutex& mutex, nmos::condition_variable& shutdown_condition, nmos::condition_variable& condition, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_registration_api(nmos::model& model, nmos::mutex& mutex, nmos::condition_variable& condition, slog::base_gate& gate);
}

#endif

#ifndef NMOS_REGISTRATION_API_H
#define NMOS_REGISTRATION_API_H

#include <condition_variable> // for condition_variable and mutex
#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// Registration API implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2-dev/APIs/RegistrationAPI.raml
namespace nmos
{
    struct model;

    void erase_expired_resources_thread(nmos::model& model, std::mutex& mutex, std::condition_variable& condition, bool& shutdown, std::condition_variable& query_ws_events_condition, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_registration_api(nmos::model& model, std::mutex& mutex, std::condition_variable& query_ws_events_condition, slog::base_gate& gate);
}

#endif

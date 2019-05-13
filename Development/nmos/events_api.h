#ifndef NMOS_EVENTS_API_H
#define NMOS_EVENTS_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// Events API implementation
// See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/EventsAPI.raml
namespace nmos
{
    struct node_model;

    web::http::experimental::listener::api_router make_events_api(const nmos::node_model& model, slog::base_gate& gate);
}

#endif

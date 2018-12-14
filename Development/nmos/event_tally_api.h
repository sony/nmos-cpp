#ifndef NMOS_EVENT_TALLY_API_H
#define NMOS_EVENT_TALLY_API_H

#include <atomic>
#include "cpprest/api_router.h"
#include "nmos/slog.h" // for slog::base_gate and slog::severity, etc.

// This is an experimental extension to expose configuration event-tally via a REST API
namespace nmos
{
    struct node_model;

    namespace experimental
    {

        web::http::experimental::listener::api_router make_event_tally_api(nmos::node_model& model, slog::base_gate& gate);

    }

}

#endif

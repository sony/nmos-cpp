#ifndef NMOS_EVENTS_API_H
#define NMOS_EVENTS_API_H

#include <atomic>
#include "cpprest/api_router.h"
#include "nmos/slog.h" // for slog::base_gate and slog::severity, etc.

// IS-07 Rest api exposing state and event types
namespace nmos
{
    struct node_model;

    web::http::experimental::listener::api_router make_events_api(const nmos::node_model& model, slog::base_gate& gate);
}

#endif

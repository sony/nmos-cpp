#ifndef NMOS_CONNECTION_EVENTS_ACTIVATION_H
#define NMOS_CONNECTION_EVENTS_ACTIVATION_H

#include "nmos/connection_activation.h"
#include "nmos/events_ws_client.h" // for nmos::events_ws_message_handler, etc.
#include "nmos/settings.h" // just a forward declaration of nmos::settings

namespace nmos
{
    struct node_model;

    nmos::connection_activation_handler make_connection_events_websocket_activation_handler(nmos::events_ws_message_handler message_handler, const nmos::settings& settings, slog::base_gate& gate);

    namespace experimental
    {
        typedef std::function<void(const nmos::resource& resource, const nmos::resource& connection_resource, const web::json::value& message)> events_ws_receiver_event_handler;

        nmos::events_ws_message_handler make_events_ws_message_handler(const nmos::node_model& model, events_ws_receiver_event_handler event_handler, slog::base_gate& gate);
    }
}

#endif

#ifndef NMOS_CONNECTION_EVENTS_ACTIVATION_H
#define NMOS_CONNECTION_EVENTS_ACTIVATION_H

#include "nmos/certificate_handlers.h"
#include "nmos/connection_activation.h"
#include "nmos/events_ws_client.h" // for nmos::events_ws_message_handler, etc.
#include "nmos/settings.h" // just a forward declaration of nmos::settings

namespace nmos
{
    struct node_model;

    // this handler can be used to (un)subscribe IS-07 Events WebSocket receivers with the specified handlers, when they are activated
    nmos::connection_activation_handler make_connection_events_websocket_activation_handler(nmos::load_ca_certificates_handler load_ca_certificates, nmos::events_ws_message_handler message_handler, nmos::events_ws_close_handler close_handler, const nmos::settings& settings, slog::base_gate& gate);

    inline nmos::connection_activation_handler make_connection_events_websocket_activation_handler(nmos::events_ws_message_handler message_handler, nmos::events_ws_close_handler close_handler, const nmos::settings& settings, slog::base_gate& gate)
    {
        return make_connection_events_websocket_activation_handler({}, std::move(message_handler), std::move(close_handler), settings, gate);
    }

    inline nmos::connection_activation_handler make_connection_events_websocket_activation_handler(nmos::events_ws_message_handler message_handler, const nmos::settings& settings, slog::base_gate& gate)
    {
        return make_connection_events_websocket_activation_handler(std::move(message_handler), {}, settings, gate);
    }

    namespace experimental
    {
        // an events_ws_receiver_event_handler callback indicates a "state" message (event) has been received for the specified (IS-04/IS-05) receiver/connection_receiver
        typedef std::function<void(const nmos::resource& receiver, const nmos::resource& connection_receiver, const web::json::value& message)> events_ws_receiver_event_handler;

        // since each Events WebSocket connection may potentially have subscriptions to a number of sources, for multiple receivers
        // this handler adaptor enables simple processing of "state" messages (events) per receiver
        nmos::events_ws_message_handler make_events_ws_message_handler(const nmos::node_model& model, events_ws_receiver_event_handler event_handler, slog::base_gate& gate);

        // this handler reflects Events WebSocket connection errors into the /active endpoint of all associated receivers by setting master_enable to false
        nmos::events_ws_close_handler make_events_ws_close_handler(nmos::node_model& model, slog::base_gate& gate);
    }
}

#endif

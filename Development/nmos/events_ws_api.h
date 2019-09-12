#ifndef NMOS_EVENTS_WS_API_H
#define NMOS_EVENTS_WS_API_H

#include "nmos/events_resources.h"
#include "nmos/websockets.h"

namespace slog
{
    class base_gate;
}

// Events API websocket implementation
// See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.2.%20Transport%20-%20Websocket.md
namespace nmos
{
    struct node_model;

    web::websockets::experimental::listener::validate_handler make_events_ws_validate_handler(nmos::node_model& model, slog::base_gate& gate);
    web::websockets::experimental::listener::open_handler make_events_ws_open_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    web::websockets::experimental::listener::close_handler make_events_ws_close_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    web::websockets::experimental::listener::message_handler make_events_ws_message_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);

    inline web::websockets::experimental::listener::websocket_listener_handlers make_events_ws_api(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate)
    {
        return{
            nmos::make_events_ws_validate_handler(model, gate),
            nmos::make_events_ws_open_handler(model, websockets, gate),
            nmos::make_events_ws_close_handler(model, websockets, gate),
            nmos::make_events_ws_message_handler(model, websockets, gate)
        };
    }

    // Events messages
    // Maybe these belong in their own file, e.g. nmos/events_messages.h?

    // reboot message
    // see https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.1/docs/2.0.%20Message%20types.md#12-the-reboot-message-type
    web::json::value make_events_reboot_message(const nmos::details::events_state_identity& identity, const nmos::details::events_state_timing& timing = {});

    // shutdown message
    // see https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.1/docs/2.0.%20Message%20types.md#13-the-shutdown-message-type
    web::json::value make_events_shutdown_message(const nmos::details::events_state_identity& identity, const nmos::details::events_state_timing& timing = {});

    // health message
    // see https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/2.0.%20Message%20types.md#15-the-health-message
    web::json::value make_events_health_message(const nmos::details::events_state_timing& timing);

    void send_events_ws_messages_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    void erase_expired_events_resources_thread(nmos::node_model& model, slog::base_gate& gate);
}

#endif

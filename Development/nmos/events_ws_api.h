#ifndef NMOS_EVENTS_WS_API_H
#define NMOS_EVENTS_WS_API_H

#include <boost/bimap.hpp>
#include "cpprest/json.h"
#include "cpprest/ws_listener.h" // for web::websockets::experimental::listener::connection_id, etc.
#include "nmos/id.h"

namespace slog
{
    class base_gate;
}

// Query API websocket implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.2.%20Behaviour%20-%20Querying.md
namespace nmos
{
    struct node_model;
    struct tai;

    typedef boost::bimap<nmos::id, web::websockets::experimental::listener::connection_id> websockets;

    web::websockets::experimental::listener::validate_handler make_eventntally_ws_validate_handler(nmos::node_model& model, slog::base_gate& gate);
    web::websockets::experimental::listener::open_handler make_eventntally_ws_open_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    web::websockets::experimental::listener::close_handler make_eventntally_ws_close_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    web::websockets::experimental::listener::message_handler make_eventntally_ws_message_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);

    void send_eventntally_ws_events_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    void erase_expired_resources_thread(nmos::node_model& model, slog::base_gate& gate_);
}

#endif

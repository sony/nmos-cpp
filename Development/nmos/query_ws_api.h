#ifndef NMOS_QUERY_WS_API_H
#define NMOS_QUERY_WS_API_H

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
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
    struct registry_model;
    struct tai;

    typedef boost::bimaps::bimap<boost::bimaps::unordered_set_of<nmos::id>, web::websockets::experimental::listener::connection_id> websockets;

    web::websockets::experimental::listener::validate_handler make_query_ws_validate_handler(nmos::registry_model& model, slog::base_gate& gate);
    web::websockets::experimental::listener::open_handler make_query_ws_open_handler(const nmos::id& source_id, nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    web::websockets::experimental::listener::close_handler make_query_ws_close_handler(nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate);

    void send_query_ws_events_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate);
}

#endif

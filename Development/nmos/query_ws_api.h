#ifndef NMOS_QUERY_WS_API_H
#define NMOS_QUERY_WS_API_H

#include "nmos/websockets.h"

namespace slog
{
    class base_gate;
}

// Query API websocket implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.2.%20Behaviour%20-%20Querying.md
namespace nmos
{
    struct registry_model;

    web::websockets::experimental::listener::validate_handler make_query_ws_validate_handler(nmos::registry_model& model, slog::base_gate& gate);
    // note, model mutex is assumed to also protect websockets
    web::websockets::experimental::listener::open_handler make_query_ws_open_handler(const nmos::id& source_id, nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    // note, model mutex is assumed to also protect websockets
    web::websockets::experimental::listener::close_handler make_query_ws_close_handler(nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate);

    // note, model mutex is assumed to also protect websockets
    inline web::websockets::experimental::listener::websocket_listener_handlers make_query_ws_api(const nmos::id& source_id, nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate)
    {
        return{
            nmos::make_query_ws_validate_handler(model, gate),
            nmos::make_query_ws_open_handler(source_id, model, websockets, gate),
            nmos::make_query_ws_close_handler(model, websockets, gate),
            {}
        };
    }

    // note, model mutex is assumed to also protect websockets
    void send_query_ws_events_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate);
}

#endif

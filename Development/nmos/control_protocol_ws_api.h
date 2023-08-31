#ifndef NMOS_CONTROL_PROTOCOL_WS_API_H
#define NMOS_CONTROL_PROTOCOL_WS_API_H

#include "nmos/control_protocol_handlers.h"
#include "nmos/websockets.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;

    web::websockets::experimental::listener::validate_handler make_control_protocol_ws_validate_handler(nmos::node_model& model, slog::base_gate& gate);
    web::websockets::experimental::listener::open_handler make_control_protocol_ws_open_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    web::websockets::experimental::listener::close_handler make_control_protocol_ws_close_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate);
    web::websockets::experimental::listener::message_handler make_control_protocol_ws_message_handler(nmos::node_model& model, nmos::websockets& websockets, nmos::get_control_protocol_class_handler get_control_protocol_class, nmos::get_control_protocol_datatype_handler get_control_protocol_datatype, nmos::get_control_protocol_methods_handler get_control_protocol_methods, slog::base_gate& gate);

    inline web::websockets::experimental::listener::websocket_listener_handlers make_control_protocol_ws_api(nmos::node_model& model, nmos::websockets& websockets, nmos::get_control_protocol_class_handler get_control_protocol_class, nmos::get_control_protocol_datatype_handler get_control_protocol_datatype, nmos::get_control_protocol_methods_handler get_control_protocol_methods, slog::base_gate& gate)
    {
        return{
            nmos::make_control_protocol_ws_validate_handler(model, gate),
            nmos::make_control_protocol_ws_open_handler(model, websockets, gate),
            nmos::make_control_protocol_ws_close_handler(model, websockets, gate),
            nmos::make_control_protocol_ws_message_handler(model, websockets, get_control_protocol_class, get_control_protocol_datatype, get_control_protocol_methods, gate)
        };
    }

    void send_control_protocol_ws_messages_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_);
}

#endif

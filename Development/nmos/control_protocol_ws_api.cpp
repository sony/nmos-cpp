#include "nmos/control_protocol_ws_api.h"

#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    // IS-12 Control Protocol WebSocket API

    web::websockets::experimental::listener::validate_handler make_control_protocol_ws_validate_handler(nmos::node_model& model, slog::base_gate& gate_)
    {
        return [&model, &gate_](web::http::http_request req)
        {
            nmos::ws_api_gate gate(gate_, req.request_uri());

            // RFC 6750 defines two methods of sending bearer access tokens which are applicable to WebSocket
            // Clients SHOULD use the "Authorization Request Header Field" method.
            // Clients MAY use a "URI Query Parameter".
            // See https://tools.ietf.org/html/rfc6750#section-2

            // For now just return true
            const auto& ws_ncp_path = req.request_uri().path();
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating websocket connection to: " << ws_ncp_path;

            return true;
        };
    }

    web::websockets::experimental::listener::open_handler make_control_protocol_ws_open_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Opening websocket connection to: " << ws_ncp_path;
            {
                // create a websocket connection resource

                nmos::id id = nmos::make_id();
                websockets.insert({ id, connection_id });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Creating websocket connection: " << id;
            }
        };
    }

    web::websockets::experimental::listener::close_handler make_control_protocol_ws_close_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, web::websockets::websocket_close_status close_status, const utility::string_t& close_reason)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing websocket connection to: " << ws_ncp_path << " [" << (int)close_status << ": " << close_reason << "]";

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Deleting websocket connection";

                websockets.right.erase(websocket);
            }
        };
    }

    web::websockets::experimental::listener::message_handler make_control_protocol_ws_message_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, const web::websockets::websocket_incoming_message& msg_)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.read_lock();

            // theoretically blocking, but in fact not
            auto msg = msg_.extract_string().get();

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received websocket message: " << msg << " on connection to: " << ws_ncp_path;

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                // hmm, todo message handling....
            }
        };
    }
}

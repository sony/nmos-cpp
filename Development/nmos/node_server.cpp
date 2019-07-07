#include "nmos/node_server.h"

#include "cpprest/ws_utils.h"
#include "nmos/api_utils.h"
#include "nmos/events_api.h"
#include "nmos/events_ws_api.h"
#include "nmos/logging_api.h"
#include "nmos/node_api.h"
#include "nmos/node_behaviour.h"
#include "nmos/server.h"
#include "nmos/server_utils.h"
#include "nmos/settings_api.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        // Construct a server instance for an NMOS Node, implementing the IS-04 Node API, IS-05 Connection API, IS-07 Events API
        // and the experimental Logging API and Settings API, according to the specified data models and callbacks
        nmos::server make_node_server(nmos::node_model& node_model, nmos::transport_file_parser parse_transport_file, nmos::details::connection_resource_patch_validator validate_merged, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::experimental::log_model& log_model, slog::base_gate& gate)
        {
            // Log the API addresses we'll be using

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp node with its primary Node API at: " << nmos::get_host(node_model.settings) << ":" << nmos::fields::node_port(node_model.settings);

            nmos::server node_server{ node_model };

            // Set up the APIs, assigning them to the configured ports

            const auto server_secure = nmos::experimental::fields::server_secure(node_model.settings);

            // Configure the Settings API

            const host_port settings_address(nmos::experimental::fields::settings_address(node_model.settings), nmos::experimental::fields::settings_port(node_model.settings));
            node_server.api_routers[settings_address].mount({}, nmos::experimental::make_settings_api(node_model, log_model, gate));

            // Configure the Logging API

            const host_port logging_address(nmos::experimental::fields::logging_address(node_model.settings), nmos::experimental::fields::logging_port(node_model.settings));
            node_server.api_routers[logging_address].mount({}, nmos::experimental::make_logging_api(log_model, gate));

            // Configure the Node API

            nmos::node_api_target_handler target_handler = nmos::make_node_api_target_handler(node_model);
            node_server.api_routers[{ {}, nmos::fields::node_port(node_model.settings) }].mount({}, nmos::make_node_api(node_model, target_handler, gate));

            // Configure the Connection API

            node_server.api_routers[{ {}, nmos::fields::connection_port(node_model.settings) }].mount({}, nmos::make_connection_api(node_model, parse_transport_file, validate_merged, gate));

            // Configure the Events API
            node_server.api_routers[{ {}, nmos::fields::events_port(node_model.settings) }].mount({}, nmos::make_events_api(node_model, gate));

            auto& events_ws_api = node_server.ws_handlers[{ {}, nmos::fields::events_ws_port(node_model.settings) }];
            events_ws_api.first = nmos::make_events_ws_api(node_model, events_ws_api.second, gate);

            // Set up the listeners for each HTTP API port

            auto http_config = nmos::make_http_listener_config(node_model.settings);

            for (auto& api_router : node_server.api_routers)
            {
                // default empty string means the wildcard address
                const auto& host = !api_router.first.first.empty() ? api_router.first.first : web::http::experimental::listener::host_wildcard;
                // map the configured client port to the server port on which to listen
                // hmm, this should probably also take account of the address
                node_server.http_listeners.push_back(nmos::make_api_listener(server_secure, host, nmos::experimental::server_port(api_router.first.second, node_model.settings), api_router.second, http_config, gate));
            }

            // Set up the handlers for each WebSocket API port

            auto websocket_config = nmos::make_websocket_listener_config(node_model.settings);
            websocket_config.set_log_callback(nmos::make_slog_logging_callback(gate));

            for (auto& ws_handler : node_server.ws_handlers)
            {
                // default empty string means the wildcard address
                const auto& host = !ws_handler.first.first.empty() ? ws_handler.first.first : web::websockets::experimental::listener::host_wildcard;
                // map the configured client port to the server port on which to listen
                // hmm, this should probably also take account of the address
                node_server.ws_listeners.push_back(nmos::make_ws_api_listener(server_secure, host, nmos::experimental::server_port(ws_handler.first.second, node_model.settings), ws_handler.second.first, websocket_config, gate));
            }

            auto& events_ws_listener = node_server.ws_listeners.back();

            // Set up node operation (including the mDNS advertisements)

            node_server.thread_functions.assign({
                [&] { nmos::node_behaviour_thread(node_model, gate); },
                [&] { nmos::send_events_ws_messages_thread(events_ws_listener, node_model, events_ws_api.second, gate); },
                [&] { nmos::erase_expired_events_resources_thread(node_model, gate); },
                [&, resolve_auto, set_transportfile] { nmos::connection_activation_thread(node_model, resolve_auto, set_transportfile, gate); }
            });

            return node_server;
        }

        nmos::server make_node_server(nmos::node_model& node_model, nmos::transport_file_parser parse_transport_file, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::experimental::log_model& log_model, slog::base_gate& gate)
        {
            return make_node_server(node_model, parse_transport_file, {}, resolve_auto, set_transportfile, log_model, gate);
        }

        nmos::server make_node_server(nmos::node_model& node_model, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::experimental::log_model& log_model, slog::base_gate& gate)
        {
            return make_node_server(node_model, &nmos::parse_rtp_transport_file, resolve_auto, set_transportfile, log_model, gate);
        }
    }
}

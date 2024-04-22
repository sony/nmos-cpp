#include "nmos/node_server.h"

#include "cpprest/ws_utils.h"
#include "nmos/api_utils.h"
#include "nmos/channelmapping_activation.h"
#include "nmos/configuration_api.h"
#include "nmos/control_protocol_ws_api.h"
#include "nmos/events_api.h"
#include "nmos/events_ws_api.h"
#include "nmos/is04_versions.h"
#include "nmos/logging_api.h"
#include "nmos/manifest_api.h"
#include "nmos/model.h"
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
        // Construct a server instance for an NMOS Node, implementing the IS-04 Node API, IS-05 Connection API, IS-07 Events API, the IS-10 Authorization API
        // and the experimental Logging API and Settings API, according to the specified data models and callbacks
        nmos::server make_node_server(nmos::node_model& node_model, nmos::experimental::node_implementation node_implementation, nmos::experimental::log_model& log_model, slog::base_gate& gate)
        {
            // Log the API addresses we'll be using

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp node with its primary Node API at: "
                << web::uri_builder()
                .set_scheme(nmos::http_scheme(node_model.settings))
                .set_host(nmos::get_host(node_model.settings))
                .set_port(nmos::fields::node_port(node_model.settings))
                .set_path(U("/x-nmos/node/") + nmos::make_api_version(*nmos::is04_versions::from_settings(node_model.settings).rbegin()))
                .to_string();

            nmos::server node_server{ node_model };

            // Set up the APIs, assigning them to the configured ports

            const auto server_secure = nmos::experimental::fields::server_secure(node_model.settings);

            const auto hsts = nmos::experimental::get_hsts(node_model.settings);

            const auto server_address = nmos::experimental::fields::server_address(node_model.settings);

            // Configure the Settings API

            const host_port settings_address(nmos::experimental::fields::settings_address(node_model.settings), nmos::experimental::fields::settings_port(node_model.settings));
            node_server.api_routers[settings_address].mount({}, nmos::experimental::make_settings_api(node_model, log_model, gate));

            // Configure the Logging API

            const host_port logging_address(nmos::experimental::fields::logging_address(node_model.settings), nmos::experimental::fields::logging_port(node_model.settings));
            node_server.api_routers[logging_address].mount({}, nmos::experimental::make_logging_api(log_model, gate));

            // Configure the Node API

            nmos::node_api_target_handler target_handler = nmos::make_node_api_target_handler(node_model, node_implementation.load_ca_certificates, node_implementation.parse_transport_file, node_implementation.validate_staged, node_implementation.get_authorization_bearer_token);
            auto validate_authorization = node_implementation.validate_authorization;
            node_server.api_routers[{ {}, nmos::fields::node_port(node_model.settings) }].mount({}, nmos::make_node_api(node_model, target_handler, validate_authorization ? validate_authorization(nmos::experimental::scopes::node) : nullptr, gate));
            node_server.api_routers[{ {}, nmos::experimental::fields::manifest_port(node_model.settings) }].mount({}, nmos::experimental::make_manifest_api(node_model, gate));

            // Configure the Connection API

            node_server.api_routers[{ {}, nmos::fields::connection_port(node_model.settings) }].mount({}, nmos::make_connection_api(node_model, node_implementation.parse_transport_file, node_implementation.validate_staged, validate_authorization ? validate_authorization(nmos::experimental::scopes::connection) : nullptr, gate));

            // Configure the Events API

            node_server.api_routers[{ {}, nmos::fields::events_port(node_model.settings) }].mount({}, nmos::make_events_api(node_model, validate_authorization ? validate_authorization(nmos::experimental::scopes::events) : nullptr, gate));

            // Configure the Channel Mapping API

            node_server.api_routers[{ {}, nmos::fields::channelmapping_port(node_model.settings) }].mount({}, nmos::make_channelmapping_api(node_model, node_implementation.validate_map, validate_authorization ? validate_authorization(nmos::experimental::scopes::channelmapping) : nullptr, gate));

            // Configure the Configuration API

            node_server.api_routers[{ {}, nmos::fields::configuration_port(node_model.settings) }].mount({}, nmos::make_configuration_api(node_model, validate_authorization ? validate_authorization(nmos::experimental::scopes::configuration) : nullptr, node_implementation.get_control_protocol_class_descriptor, gate));

            const auto& events_ws_port = nmos::fields::events_ws_port(node_model.settings);
            auto& events_ws_api = node_server.ws_handlers[{ {}, nmos::fields::events_ws_port(node_model.settings) }];
            events_ws_api.first = nmos::make_events_ws_api(node_model, events_ws_api.second, node_implementation.ws_validate_authorization, gate);

            // can't share a port between the events ws and the control protocol ws
            const auto& control_protocol_enabled = (0 <= nmos::fields::control_protocol_ws_port(node_model.settings));
            const auto& control_protocol_ws_port = nmos::fields::control_protocol_ws_port(node_model.settings);
            if (control_protocol_enabled)
            {
                if (control_protocol_ws_port == events_ws_port) throw std::runtime_error("Same port used for events and control protocol websockets are not supported");
                auto& control_protocol_ws_api = node_server.ws_handlers[{ {}, control_protocol_ws_port }];
                control_protocol_ws_api.first = nmos::make_control_protocol_ws_api(node_model, control_protocol_ws_api.second, node_implementation.ws_validate_authorization, node_implementation.get_control_protocol_class_descriptor, node_implementation.get_control_protocol_datatype_descriptor, node_implementation.get_control_protocol_method_descriptor, node_implementation.control_protocol_property_changed, gate);
            }

            // Set up the listeners for each HTTP API port

            auto http_config = nmos::make_http_listener_config(node_model.settings, node_implementation.load_server_certificates, node_implementation.load_dh_param, node_implementation.get_ocsp_response, gate);

            for (auto& api_router : node_server.api_routers)
            {
                // if IP address isn't specified for this router, use default server address or wildcard address
                const auto& host = !api_router.first.first.empty() ? api_router.first.first : !server_address.empty() ? server_address : web::http::experimental::listener::host_wildcard;
                // map the configured client port to the server port on which to listen
                // hmm, this should probably also take account of the address
                node_server.http_listeners.push_back(nmos::make_api_listener(server_secure, host, nmos::experimental::server_port(api_router.first.second, node_model.settings), api_router.second, http_config, hsts, gate));
            }

            // Set up the handlers for each WebSocket API port

            auto websocket_config = nmos::make_websocket_listener_config(node_model.settings, node_implementation.load_server_certificates, node_implementation.load_dh_param, node_implementation.get_ocsp_response, gate);
            websocket_config.set_log_callback(nmos::make_slog_logging_callback(gate));

            size_t event_ws_pos{ 0 };
            bool found_event_ws{ false };
            size_t control_protocol_ws_pos{ 0 };
            bool found_control_protocol_ws{ false };
            for (auto& ws_handler : node_server.ws_handlers)
            {
                // if IP address isn't specified for this router, use default server address or wildcard address
                const auto& host = !ws_handler.first.first.empty() ? ws_handler.first.first : !server_address.empty() ? server_address : web::websockets::experimental::listener::host_wildcard;
                // map the configured client port to the server port on which to listen
                // hmm, this should probably also take account of the address
                node_server.ws_listeners.push_back(nmos::make_ws_api_listener(server_secure, host, nmos::experimental::server_port(ws_handler.first.second, node_model.settings), ws_handler.second.first, websocket_config, gate));

                if (!found_event_ws)
                {
                    if (ws_handler.first.second == events_ws_port) { found_event_ws = true; }
                    else { ++event_ws_pos; }
                }

                if (control_protocol_enabled && !found_control_protocol_ws)
                {
                    if (ws_handler.first.second == control_protocol_ws_port) { found_control_protocol_ws = true; }
                    else { ++control_protocol_ws_pos; }
                }
            }

            auto& events_ws_listener = node_server.ws_listeners.at(event_ws_pos);

            // Set up node operation (including the DNS-SD advertisements)

            auto load_ca_certificates = node_implementation.load_ca_certificates;
            auto registration_changed = node_implementation.registration_changed;
            auto resolve_auto = node_implementation.resolve_auto;
            auto set_transportfile = node_implementation.set_transportfile;
            auto connection_activated = node_implementation.connection_activated;
            auto channelmapping_activated = node_implementation.channelmapping_activated;
            auto get_authorization_bearer_token = node_implementation.get_authorization_bearer_token;
            node_server.thread_functions.assign({
                [&, load_ca_certificates, registration_changed, get_authorization_bearer_token] { nmos::node_behaviour_thread(node_model, load_ca_certificates, registration_changed, get_authorization_bearer_token, gate); },
                [&] { nmos::send_events_ws_messages_thread(events_ws_listener, node_model, events_ws_api.second, gate); },
                [&] { nmos::erase_expired_events_resources_thread(node_model, gate); },
                [&, resolve_auto, set_transportfile, connection_activated] { nmos::connection_activation_thread(node_model, resolve_auto, set_transportfile, connection_activated, gate); },
                [&, channelmapping_activated] { nmos::channelmapping_activation_thread(node_model, channelmapping_activated, gate); }
            });

            auto system_changed = node_implementation.system_changed;
            if (system_changed)
            {
                node_server.thread_functions.push_back([&, load_ca_certificates, system_changed] { nmos::node_system_behaviour_thread(node_model, load_ca_certificates, system_changed, gate); });
            }

            if (control_protocol_enabled)
            {
                auto& control_protocol_ws_listener = node_server.ws_listeners.at(control_protocol_ws_pos);
                auto& control_protocol_ws_api = node_server.ws_handlers.at({ {}, control_protocol_ws_port });
                node_server.thread_functions.push_back([&] { nmos::send_control_protocol_ws_messages_thread(control_protocol_ws_listener, node_model, control_protocol_ws_api.second, gate); });
            }

            return node_server;
        }

        // deprecated
        nmos::server make_node_server(nmos::node_model& node_model, nmos::transport_file_parser parse_transport_file, nmos::details::connection_resource_patch_validator validate_merged, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::connection_activation_handler connection_activated, nmos::experimental::log_model& log_model, slog::base_gate& gate)
        {
            return make_node_server(node_model, node_implementation().on_parse_transport_file(std::move(parse_transport_file)).on_validate_merged(std::move(validate_merged)).on_resolve_auto(std::move(resolve_auto)).on_set_transportfile(std::move(set_transportfile)).on_connection_activated(std::move(connection_activated)), log_model, gate);
        }
        // deprecated
        nmos::server make_node_server(nmos::node_model& node_model, nmos::transport_file_parser parse_transport_file, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::connection_activation_handler connection_activated, nmos::experimental::log_model& log_model, slog::base_gate& gate)
        {
            return make_node_server(node_model, node_implementation().on_parse_transport_file(std::move(parse_transport_file)).on_resolve_auto(std::move(resolve_auto)).on_set_transportfile(std::move(set_transportfile)).on_connection_activated(std::move(connection_activated)), log_model, gate);
        }
        // deprecated
        nmos::server make_node_server(nmos::node_model& node_model, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::connection_activation_handler connection_activated, nmos::experimental::log_model& log_model, slog::base_gate& gate)
        {
            return make_node_server(node_model, node_implementation().on_resolve_auto(std::move(resolve_auto)).on_set_transportfile(std::move(set_transportfile)).on_connection_activated(std::move(connection_activated)), log_model, gate);
        }
    }
}

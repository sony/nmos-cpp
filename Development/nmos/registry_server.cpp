#include "nmos/registry_server.h"

#include "cpprest/ws_listener.h"
#include "cpprest/ws_utils.h"
#include "mdns/service_advertiser.h"
#include "nmos/admin_ui.h"
#include "nmos/api_utils.h"
#include "nmos/logging_api.h"
#include "nmos/model.h"
#include "nmos/mdns.h"
#include "nmos/mdns_api.h"
#include "nmos/node_api.h"
#include "nmos/query_api.h"
#include "nmos/query_ws_api.h"
#include "nmos/registration_api.h"
#include "nmos/registry_resources.h"
#include "nmos/schemas_api.h"
#include "nmos/server.h"
#include "nmos/server_utils.h"
#include "nmos/settings_api.h"
#include "nmos/system_api.h"
#include "nmos/system_resources.h"

namespace nmos
{
    void advertise_registry_thread(nmos::registry_model& model, slog::base_gate& gate);

    namespace experimental
    {
        // Construct a server instance for an NMOS Registry instance, implementing the IS-04 Registration and Query APIs, the Node API, the IS-09 System API
        // and the experimental DNS-SD Browsing API, Logging API and Settings API, according to the specified data models
        nmos::server make_registry_server(nmos::registry_model& registry_model, nmos::experimental::registry_implementation registry_implementation, nmos::experimental::log_model& log_model, slog::base_gate& gate)
        {
            // Log the API addresses we'll be using

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Node API at: " << nmos::get_host(registry_model.settings) << ":" << nmos::fields::node_port(registry_model.settings);
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Registration API at: " << nmos::get_host(registry_model.settings) << ":" << nmos::fields::registration_port(registry_model.settings);
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Query API at: " << nmos::get_host(registry_model.settings) << ":" << nmos::fields::query_port(registry_model.settings);

            nmos::server registry_server{ registry_model };

            // Set up the APIs, assigning them to the configured ports

            const auto server_secure = nmos::experimental::fields::server_secure(registry_model.settings);

            // Configure the DNS-SD Browsing API

            const host_port mdns_address(nmos::experimental::fields::mdns_address(registry_model.settings), nmos::experimental::fields::mdns_port(registry_model.settings));
            registry_server.api_routers[mdns_address].mount({}, nmos::experimental::make_mdns_api(registry_model, gate));

            // Configure the Settings API

            const host_port settings_address(nmos::experimental::fields::settings_address(registry_model.settings), nmos::experimental::fields::settings_port(registry_model.settings));
            registry_server.api_routers[settings_address].mount({}, nmos::experimental::make_settings_api(registry_model, log_model, gate));

            // Configure the Logging API

            const host_port logging_address(nmos::experimental::fields::logging_address(registry_model.settings), nmos::experimental::fields::logging_port(registry_model.settings));
            registry_server.api_routers[logging_address].mount({}, nmos::experimental::make_logging_api(log_model, gate));

            // Configure the Query API

            registry_server.api_routers[{ {}, nmos::fields::query_port(registry_model.settings) }].mount({}, nmos::make_query_api(registry_model, gate));

            // "Source ID of the Query API instance issuing the data Grain"
            // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/queryapi-subscriptions-websocket.json
            const nmos::id query_id = nmos::make_repeatable_id(nmos::experimental::fields::seed_id(registry_model.settings), U("/x-nmos/query"));

            auto& query_ws_api = registry_server.ws_handlers[{ {}, nmos::fields::query_ws_port(registry_model.settings) }];
            query_ws_api.first = nmos::make_query_ws_api(query_id, registry_model, query_ws_api.second, gate);

            // Configure the Registration API

            registry_server.api_routers[{ {}, nmos::fields::registration_port(registry_model.settings) }].mount({}, nmos::make_registration_api(registry_model, gate));

            // Configure the Node API

            registry_server.api_routers[{ {}, nmos::fields::node_port(registry_model.settings) }].mount({}, nmos::make_node_api(registry_model, {}, gate));

            // set up the node resources
            auto& self_resources = registry_model.node_resources;
            nmos::experimental::insert_registry_resources(self_resources, registry_model.settings);

            // add the self resources to the Registration API resources
            // (for now just copy them directly, since these resources currently do not change and are configured to never expire)
            registry_model.registry_resources.insert(self_resources.begin(), self_resources.end());

            // Configure the System API

            // set up the system global configuration resource
            nmos::experimental::assign_system_global_resource(registry_model.system_global_resource, registry_model.settings);

            registry_server.api_routers[{ {}, nmos::fields::system_port(registry_model.settings) }].mount({}, nmos::make_system_api(registry_model, gate));

            // Configure the Schemas API

            const host_port schemas_address(nmos::experimental::fields::schemas_address(registry_model.settings), nmos::experimental::fields::schemas_port(registry_model.settings));
            registry_server.api_routers[schemas_address].mount({}, nmos::experimental::make_schemas_api(gate));

            // Configure the Admin UI

            const utility::string_t admin_filesystem_root = U("./admin");
            const host_port admin_address(nmos::experimental::fields::admin_address(registry_model.settings), nmos::experimental::fields::admin_port(registry_model.settings));
            registry_server.api_routers[admin_address].mount({}, nmos::experimental::make_admin_ui(admin_filesystem_root, gate));

            // Set up the listeners for each HTTP API port

            auto http_config = nmos::make_http_listener_config(registry_model.settings, registry_implementation.load_server_certificates, registry_implementation.load_dh_param, gate);

            for (auto& api_router : registry_server.api_routers)
            {
                // default empty string means the wildcard address
                const auto& host = !api_router.first.first.empty() ? api_router.first.first : web::http::experimental::listener::host_wildcard;
                // map the configured client port to the server port on which to listen
                // hmm, this should probably also take account of the address
                registry_server.http_listeners.push_back(nmos::make_api_listener(server_secure, host, nmos::experimental::server_port(api_router.first.second, registry_model.settings), api_router.second, http_config, gate));
            }

            // Set up the handlers for each WebSocket API port

            auto websocket_config = nmos::make_websocket_listener_config(registry_model.settings, registry_implementation.load_server_certificates, registry_implementation.load_dh_param, gate);
            websocket_config.set_log_callback(nmos::make_slog_logging_callback(gate));

            for (auto& ws_handler : registry_server.ws_handlers)
            {
                // default empty string means the wildcard address
                const auto& host = !ws_handler.first.first.empty() ? ws_handler.first.first : web::websockets::experimental::listener::host_wildcard;
                // map the configured client port to the server port on which to listen
                // hmm, this should probably also take account of the address
                registry_server.ws_listeners.push_back(nmos::make_ws_api_listener(server_secure, host, nmos::experimental::server_port(ws_handler.first.second, registry_model.settings), ws_handler.second.first, websocket_config, gate));
            }

            auto& query_ws_listener = registry_server.ws_listeners.back();

            // Set up registry management (including the DNS-SD advertisements)

            registry_server.thread_functions.assign({
                [&] { nmos::send_query_ws_events_thread(query_ws_listener, registry_model, query_ws_api.second, gate); },
                [&] { nmos::erase_expired_resources_thread(registry_model, gate); },
                [&] { nmos::advertise_registry_thread(registry_model, gate); }
            });

            return registry_server;
        }

        // deprecated
        nmos::server make_registry_server(nmos::registry_model& registry_model, nmos::experimental::log_model& log_model, slog::base_gate& gate)
        {
            return make_registry_server(registry_model, registry_implementation(), log_model, gate);
        }
    }

    void advertise_registry_thread(nmos::registry_model& model, slog::base_gate& gate)
    {
        auto lock = model.read_lock();

        // Configure the DNS-SD advertisements for our APIs

        mdns::service_advertiser advertiser(gate);
        mdns::service_advertiser_guard advertiser_guard(advertiser);
        const auto pri = nmos::fields::pri(model.settings);
        if (nmos::service_priorities::no_priority != pri) // no_priority allows the registry to run unadvertised
        {
            nmos::experimental::register_addresses(advertiser, model.settings);
            nmos::experimental::register_service(advertiser, nmos::service_types::query, model.settings);
            nmos::experimental::register_service(advertiser, nmos::service_types::registration, model.settings);
            nmos::experimental::register_service(advertiser, nmos::service_types::node, model.settings);
            nmos::experimental::register_service(advertiser, nmos::service_types::system, model.settings);
        }

        // wait for the thread to be interrupted because the server is being shut down
        model.shutdown_condition.wait(lock, [&] { return model.shutdown; });
    }
}

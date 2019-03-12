#include <fstream>
#include <iostream>
#include "cpprest/ws_listener.h"
#include "mdns/service_advertiser.h"
#include "nmos/admin_ui.h"
#include "nmos/api_utils.h"
#include "nmos/log_gate.h"
#include "nmos/logging_api.h"
#include "nmos/model.h"
#include "nmos/mdns.h"
#include "nmos/mdns_api.h"
#include "nmos/node_api.h"
#include "nmos/process_utils.h"
#include "nmos/query_api.h"
#include "nmos/query_ws_api.h"
#include "nmos/registration_api.h"
#include "nmos/registry_resources.h"
#include "nmos/server_utils.h"
#include "nmos/settings_api.h"
#include "nmos/system_api.h"
#include "nmos/system_resources.h"
#include "nmos/thread_utils.h"

int main(int argc, char* argv[])
{
    // Construct our data models including mutexes to protect them

    nmos::registry_model registry_model;

    nmos::experimental::log_model log_model;

    // Streams for logging, initially configured to write errors to stderr and to discard the access log
    std::filebuf error_log_buf;
    std::ostream error_log(std::cerr.rdbuf());
    std::filebuf access_log_buf;
    std::ostream access_log(&access_log_buf);

    // Logging should all go through this logging gateway
    nmos::experimental::log_gate gate(error_log, access_log, log_model);

    try
    {
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting nmos-cpp registry";

        // Settings can be passed on the command-line, directly or in a configuration file, and a few may be changed dynamically by PATCH to /settings/all on the Settings API
        //
        // * "logging_level": integer value, between 40 (least verbose, only fatal messages) and -40 (most verbose)
        //
        // E.g.
        //
        // # ./nmos-cpp-registry "{\"logging_level\":-40}"
        // # ./nmos-cpp-registry config.json
        // # curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -d "{\"logging_level\":-40}"
        // # curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -T config.json

        if (argc > 1)
        {
            std::error_code error;
            registry_model.settings = web::json::value::parse(utility::s2us(argv[1]), error);
            if (error)
            {
                std::ifstream file(argv[1]);
                registry_model.settings = web::json::value::parse(file, error);
            }
            if (error || !registry_model.settings.is_object())
            {
                slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Bad command-line settings [" << error << "]";
                return -1;
            }
        }

        // Prepare run-time default settings (different than header defaults)

        nmos::insert_registry_default_settings(registry_model.settings);

        // copy to the logging settings
        // hmm, this is a bit icky, but simplest for now
        log_model.settings = registry_model.settings;

        // the logging level is a special case because we want to turn it into an atomic value
        // that can be read by logging statements without locking the mutex protecting the settings
        log_model.level = nmos::fields::logging_level(log_model.settings);

        // Reconfigure the logging streams according to settings
        // (obviously, until this point, the logging gateway has its default behaviour...)

        if (!nmos::fields::error_log(registry_model.settings).empty())
        {
            error_log_buf.open(nmos::fields::error_log(registry_model.settings), std::ios_base::out | std::ios_base::app);
            auto lock = log_model.write_lock();
            error_log.rdbuf(&error_log_buf);
        }

        if (!nmos::fields::access_log(registry_model.settings).empty())
        {
            access_log_buf.open(nmos::fields::access_log(registry_model.settings), std::ios_base::out | std::ios_base::app);
            auto lock = log_model.write_lock();
            access_log.rdbuf(&access_log_buf);
        }

        // Log the process ID and the API addresses we'll be using

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Process ID: " << nmos::details::get_process_id();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Initial settings: " << registry_model.settings.serialize();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Node API at: " << nmos::get_host(registry_model.settings) << ":" << nmos::fields::node_port(registry_model.settings);
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Registration API at: " << nmos::get_host(registry_model.settings) << ":" << nmos::fields::registration_port(registry_model.settings);
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Query API at: " << nmos::get_host(registry_model.settings) << ":" << nmos::fields::query_port(registry_model.settings);

        // Set up the APIs, assigning them to the configured ports

        typedef std::pair<utility::string_t, int> address_port;
        std::map<address_port, web::http::experimental::listener::api_router> port_routers;

        // Configure the DNS-SD Browsing API

        const address_port mdns_address(nmos::experimental::fields::mdns_address(registry_model.settings), nmos::experimental::fields::mdns_port(registry_model.settings));
        port_routers[mdns_address].mount({}, nmos::experimental::make_mdns_api(registry_model, gate));

        // Configure the Settings API

        const address_port settings_address(nmos::experimental::fields::settings_address(registry_model.settings), nmos::experimental::fields::settings_port(registry_model.settings));
        port_routers[settings_address].mount({}, nmos::experimental::make_settings_api(registry_model, log_model, gate));

        // Configure the Logging API

        const address_port logging_address(nmos::experimental::fields::logging_address(registry_model.settings), nmos::experimental::fields::logging_port(registry_model.settings));
        port_routers[logging_address].mount({}, nmos::experimental::make_logging_api(log_model, gate));

        // Configure the Query API

        port_routers[{ {}, nmos::fields::query_port(registry_model.settings) }].mount({}, nmos::make_query_api(registry_model, gate));

        // "Source ID of the Query API instance issuing the data Grain"
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/queryapi-subscriptions-websocket.json
        const nmos::id query_id = nmos::make_repeatable_id(nmos::experimental::fields::seed_id(registry_model.settings), U("/x-nmos/query"));

        nmos::websockets registry_websockets;

        auto websocket_config = nmos::make_websocket_listener_config(registry_model.settings);
        websocket_config.set_log_callback(nmos::make_slog_logging_callback(gate));
        web::websockets::experimental::listener::validate_handler query_ws_validate_handler = nmos::make_query_ws_validate_handler(registry_model, gate);
        web::websockets::experimental::listener::open_handler query_ws_open_handler = nmos::make_query_ws_open_handler(query_id, registry_model, registry_websockets, gate);
        web::websockets::experimental::listener::close_handler query_ws_close_handler = nmos::make_query_ws_close_handler(registry_model, registry_websockets, gate);
        auto query_ws_uri = web::websockets::experimental::listener::make_listener_uri(web::websockets::experimental::listener::host_wildcard, nmos::experimental::server_port(nmos::fields::query_ws_port(registry_model.settings), registry_model.settings));
        web::websockets::experimental::listener::websocket_listener query_ws_listener(query_ws_uri, websocket_config);
        query_ws_listener.set_validate_handler(std::ref(query_ws_validate_handler));
        query_ws_listener.set_open_handler(std::ref(query_ws_open_handler));
        query_ws_listener.set_close_handler(std::ref(query_ws_close_handler));

        // Configure the Registration API

        port_routers[{ {}, nmos::fields::registration_port(registry_model.settings) }].mount({}, nmos::make_registration_api(registry_model, gate));

        // Configure the Node API

        port_routers[{ {}, nmos::fields::node_port(registry_model.settings) }].mount({}, nmos::make_node_api(registry_model, {}, gate));

        // set up the node resources
        auto& self_resources = registry_model.node_resources;
        nmos::experimental::insert_registry_resources(self_resources, registry_model.settings);

        // add the self resources to the Registration API resources
        // (for now just copy them directly, since these resources currently do not change and are configured to never expire)
        registry_model.registry_resources.insert(self_resources.begin(), self_resources.end());

        // Configure the System API

        // set up the system global configuration resource
        nmos::experimental::assign_system_global_resource(registry_model.system_global_resource, registry_model.settings);

        port_routers[{ {}, nmos::fields::system_port(registry_model.settings) }].mount({}, nmos::make_system_api(registry_model, gate));

        // Configure the Admin UI

        const utility::string_t admin_filesystem_root = U("./admin");
        const address_port admin_address(nmos::experimental::fields::admin_address(registry_model.settings), nmos::experimental::fields::admin_port(registry_model.settings));
        port_routers[admin_address].mount({}, nmos::experimental::make_admin_ui(admin_filesystem_root, gate));

        // Set up the listeners for each API port

        auto http_config = nmos::make_http_listener_config(registry_model.settings);

        std::vector<web::http::experimental::listener::http_listener> port_listeners;
        for (auto& port_router : port_routers)
        {
            // default empty string means the wildcard address
            const auto& router_address = !port_router.first.first.empty() ? port_router.first.first : web::http::experimental::listener::host_wildcard;
            // map the configured client port to the server port on which to listen
            // hmm, this should probably also take account of the address
            port_listeners.push_back(nmos::make_api_listener(router_address, nmos::experimental::server_port(port_router.first.second, registry_model.settings), port_router.second, http_config, gate));
        }

        // Start up registry management before any NMOS APIs are open

        auto send_query_ws_events = nmos::details::make_thread_guard([&] { nmos::send_query_ws_events_thread(query_ws_listener, registry_model, registry_websockets, gate); }, [&] { registry_model.controlled_shutdown(); });
        auto erase_expired_resources = nmos::details::make_thread_guard([&] { nmos::erase_expired_resources_thread(registry_model, gate); }, [&] { registry_model.controlled_shutdown(); });

        // Open the API ports

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing for connections";

        std::vector<web::http::experimental::listener::http_listener_guard> port_guards;
        for (auto& port_listener : port_listeners)
        {
            if (0 <= port_listener.uri().port()) port_guards.push_back({ port_listener });
        }
        web::websockets::experimental::listener::websocket_listener_guard query_ws_guard;
        if (0 <= query_ws_listener.uri().port()) query_ws_guard = { query_ws_listener };

        // Configure the mDNS advertisements for our APIs

        mdns::service_advertiser advertiser(gate);
        mdns::service_advertiser_guard advertiser_guard(advertiser);
        const auto pri = nmos::fields::pri(registry_model.settings);
        if (nmos::service_priorities::no_priority != pri) // no_priority allows the registry to run unadvertised
        {
            nmos::experimental::register_service(advertiser, nmos::service_types::query, registry_model.settings);
            nmos::experimental::register_service(advertiser, nmos::service_types::registration, registry_model.settings);
            nmos::experimental::register_service(advertiser, nmos::service_types::node, registry_model.settings);
            nmos::experimental::register_service(advertiser, nmos::service_types::system, registry_model.settings);
        }

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Ready for connections";

        // Wait for a process termination signal
        nmos::details::wait_term_signal();

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing connections";
    }
    catch (const web::json::json_exception& e)
    {
        // most likely from incorrect types in the command line settings
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();
    }
    catch (const web::http::http_exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "HTTP error: " << e.what() << " [" << e.error_code() << "]";
    }
    catch (const web::websockets::experimental::listener::websocket_exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "WebSocket error: " << e.what() << " [" << e.error_code() << "]";
    }
    catch (const std::system_error& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "System error: " << e.what() << " [" << e.code() << "]";
    }
    catch (const std::runtime_error& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error: " << e.what();
    }
    catch (const std::exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what();
    }
    catch (...)
    {
        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception";
    }

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Stopping nmos-cpp registry";

    return 0;
}

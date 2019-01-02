#include <fstream>
#include <iostream>
#include "cpprest/host_utils.h"
#include "cpprest/ws_listener.h"
#include "mdns/service_advertiser.h"
#include "nmos/admin_ui.h"
#include "nmos/api_utils.h"
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
#include "nmos/settings_api.h"
#include "nmos/thread_utils.h"
#include "main_gate.h"

int main(int argc, char* argv[])
{
    // Construct our data models including mutexes to protect them

    nmos::registry_model registry_model;

    nmos::experimental::log_model log_model;
    std::atomic<slog::severity> level{ slog::severities::more_info };

    // Streams for logging, initially configured to write errors to stderr and to discard the access log
    std::filebuf error_log_buf;
    std::ostream error_log(std::cerr.rdbuf());
    std::filebuf access_log_buf;
    std::ostream access_log(&access_log_buf);

    // Logging should all go through this logging gateway
    main_gate gate(error_log, access_log, log_model, level);

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

        web::json::insert(registry_model.settings, std::make_pair(nmos::experimental::fields::seed_id, web::json::value::string(nmos::make_id())));

        web::json::insert(registry_model.settings, std::make_pair(nmos::fields::logging_level, web::json::value::number(level)));
        level = nmos::fields::logging_level(registry_model.settings); // synchronize atomic value with settings

        // if the "host_addresses" setting was omitted, add all the interface addresses
        const auto interface_addresses = web::http::experimental::interface_addresses();
        if (!interface_addresses.empty())
        {
            web::json::insert(registry_model.settings, std::make_pair(nmos::fields::host_addresses, web::json::value_from_elements(interface_addresses)));
        }

        // if the "host_address" setting was omitted, use the first of the "host_addresses"
        if (registry_model.settings.has_field(nmos::fields::host_addresses))
        {
            web::json::insert(registry_model.settings, std::make_pair(nmos::fields::host_address, nmos::fields::host_addresses(registry_model.settings)[0]));
        }

        // if any of the specific "<api>_port" settings were omitted, use "http_port" if present
        if (registry_model.settings.has_field(nmos::fields::http_port))
        {
            const auto http_port = nmos::fields::http_port(registry_model.settings);
            web::json::insert(registry_model.settings, std::make_pair(nmos::fields::query_port, http_port));
            // can't share a port between an http_listener and a websocket_listener, so don't apply this one...
            //web::json::insert(registry_model.settings, std::make_pair(nmos::fields::query_ws_port, http_port));
            web::json::insert(registry_model.settings, std::make_pair(nmos::fields::registration_port, http_port));
            web::json::insert(registry_model.settings, std::make_pair(nmos::fields::node_port, http_port));
            web::json::insert(registry_model.settings, std::make_pair(nmos::experimental::fields::settings_port, http_port));
            web::json::insert(registry_model.settings, std::make_pair(nmos::experimental::fields::logging_port, http_port));
            web::json::insert(registry_model.settings, std::make_pair(nmos::experimental::fields::admin_port, http_port));
            web::json::insert(registry_model.settings, std::make_pair(nmos::experimental::fields::mdns_port, http_port));
        }

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
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Node API at: " << nmos::fields::host_address(registry_model.settings) << ":" << nmos::fields::node_port(registry_model.settings);
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Registration API at: " << nmos::fields::host_address(registry_model.settings) << ":" << nmos::fields::registration_port(registry_model.settings);
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Query API at: " << nmos::fields::host_address(registry_model.settings) << ":" << nmos::fields::query_port(registry_model.settings);

        // Set up the APIs, assigning them to the configured ports

        std::map<int, web::http::experimental::listener::api_router> port_routers;

        // Configure the DNS-SD Browsing API

        port_routers[nmos::experimental::fields::mdns_port(registry_model.settings)].mount({}, nmos::experimental::make_mdns_api(registry_model, gate));

        // Configure the Settings API

        port_routers[nmos::experimental::fields::settings_port(registry_model.settings)].mount({}, nmos::experimental::make_settings_api(registry_model, level, gate));

        // Configure the Logging API

        port_routers[nmos::experimental::fields::logging_port(registry_model.settings)].mount({}, nmos::experimental::make_logging_api(log_model, gate));

        // Configure the Query API

        port_routers[nmos::fields::query_port(registry_model.settings)].mount({}, nmos::make_query_api(registry_model, gate));

        // "Source ID of the Query API instance issuing the data Grain"
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/queryapi-subscriptions-websocket.json
        const nmos::id query_id = nmos::make_repeatable_id(nmos::experimental::fields::seed_id(registry_model.settings), U("/x-nmos/query"));

        nmos::websockets registry_websockets;

        web::websockets::experimental::listener::validate_handler query_ws_validate_handler = nmos::make_query_ws_validate_handler(registry_model, gate);
        web::websockets::experimental::listener::open_handler query_ws_open_handler = nmos::make_query_ws_open_handler(query_id, registry_model, registry_websockets, gate);
        web::websockets::experimental::listener::close_handler query_ws_close_handler = nmos::make_query_ws_close_handler(registry_model, registry_websockets, gate);
        web::websockets::experimental::listener::websocket_listener query_ws_listener(nmos::fields::query_ws_port(registry_model.settings), nmos::make_slog_logging_callback(gate));
        query_ws_listener.set_validate_handler(std::ref(query_ws_validate_handler));
        query_ws_listener.set_open_handler(std::ref(query_ws_open_handler));
        query_ws_listener.set_close_handler(std::ref(query_ws_close_handler));

        // Configure the Registration API

        port_routers[nmos::fields::registration_port(registry_model.settings)].mount({}, nmos::make_registration_api(registry_model, gate));

        // Configure the Node API

        port_routers[nmos::fields::node_port(registry_model.settings)].mount({}, nmos::make_node_api(registry_model, {}, gate));

        // set up the node resources
        auto& self_resources = registry_model.node_resources;
        nmos::experimental::insert_registry_resources(self_resources, registry_model.settings);

        // add the self resources to the Registration API resources
        // (for now just copy them directly, since these resources currently do not change and are configured to never expire)
        registry_model.registry_resources.insert(self_resources.begin(), self_resources.end());

        // Configure the Admin UI

        const utility::string_t admin_filesystem_root = U("./admin");
        port_routers[nmos::experimental::fields::admin_port(registry_model.settings)].mount({}, nmos::experimental::make_admin_ui(admin_filesystem_root, gate));

        // Set up the listeners for each API port

        // try to use the configured TCP listen backlog
        web::http::experimental::listener::http_listener_config listener_config;
        listener_config.set_backlog(nmos::fields::listen_backlog(registry_model.settings));

        std::vector<web::http::experimental::listener::http_listener> port_listeners;
        for (auto& port_router : port_routers) port_listeners.push_back(nmos::make_api_listener(port_router.first, port_router.second, listener_config, gate));

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
        if (0 <= query_ws_listener.port()) query_ws_guard = { query_ws_listener };

        // Configure the mDNS advertisements for our APIs

        mdns::service_advertiser advertiser(gate);
        mdns::service_advertiser_guard advertiser_guard(advertiser);
        const auto pri = nmos::fields::pri(registry_model.settings);
        if (nmos::service_priorities::no_priority != pri) // no_priority allows the registry to run unadvertised
        {
            nmos::experimental::register_service(advertiser, nmos::service_types::query, registry_model.settings, nmos::make_txt_records(nmos::service_types::query, pri));
            nmos::experimental::register_service(advertiser, nmos::service_types::registration, registry_model.settings, nmos::make_txt_records(nmos::service_types::registration, pri));
            nmos::experimental::register_service(advertiser, nmos::service_types::node, registry_model.settings, nmos::make_txt_records(nmos::service_types::node));
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
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "WebSockets error: " << e.what() << " [" << e.error_code() << "]";
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

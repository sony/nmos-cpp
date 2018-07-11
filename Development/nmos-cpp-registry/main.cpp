#include <iostream>
#include "cpprest/host_utils.h"
#include "cpprest/ws_listener.h"
#include "mdns/service_advertiser.h"
#include "nmos/api_utils.h"
#include "nmos/admin_ui.h"
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
    // Construct our data models and mutexes to protect each of them
    // plus variables to signal when the server is stopping

    bool shutdown{ false };
    nmos::condition_variable shutdown_condition; // associated with registry_mutex; notify on shutdown

    nmos::resources self_resources;
    nmos::mutex self_mutex;

    nmos::model registry_model;
    nmos::mutex registry_mutex;
    nmos::condition_variable registry_condition; // associated with registry_mutex; notify on any change to registry_model, and on shutdown

    nmos::experimental::log_model log_model;
    nmos::mutex log_mutex;
    std::atomic<slog::severity> level{ slog::severities::more_info };

    // Streams for logging, initially configured to write errors to stderr and to discard the access log
    std::filebuf error_log_buf;
    std::ostream error_log(std::cerr.rdbuf());
    std::filebuf access_log_buf;
    std::ostream access_log(&access_log_buf);

    // Logging should all go through this logging gateway
    main_gate gate(error_log, access_log, log_model, log_mutex, level);

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting nmos-cpp registry";

    // Settings can be passed on the command-line, and some may be changed dynamically by POST to /settings/all on the Settings API
    //
    // * "logging_level": integer value, between 40 (least verbose, only fatal messages) and -40 (most verbose)
    //
    // E.g.
    //
    // # nmos-cpp-registry.exe "{\"logging_level\":-40}"
    // # curl -H "Content-Type: application/json" http://localhost:3209/settings/all -d "{\"logging_level\":-40}"
    //
    // In either case, omitted settings will assume their defaults (invisibly, currently)

    if (argc > 1)
    {
        std::error_code error;
        registry_model.settings = web::json::value::parse(utility::s2us(argv[1]), error);
        if (error || !registry_model.settings.is_object())
        {
            registry_model.settings = web::json::value::null();
            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Bad command-line settings [" << error << "]";
        }
    }

    // Prepare run-time default settings (different than header defaults)

    web::json::insert(registry_model.settings, std::make_pair(nmos::experimental::fields::seed_id, web::json::value::string(nmos::make_id())));

    web::json::insert(registry_model.settings, std::make_pair(nmos::fields::logging_level, web::json::value::number(level)));
    level = nmos::fields::logging_level(registry_model.settings); // synchronize atomic value with settings

    web::json::insert(registry_model.settings, std::make_pair(nmos::fields::host_name, web::json::value::string(web::http::experimental::host_name())));

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

    // Reconfigure the logging streams according to settings

    if (!nmos::fields::error_log(registry_model.settings).empty())
    {
        error_log_buf.open(nmos::fields::error_log(registry_model.settings), std::ios_base::out | std::ios_base::ate);
        nmos::write_lock lock(log_mutex);
        error_log.rdbuf(&error_log_buf);
    }

    if (!nmos::fields::access_log(registry_model.settings).empty())
    {
        access_log_buf.open(nmos::fields::access_log(registry_model.settings), std::ios_base::out | std::ios_base::ate);
        nmos::write_lock lock(log_mutex);
        access_log.rdbuf(&access_log_buf);
    }

    // Log the process ID and the API addresses we'll be using

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Process ID: " << nmos::details::get_process_id();
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Initial settings: " << registry_model.settings.serialize();
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Node API at: " << nmos::fields::host_address(registry_model.settings) << ":" << nmos::fields::node_port(registry_model.settings);
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Registration API at: " << nmos::fields::host_address(registry_model.settings) << ":" << nmos::fields::registration_port(registry_model.settings);
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp registry with its primary Query API at: " << nmos::fields::host_address(registry_model.settings) << ":" << nmos::fields::query_port(registry_model.settings);

    // Configure the mDNS Service Discovery API

    web::http::experimental::listener::api_router mdns_api = nmos::experimental::make_mdns_api(registry_model.settings, registry_mutex, gate);
    web::http::experimental::listener::http_listener mdns_listener(web::http::experimental::listener::make_listener_uri(nmos::experimental::fields::mdns_port(registry_model.settings)));
    nmos::support_api(mdns_listener, mdns_api);

    // Configure the Settings API

    web::http::experimental::listener::api_router settings_api = nmos::experimental::make_settings_api(registry_model.settings, level, registry_mutex, registry_condition, gate);
    web::http::experimental::listener::http_listener settings_listener(web::http::experimental::listener::make_listener_uri(nmos::experimental::fields::settings_port(registry_model.settings)));
    nmos::support_api(settings_listener, settings_api);

    // Configure the Logging API

    web::http::experimental::listener::api_router logging_api = nmos::experimental::make_logging_api(log_model, log_mutex, gate);
    web::http::experimental::listener::http_listener logging_listener(web::http::experimental::listener::make_listener_uri(nmos::experimental::fields::logging_port(registry_model.settings)));
    nmos::support_api(logging_listener, logging_api);

    // Configure the NMOS APIs

    web::http::experimental::listener::http_listener_config listener_config;
    listener_config.set_backlog(nmos::fields::listen_backlog(registry_model.settings));

    // Configure the Query API

    web::http::experimental::listener::api_router query_api = nmos::make_query_api(registry_model, registry_mutex, gate);
    web::http::experimental::listener::http_listener query_listener(web::http::experimental::listener::make_listener_uri(nmos::fields::query_port(registry_model.settings)), listener_config);
    nmos::support_api(query_listener, query_api);

    // "Source ID of the Query API instance issuing the data Grain"
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/queryapi-subscriptions-websocket.json
    const nmos::id query_id = nmos::make_repeatable_id(nmos::experimental::fields::seed_id(registry_model.settings), U("/x-nmos/query"));

    nmos::websockets registry_websockets;

    web::websockets::experimental::listener::validate_handler query_ws_validate_handler = nmos::make_query_ws_validate_handler(registry_model, registry_mutex, gate);
    web::websockets::experimental::listener::open_handler query_ws_open_handler = nmos::make_query_ws_open_handler(query_id, registry_model, registry_websockets, registry_mutex, registry_condition, gate);
    web::websockets::experimental::listener::close_handler query_ws_close_handler = nmos::make_query_ws_close_handler(registry_model, registry_websockets, registry_mutex, gate);
    web::websockets::experimental::listener::websocket_listener query_ws_listener(nmos::fields::query_ws_port(registry_model.settings), nmos::make_slog_logging_callback(gate));
    query_ws_listener.set_validate_handler(std::ref(query_ws_validate_handler));
    query_ws_listener.set_open_handler(std::ref(query_ws_open_handler));
    query_ws_listener.set_close_handler(std::ref(query_ws_close_handler));

    // Configure the Registration API

    web::http::experimental::listener::api_router registration_api = nmos::make_registration_api(registry_model, registry_mutex, registry_condition, gate);
    web::http::experimental::listener::http_listener registration_listener(web::http::experimental::listener::make_listener_uri(nmos::fields::registration_port(registry_model.settings)), listener_config);
    nmos::support_api(registration_listener, registration_api);

    // Configure the Node API

    web::http::experimental::listener::api_router node_api = nmos::make_node_api(self_resources, self_mutex, gate);
    web::http::experimental::listener::http_listener node_listener(web::http::experimental::listener::make_listener_uri(nmos::fields::node_port(registry_model.settings)), listener_config);
    nmos::support_api(node_listener, node_api);

    // set up the node resources
    nmos::experimental::insert_registry_resources(self_resources, registry_model.settings);

    // add the self resources to the registration API resources
    // (for now just copy them directly, since these resources currently do not change and are configured to never expire)
    registry_model.resources.insert(self_resources.begin(), self_resources.end());

    // Configure the Admin UI

    const utility::string_t admin_filesystem_root = U("./admin");
    web::http::experimental::listener::api_router admin_ui = nmos::experimental::make_admin_ui(admin_filesystem_root, gate);
    web::http::experimental::listener::http_listener admin_listener(web::http::experimental::listener::make_listener_uri(nmos::experimental::fields::admin_port(registry_model.settings)));
    nmos::support_api(admin_listener, admin_ui);

    // Configure the mDNS advertisements for our APIs

    std::unique_ptr<mdns::service_advertiser> advertiser = mdns::make_advertiser(gate);
    const auto pri = nmos::fields::pri(registry_model.settings);
    if (nmos::service_priorities::no_priority != pri) // no_priority allows the registry to run unadvertised
    {
        nmos::experimental::register_service(*advertiser, nmos::service_types::query, registry_model.settings, nmos::make_txt_records(nmos::service_types::query, pri));
        nmos::experimental::register_service(*advertiser, nmos::service_types::registration, registry_model.settings, nmos::make_txt_records(nmos::service_types::registration, pri));
        nmos::experimental::register_service(*advertiser, nmos::service_types::node, registry_model.settings, nmos::make_txt_records(nmos::service_types::node));
    }

    try
    {
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing for connections";

        // start up registry management before any NMOS APIs are open

        auto send_query_ws_events = nmos::details::make_thread_guard([&] { nmos::send_query_ws_events_thread(query_ws_listener, registry_model, registry_websockets, shutdown, registry_mutex, registry_condition, gate); }, [&] { nmos::read_lock lock(registry_mutex); shutdown = true; registry_condition.notify_all(); });
        auto erase_expired_resources = nmos::details::make_thread_guard([&] { nmos::erase_expired_resources_thread(registry_model, shutdown, registry_mutex, shutdown_condition, registry_condition, gate); }, [&] { nmos::write_lock lock(registry_mutex); shutdown = true; shutdown_condition.notify_all(); });

        // open in an order that means NMOS APIs don't expose references to others that aren't open yet

        web::http::experimental::listener::http_listener_guard logging_guard(logging_listener);
        web::http::experimental::listener::http_listener_guard settings_guard(settings_listener);

        web::http::experimental::listener::http_listener_guard node_guard(node_listener);
        web::websockets::experimental::listener::websocket_listener_guard query_ws_guard(query_ws_listener);
        web::http::experimental::listener::http_listener_guard query_guard(query_listener);
        web::http::experimental::listener::http_listener_guard registration_guard(registration_listener);

        web::http::experimental::listener::http_listener_guard admin_guard(admin_listener);

        web::http::experimental::listener::http_listener_guard mdns_guard(mdns_listener);

        mdns::service_advertiser_guard advertiser_guard(*advertiser);

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Ready for connections";

        // Wait for a process termination signal
        nmos::details::wait_term_signal();

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing connections";
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

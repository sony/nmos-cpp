#include <iostream>
#include "cpprest/host_utils.h"
#include "mdns/service_advertiser.h"
#include "mdns/service_discovery.h"
#include "nmos/admin_ui.h"
#include "nmos/api_utils.h"
#include "nmos/connection_api.h"
#include "nmos/logging_api.h"
#include "nmos/mdns.h"
#include "nmos/model.h"
#include "nmos/node_api.h"
#include "nmos/node_resources.h"
#include "nmos/node_registration.h"
#include "nmos/process_utils.h"
#include "nmos/settings_api.h"
#include "nmos/thread_utils.h"
#include "main_gate.h"

int main(int argc, char* argv[])
{
    // Construct our data models and mutexes to protect each of them
    // plus variables to signal when the server is stopping

    nmos::model node_model;
    nmos::mutex node_mutex;

    nmos::experimental::log_model log_model;
    nmos::mutex log_mutex;
    std::atomic<slog::severity> level{ slog::severities::more_info };

    bool shutdown = false;

    // Streams for logging, initially configured to write errors to stderr and to discard the access log
    std::filebuf error_log_buf;
    std::ostream error_log(std::cerr.rdbuf());
    std::filebuf access_log_buf;
    std::ostream access_log(&access_log_buf);

    // Logging should all go through this logging gateway
    main_gate gate(error_log, access_log, log_model, log_mutex, level);

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting nmos-cpp node";

    // Settings can be passed on the command-line, and some may be changed dynamically by POST to /settings/all on the Settings API
    //
    // * "logging_level": integer value, between 40 (least verbose, only fatal messages) and -40 (most verbose)
    // * "registry_address": string value, e.g. "127.0.0.1" (instead of using mDNS service discovery)
    //
    // E.g.
    //
    // # nmos-cpp-node.exe "{\"logging_level\":-40}"
    // # curl -H "Content-Type: application/json" http://localhost:3209/settings/all -d "{\"logging_level\":-40}"
    //
    // In either case, omitted settings will assume their defaults (invisibly, currently)

    if (argc > 1)
    {
        std::error_code error;
        node_model.settings = web::json::value::parse(utility::s2us(argv[1]), error);
        if (error || !node_model.settings.is_object())
        {
            node_model.settings = web::json::value::null();
            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Bad command-line settings [" << error << "]";
        }
    }

    // Prepare run-time default settings (different than header defaults)

    web::json::insert(node_model.settings, std::make_pair(nmos::fields::logging_level, web::json::value::number(level)));
    level = nmos::fields::logging_level(node_model.settings); // synchronize atomic value with settings
    web::json::insert(node_model.settings, std::make_pair(nmos::fields::host_name, web::json::value::string(web::http::experimental::host_name())));
    const auto host_addresses = web::http::experimental::host_addresses(nmos::fields::host_name(node_model.settings));
    if (!host_addresses.empty())
    {
        web::json::insert(node_model.settings, std::make_pair(nmos::fields::host_address, web::json::value::string(host_addresses[0])));
    }
    web::json::insert(node_model.settings, std::make_pair(nmos::fields::registry_address, nmos::fields::host_address(node_model.settings)));

    // Reconfigure the logging streams according to settings

    if (!nmos::fields::error_log(node_model.settings).empty())
    {
        error_log_buf.open(nmos::fields::error_log(node_model.settings), std::ios_base::out | std::ios_base::ate);
        nmos::write_lock lock(log_mutex);
        error_log.rdbuf(&error_log_buf);
    }

    if (!nmos::fields::access_log(node_model.settings).empty())
    {
        access_log_buf.open(nmos::fields::access_log(node_model.settings), std::ios_base::out | std::ios_base::ate);
        nmos::write_lock lock(log_mutex);
        access_log.rdbuf(&access_log_buf);
    }

    // Discover a Registration API

    std::unique_ptr<mdns::service_discovery> discovery = mdns::make_discovery(gate);
    auto registration_services = nmos::experimental::resolve_service(*discovery, nmos::service_types::registration);

    if (!registration_services.empty())
    {
        auto registration_uri = registration_services.begin()->second;
        node_model.settings[nmos::fields::registry_address] = web::json::value::string(registration_uri.host());
        node_model.settings[nmos::fields::registration_port] = registration_uri.port();
        node_model.settings[nmos::fields::registry_version] = web::json::value::string(web::uri::split_path(registration_uri.path()).back());
    }
    else
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Did not discover a suitable Registration API via mDNS";
    }

    // Log the process ID and the API addresses we'll be using

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Process ID: " << nmos::details::get_process_id();
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Initial settings: " << node_model.settings.serialize();
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp node with its Node API at: " << nmos::fields::host_address(node_model.settings) << ":" << nmos::fields::node_port(node_model.settings);
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Registering nmos-cpp node with the Registration API at: " << nmos::fields::registry_address(node_model.settings) << ":" << nmos::fields::registration_port(node_model.settings);

    // Configure the Settings API

    web::http::experimental::listener::api_router settings_api = nmos::experimental::make_settings_api(node_model.settings, node_mutex, level, gate);
    web::http::experimental::listener::http_listener settings_listener(web::http::experimental::listener::make_listener_uri(nmos::experimental::fields::settings_port(node_model.settings)));
    nmos::support_api(settings_listener, settings_api);

    // Configure the Logging API

    web::http::experimental::listener::api_router logging_api = nmos::experimental::make_logging_api(log_model, log_mutex, gate);
    web::http::experimental::listener::http_listener logging_listener(web::http::experimental::listener::make_listener_uri(nmos::experimental::fields::logging_port(node_model.settings)));
    nmos::support_api(logging_listener, logging_api);

    // Configure the NMOS APIs

    web::http::experimental::listener::http_listener_config listener_config;
    listener_config.set_backlog(nmos::fields::listen_backlog(node_model.settings));

    // Configure the Node API

    web::http::experimental::listener::api_router node_api = nmos::make_node_api(node_model.resources, node_mutex, gate);
    web::http::experimental::listener::http_listener node_listener(web::http::experimental::listener::make_listener_uri(nmos::fields::node_port(node_model.settings)), listener_config);
    nmos::support_api(node_listener, node_api);

    // set up the node resources
    nmos::experimental::make_node_resources(node_model.resources, node_model.settings);

    nmos::condition_variable node_model_condition; // associated with node_mutex; notify on any change to node_model, and on shutdown

    // Configure the Connection API

    web::http::experimental::listener::api_router connection_api = nmos::make_connection_api(node_model.resources, node_mutex, gate);
    web::http::experimental::listener::http_listener connection_listener(web::http::experimental::listener::make_listener_uri(nmos::fields::connection_port(node_model.settings)), listener_config);
    nmos::support_api(connection_listener, connection_api);

    // Configure the mDNS advertisements for our APIs

    std::unique_ptr<mdns::service_advertiser> advertiser = mdns::make_advertiser(gate);
    const auto pri = nmos::fields::pri(node_model.settings);
    if (nmos::service_priorities::no_priority != pri) // no_priority allows the node to run unadvertised
    {
        const auto records = nmos::make_txt_records(pri);
        nmos::experimental::register_service(*advertiser, nmos::service_types::node, node_model.settings, records);
    }

    try
    {
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing for connections";

        // open in an order that means NMOS APIs don't expose references to others that aren't open yet

        web::http::experimental::listener::http_listener_guard logging_guard(logging_listener);
        web::http::experimental::listener::http_listener_guard settings_guard(settings_listener);

        web::http::experimental::listener::http_listener_guard node_guard(node_listener);
        web::http::experimental::listener::http_listener_guard connection_guard(connection_listener);

        mdns::service_advertiser_guard advertiser_guard(*advertiser);

        // start up communication with the registry once all NMOS APIs are open

        auto node_registration = nmos::details::make_thread_guard([&] { nmos::node_registration_thread(node_model, node_mutex, node_model_condition, shutdown, gate); }, [&] { shutdown = true; node_model_condition.notify_all(); });
        auto node_heartbeat = nmos::details::make_thread_guard([&] { nmos::node_registration_heartbeat_thread(node_model, node_mutex, node_model_condition, shutdown, gate); }, [&] { shutdown = true; node_model_condition.notify_all(); });

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Ready for connections";

        // Wait for a process termination signal
        nmos::details::wait_term_signal();

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing connections";
    }
    catch (const web::http::http_exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << e.what() << " [" << e.error_code() << "]";
    }
    catch (const std::system_error& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << e.what() << " [" << e.code() << "]";
    }
    catch (const std::exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what();
    }
    catch (...)
    {
        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception";
    }

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Stopping nmos-cpp node";

    return 0;
}

#include <iostream>
#include "cpprest/host_utils.h"
#include "nmos/admin_ui.h"
#include "nmos/api_utils.h"
#include "nmos/connection_api.h"
#include "nmos/logging_api.h"
#include "nmos/model.h"
#include "nmos/node_api.h"
#include "nmos/node_behaviour.h"
#include "nmos/node_resources.h"
#include "nmos/process_utils.h"
#include "nmos/settings_api.h"
#include "nmos/thread_utils.h"
#include "main_gate.h"

int main(int argc, char* argv[])
{
    // Construct our data models and mutexes to protect each of them
    // plus variables to signal when the server is stopping

    bool shutdown{ false };

    nmos::model node_model;
    nmos::mutex node_mutex;
    nmos::condition_variable node_condition; // associated with node_mutex; notify on any change to node_model, and on shutdown

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

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting nmos-cpp node";

    // Settings can be passed on the command-line, and some may be changed dynamically by POST to /settings/all on the Settings API
    //
    // * "logging_level": integer value, between 40 (least verbose, only fatal messages) and -40 (most verbose)
    // * "registry_address": used to construct request URLs for registry APIs (if not discovered via DNS-SD)
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

    web::json::insert(node_model.settings, std::make_pair(nmos::experimental::fields::seed_id, web::json::value::string(nmos::make_id())));

    web::json::insert(node_model.settings, std::make_pair(nmos::fields::logging_level, web::json::value::number(level)));
    level = nmos::fields::logging_level(node_model.settings); // synchronize atomic value with settings

    // if the "host_addresses" setting was omitted, add all the interface addresses
    const auto interface_addresses = web::http::experimental::interface_addresses();
    if (!interface_addresses.empty())
    {
        web::json::insert(node_model.settings, std::make_pair(nmos::fields::host_addresses, web::json::value_from_elements(interface_addresses)));
    }

    // if the "host_address" setting was omitted, use the first of the "host_addresses"
    if (node_model.settings.has_field(nmos::fields::host_addresses))
    {
        web::json::insert(node_model.settings, std::make_pair(nmos::fields::host_address, nmos::fields::host_addresses(node_model.settings)[0]));
    }

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

    // Log the process ID and the API addresses we'll be using

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Process ID: " << nmos::details::get_process_id();
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Initial settings: " << node_model.settings.serialize();
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp node with its primary Node API at: " << nmos::fields::host_address(node_model.settings) << ":" << nmos::fields::node_port(node_model.settings);

    // Configure the Settings API

    web::http::experimental::listener::api_router settings_api = nmos::experimental::make_settings_api(node_model.settings, level, node_mutex, node_condition, gate);
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
    nmos::experimental::insert_node_resources(node_model.resources, node_model.settings);

    // Configure the Connection API

    web::http::experimental::listener::api_router connection_api = nmos::make_connection_api(node_model.resources, node_mutex, node_condition, gate);
    web::http::experimental::listener::http_listener connection_listener(web::http::experimental::listener::make_listener_uri(nmos::fields::connection_port(node_model.settings)), listener_config);
    nmos::support_api(connection_listener, connection_api);

    try
    {
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing for connections";

        // open in an order that means NMOS APIs don't expose references to others that aren't open yet

        web::http::experimental::listener::http_listener_guard logging_guard(logging_listener);
        web::http::experimental::listener::http_listener_guard settings_guard(settings_listener);

        web::http::experimental::listener::http_listener_guard node_guard(node_listener);
        web::http::experimental::listener::http_listener_guard connection_guard(connection_listener);

        // start up node operation (including the mDNS advertisements) once all NMOS APIs are open

        auto node_behaviour = nmos::details::make_thread_guard([&] { nmos::node_behaviour_thread(node_model, shutdown, node_mutex, node_condition, gate); }, [&] { nmos::write_lock lock(node_mutex); shutdown = true; node_condition.notify_all(); });

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Ready for connections";

        // Wait for a process termination signal
        nmos::details::wait_term_signal();

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing connections";
    }
    catch (const web::http::http_exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "HTTP error: " << e.what() << " [" << e.error_code() << "]";
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

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Stopping nmos-cpp node";

    return 0;
}

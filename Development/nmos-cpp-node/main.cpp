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
#include "nmos/settings_api.h"
#include "main_gate.h"

int main(int argc, char* argv[])
{
    // Construct our data models and mutexes to protect each of them
    // plus variables to signal when the server is stopping

    nmos::model node_model;
    std::mutex node_mutex;

    nmos::experimental::log_model log_model;
    std::mutex log_mutex;
    std::atomic<slog::severity> level = slog::severities::more_info;

    bool shutdown = false;

    // Logging should all go through this logging gateway
    main_gate gate(log_model, log_mutex, level);

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting nmos-cpp node";

    // Settings can be passed on the command-line, and changed dynamically by POST to /settings/all on the Settings API
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
        else
        {
            // Logging level is a special case (see nmos/settings_api.h)
            level = nmos::fields::logging_level(node_model.settings);
        }
    }

    if (node_model.settings.is_null())
    {
        // Prepare initial settings (different than defaults)
        node_model.settings = web::json::value::object();
        node_model.settings[nmos::fields::logging_level] = web::json::value::number(level);
        node_model.settings[nmos::fields::host_name] = web::json::value::string(web::http::experimental::host_name());
        node_model.settings[nmos::fields::host_address] = web::json::value::string(web::http::experimental::host_addresses(web::http::experimental::host_name())[0]);
    }

    // Discover a Registration API

    std::unique_ptr<mdns::service_discovery> discovery = mdns::make_discovery(gate);
    web::uri registration_uri = nmos::mdns::experimental::resolve_service(*discovery, nmos::mdns::services::registration);

    if (!registration_uri.is_empty())
    {
        node_model.settings[nmos::fields::registry_address] = web::json::value::string(registration_uri.host());
        node_model.settings[nmos::fields::registration_port] = registration_uri.port();
        node_model.settings[nmos::fields::registry_version] = web::json::value::string(web::uri::split_path(registration_uri.path()).back());
    }
    else
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Did not discover a suitable Registration API via mDNS";
    }

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

    // Configure the Node API

    web::http::experimental::listener::api_router node_api = nmos::make_node_api(node_model.resources, node_mutex, gate);
    web::http::experimental::listener::http_listener node_listener(web::http::experimental::listener::make_listener_uri(nmos::fields::node_port(node_model.settings)));
    nmos::support_api(node_listener, node_api);

    // set up the node resources
    nmos::experimental::make_node_resources(node_model.resources, node_model.settings);

    std::condition_variable node_model_condition; // associated with node_mutex; notify on any change to node_model, and on shutdown
    std::thread node_registration([&] { nmos::node_registration_thread(node_model, node_mutex, node_model_condition, shutdown, gate); });
    std::thread node_heartbeat([&] { nmos::node_registration_heartbeat_thread(node_model, node_mutex, node_model_condition, shutdown, gate); });

    // Configure the Connection API

    web::http::experimental::listener::api_router connection_api = nmos::make_connection_api(node_model.resources, node_mutex, gate);
    web::http::experimental::listener::http_listener connection_listener(web::http::experimental::listener::make_listener_uri(nmos::fields::connection_port(node_model.settings)));
    nmos::support_api(connection_listener, connection_api);

    // Configure the mDNS advertisements for our APIs
    
    std::unique_ptr<mdns::service_advertiser> advertiser = mdns::make_advertiser(gate);
    nmos::mdns::experimental::register_service(*advertiser, nmos::mdns::services::node, node_model.settings);

    // Advertise our APIs

    advertiser->start();  

    try
    {
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing for connections";

        // open in an order that means NMOS APIs don't expose references to others that aren't open yet

        logging_listener.open().wait();
        settings_listener.open().wait();

        node_listener.open().wait();
        connection_listener.open().wait();

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Ready for connections";

        std::string command;
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            std::cout << "Press return to quit." << std::endl;
        }
        std::cin >> std::noskipws >> command;

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing connections";

        // close in reverse order

        connection_listener.close().wait();
        node_listener.close().wait();

        settings_listener.close().wait();
        logging_listener.close().wait();
    }
    catch (const web::http::http_exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << e.what() << " [" << e.error_code() << "]";
    }

    advertiser->stop();

    shutdown = true;
    node_model_condition.notify_all();
    node_heartbeat.join();
    node_registration.join();

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Stopping nmos-cpp node";

    return 0;
}

#include <fstream>
#include <iostream>
#include "nmos/log_gate.h"
#include "nmos/model.h"
#include "nmos/node_server.h"
#include "nmos/ocsp_behaviour.h"
#include "nmos/ocsp_response_handler.h"
#include "nmos/ocsp_state.h"
#include "nmos/process_utils.h"
#include "nmos/server.h"
#include "node_implementation.h"

int main(int argc, char* argv[])
{
    // Construct our data models including mutexes to protect them

    nmos::node_model node_model;

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
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting nmos-cpp node";

        // Settings can be passed on the command-line, directly or in a configuration file, and a few may be changed dynamically by PATCH to /settings/all on the Settings API
        //
        // * "logging_level": integer value, between 40 (least verbose, only fatal messages) and -40 (most verbose)
        // * "registry_address": used to construct request URLs for registry APIs (if not discovered via DNS-SD)
        //
        // E.g.
        //
        // # ./nmos-cpp-node "{\"logging_level\":-40}"
        // # ./nmos-cpp-node config.json
        // # curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -d "{\"logging_level\":-40}"
        // # curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -T config.json

        if (argc > 1)
        {
            std::error_code error;
            node_model.settings = web::json::value::parse(utility::s2us(argv[1]), error);
            if (error)
            {
                std::ifstream file(argv[1]);
                // check the file can be opened, and is parsed to an object
                file.exceptions(std::ios_base::failbit);
                node_model.settings = web::json::value::parse(file);
                node_model.settings.as_object();
            }
        }

        // Prepare run-time default settings (different than header defaults)

        nmos::insert_node_default_settings(node_model.settings);

        // copy to the logging settings
        // hmm, this is a bit icky, but simplest for now
        log_model.settings = node_model.settings;

        // the logging level is a special case because we want to turn it into an atomic value
        // that can be read by logging statements without locking the mutex protecting the settings
        log_model.level = nmos::fields::logging_level(log_model.settings);

        // Reconfigure the logging streams according to settings
        // (obviously, until this point, the logging gateway has its default behaviour...)

        if (!nmos::fields::error_log(node_model.settings).empty())
        {
            error_log_buf.open(nmos::fields::error_log(node_model.settings), std::ios_base::out | std::ios_base::app);
            auto lock = log_model.write_lock();
            error_log.rdbuf(&error_log_buf);
        }

        if (!nmos::fields::access_log(node_model.settings).empty())
        {
            access_log_buf.open(nmos::fields::access_log(node_model.settings), std::ios_base::out | std::ios_base::app);
            auto lock = log_model.write_lock();
            access_log.rdbuf(&access_log_buf);
        }

        // Log the process ID and initial settings

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Process ID: " << nmos::details::get_process_id();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Build settings: " << nmos::get_build_settings_info();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Initial settings: " << node_model.settings.serialize();

        // Set up the callbacks between the node server and the underlying implementation

        auto node_implementation = make_node_implementation(node_model, gate);

// only implement communication with OCSP server if http_listener supports OCSP stapling
// cf. preprocessor conditions in nmos::make_http_listener_config
// Note: the get_ocsp_response callback must be set up before executing the make_node_server where make_http_listener_config is set up
#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
        nmos::experimental::ocsp_state ocsp_state;
        if (nmos::experimental::fields::server_secure(node_model.settings))
        {
            node_implementation.on_get_ocsp_response(nmos::make_ocsp_response_handler(ocsp_state, gate));
        }
#endif

        // Set up the node server

        auto node_server = nmos::experimental::make_node_server(node_model, node_implementation, log_model, gate);

        if (!nmos::experimental::fields::http_trace(node_model.settings))
        {
            // Disable TRACE method

            for (auto& http_listener : node_server.http_listeners)
            {
                http_listener.support(web::http::methods::TRCE, [](web::http::http_request req) { req.reply(web::http::status_codes::MethodNotAllowed); });
            }
        }

        // Add the underlying implementation, which will set up the node resources, etc.

        node_server.thread_functions.push_back([&] { node_implementation_thread(node_model, gate); });

// only implement communication with OCSP server if http_listener supports OCSP stapling
// cf. preprocessor conditions in nmos::make_http_listener_config
#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
        if (nmos::experimental::fields::server_secure(node_model.settings))
        {
            auto load_ca_certificates = node_implementation.load_ca_certificates;
            auto load_server_certificates = node_implementation.load_server_certificates;
            node_server.thread_functions.push_back([&, load_ca_certificates, load_server_certificates] { nmos::ocsp_behaviour_thread(node_model, ocsp_state, load_ca_certificates, load_server_certificates, gate); });
        }
#endif

        // Open the API ports and start up node operation (including the DNS-SD advertisements)

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing for connections";

        nmos::server_guard node_server_guard(node_server);

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Ready for connections";

        // Wait for a process termination signal
        nmos::details::wait_term_signal();

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing connections";
    }
    catch (const web::json::json_exception& e)
    {
        // most likely from incorrect syntax or incorrect value types in the command line settings
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();
        return 1;
    }
    catch (const web::http::http_exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "HTTP error: " << e.what() << " [" << e.error_code() << "]";
        return 1;
    }
    catch (const web::websockets::websocket_exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "WebSocket error: " << e.what() << " [" << e.error_code() << "]";
        return 1;
    }
    catch (const std::ios_base::failure& e)
    {
        // most likely from failing to open the command line settings file
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "File error: " << e.what();
        return 1;
    }
    catch (const std::system_error& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "System error: " << e.what() << " [" << e.code() << "]";
        return 1;
    }
    catch (const std::runtime_error& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error: " << e.what();
        return 1;
    }
    catch (const std::exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what();
        return 1;
    }
    catch (...)
    {
        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception";
        return 1;
    }

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Stopping nmos-cpp node";

    return 0;
}

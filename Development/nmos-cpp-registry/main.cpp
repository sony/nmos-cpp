#include <fstream>
#include <iostream>
#include "nmos/log_gate.h"
#include "nmos/model.h"
#include "nmos/process_utils.h"
#include "nmos/registry_server.h"
#include "nmos/server.h"
#include "registry_implementation.h"

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

        // Log the process ID and initial settings

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Process ID: " << nmos::details::get_process_id();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Build settings: " << nmos::get_build_settings_info();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Initial settings: " << registry_model.settings.serialize();

        // Set up the callbacks between the registry server and the underlying implementation

        auto registry_implementation = make_registry_implementation(registry_model, gate);

        // Set up the registry server

        auto registry_server = nmos::experimental::make_registry_server(registry_model, registry_implementation, log_model, gate);

        if (!nmos::experimental::fields::http_trace(registry_model.settings))
        {
            // Disable TRACE method

            for (auto& http_listener : registry_server.http_listeners)
            {
                http_listener.support(web::http::methods::TRCE, [](web::http::http_request req) { req.reply(web::http::status_codes::MethodNotAllowed); });
            }
        }

        // Open the API ports and start up registry management

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing for connections";

        nmos::server_guard registry_server_guard(registry_server);

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
    catch (const web::websockets::websocket_exception& e)
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

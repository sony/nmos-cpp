#include <fstream>
#include <iostream>
#include "cpprest/grant_type.h"
#include "cpprest/token_endpoint_auth_method.h"
#include "nmos/api_utils.h" // for make_api_listener
#include "nmos/authorization_behaviour.h"
#include "nmos/authorization_redirect_api.h"
#include "nmos/authorization_state.h"
#include "nmos/jwks_uri_api.h"
#include "nmos/log_gate.h"
#include "nmos/model.h"
#include "nmos/node_server.h"
#include "nmos/ocsp_behaviour.h"
#include "nmos/ocsp_response_handler.h"
#include "nmos/ocsp_state.h"
#include "nmos/process_utils.h"
#include "nmos/server.h"
#include "nmos/server_utils.h" // for make_http_listener_config
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

        // only configure communication with Authorization server if IS-10/BCP-003-02 is required
        // Note:
        // the validate_authorization callback must be set up before executing the make_node_server where make_node_api, make_connection_api, make_events_api, and make_channelmapping_api are set up
        // the ws_validate_authorization callback must be set up before executing the make_node_server where make_events_ws_validate_handler is set up
        // the get_authorization_bearer_token callback must be set up before executing the make_node_server where make_http_client_config is set up
        nmos::experimental::authorization_state authorization_state;
        if (nmos::experimental::fields::server_authorization(node_model.settings))
        {
            node_implementation
                .on_validate_authorization(nmos::experimental::make_validate_authorization_handler(node_model, authorization_state, nmos::experimental::make_validate_authorization_token_handler(authorization_state, gate), gate))
                .on_ws_validate_authorization(nmos::experimental::make_ws_validate_authorization_handler(node_model, authorization_state, nmos::experimental::make_validate_authorization_token_handler(authorization_state, gate), gate));
        }
        if (nmos::experimental::fields::client_authorization(node_model.settings))
        {
            node_implementation
                .on_get_authorization_bearer_token(nmos::experimental::make_get_authorization_bearer_token_handler(authorization_state, gate))
                .on_load_authorization_clients(nmos::experimental::make_load_authorization_clients_handler(node_model.settings, gate))
                .on_save_authorization_client(nmos::experimental::make_save_authorization_client_handler(node_model.settings, gate))
                .on_load_rsa_private_keys(nmos::make_load_rsa_private_keys_handler(node_model.settings, gate)) // may be omitted, only required for OAuth client which is using Private Key JWT as the requested authentication method for the token endpoint
                .on_request_authorization_code(nmos::experimental::make_request_authorization_code_handler(gate)); // may be omitted, only required for OAuth client which is using the Authorization Code Flow to obtain the access token
        }

        // Set up the node server

        auto node_server = nmos::experimental::make_node_server(node_model, node_implementation, log_model, gate);

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

        // only configure communication with Authorization server if IS-10/BCP-003-02 is required
        if (nmos::experimental::fields::client_authorization(node_model.settings))
        {
            std::map<nmos::host_port, web::http::experimental::listener::api_router> api_routers;

            // Configure the authorization_redirect API (require for Authorization Code Flow support)

            if (web::http::oauth2::experimental::grant_types::authorization_code.name == nmos::experimental::fields::authorization_flow(node_model.settings))
            {
                auto load_ca_certificates = node_implementation.load_ca_certificates;
                auto load_rsa_private_keys = node_implementation.load_rsa_private_keys;
                api_routers[{ {}, nmos::experimental::fields::authorization_redirect_port(node_model.settings) }].mount({}, nmos::experimental::make_authorization_redirect_api(node_model, authorization_state, load_ca_certificates, load_rsa_private_keys, gate));
            }

            // Configure the jwks_uri API (require for Private Key JWK support)

            if (web::http::oauth2::experimental::token_endpoint_auth_methods::private_key_jwt.name == nmos::experimental::fields::token_endpoint_auth_method(node_model.settings))
            {
                auto load_rsa_private_keys = node_implementation.load_rsa_private_keys;
                api_routers[{ {}, nmos::experimental::fields::jwks_uri_port(node_model.settings) }].mount({}, nmos::experimental::make_jwk_uri_api(node_model, load_rsa_private_keys, gate));
            }

            auto http_config = nmos::make_http_listener_config(node_model.settings, node_implementation.load_server_certificates, node_implementation.load_dh_param, node_implementation.get_ocsp_response, gate);
            const auto server_secure = nmos::experimental::fields::server_secure(node_model.settings);
            const auto hsts = nmos::experimental::get_hsts(node_model.settings);
            for (auto& api_router : api_routers)
            {
                auto found = node_server.api_routers.find(api_router.first);

                const auto& host = !api_router.first.first.empty() ? api_router.first.first : web::http::experimental::listener::host_wildcard;
                const auto& port = nmos::experimental::server_port(api_router.first.second, node_model.settings);

                if (node_server.api_routers.end() != found)
                {
                    const auto uri = web::http::experimental::listener::make_listener_uri(server_secure, host, port);
                    auto listener = std::find_if(node_server.http_listeners.begin(), node_server.http_listeners.end(), [&](const web::http::experimental::listener::http_listener& listener) { return listener.uri() == uri; });
                    if (node_server.http_listeners.end() != listener)
                    {
                        found->second.pop_back(); // remove the api_finally_handler which was previously added in the make_node_server, the api_finally_handler will be re-inserted in the make_api_listener
                        node_server.http_listeners.erase(listener);
                    }
                    found->second.mount({}, api_router.second);
                    node_server.http_listeners.push_back(nmos::make_api_listener(server_secure, host, port, found->second, http_config, hsts, gate));
                }
                else
                {
                    node_server.http_listeners.push_back(nmos::make_api_listener(server_secure, host, port, api_router.second, http_config, hsts, gate));
                }
            }
        }

        if (!nmos::experimental::fields::http_trace(node_model.settings))
        {
            // Disable TRACE method

            for (auto& http_listener : node_server.http_listeners)
            {
                http_listener.support(web::http::methods::TRCE, [](web::http::http_request req) { req.reply(web::http::status_codes::MethodNotAllowed); });
            }
        }

        // only configure communication with Authorization server if IS-10/BCP-003-02 is required
        if (nmos::experimental::fields::client_authorization(node_model.settings) || nmos::experimental::fields::server_authorization(node_model.settings))
        {
            // IS-10 client registration, fetch access token, and fetch authorization server token public key
            // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.2._Behaviour_-_Clients.html
            // and https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.5._Behaviour_-_Resource_Servers.html#public-keys
            auto load_ca_certificates = node_implementation.load_ca_certificates;
            auto load_rsa_private_keys = node_implementation.load_rsa_private_keys;
            auto load_authorization_clients = node_implementation.load_authorization_clients;
            auto save_authorization_client = node_implementation.save_authorization_client;
            auto request_authorization_code = node_implementation.request_authorization_code;
            node_server.thread_functions.push_back([&, load_ca_certificates, load_rsa_private_keys, load_authorization_clients, save_authorization_client, request_authorization_code] { nmos::experimental::authorization_behaviour_thread(node_model, authorization_state, load_ca_certificates, load_rsa_private_keys, load_authorization_clients, save_authorization_client, request_authorization_code, gate); });

            if (nmos::experimental::fields::server_authorization(node_model.settings))
            {
                // When no matching public key for a given access token, it SHOULD attempt to obtain the missing public key
                // via the the token iss claim as specified in RFC 8414 section 3.
                // see https://tools.ietf.org/html/rfc8414#section-3
                // and https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.5._Behaviour_-_Resource_Servers.html#public-keys
                node_server.thread_functions.push_back([&, load_ca_certificates] { nmos::experimental::authorization_token_issuer_thread(node_model, authorization_state, load_ca_certificates, gate); });
            }
        }

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

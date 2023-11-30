#include "nmos/authorization_handlers.h"

#include <jwt-cpp/jwt.h>
#include "cpprest/basic_utils.h"
#include "cpprest/json_validator.h"
#include "cpprest/response_type.h"
#include "nmos/api_utils.h"  // for nmos::experimental::details::make_validate_authorization_handler
#include "nmos/authorization_state.h"
#include "nmos/is10_versions.h"
#include "nmos/json_schema.h"
#include "nmos/json_fields.h"
#include "nmos/slog.h"
#if defined(_WIN32) && !defined(__cplusplus_winrt)
#include <Windows.h>
#include <shellapi.h>
#endif

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            static const web::json::experimental::json_validator& auth_clients_schema_validator()
            {
                static const web::json::experimental::json_validator validator
                {
                    nmos::experimental::load_json_schema,
                    boost::copy_range<std::vector<web::uri>>(is10_versions::all | boost::adaptors::transformed(experimental::make_authapi_register_client_response_uri))
                };

                return validator;
            }
        }

        // helper function to load the authorization clients file
        // example of the file
        //  [
        //    {
        //      "authorization_server_uri": "https://example.com"
        //    },
        //    {
        //      "client_metadata": {
        //        "client_id": "acc8fd35-327d-4486-a02f-9a8fdc25a609",
        //        "client_name" : "example client",
        //        "grant_types" : [ "authorization_code", "client_credentials","refresh_token" ],
        //        "jwks_uri" : "https://example_client/jwks",
        //        "redirect_uris" : [ "https://example_client/callback" ],
        //        "registration_access_token" : "eyJhbGci....",
        //        "registration_client_uri" : "https://example.com/openid-connect/acc8fd35-327d-4486-a02f-9a8fdc25a609",
        //        "response_types" : [ "code" ],
        //        "scope" : "registration",
        //        "subject_type" : "public",
        //        "tls_client_certificate_bound_access_tokens" : false,
        //        "token_endpoint_auth_method" : "private_key_jwt"
        //      }
        //    }
        //  ]
        web::json::value load_authorization_clients_file(const utility::string_t& filename, slog::base_gate& gate)
        {
            using web::json::value;

            try
            {
                utility::ifstream_t is(filename);
                if (is.is_open())
                {
                    const auto authorization_clients = value::parse(is);

                    if (!authorization_clients.is_null() && authorization_clients.is_array() && authorization_clients.as_array().size())
                    {
                        for (auto& authorization_client : authorization_clients.as_array())
                        {
                            if (authorization_client.has_field(nmos::experimental::fields::authorization_server_uri) &&
                                !authorization_client.at(nmos::experimental::fields::authorization_server_uri).as_string().empty() &&
                                authorization_client.has_field(nmos::experimental::fields::client_metadata))
                            {
                                // validate client metadata
                                const auto& client_metadata = authorization_client.at(nmos::experimental::fields::client_metadata);
                                details::auth_clients_schema_validator().validate(client_metadata, experimental::make_authapi_register_client_response_uri(is10_versions::v1_0)); // may throw json_exception
                            }
                        }
                    }

                    return authorization_clients;
                }
            }
            catch (const web::json::json_exception& e)
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Unable to load authorization clients from non-volatile memory: " << filename << ": " << e.what();
            }
            return web::json::value::array();
        }

        // helper function to update the authorization clients file
        // example of authorization_client
        //  {
        //    {
        //      "authorization_server_uri": "https://example.com"
        //    },
        //    {
        //      "client_metadata": {
        //        "client_id": "acc8fd35-327d-4486-a02f-9a8fdc25a609",
        //        "client_name" : "example client",
        //        "grant_types" : [ "authorization_code", "client_credentials","refresh_token" ],
        //        "issuer" : "https://example.com",
        //        "jwks_uri" : "https://example_client/jwks",
        //        "redirect_uris" : [ "https://example_client/callback" ],
        //        "registration_access_token" : "eyJhbGci....",
        //        "registration_client_uri" : "https://example.com/openid-connect/acc8fd35-327d-4486-a02f-9a8fdc25a609",
        //        "response_types" : [ "code" ],
        //        "scope" : "registration",
        //        "subject_type" : "public",
        //        "tls_client_certificate_bound_access_tokens" : false,
        //        "token_endpoint_auth_method" : "private_key_jwt"
        //      }
        //    }
        //  }
        void update_authorization_clients_file(const utility::string_t& filename, const web::json::value& authorization_client, slog::base_gate& gate)
        {
            // load authorization_clients from file
            auto authorization_clients = load_authorization_clients_file(filename, gate);

            // update/append to the authorization_clients
            bool updated{ false };
            if (authorization_clients.as_array().size())
            {
                for (auto& auth_client : authorization_clients.as_array())
                {
                    const auto& authorization_server_uri = auth_client.at(nmos::experimental::fields::authorization_server_uri);
                    if (authorization_server_uri == authorization_client.at(nmos::experimental::fields::authorization_server_uri))
                    {
                        auth_client[nmos::experimental::fields::client_metadata] = authorization_client.at(nmos::experimental::fields::client_metadata);
                        updated = true;
                        break;
                    }
                }
            }
            if (!updated)
            {
                web::json::push_back(authorization_clients, authorization_client);
            }

            // save the updated authorization_clients to file
            utility::ofstream_t os(filename, std::ios::out | std::ios::trunc);
            if (os.is_open())
            {
                os << authorization_clients.serialize();
                os.close();
            }
        }

        // construct callback to load a table of authorization server uri vs authorization client metadata from file based on settings seed_id
        // it is not required for scopeless OAuth 2.0 client (client not require to access any protected APIs)
        load_authorization_clients_handler make_load_authorization_clients_handler(const nmos::settings& settings, slog::base_gate& gate)
        {
            return [&]()
            {
                // obtain client metadata from the safe, permission-restricted, location in the non-volatile memory, e.g. a file
                // Client metadata SHOULD consist of the client_id, client_secret, client_secret_expires_at, client_uri, grant_types, redirect_uris, response_types, scope, token_endpoint_auth_method
                auto filename = nmos::experimental::fields::seed_id(settings) + U(".json");
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Load authorization clients from non-volatile memory: " << filename;

                return load_authorization_clients_file(filename, gate);
            };
        }

        // construct callback to save the authorization server uri vs authorization client metadata table to file, using seed_id for the filename
        // it is not required for scopeless OAuth 2.0 client (client not require to access any protected APIs)
        save_authorization_client_handler make_save_authorization_client_handler(const nmos::settings& settings, slog::base_gate& gate)
        {
            return [&](const web::json::value& authorization_client)
            {
                // Client metadata SHOULD be stored in a safe, permission-restricted, location in non-volatile memory in case of a device restart to prevent re-registration.
                // Client secrets SHOULD be encrypted before being stored to reduce the chance of client secret leaking.
                // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.2._Behaviour_-_Clients.html#client-credentials
                const auto filename = nmos::experimental::fields::seed_id(settings) + U(".json");
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Save authorization clients to non-volatile memory: " << filename;

                update_authorization_clients_file(filename, authorization_client, gate);
            };
        }

        // construct callback to start the authorization code flow request on a browser
        // it is required for those OAuth client which is using the Authorization Code Flow to obtain the access token
        // note: as it is not easy to specify the 'content-type' used in the browser programmatically, this can be easily
        // fixed by installing a browser header modifier
        // such extension e.g. ModHeader can be used to add the missing 'content-type' header accordingly
        // for Windows https://chrome.google.com/webstore/detail/modheader-modify-http-hea/idgpnmonknjnojddfkpgkljpfnnfcklj
        // for Linux   https://addons.mozilla.org/en-GB/firefox/addon/modheader-firefox/
        request_authorization_code_handler make_request_authorization_code_handler(slog::base_gate& gate)
        {
            return[&gate](const web::uri& authorization_code_uri)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Open a browser to start the authorization code flow: " << authorization_code_uri.to_string();

                std::string browser_cmd;
#if defined(_WIN32) && !defined(__cplusplus_winrt)
                browser_cmd = "start \"\" \"" + utility::us2s(authorization_code_uri.to_string()) + "\"";
#else
                browser_cmd = "xdg-open \"" + utility::us2s(authorization_code_uri.to_string()) + "\"";
#endif
                std::ignore = system(browser_cmd.c_str());

                // hmm, notify authorization_code_flow in the authorization_behaviour thread
                // in the event of user cancels the authorization code flow process
            };
        }

        // construct callback to make OAuth 2.0 config
        authorization_config_handler make_authorization_config_handler(const web::json::value& authorization_server_metadata, const web::json::value& client_metadata, slog::base_gate& gate)
        {
            return[&](const web::http::oauth2::experimental::oauth2_token& bearer_token)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Make OAuth 2.0 config";

                web::http::oauth2::experimental::oauth2_config config(
                    client_metadata.is_null() ? U("") : nmos::experimental::fields::client_id(client_metadata),
                    client_metadata.is_null() ? U("") : client_metadata.has_string_field(nmos::experimental::fields::client_secret) ? nmos::experimental::fields::client_secret(client_metadata) : U(""),
                    authorization_server_metadata.is_null() ? U("") : nmos::experimental::fields::authorization_endpoint(authorization_server_metadata),
                    authorization_server_metadata.is_null() ? U("") : nmos::experimental::fields::token_endpoint(authorization_server_metadata),
                    client_metadata.is_null() ? U("") : client_metadata.has_array_field(nmos::experimental::fields::redirect_uris) && nmos::experimental::fields::redirect_uris(client_metadata).size() ? nmos::experimental::fields::redirect_uris(client_metadata).at(0).as_string() : U(""),
                    client_metadata.is_null() ? U("") : client_metadata.has_string_field(nmos::experimental::fields::scope) ? nmos::experimental::fields::scope(client_metadata) : U(""));

                if (!client_metadata.is_null())
                {
                    const auto& response_types = nmos::experimental::fields::response_types(client_metadata);
                    bool found_code = false;
                    bool found_token = false;
                    for (auto response_type : response_types)
                    {
                        if (web::http::oauth2::experimental::response_types::code.name == response_type.as_string()) { found_code = true; }
                        else if (web::http::oauth2::experimental::response_types::token.name == response_type.as_string()) { found_token = true; }
                    };
                    config.set_bearer_auth(found_code || !found_token);
                }

                config.set_token(bearer_token);

                return config;
            };
        }
        authorization_config_handler make_authorization_config_handler(const authorization_state& authorization_state, slog::base_gate& gate)
        {
            return[&](const web::http::oauth2::experimental::oauth2_token& /*bearer_token*/)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Make OAuth 2.0 config using bearer_token cache";

                const auto authorization_server_metadata = get_authorization_server_metadata(authorization_state);
                const auto client_metadata = get_client_metadata(authorization_state);

                auto make_authorization_config = make_authorization_config_handler(authorization_server_metadata, client_metadata, gate);
                return make_authorization_config(authorization_state.bearer_token);
            };
        }

        // construct callback to validate OAuth 2.0 authorization access token
        validate_authorization_token_handler make_validate_authorization_token_handler(authorization_state& authorization_state, slog::base_gate& gate)
        {
            return[&](const utility::string_t& access_token)
            {
                try
                {
                    // extract the token issuer from the token
                    const auto token_issuer = nmos::experimental::jwt_validator::get_token_issuer(access_token);

                    auto lock = authorization_state.read_lock();

                    std::string error;
                    auto issuer = authorization_state.issuers.find(token_issuer);
                    if (authorization_state.issuers.end() != issuer)
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validate access token against " << utility::us2s(issuer->first.to_string()) << " public keys";

                        try
                        {
                            // if jwt_validator has not already set up, treat it as no public keys to validate token
                            if (issuer->second.jwt_validator.is_initialized())
                            {
                                // do access token basic validation, including token schema validation and token issuer public keys validation
                                issuer->second.jwt_validator.basic_validation(access_token);

                                return authorization_error{ authorization_error::succeeded };
                            }
                            else
                            {
                                std::stringstream ss;
                                ss << "No " << utility::us2s(issuer->first.to_string()) << " public keys to validate access token";
                                error = ss.str();
                                slog::log<slog::severities::error>(gate, SLOG_FLF) << error;

                                return authorization_error{ authorization_error::no_matching_keys, error };
                            }
                        }
                        catch (const web::json::json_exception& e)
                        {
#if defined (NDEBUG)
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();
#else
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what() << "; access_token: " << access_token;
#endif
                            return authorization_error{ authorization_error::failed, e.what() };
                        }
                        catch (const jwt::error::token_verification_exception& e)
                        {
#if defined (NDEBUG)
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Token verification error: " << e.what();
#else
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Token verification error: " << e.what() << "; access_token: " << access_token;
#endif
                            return authorization_error{ authorization_error::failed, e.what() };
                        }
                        catch (const no_matching_keys_exception& e)
                        {
#if defined (NDEBUG)
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "No matching public keys error: " << e.what();
#else
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "No matching public keys error: " << e.what() << "; access_token: " << access_token;
#endif
                            return authorization_error{ authorization_error::no_matching_keys, e.what() };
                        }
                        catch (const std::exception& e)
                        {
#if defined (NDEBUG)
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what();
#else
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what() << "; access_token: " << access_token;
#endif
                            return authorization_error{ authorization_error::failed, e.what() };
                        }
                    }
                    else
                    {
                        std::stringstream ss;
                        ss << "No " << utility::us2s(token_issuer.to_string()) << " public keys to validate access token";
                        error = ss.str();
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << error;

                        // no public keys to validate token
                        return authorization_error{ authorization_error::no_matching_keys, error };
                    }
                }
                catch (const std::exception& e)
                {
#if defined (NDEBUG)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unable to extract token issuer from access token: " << e.what();
#else
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unable to extract token issuer from access token: " << e.what() << "; access_token: " << access_token;
#endif
                    return authorization_error{ authorization_error::failed, e.what() };
                }
            };
        }

        // construct callback to validate OAuth 2.0 authorization
        validate_authorization_handler make_validate_authorization_handler(nmos::base_model& model, authorization_state& authorization_state, validate_authorization_token_handler access_token_validation, slog::base_gate& gate)
        {
            return[&, access_token_validation](const nmos::experimental::scope& scope)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Make authorization validation";

                return nmos::experimental::details::make_validate_authorization_handler(model, authorization_state, scope, access_token_validation, gate);
            };
        }

        // construct callback to retrieve OAuth 2.0 authorization bearer token
        authorization_token_handler make_authorization_token_handler(authorization_state& authorization_state, slog::base_gate& gate)
        {
            return[&]()
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Retrieve bearer token from cache";

                auto lock = authorization_state.read_lock();
                return authorization_state.bearer_token;
            };
        }
    }
}

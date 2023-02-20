#include "nmos/authorization_redirect_api.h"

#include "cpprest/access_token_error.h"
#include "cpprest/response_type.h"
#include "nmos/api_utils.h"
#include "nmos/authorization_behaviour.h" // for top_authorization_service
#include "nmos/authorization_operation.h" // for request_token_from_redirected_uri
#include "nmos/authorization_state.h"
#include "nmos/authorization_utils.h"
#include "nmos/certificate_settings.h"
#include "nmos/client_utils.h" // for make_http_client_config
#include "nmos/jwt_generator.h"
#include "nmos/jwk_utils.h"
#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        struct authorization_flow_exception : std::runtime_error
        {
            web::http::oauth2::experimental::access_token_error error;
            utility::string_t description;

            explicit authorization_flow_exception(web::http::oauth2::experimental::access_token_error error)
                : std::runtime_error(utility::us2s(error.name))
                , error(std::move(error)) {}

            explicit authorization_flow_exception(web::http::oauth2::experimental::access_token_error error, utility::string_t description)
                : std::runtime_error(utility::us2s(error.name))
                , error(std::move(error))
                , description(std::move(description)) {}
        };

        namespace details
        {
            typedef std::pair<web::http::status_code, web::json::value> authorization_flow_response;

            inline authorization_flow_response make_authorization_flow_error_response(web::http::status_code code, const utility::string_t& error = {}, const utility::string_t& debug = {})
            {
                return{ code, make_error_response_body(code, error, debug) };
            }

            inline authorization_flow_response make_authorization_flow_error_response(web::http::status_code code, const std::exception& debug)
            {
                return make_authorization_flow_error_response(code, {}, utility::s2us(debug.what()));
            }

            void process_error_response(const web::uri& redirected_uri, const utility::string_t& response_type, const utility::string_t& state)
            {
                using web::http::oauth2::experimental::oauth2_exception;
                using web::http::oauth2::details::oauth2_strings;
                namespace response_types = web::http::oauth2::experimental::response_types;
                namespace access_token_errors = web::http::oauth2::experimental::access_token_errors;

                std::map<utility::string_t, utility::string_t> query;

                // for Authorization Code Grant
                // "If the resource owner denies the access request or if the request
                // fails for reasons other than a missing or invalid redirection URI,
                // the authorization server informs the client by adding the following
                // parameters to the query component of the redirection URI using the
                // "application/x-www-form-urlencoded" format"
                //
                // For example, the authorization server redirects the user-agent by
                // sending the following HTTP response
                //   HTTP/1.1 302 Found
                //   Location: https://client.example.com/cb?error=access_denied&state=xyz
                // see https://tools.ietf.org/html/rfc6749#section-4.1.2.1
                if (response_type == response_types::code.name)
                {
                    query = web::uri::split_query(redirected_uri.query());
                }
                // for Implicit Grant
                // "If the resource owner denies the access request or if the request
                // fails for reasons other than a missing or invalid redirection URI,
                // the authorization server informs the client by adding the following
                // parameters to the fragment component of the redirection URI using the
                // "application/x-www-form-urlencoded" format"
                //
                // For example, the authorization server redirects the user-agent by
                // sending the following HTTP response
                //   HTTP/1.1 302 Found
                //   Location: https://client.example.com/cb#error=access_denied&state=xyz
                // see https://tools.ietf.org/html/rfc6749#section-4.2.2.1
                else if (response_type == response_types::token.name)
                {
                    query = web::uri::split_query(redirected_uri.fragment());
                }
                else
                {
                    throw oauth2_exception(U("response_type: '") + response_type + U("' is not supported"));
                }

                auto state_param = query.find(oauth2_strings::state);
                if (state_param == query.end())
                {
                    throw oauth2_exception(U("parameter 'state' missing from redirected URI"));
                }

                if (state != state_param->second)
                {
                    throw oauth2_exception(U("parameter 'state': '") + state_param->second + U("' does not match with the expected 'state': '") + state + U("'"));
                }

                auto error_param = query.find(U("error"));
                if (error_param != query.end())
                {
                    const auto error = web::http::oauth2::experimental::to_access_token_error(error_param->second);
                    if (error.empty())
                    {
                        throw oauth2_exception(U("invalid 'error' parameter"));
                    }

                    auto error_description_param = query.find(U("error_description"));
                    if (error_description_param != query.end())
                    {
                        auto error_description = web::uri::decode(error_description_param->second);
                        std::replace(error_description.begin(), error_description.end(), '+', ' ');
                        throw authorization_flow_exception(error, error_description);
                    }
                    else
                    {
                        throw authorization_flow_exception(error);
                    }

                    // hmm, error_uri is ignored for now
                }
            }
        }

        web::http::experimental::listener::api_router make_authorization_redirect_api(nmos::base_model& model, authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::load_rsa_private_keys_handler load_rsa_private_keys, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router authorization_api;

            authorization_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-authorization/") }, req, res));
                return pplx::task_from_result(true);
            });

            authorization_api.support(U("/x-authorization/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("callback/"), U("jwks/")}, req, res));
                return pplx::task_from_result(true);
            });

            authorization_api.support(U("/x-authorization/callback/?"), methods::GET, [&model, &authorization_state, load_ca_certificates, load_rsa_private_keys, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);

                details::authorization_flow_response result{ status_codes::BadRequest, {} };

                utility::string_t state;
                utility::string_t code_verifier;
                web::json::value authorization_server_metadata;
                web::json::value client_metadata;
                with_write_lock(authorization_state.mutex, [&]
                {
                    state = authorization_state.state;
                    code_verifier = authorization_state.code_verifier;
                    authorization_state.authorization_flow = authorization_state::request_code;
                    authorization_server_metadata = get_authorization_server_metadata(authorization_state);
                    client_metadata = get_client_metadata(authorization_state);
                });

                web::uri token_endpoint;
                web::http::client::http_client_config config;
                utility::string_t response_type;
                nmos::api_version version;
                utility::string_t client_id;
                utility::string_t client_secret;
                utility::string_t scope;
                utility::string_t redirect_uri;
                utility::string_t token_endpoint_auth_method;
                utility::string_t rsa_private_key;
                utility::string_t keyid;
                auto client_assertion_lifespan(std::chrono::seconds(30));
                with_read_lock(model.mutex, [&, load_ca_certificates, load_rsa_private_keys]
                {
                    const auto& settings = model.settings;
                    token_endpoint = nmos::experimental::fields::token_endpoint(authorization_server_metadata);
                    config = nmos::make_http_client_config(settings, load_ca_certificates, gate);
                    response_type = web::http::oauth2::experimental::response_types::code.name;
                    client_id = nmos::experimental::fields::client_id(client_metadata);
                    client_secret = client_metadata.has_string_field(nmos::experimental::fields::client_secret) ? nmos::experimental::fields::client_secret(client_metadata) : U("");
                    scope = nmos::experimental::fields::scope(client_metadata);
                    redirect_uri = nmos::experimental::fields::redirect_uris(client_metadata).at(0).as_string();
                    version = details::top_authorization_service(model.settings).first.first;
                    token_endpoint_auth_method = client_metadata.has_string_field(nmos::experimental::fields::token_endpoint_auth_method) ? nmos::experimental::fields::token_endpoint_auth_method(client_metadata) : web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_basic.name;

                    if (load_rsa_private_keys)
                    {
                        // use the 1st RSA private key from RSA private keys list to create the client_assertion
                        auto rsa_private_keys = load_rsa_private_keys();
                        if (!rsa_private_keys.empty())
                        {
                            rsa_private_key = rsa_private_keys[0];
                            keyid = U("1");
                        }
                    }
                    client_assertion_lifespan = std::chrono::seconds(nmos::experimental::fields::authorization_request_max(settings));
                });

                // The Authorization server may redirect error back due to something have went wrong
                // such as resource owner rejects the request or the developer did something wrong
                // when creating the Authorization request
                {
                    auto lock = authorization_state.write_lock(); // in order to update shared state
                    try
                    {
                        details::process_error_response(req.request_uri(), response_type, state);
                    }
                    catch (const web::http::oauth2::experimental::oauth2_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization flow token request OAuth 2.0 error: " << e.what();
                        result = details::make_authorization_flow_error_response(status_codes::InternalError, e);
                        authorization_state.authorization_flow = authorization_state::failed;
                    }
                    catch (const authorization_flow_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization flow token request Authorization Flow error: " << utility::us2s(e.error.name) << " description: " << utility::us2s(e.description);
                        result = details::make_authorization_flow_error_response(status_codes::BadRequest, e.error.name, e.description);
                        authorization_state.authorization_flow = authorization_state::failed;
                    }

                    if (authorization_state::failed == authorization_state.authorization_flow)
                    {
                        with_write_lock(model.mutex, [&]
                        {
                            model.notify();
                        });

                        set_reply(res, result.first, !result.second.is_null() ? result.second : nmos::make_error_response_body(result.first));

                        return pplx::task_from_result(true);
                    }
                }

                web::http::client::http_client client(token_endpoint, config);

                auto request_token = pplx::task_from_result(web::http::oauth2::experimental::oauth2_token());

                const auto token_endpoint_auth_meth = web::http::oauth2::experimental::to_token_endpoint_auth_method(token_endpoint_auth_method);

                // create client assertion for private_key_jwt
                utility::string_t client_assertion;
                if (web::http::oauth2::experimental::token_endpoint_auth_methods::private_key_jwt == token_endpoint_auth_meth)
                {
                    auto lock = authorization_state.write_lock(); // in order to update shared state
                    try
                    {
                        client_assertion = jwt_generator::create_client_assertion(client_id, client_id, token_endpoint, client_assertion_lifespan, rsa_private_key, keyid);
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization flow token request Create Client Assertion error: " << e.what();
                        result = details::make_authorization_flow_error_response(status_codes::InternalError, e);
                        authorization_state.authorization_flow = authorization_state::failed;
                    }

                    if (authorization_state::failed == authorization_state.authorization_flow)
                    {
                        with_write_lock(model.mutex, [&]
                        {
                            model.notify();
                        });

                        set_reply(res, result.first, !result.second.is_null() ? result.second : nmos::make_error_response_body(result.first));

                        return pplx::task_from_result(true);
                    }
                }

                // exchange authorization code for bearer token
                request_token = details::request_token_from_redirected_uri(client, version, req.request_uri(), response_type, client_id, client_secret, scope, redirect_uri, state, code_verifier, token_endpoint_auth_meth, client_assertion, gate);

                auto request = request_token.then([&model, &authorization_state, &scope, &gate](web::http::oauth2::experimental::oauth2_token bearer_token)
                {
                    auto lock = authorization_state.write_lock();

                    // signal authorization_flow that bearer token has just been received
                    authorization_state.authorization_flow = authorization_state::access_token_received;

                    // update bearer token cache, which will be used for accessing protected APIs
                    authorization_state.bearer_token = bearer_token;

                    slog::log<slog::severities::info>(gate, SLOG_FLF) << utility::us2s(bearer_token.scope()) << " bearer token updated";

                }).then([&](pplx::task<void> finally)
                {
                    auto lock = authorization_state.write_lock(); // in order to update shared state

                    try
                    {
                        finally.get();
                        result = { status_codes::OK, web::json::value_of({{ U("status"), U("Bearer token received") }}, true) };
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization flow token request HTTP error: " << e.what() << " [" << e.error_code() << "]";
                        result = details::make_authorization_flow_error_response(status_codes::BadRequest, e);
                        authorization_state.authorization_flow = authorization_state::failed;
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization flow token request JSON error: " << e.what();
                        result = details::make_authorization_flow_error_response(status_codes::InternalError, e);
                        authorization_state.authorization_flow = authorization_state::failed;
                    }
                    catch (const web::http::oauth2::experimental::oauth2_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization flow token request OAuth 2.0 error: " << e.what();
                        result = details::make_authorization_flow_error_response(status_codes::InternalError, e);
                        authorization_state.authorization_flow = authorization_state::failed;
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization flow token request error: " << e.what();
                        result = details::make_authorization_flow_error_response(status_codes::InternalError, e);
                        authorization_state.authorization_flow = authorization_state::failed;
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization flow token request error";
                        result = details::make_authorization_flow_error_response(status_codes::InternalError);
                        authorization_state.authorization_flow = authorization_state::failed;
                    }

                    with_write_lock(model.mutex, [&]
                    {
                        model.notify();
                    });
                });

                // hmm, perhaps wait with timeout?
                request.wait();

                if (web::http::is_success_status_code(result.first))
                {
                    set_reply(res, result.first, result.second);
                }
                else
                {
                    set_reply(res, result.first, !result.second.is_null() ? result.second : nmos::make_error_response_body(result.first));
                }

                return pplx::task_from_result(true);
            });

            return authorization_api;
        }
    }
}

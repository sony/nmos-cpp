#include "nmos/authorization_redirect_api.h"

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

                web::http::client::http_client client(token_endpoint, config);

                auto request_token = pplx::task_from_result(web::http::oauth2::experimental::oauth2_token());

                if (web::http::oauth2::experimental::token_endpoint_auth_methods::private_key_jwt.name == token_endpoint_auth_method)
                {
                    const auto client_assertion = jwt_generator::create_client_assertion(client_id, client_id, token_endpoint, client_assertion_lifespan, rsa_private_key, keyid);

                    // exchange authorization code for bearer token
                    // where redirected URI: /x-authorization/callback/?state=<state>&code=<authorization code>
                    request_token = details::request_token_from_redirected_uri(client, version, req.request_uri(), response_type, client_id, scope, redirect_uri, state, code_verifier, token_endpoint_auth_method, client_assertion, gate);
                }
                else
                {
                    // exchange authorization code for bearer token
                    // where redirected URI: /x-authorization/callback/?state=<state>&code=<authorization code>
                    request_token = details::request_token_from_redirected_uri(client, version, req.request_uri(), response_type, client_id, client_secret, scope, redirect_uri, state, code_verifier, gate);
                }

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
                        result = { status_codes::OK, web::json::value_of({U("Bearer token received")}) };
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

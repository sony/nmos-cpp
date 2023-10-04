#include "nmos/authorization_operation.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/range/join.hpp>
#include "cpprest/code_challenge_method.h"
#include "cpprest/json_validator.h"
#include "cpprest/response_type.h"
#include "cpprest/token_endpoint_auth_method.h"
#include "nmos/api_utils.h"
#include "nmos/authorization.h"
#include "nmos/authorization_scopes.h"
#include "nmos/authorization_state.h"
#include "nmos/authorization_utils.h"
#include "nmos/client_utils.h"
#include "nmos/is10_versions.h"
#include "nmos/json_schema.h"
#include "nmos/jwt_generator.h"
#include "nmos/jwk_utils.h"
#include "nmos/model.h"
#include "nmos/random.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        // authorization operation
        namespace details
        {
            static const web::json::experimental::json_validator& authapi_validator()
            {
                static const web::json::experimental::json_validator validator
                {
                    nmos::experimental::load_json_schema,
                    boost::copy_range<std::vector<web::uri>>(boost::join(boost::join(boost::join(boost::join(boost::join(
                        is10_versions::all | boost::adaptors::transformed(experimental::make_authapi_auth_metadata_schema_uri),
                        is10_versions::all | boost::adaptors::transformed(experimental::make_authapi_jwks_response_schema_uri)),
                        is10_versions::all | boost::adaptors::transformed(experimental::make_authapi_register_client_response_uri)),
                        is10_versions::all | boost::adaptors::transformed(experimental::make_authapi_token_error_response_uri)),
                        is10_versions::all | boost::adaptors::transformed(experimental::make_authapi_token_response_schema_uri)),
                        is10_versions::all | boost::adaptors::transformed(experimental::make_authapi_token_schema_schema_uri)))
                };
                return validator;
            }

            // build the scope string with given list of scopes
            utility::string_t make_scope(const std::set<nmos::experimental::scope>& scopes_)
            {
                utility::string_t scopes;
                for (const auto& scope : scopes_)
                {
                    if (!scopes.empty()) { scopes += U(" "); }
                    scopes += scope.name;
                }
                return scopes;
            }

            // build grant array with given list of grants
            web::json::value make_grant_types(const std::set<web::http::oauth2::experimental::grant_type>& grants)
            {
                auto grant_types = web::json::value::array();
                for (const auto& grant : grants)
                {
                    web::json::push_back(grant_types, grant.name);
                }
                return grant_types;
            }

            // generate SHA256 with the given string
            std::vector<uint8_t> sha256(const std::string& text)
            {
                uint8_t hash[SHA256_DIGEST_LENGTH];
                SHA256_CTX ctx;
                if (SHA256_Init(&ctx) && SHA256_Update(&ctx, text.c_str(), text.size()) && SHA256_Final(hash, &ctx))
                {
                    return{ hash, hash + SHA256_DIGEST_LENGTH };
                }
                return{};
            }

            // use the authorization URI on a web browser to start the authorization code grant workflow
            web::uri make_authorization_code_uri(const web::uri& authorization_endpoint, const utility::string_t& client_id, const web::uri& redirect_uri, const web::http::oauth2::experimental::response_type& response_type, const std::set<scope>& scopes, const web::json::array& code_challenge_methods_supported, utility::string_t& state, utility::string_t& code_verifier)
            {
                using web::http::oauth2::details::oauth2_strings;

                web::uri_builder ub(authorization_endpoint);
                ub.append_query(oauth2_strings::client_id, client_id);
                ub.append_query(oauth2_strings::redirect_uri, redirect_uri.to_string());
                ub.append_query(oauth2_strings::response_type, response_type.name);

                // using PKCE?
                if (code_challenge_methods_supported.size())
                {
                    const auto found = std::find_if(code_challenge_methods_supported.begin(), code_challenge_methods_supported.end(), [&](const web::json::value& code_challenge_method)
                    {
                        return web::http::oauth2::experimental::code_challenge_methods::S256.name == code_challenge_method.as_string();
                    });

                    const auto code_challenge_method = (code_challenge_methods_supported.end() != found) ? web::http::oauth2::experimental::code_challenge_methods::S256 : web::http::oauth2::experimental::code_challenge_methods::plain;

                    // code_verifier = high-entropy cryptographic random STRING using the
                    // unreserved characters[A - Z] / [a - z] / [0 - 9] / "-" / "." / "_" / "~"
                    //    from Section 2.3 of[RFC3986], with a minimum length of 43 characters
                    //    and a maximum length of 128 characters
                    // see https://tools.ietf.org/html/rfc7636#section-4.1
                    {
                        utility::nonce_generator generator(128);
                        code_verifier = generator.generate();
                    }

                    // creates code challenge from code verifier
                    // see https://tools.ietf.org/html/rfc7636#section-4.2
                    utility::string_t code_challenge{};
                    if (web::http::oauth2::experimental::code_challenge_methods::plain == code_challenge_method)
                    {
                        code_challenge = code_verifier;
                    }
                    else
                    {
                        const auto sha256 = nmos::experimental::details::sha256(utility::us2s(code_verifier));
                        code_challenge = utility::conversions::to_base64url(sha256);
                    }
                    ub.append_query(U("code_challenge"), code_challenge);
                    ub.append_query(U("code_challenge_method"), code_challenge_method.name);
                }

                utility::nonce_generator generator;
                state = generator.generate();
                ub.append_query(oauth2_strings::state, state);

                if (scopes.size())
                {
                    ub.append_query(oauth2_strings::scope, make_scope(scopes));
                }

                return ub.to_uri();
            }

            // it is used to strip the trailing dot of the FQDN if it is presented
            utility::string_t strip_trailing_dot(const utility::string_t& host_)
            {
                auto host = host_;
                if (!host.empty() && U('.') == host.back())
                {
                    host.pop_back();
                }
                return host;
            }

            // construct the redirect URI from settings
            // format of the authorization_redirect_uri "<http scheme>://<FQDN>:<port>/x-authorization/callback/"
            web::uri make_authorization_redirect_uri(const nmos::settings& settings)
            {
                return web::uri_builder()
                    .set_scheme(web::http_scheme(nmos::experimental::fields::client_secure(settings)))
                    .set_host(nmos::experimental::fields::no_trailing_dot_for_authorization_callback_uri(settings) ? strip_trailing_dot(get_host(settings)) : get_host(settings))
                    .set_port(nmos::experimental::fields::authorization_redirect_port(settings))
                    .set_path(U("/x-authorization/callback"))
                    .to_uri();
            }

            // construct the jwks URI from settings
            // format of the jwks_uri "<http scheme>://<FQDN>:<port>/x-authorization/jwks/"
            web::uri make_jwks_uri(const nmos::settings& settings)
            {
                return web::uri_builder()
                    .set_scheme(web::http_scheme(nmos::experimental::fields::client_secure(settings)))
                    .set_host(nmos::experimental::fields::no_trailing_dot_for_authorization_callback_uri(settings) ? strip_trailing_dot(get_host(settings)) : get_host(settings))
                    .set_port(nmos::experimental::fields::jwks_uri_port(settings))
                    .set_path(U("/x-authorization/jwks"))
                    .to_uri();
            }

            // construct the authorization server URI using the given URI authority
            // format of the authorization_service_uri "<http scheme>://<FQDN>:<port>/.well-known/oauth-authorization-server[/<api_selector>]"
            web::uri make_authorization_service_uri(const web::uri& uri, const utility::string_t& api_selector = {})
            {
                return web::uri_builder(uri.authority()).set_path(U("/.well-known/oauth-authorization-server")).append_path(!api_selector.empty() ? U("/") + api_selector : U("")).to_uri();
            }

            // construct authorization client config based on settings
            // with the remaining options defaulted, e.g. authorization request timeout
            web::http::client::http_client_config make_authorization_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, authorization_config_handler make_authorization_config, const web::http::oauth2::experimental::oauth2_token& bearer_token, slog::base_gate& gate)
            {
                auto config = nmos::make_http_client_config(settings, load_ca_certificates, make_authorization_config, bearer_token, gate);
                config.set_timeout(std::chrono::seconds(nmos::experimental::fields::authorization_request_max(settings)));

                return config;
            }

            struct authorization_exception {};

            // parse the given json to obtain access token
            // this function is based on the oauth2_config::_parse_token_from_json(const json::value& token_json) from cpprestsdk's oauth2.cpp
            web::http::oauth2::experimental::oauth2_token parse_token_from_json(const web::json::value& token_json)
            {
                using web::http::oauth2::details::oauth2_strings;
                using web::http::oauth2::experimental::oauth2_token;
                using web::http::oauth2::experimental::oauth2_exception;

                oauth2_token result;

                if (token_json.has_string_field(oauth2_strings::access_token))
                {
                    result.set_access_token(token_json.at(oauth2_strings::access_token).as_string());
                }
                else
                {
#if defined (NDEBUG)
                    throw oauth2_exception(U("response json contains no 'access_token'"));
#else
                    throw oauth2_exception(U("response json contains no 'access_token': ") + token_json.serialize());
#endif
                }

                if (token_json.has_string_field(oauth2_strings::token_type))
                {
                    result.set_token_type(token_json.at(oauth2_strings::token_type).as_string());
                }
                else
                {
                    // Some services don't return 'token_type' while it's required by OAuth 2.0 spec:
                    // http://tools.ietf.org/html/rfc6749#section-5.1
                    // As workaround we act as if 'token_type=bearer' was received.
                    result.set_token_type(oauth2_strings::bearer);
                }
                if (!utility::details::str_iequal(result.token_type(), oauth2_strings::bearer))
                {
#if defined (NDEBUG)
                    throw oauth2_exception(U("only bearer tokens are currently supported"));
#else
                    throw oauth2_exception(U("only bearer tokens are currently supported: ") + token_json.serialize());
#endif
                }

                if (token_json.has_string_field(oauth2_strings::refresh_token))
                {
                    result.set_refresh_token(token_json.at(oauth2_strings::refresh_token).as_string());
                }
                else
                {
                    // Do nothing. Preserves the old refresh token
                }

                if (token_json.has_field(oauth2_strings::expires_in))
                {
                    const auto& json_expires_in_val = token_json.at(oauth2_strings::expires_in);

                    if (json_expires_in_val.is_number())
                    {
                        result.set_expires_in(json_expires_in_val.as_number().to_int64());
                    }
                    else
                    {
                        // Handle the case of a number as a JSON "string"
                        int64_t expires;
                        utility::istringstream_t iss(json_expires_in_val.as_string());
                        iss.exceptions(std::ios::badbit | std::ios::failbit);
                        iss >> expires;
                        result.set_expires_in(expires);
                    }
                }
                else
                {
                    result.set_expires_in(oauth2_token::undefined_expiration);
                }

                if (token_json.has_string_field(oauth2_strings::scope))
                {
                    // The authorization server may return different scope from the one requested
                    // This however doesn't necessarily mean the token authorization scope is different
                    // See: http://tools.ietf.org/html/rfc6749#section-3.3
                    result.set_scope(token_json.at(oauth2_strings::scope).as_string());
                }

                return result;
            }

            // make an asynchronously GET request on the Authorization API to fetch authorization server metadata
            pplx::task<web::json::value> request_authorization_server_metadata(web::http::client::http_client client, const std::set<nmos::experimental::scope>& scopes, const std::set<web::http::oauth2::experimental::grant_type>& grants, const web::http::oauth2::experimental::token_endpoint_auth_method& token_endpoint_auth_method, const nmos::api_version& version, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting authorization server metadata at " << client.base_uri().to_string();

                using namespace web::http;

                // <api_proto>://<hostname>:<port>/.well-known/oauth-authorization-server[/<api_selector>]
                // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/3.0._Discovery.html#authorization-server-metadata-endpoint
                return nmos::api_request(client, methods::GET, gate, token).then([=, &gate](pplx::task<http_response> response_task)
                {
                    namespace response_types = web::http::oauth2::experimental::response_types;
                    namespace grant_types = web::http::oauth2::experimental::grant_types;

                    auto response = response_task.get(); // may throw http_exception

                    if (status_codes::OK == response.status_code())
                    {
                        if (response.body())
                        {
                            return response.extract_json().then([=, &gate](web::json::value metadata)
                            {
                                // validate server metadata
                                authapi_validator().validate(metadata, experimental::make_authapi_auth_metadata_schema_uri(version)); // may throw json_exception

                                // hmm, verify Authorization server meeting the minimum client requirement

                                // is the required response_types supported by the Authorization server
                                std::set<web::http::oauth2::experimental::response_type> response_types = { response_types::code };
                                if (grants.end() != std::find_if(grants.begin(), grants.end(), [](const web::http::oauth2::experimental::grant_type& grant) { return grant_types::implicit == grant; }))
                                {
                                    response_types.insert(response_types::token);
                                }
                                if (response_types.size())
                                {
                                    const auto supported = std::all_of(response_types.begin(), response_types.end(), [&](const web::http::oauth2::experimental::response_type& response_type)
                                    {
                                        const auto& response_types_supported = nmos::experimental::fields::response_types_supported(metadata);
                                        const auto found = std::find_if(response_types_supported.begin(), response_types_supported.end(), [&response_type](const web::json::value& response_type_) { return response_type_.as_string() == response_type.name; });
                                        return response_types_supported.end() != found;
                                    });
                                    if (!supported)
                                    {
                                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request authorization server metadata error: server does not supporting all the required response types";
                                        throw authorization_exception();
                                    }
                                }

                                // scopes_supported is optional
                                if (scopes.size() && metadata.has_array_field(nmos::experimental::fields::scopes_supported))
                                {
                                    // is the required scopes supported by the Authorization server
                                    const auto supported = std::all_of(scopes.begin(), scopes.end(), [&](const nmos::experimental::scope& scope)
                                    {
                                        const auto& scopes_supported = nmos::experimental::fields::scopes_supported(metadata);
                                        const auto found = std::find_if(scopes_supported.begin(), scopes_supported.end(), [&scope](const web::json::value& scope_) { return scope_.as_string() == scope.name; });
                                        return scopes_supported.end() != found;
                                    });
                                    if (!supported)
                                    {
                                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request authorization server metadata error: server does not supporting all the required scopes: " << [&scopes]() { std::stringstream ss; for (auto scope : scopes) ss << utility::us2s(scope.name) << " "; return ss.str();  }();
                                        throw authorization_exception();
                                    }
                                }

                                // grant_types_supported is optional
                                if (grants.size() && metadata.has_array_field(nmos::experimental::fields::grant_types_supported))
                                {
                                    // is the required grants supported by the Authorization server
                                    const auto supported = std::all_of(grants.begin(), grants.end(), [&](const web::http::oauth2::experimental::grant_type& grant)
                                    {
                                        const auto& grants_supported = nmos::experimental::fields::grant_types_supported(metadata);
                                        const auto found = std::find_if(grants_supported.begin(), grants_supported.end(), [&grant](const web::json::value& grant_) { return grant_.as_string() == grant.name; });
                                        return grants_supported.end() != found;
                                    });
                                    if (!supported)
                                    {
                                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request authorization server metadata error: server does not supporting all the required grants: " << [&grants]() { std::stringstream ss; for (auto grant : grants) ss << utility::us2s(grant.name) << " "; return ss.str();  }();
                                        throw authorization_exception();
                                    }
                                }

                                // token_endpoint_auth_methods_supported is optional
                                if (metadata.has_array_field(nmos::experimental::fields::token_endpoint_auth_methods_supported))
                                {
                                    // is the required token_endpoint_auth_method supported by the Authorization server
                                    const auto& supported = nmos::experimental::fields::token_endpoint_auth_methods_supported(metadata);
                                    const auto found = std::find_if(supported.begin(), supported.end(), [&token_endpoint_auth_method](const web::json::value& token_endpoint_auth_method_) { return token_endpoint_auth_method_.as_string() == token_endpoint_auth_method.name; });
                                    if (supported.end() == found)
                                    {
                                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request authorization server metadata error: server does not supporting the required token_endpoint_auth_method:" << token_endpoint_auth_method.name;
                                        throw authorization_exception();
                                    }
                                }

                                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received authorization server metadata: " << utility::us2s(metadata.serialize());
                                return metadata;
                            }, token);
                        }
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request authorization server metadata error: no response body";
                    }
                    else
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request authorization server metadata error: " << response.status_code() << " " << response.reason_phrase();
                    }
                    throw authorization_exception();

                }, token);
            }

            // make an asynchronously POST request on the Authorization API to register a client
            // see https://tools.ietf.org/html/rfc6749#section-2
            // see https://tools.ietf.org/html/rfc7591#section-3.1
            // e.g. curl -X POST "https://authorization.server.example.com/register" -H "Content-Type: application/json" -d "{\"redirect_uris\": [\"https://client.example.com/callback/\"],\"client_name\": \"My Example Client\",\"client_uri\": \"https://client.example.com/details.html\",\"token_endpoint_auth_method\": \"client_secret_basic\",\"response_types\": [\"code\",\"token\"],\"scope\": \"registration query node connection\",\"grant_types\": [\"authorization_code\",\"refresh_token\",\"client_credentials\"],\"token_endpoint_auth_method\": \"client_secret_basic\"}"
            pplx::task<web::json::value> request_client_registration(web::http::client::http_client client, const utility::string_t& client_name, const std::vector<web::uri>& redirect_uris, const web::uri& client_uri, const std::set<web::http::oauth2::experimental::response_type>& response_types, const std::set<scope>& scopes, const std::set<web::http::oauth2::experimental::grant_type>& grants, const web::http::oauth2::experimental::token_endpoint_auth_method& token_endpoint_auth_method, const web::json::value& jwk, const web::uri& jwks_uri, const nmos::api_version& version, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting authorization client registration at " << client.base_uri().to_string();

                using namespace web;
                using namespace web::http;
                using web::json::value;
                using web::json::value_of;

                const auto make_uris = [](const std::vector<web::uri>& uris)
                {
                    auto result = value::array();
                    for (const auto& uri : uris) { web::json::push_back(result, uri.to_string()); }
                    return result;
                };

                const auto make_response_types = [](const std::set<web::http::oauth2::experimental::response_type>& response_types)
                {
                    auto result = value::array();
                    for (const auto& response_type : response_types) { web::json::push_back(result, response_type.name); }
                    return result;
                };

                const auto make_scope = [](const std::set<scope>& scopes)
                {
                    std::ostringstream os;
                    int idx{ 0 };
                    for (const auto& scope : scopes)
                    {
                        if (idx++) { os << " "; }
                        os << utility::us2s(scope.name);
                    }
                    return value(utility::s2us(os.str()));
                };

                const auto make_grant_type = [](const std::set<web::http::oauth2::experimental::grant_type>& grants)
                {
                    auto result = value::array();
                    for (const auto& grant : grants) { web::json::push_back(result, grant.name); }
                    return result;
                };

                // required
                auto metadata = value_of({
                    { nmos::experimental::fields::client_name, client_name }
                });

                // optional
                if (grants.end() != std::find_if(grants.begin(), grants.end(), [](const web::http::oauth2::experimental::grant_type& grant) { return web::http::oauth2::experimental::grant_types::authorization_code == grant; }))
                {
                    metadata[nmos::experimental::fields::redirect_uris] = make_uris(redirect_uris);
                }
                if (!client_uri.is_empty())
                {
                    metadata[nmos::experimental::fields::client_uri] = value::string(client_uri.to_string());
                }
                if (response_types.size())
                {
                    metadata[nmos::experimental::fields::response_types] = make_response_types(response_types);
                }
                if (scopes.size())
                {
                    metadata[nmos::experimental::fields::scope] = make_scope(scopes);
                }
                if (grants.size())
                {
                    metadata[nmos::experimental::fields::grant_types] = make_grant_type(grants);
                }

                metadata[nmos::experimental::fields::token_endpoint_auth_method] = value::string(token_endpoint_auth_method.name);
                if (web::http::oauth2::experimental::token_endpoint_auth_methods::private_key_jwt == token_endpoint_auth_method)
                {
                    if (!jwks_uri.is_empty())
                    {
                        metadata[nmos::experimental::fields::jwks_uri] = value::string(jwks_uri.to_string());
                    }
                    else
                    {
                        metadata[nmos::experimental::fields::jwks] = value_of({
                            { nmos::experimental::fields::keys, value_of({ jwk }) }
                        });
                    }
                }

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Request to register client metadata: " << utility::us2s(metadata.serialize()) << " at " << client.base_uri().to_string();

                return nmos::api_request(client, methods::POST, {}, metadata, gate, token).then([=, &gate](pplx::task<http_response> response_task)
                {
                    auto response = response_task.get(); // may throw http_exception

                    if (response.body())
                    {
                        return response.extract_json().then([=, &gate](web::json::value client_metadata)
                        {
                            if (status_codes::Created == response.status_code())
                            {
                                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Registered client metadata: " << utility::us2s(client_metadata.serialize());

                                // validate client metadata
                                authapi_validator().validate(client_metadata, experimental::make_authapi_register_client_response_uri(version)); // may throw json_exception

                                return client_metadata;
                            }
                            else
                            {
                                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request client registration error: " << response.status_code() << " " << response.reason_phrase() << " " << utility::us2s(client_metadata.serialize());
                                throw authorization_exception();
                            }
                        }, token);
                    }
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request client registration error: " << response.status_code() << " " << response.reason_phrase();
                    throw authorization_exception();

                }, token);
            }

            // make an asynchronously GET request on the Authorization API to fetch the authorization JSON Web Keys (public keys)
            pplx::task<web::json::value> request_jwks(web::http::client::http_client client, const nmos::api_version& version, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting authorization jwks at " << client.base_uri().to_string();

                using namespace web::http;
                using oauth2::experimental::oauth2_exception;

                return nmos::api_request(client, methods::GET, gate, token).then([=, &gate](pplx::task<http_response> response_task)
                {
                    auto response = response_task.get(); // may throw http_exception

                    if (status_codes::OK == response.status_code())
                    {
                        if (response.body())
                        {
                            return nmos::details::extract_json(response, gate).then([version, &gate](web::json::value body)
                            {
                                // validate jwks JSON
                                authapi_validator().validate(body, experimental::make_authapi_jwks_response_schema_uri(version));  // may throw json_exception

                                // MUST have a "keys" member!
                                // see https://tools.ietf.org/html/rfc7517#section-5
                                if (!body.has_array_field(U("keys"))) throw web::http::http_exception(U("jwks contains no 'keys': ") + body.serialize());

                                const auto jwks = body.at(U("keys"));

                                jwks.as_array().size() ? slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received authorization jwks: " << utility::us2s(jwks.serialize()) :
                                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Request authorization jwks: no jwk";

                                return jwks;

                            }, token);
                        }
                        else
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request authorization jwks error: no response body";
                        }
                    }
                    else
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request authorization jwks error: " << response.status_code() << " " << response.reason_phrase();
                    }
                    throw authorization_exception();

                }, token);
            }

            // make an asynchronously GET request on the OpenID Connect Authorization API to fetch the client metdadata
            pplx::task<web::json::value> request_client_metadata_from_openid_connect(web::http::client::http_client client, const nmos::api_version& version, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting OpenID Connect client metadata at " << client.base_uri().to_string();

                using namespace web::http;

                return api_request(client, methods::GET, gate, token).then([=, &gate](pplx::task<http_response> response_task)
                {
                    auto response = response_task.get(); // may throw http_exception

                    if (response.body())
                    {
                        return nmos::details::extract_json(response, gate).then([=, &gate](web::json::value body)
                        {
                            if (status_codes::OK == response.status_code())
                            {
                                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received OpenID Connect client metadata: " << utility::us2s(body.serialize());

                                // validate client metadata JSON
                                authapi_validator().validate(body, experimental::make_authapi_register_client_response_uri(version)); // may throw json_exception

                                return body;
                            }
                            else
                            {
                                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Requesting OpenID Connect client metadata error: " << response.status_code() << " " << response.reason_phrase() << " " << utility::us2s(body.serialize());
                                throw authorization_exception();
                            }
                        });
                    }
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Requesting OpenID Connect client metadata error: no response json: no client metadata";
                    throw authorization_exception();

                }, token);
            }

            // make an asynchronously POST request on the Authorization API to fetch the bearer token,
            // this is a helper function which is used by the request_token_from_client_credentials and request_token_from_refresh_token
            // see https://medium.com/@software_factotum/pkce-public-clients-and-refresh-token-d1faa4ef6965#:~:text=Refresh%20Token%20are%20credentials%20that,application%20needs%20additional%20access%20tokens.&text=Authorization%20Server%20may%20issue%20a,Client%20it%20was%20issued%20to.
            pplx::task<web::http::oauth2::experimental::oauth2_token> request_token(web::http::client::http_client client, const nmos::api_version& version, web::uri_builder& request_body_ub, const utility::string_t& client_id, const utility::string_t& client_secret, const utility::string_t& scope, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting '" << utility::us2s(scope) << "' bearer token at " << client.base_uri().to_string();

                using namespace web::http;
                using oauth2::details::oauth2_strings;
                using oauth2::experimental::oauth2_exception;
                using oauth2::experimental::oauth2_token;
                using web::http::details::mime_types;

                if (!scope.empty())
                {
                    request_body_ub.append_query(oauth2_strings::scope, uri::encode_data_string(scope), false);
                }

                http_request req(methods::POST);

                if (client_secret.empty())
                {
                    if (!client_id.empty())
                    {
                        // for Public Client or using private_key_jwt just append the client_id to query
                        request_body_ub.append_query(oauth2_strings::client_id, client_id, false);
                    }
                }
                else
                {
                    // for Confidential Client and not using private_key_jwt
                    // Build HTTP Basic authorization header with 'client_id' and 'client_secret'
                    const std::string creds_utf8(utility::conversions::to_utf8string(uri::encode_data_string(client_id) + U(":") + uri::encode_data_string(client_secret)));
                    req.headers().add(header_names::authorization, U("Basic ") + utility::conversions::to_base64(std::vector<unsigned char>(creds_utf8.begin(), creds_utf8.end())));
                }

                req.set_body(request_body_ub.query(), mime_types::application_x_www_form_urlencoded);

                return nmos::api_request(client, req, gate, token).then([=, &gate](pplx::task<http_response> response_task)
                {
                    auto response = response_task.get(); // may throw http_exception

                    if (response.body())
                    {
                        return nmos::details::extract_json(response, gate).then([=, &gate](web::json::value body)
                        {
                            if (status_codes::OK == response.status_code())
                            {
#if defined (NDEBUG)
                                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received bearer token";
#else
                                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received bearer token: " << utility::us2s(body.serialize());
#endif
                                // validate bearer token JSON
                                authapi_validator().validate(body, experimental::make_authapi_token_response_schema_uri(version)); // may throw json_exception

                                return parse_token_from_json(body); // may throw oauth2_exception
                            }
                            else
                            {
                                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Requesting '" << utility::us2s(scope) << "' bearer token error: " << response.status_code() << " " << utility::us2s(body.serialize());

                                // validate token error response JSON
                                authapi_validator().validate(body, experimental::make_authapi_token_error_response_uri(version)); // may throw json_exception

                                throw authorization_exception();
                            }
                        });
                    }
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Requesting '" << utility::us2s(scope) << "' bearer token error: no response json: no bearer token";
                    throw authorization_exception();

                }, token);
            }

            // make an asynchronously POST request on the Authorization API to fetch the bearer token using client_credentials grant
            pplx::task<web::http::oauth2::experimental::oauth2_token> request_token_from_client_credentials(web::http::client::http_client client, const nmos::api_version& version, const utility::string_t& client_id, const utility::string_t& client_secret, const utility::string_t& scope, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting '" << utility::us2s(scope) << "' bearer token using client_credentials grant at " << client.base_uri().to_string();

                using web::http::oauth2::details::oauth2_strings;

                web::uri_builder ub;
                ub.append_query(oauth2_strings::grant_type, U("client_credentials"), false);

                return request_token(client, version, ub, client_id, client_secret, scope, gate, token);
            }

            // make an asynchronously POST request on the Authorization API to fetch the bearer token using client_credentials grant with private_key_jwt for client authentication
            pplx::task<web::http::oauth2::experimental::oauth2_token> request_token_from_client_credentials_using_private_key_jwt(web::http::client::http_client client, const nmos::api_version& version, const utility::string_t& client_id, const utility::string_t& scope, const utility::string_t& client_assertion, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting '" << utility::us2s(scope) << "' bearer token using client_credentials grant with private_key_jwt at " << client.base_uri().to_string();

                using web::http::oauth2::details::oauth2_strings;

                web::uri_builder ub;
                ub.append_query(oauth2_strings::grant_type, U("client_credentials"), false);

                // use private_key_jwt client authentication
                // see https://tools.ietf.org/html/rfc7523#section-2.2
                ub.append_query(U("client_assertion_type"), U("urn:ietf:params:oauth:client-assertion-type:jwt-bearer"), false);
                ub.append_query(U("client_assertion"), client_assertion, false);

                return request_token(client, version, ub, client_id, {}, scope, gate, token);
            }

            web::uri_builder make_request_token_base_query(const utility::string_t& code, const utility::string_t& redirect_uri, const utility::string_t& code_verifier)
            {
                using web::http::oauth2::details::oauth2_strings;

                web::uri_builder ub;
                ub.append_query(oauth2_strings::grant_type, oauth2_strings::authorization_code, false);
                ub.append_query(oauth2_strings::code, web::uri::encode_data_string(code), false);
                ub.append_query(oauth2_strings::redirect_uri, web::uri::encode_data_string(redirect_uri), false);
                ub.append_query(U("code_verifier"), code_verifier, false);
                return ub;
            }

            // make an asynchronously POST request on the Authorization API to exchange authorization code for bearer token
            pplx::task<web::http::oauth2::experimental::oauth2_token> request_token_from_authorization_code(web::http::client::http_client client, const nmos::api_version& version, const utility::string_t& client_id, const utility::string_t& client_secret, const utility::string_t& scope, const utility::string_t& code, const utility::string_t& redirect_uri, const utility::string_t& code_verifier, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Exchanging authorization code: " << utility::us2s(code) << " for bearer token with code_verifier: " << utility::us2s(code_verifier) << " at " << client.base_uri().to_string();

                auto ub = make_request_token_base_query(code, redirect_uri, code_verifier);

                return request_token(client, version, ub, client_id, client_secret, scope, gate, token);
            }

            // make an asynchronously POST request on the Authorization API to exchange authorization code for bearer token with private_key_jwt for client authentication
            pplx::task<web::http::oauth2::experimental::oauth2_token> request_token_from_authorization_code_with_private_key_jwt(web::http::client::http_client client, const nmos::api_version& version, const utility::string_t& client_id, const utility::string_t& scope, const utility::string_t& code, const utility::string_t& redirect_uri, const utility::string_t& code_verifier, const utility::string_t& client_assertion, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Exchanging authorization code: " << utility::us2s(code) << " for bearer token with private_key_jwt and code_verifier: " << utility::us2s(code_verifier) << " and client_assertion: " << utility::us2s(client_assertion) << " at " << client.base_uri().to_string();

                auto ub = make_request_token_base_query(code, redirect_uri, code_verifier);
                // use private_key_jwt client authentication
                // see https://tools.ietf.org/html/rfc7523#section-2.2
                ub.append_query(U("client_assertion_type"), U("urn:ietf:params:oauth:client-assertion-type:jwt-bearer"), false);
                ub.append_query(U("client_assertion"), client_assertion, false);

                return request_token(client, version, ub, client_id, {}, scope, gate, token);
            }

            web::uri_builder make_request_token_base_query(const utility::string_t& refresh_token)
            {
                using web::http::oauth2::details::oauth2_strings;

                web::uri_builder ub;
                ub.append_query(oauth2_strings::grant_type, oauth2_strings::refresh_token, false);
                ub.append_query(oauth2_strings::refresh_token, web::uri::encode_data_string(refresh_token), false);
                return ub;
            }

            // make an asynchronously POST request on the Authorization API to fetch the bearer token using refresh_token grant
            pplx::task<web::http::oauth2::experimental::oauth2_token> request_token_from_refresh_token(web::http::client::http_client client, const nmos::api_version& version, const utility::string_t& client_id, const utility::string_t& client_secret, const utility::string_t& scope, const utility::string_t& refresh_token, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting '" << utility::us2s(scope) << "' bearer token using refresh_token grant at " << client.base_uri().to_string();

                auto ub = make_request_token_base_query(refresh_token);

                return request_token(client, version, ub, client_id, client_secret, scope, gate, token);
            }

            // make an asynchronously POST request on the Authorization API to fetch the bearer token using refresh_token grant with private_key_jwt for client authentication
            pplx::task<web::http::oauth2::experimental::oauth2_token> request_token_from_refresh_token_using_private_key_jwt(web::http::client::http_client client, const nmos::api_version& version, const utility::string_t& client_id, const utility::string_t& scope, const utility::string_t& refresh_token, const utility::string_t& client_assertion, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting '" << utility::us2s(scope) << "' bearer token using refresh_token grant with private_key_jwt at " << client.base_uri().to_string();

                using web::http::oauth2::details::oauth2_strings;

                auto ub = make_request_token_base_query(refresh_token);

                // use private_key_jwt client authentication
                // see https://tools.ietf.org/html/rfc7523#section-2.2
                ub.append_query(U("client_assertion_type"), U("urn:ietf:params:oauth:client-assertion-type:jwt-bearer"), false);
                ub.append_query(U("client_assertion"), client_assertion, false);

                return request_token(client, version, ub, client_id, {}, scope, gate, token);
            }

            // verify the redirect URI and make an asynchronously POST request on the Authorization API to exchange authorization code for bearer token with private_key_jwt for client authentication
            // this function is based on the oauth2_config::token_from_redirected_uri
            pplx::task<web::http::oauth2::experimental::oauth2_token> request_token_from_redirected_uri(web::http::client::http_client client, const nmos::api_version& version, const web::uri& redirected_uri, const utility::string_t& response_type, const utility::string_t& client_id, const utility::string_t& client_secret, const utility::string_t& scope, const utility::string_t& redirect_uri, const utility::string_t& state, const utility::string_t& code_verifier, const web::http::oauth2::experimental::token_endpoint_auth_method& token_endpoint_auth_method, const utility::string_t& client_assertion, slog::base_gate& gate, const pplx::cancellation_token& token)
            {
                using web::http::oauth2::experimental::oauth2_exception;
                using web::http::oauth2::details::oauth2_strings;
                namespace response_types = web::http::oauth2::experimental::response_types;

                std::map<utility::string_t, utility::string_t> query;

                // for Authorization Code Grant Type Response (response_type = code)
                // "If the resource owner grants the access request, the authorization
                // server issues an authorization codeand delivers it to the client by
                // adding the following parameters to the query component of the
                // redirection URI using the "application/x-www-form-urlencoded" format
                //
                // For example, the authorization server redirects the user-agent by
                // sending the following HTTP response :
                //   HTTP / 1.1 302 Found
                //   Location : https://client.example.com/cb?code=SplxlOBeZQQYbYS6WxSbIA&state=xyz
                // see https://tools.ietf.org/html/rfc6749#section-4.1.2
                if (response_type == response_types::code.name)
                {
                    query = web::uri::split_query(redirected_uri.query());
                }
                // for Implicit Grant Type Response (response_type = token)
                // "If the resource owner grants the access request, the authorization
                // server issues an access tokenand delivers it to the client by adding
                // the following parameters to the fragment component of the redirection
                // URI using the "application/x-www-form-urlencoded" format"
                //
                // For example, the authorization server redirects the user-agent by
                // sending the following HTTP response
                //   HTTP / 1.1 302 Found
                //   Location : http://example.com/cb#access_token=2YotnFZFEjr1zCsicMWpAA&state=xyz&token_type=example&expires_in=3600
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

                // for Authorization Code Grant Type Response (response_type = code)
                // do request_token_from_authorization_code
                if (response_type == response_types::code.name)
                {
                    auto code_param = query.find(oauth2_strings::code);
                    if (code_param == query.end())
                    {
                        throw oauth2_exception(U("parameter 'code' missing from redirected URI"));
                    }

                    if (web::http::oauth2::experimental::token_endpoint_auth_methods::private_key_jwt == token_endpoint_auth_method)
                    {
                        return request_token_from_authorization_code_with_private_key_jwt(client, version, client_id, scope, code_param->second, redirect_uri, code_verifier, client_assertion, gate, token);
                    }
                    else if (web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_basic == token_endpoint_auth_method)
                    {
                        return request_token_from_authorization_code(client, version, client_id, client_secret, scope, code_param->second, redirect_uri, code_verifier, gate, token);
                    }
                    else
                    {
                        throw oauth2_exception(U("token_endpoint_auth_method: '") + token_endpoint_auth_method.name + U("' is not curently supported"));
                    }
                }

                // for Implicit Grant Type Response (response_type = token)
                // extract access token from query parameters
                auto token_type_param = query.find(oauth2_strings::token_type);
                if (token_type_param == query.end())
                {
                    throw oauth2_exception(U("parameter 'token_type' missing from redirected URI"));
                }

                if (boost::algorithm::to_lower_copy(token_type_param->second) != U("bearer"))
                {
                    throw oauth2_exception(U("invalid parameter 'token_type': '") + token_type_param->second + U("', expecting 'bearer'"));
                }

                auto token_param = query.find(oauth2_strings::access_token);
                if (token_param == query.end())
                {
                    throw oauth2_exception(U("parameter 'access_token' missing from redirected URI"));
                }

                return pplx::task_from_result(web::http::oauth2::experimental::oauth2_token(token_param->second));
            }

            struct token_shared_state
            {
                web::http::oauth2::experimental::grant_type grant_type;
                web::http::oauth2::experimental::oauth2_token bearer_token;
                std::unique_ptr<web::http::client::http_client> client;
                nmos::api_version version; // issuer version
                load_rsa_private_keys_handler load_rsa_private_keys;
                bool immediate; // true = do an immediate fetch; false = loop based on time interval

                explicit token_shared_state(web::http::oauth2::experimental::grant_type grant_type, web::http::oauth2::experimental::oauth2_token bearer_token, web::http::client::http_client client, nmos::api_version version, load_rsa_private_keys_handler load_rsa_private_keys, bool immediate)
                    : grant_type(std::move(grant_type))
                    , bearer_token(std::move(bearer_token))
                    , client(std::unique_ptr<web::http::client::http_client>(new web::http::client::http_client(client)))
                    , version(std::move(version))
                    , load_rsa_private_keys(std::move(load_rsa_private_keys))
                    , immediate(immediate) {}
            };

            // task to continuously fetch the bearer token on a time interval until failure or cancellation
            pplx::task<void> do_token_requests(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, token_shared_state& token_state, bool& authorization_service_error, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                const auto access_token_refresh_interval = nmos::experimental::fields::access_token_refresh_interval(model.settings);
                const auto authorization_server_metadata = nmos::experimental::get_authorization_server_metadata(authorization_state);
                const auto client_metadata = nmos::experimental::get_client_metadata(authorization_state);
                const auto client_id = nmos::experimental::fields::client_id(client_metadata);
                const auto client_secret = client_metadata.has_string_field(nmos::experimental::fields::client_secret) ? nmos::experimental::fields::client_secret(client_metadata) : U("");
                const auto scope = nmos::experimental::fields::scope(client_metadata);
                const auto token_endpoint_auth_method = client_metadata.has_string_field(nmos::experimental::fields::token_endpoint_auth_method) ?
                    web::http::oauth2::experimental::to_token_endpoint_auth_method(nmos::experimental::fields::token_endpoint_auth_method(client_metadata)) : web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_basic;
                const auto token_endpoint = nmos::experimental::fields::token_endpoint(authorization_server_metadata);
                const auto client_assertion_lifespan = std::chrono::seconds(nmos::experimental::fields::authorization_request_max(model.settings));

                // start a background task for continuous fetching bearer token in a time interval
                return pplx::do_while([=, &model, &authorization_state, &token_state, &gate]
                {
                    auto fetch_interval = std::chrono::seconds(0);
                    if (!token_state.immediate && token_state.bearer_token.is_valid_access_token())
                    {
                        // RECOMMENDED to attempt a refresh at least 15 seconds before expiry (i.e the half-life of the shortest-lived token possible)
                        // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.2._Behaviour_-_Clients.html#refreshing-a-token
                        fetch_interval = access_token_refresh_interval < 0 ? std::chrono::seconds(token_state.bearer_token.expires_in() / 2) : std::chrono::seconds(access_token_refresh_interval);
                    }
                    token_state.immediate = false;

                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Requesting '" << utility::us2s(scope) << "' bearer token for about " << fetch_interval.count() << " seconds";

                    auto fetch_time = std::chrono::steady_clock::now();
                    return pplx::complete_at(fetch_time + fetch_interval, token).then([=, &model, &token_state, &gate]()
                    {
                        // create client assertion using private key jwt
                        utility::string_t client_assertion;
                        with_read_lock(model.mutex, [&]
                        {
                            if (web::http::oauth2::experimental::token_endpoint_auth_methods::private_key_jwt == token_endpoint_auth_method)
                            {
                                // use the 1st RSA private key from RSA private keys list to create the client_assertion
                                if (!token_state.load_rsa_private_keys)
                                {
                                    throw web::http::oauth2::experimental::oauth2_exception(U("missing RSA private key loader to extract RSA private key"));
                                }
                                auto rsa_private_keys = token_state.load_rsa_private_keys();
                                if (rsa_private_keys.empty() || rsa_private_keys[0].empty())
                                {
                                    throw web::http::oauth2::experimental::oauth2_exception(U("no RSA key to create client assertion"));
                                }
                                client_assertion = jwt_generator::create_client_assertion(client_id, client_id, token_endpoint, client_assertion_lifespan, rsa_private_keys[0], U("1"));
                            }
                        });

                        if (web::http::oauth2::experimental::grant_types::authorization_code == token_state.grant_type)
                        {
                            if (web::http::oauth2::experimental::token_endpoint_auth_methods::private_key_jwt == token_endpoint_auth_method)
                            {
                                return request_token_from_refresh_token_using_private_key_jwt(*token_state.client, token_state.version, client_id, scope, token_state.bearer_token.refresh_token(), client_assertion, gate, token);
                            }
                            else
                            {
                                return request_token_from_refresh_token(*token_state.client, token_state.version, client_id, client_secret, scope, token_state.bearer_token.refresh_token(), gate, token);
                            }
                        }
                        else if (web::http::oauth2::experimental::grant_types::client_credentials == token_state.grant_type)
                        {
                            if (web::http::oauth2::experimental::token_endpoint_auth_methods::private_key_jwt == token_endpoint_auth_method)
                            {
                                return request_token_from_client_credentials_using_private_key_jwt(*token_state.client, token_state.version, client_id, scope, client_assertion, gate, token);
                            }
                            else
                            {
                                return request_token_from_client_credentials(*token_state.client, token_state.version, client_id, client_secret, scope, gate, token);
                            }
                        }
                        else
                        {
                            throw web::http::oauth2::experimental::oauth2_exception(U("Unsupported grant: ") + token_state.grant_type.name);
                        }

                    }).then([=, &authorization_state, &token_state, &gate](web::http::oauth2::experimental::oauth2_token bearer_token)
                    {
                        token_state.bearer_token = bearer_token;

                        // update token in authorization settings
                        auto lock = authorization_state.write_lock();
                        authorization_state.bearer_token = token_state.bearer_token;
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "'" << utility::us2s(scope) << "' bearer token updated";

                        return true;
                    });
                }).then([&](pplx::task<void> finally)
                {
                    auto lock = model.write_lock(); // in order to update local state

                    try
                    {
                        finally.get();
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API Bearer token request HTTP error: " << e.what() << " [" << e.error_code() << "]";
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API Bearer token request JSON error: " << e.what();
                    }
                    catch (const web::http::oauth2::experimental::oauth2_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API Bearer token request OAuth 2.0 error: " << e.what();
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API Bearer token request error: " << e.what();
                    }
                    catch (const authorization_exception&)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API Bearer token request error";
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Authorization API Bearer token request unexpected unknown exception";
                    }

                    // reaching here, there must be something has gone wrong with the Authorization Server
                    // let select the next avaliable Authorization server
                    authorization_service_error = true;

                    model.notify();
                });
            }

            // task to continuously fetch the authorization server public keys on a time interval until failure or cancellation
            pplx::task<void> do_public_keys_requests(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, pubkeys_shared_state& pubkeys_state, bool& authorization_service_error, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                const auto fetch_interval_min(nmos::experimental::fields::fetch_authorization_public_keys_interval_min(model.settings));
                const auto fetch_interval_max(nmos::experimental::fields::fetch_authorization_public_keys_interval_max(model.settings));

                // start a background task to fetch public keys on a time interval
                return pplx::do_while([=, &model, &authorization_state, &pubkeys_state, &gate]
                {
                    auto fetch_interval = std::chrono::seconds(0);
                    if (nmos::with_read_lock(authorization_state.mutex, [&]
                    {
                        auto issuer = authorization_state.issuers.find(pubkeys_state.issuer.to_string());
                        return ((authorization_state.issuers.end() != issuer) && !pubkeys_state.one_shot && !pubkeys_state.immediate);
                    }))
                    {
                        fetch_interval = std::chrono::seconds((int)(std::uniform_real_distribution<>(fetch_interval_min, fetch_interval_max)(pubkeys_state.engine)));
                    }
                    nmos::with_write_lock(authorization_state.mutex, [&] { pubkeys_state.immediate = false; });
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Requesting authorization public keys (jwks) for about " << fetch_interval.count() << " seconds";

                    auto fetch_time = std::chrono::steady_clock::now();
                    return pplx::complete_at(fetch_time + fetch_interval, token).then([=, &authorization_state, &pubkeys_state, &gate]()
                    {
                        auto lock = authorization_state.read_lock();

                        return details::request_jwks(*pubkeys_state.client, pubkeys_state.version, gate, token);

                    }).then([&authorization_state, &pubkeys_state, &gate](web::json::value jwks_)
                    {
                        web::uri issuer;
                        bool one_shot{ false };
                        nmos::api_version auth_version;
                        nmos::with_read_lock(authorization_state.mutex, [&]
                        {
                            issuer = pubkeys_state.issuer;
                            one_shot = pubkeys_state.one_shot;
                            auth_version = pubkeys_state.version;
                        });

                        const auto jwks = nmos::experimental::get_jwks(authorization_state, issuer);

                        // are changes found in new set of jwks?
                        if(jwks != jwks_)
                        {
                            // convert jwks to array of public keys
                            auto pems = web::json::value::array();
                            for (const auto& jwk : jwks_.as_array())
                            {
                                try
                                {
                                    const auto pem = jwk_to_public_key(jwk); // can throw jwk_exception

                                    web::json::push_back(pems, web::json::value_of({
                                        { U("jwk"), jwk },
                                        { U("pem"), pem }
                                    }));
                                }
                                catch (const jwk_exception& e)
                                {
                                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Invalid jwk from " << utility::us2s(issuer.to_string()) << " JWK error: " << e.what();
                                }
                            }

                            // update jwks and jwt validator cache
                            if (pems.as_array().size())
                            {
                                nmos::experimental::update_jwks(authorization_state, issuer, jwks_, nmos::experimental::jwt_validator(pems, [auth_version](const web::json::value& payload)
                                {
                                    // validate access token payload JSON
                                    authapi_validator().validate(payload, experimental::make_authapi_token_schema_schema_uri(auth_version)); // may throw json_exception
                                }));

                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "JSON Web Token validator updated using an new set of public keys for " << utility::us2s(issuer.to_string());
                            }
                            else
                            {
                                nmos::experimental::erase_jwks(authorization_state, issuer);

                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Clear JSON Web Token validator due to receiving an empty public key list for " << utility::us2s(issuer.to_string());
                            }
                        }
                        else
                        {
                            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "No public keys changes found for " << utility::us2s(issuer.to_string());
                        }

                        return !one_shot;
                    });
                }).then([&](pplx::task<void> finally)
                {
                    auto lock = model.write_lock(); // in order to update local state

                    try
                    {
                        finally.get();

                        nmos::with_write_lock(authorization_state.mutex, [&] { pubkeys_state.received = true; });
                        authorization_service_error = false;
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API jwks request HTTP error: " << e.what() << " [" << e.error_code() << "]";
                        authorization_service_error = true;
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API jwks request JSON error: " << e.what();
                        authorization_service_error = true;
                    }
                    catch (const web::http::oauth2::experimental::oauth2_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API jwks request OAuth 2.0 error: " << e.what();
                        authorization_service_error = true;
                    }
                    catch (const jwk_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API jwks request JWK error: " << e.what();
                        authorization_service_error = true;
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API jwks request error: " << e.what();
                        authorization_service_error = true;
                    }
                    catch (const authorization_exception&)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API jwks request error";
                        authorization_service_error = true;
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Authorization API jwks request unexpected unknown exception";
                    }

                    model.notify();
                });
            }

            // fetch authorization server metadata, such as endpoints use for client registration, token fetch and public keys fetch
            bool request_authorization_server_metadata(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, bool& authorization_service_error, nmos::load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting authorization server metadata fetch";

                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                std::unique_ptr<web::http::client::http_client> client;
                bool metadata_received(false);

                pplx::cancellation_token_source cancellation_source;
                pplx::task<void> request = pplx::task_from_result();

                const auto client_metadata = nmos::experimental::get_client_metadata(authorization_state);
                const auto scopes = nmos::experimental::details::scopes(client_metadata, nmos::experimental::authorization_scopes::from_settings(model.settings));
                const auto grants = grant_types(client_metadata, grant_types_from_settings(model.settings));
                const auto token_endpoint_auth_method = nmos::experimental::details::token_endpoint_auth_method(client_metadata, token_endpoint_auth_method_from_settings(model.settings));

                for (;;)
                {
                    // wait for the thread to be interrupted because an error has been encountered with the selected authorization service
                    // or because the server is being shut down
                    condition.wait(lock, [&] { return shutdown || authorization_service_error || metadata_received || !client; });
                    if (authorization_service_error)
                    {
                        pop_authorization_service(model.settings);
                        model.notify();
                        authorization_service_error = false;

                        cancellation_source.cancel();
                        // wait without the lock since it is also used by the background tasks
                        nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };

                        request.wait();

                        client.reset();
                        cancellation_source = pplx::cancellation_token_source();
                    }
                    if (shutdown || empty_authorization_services(model.settings) || metadata_received) break;

                    const auto service = top_authorization_service(model.settings);

                    const auto auth_uri = service.second;
                    client.reset(new web::http::client::http_client(auth_uri, make_authorization_http_client_config(model.settings, load_ca_certificates, gate)));

                    auto token = cancellation_source.get_token();

                    const auto auth_version = service.first.first;
                    request = details::request_authorization_server_metadata(*client, scopes, grants, token_endpoint_auth_method, auth_version, gate, token).then([&authorization_state](web::json::value metadata)
                    {
                        // record the current connected authorization server uri
                        with_write_lock(authorization_state.mutex, [&]
                        {
                            authorization_state.authorization_server_uri = nmos::experimental::fields::issuer(metadata);
                        });

                        // cache the authorization server metadata
                        nmos::experimental::update_authorization_server_metadata(authorization_state, metadata);

                    }).then([&](pplx::task<void> finally)
                    {
                        auto lock = model.write_lock(); // in order to update local state

                        try
                        {
                            finally.get();

                            metadata_received = true;
                        }
                        catch (const web::http::http_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API metadata request HTTP error: " << e.what() << " [" << e.error_code() << "]";

                            authorization_service_error = true;
                        }
                        catch (const web::json::json_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API metadata request JSON error: " << e.what();

                            authorization_service_error = true;
                        }
                        catch (const std::exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API metadata request error: " << e.what();

                            authorization_service_error = true;
                        }
                        catch (const authorization_exception&)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API metadata request error";

                            authorization_service_error = true;
                        }
                        catch (...)
                        {
                            slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Authorization API metadata unexpected unknown exception";

                            authorization_service_error = true;
                        }
                    });
                    request.then([&]
                    {
                        condition.notify_all();
                    });

                    // wait for the request because interactions with the Authorization API endpoint must be sequential
                    condition.wait(lock, [&] { return shutdown || authorization_service_error || metadata_received; });
                }
                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                request.wait();

                return !authorization_service_error && metadata_received;
            }

            // fetch client metadata via OpenID Connect server
            bool request_client_metadata_from_openid_connect(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::experimental::save_authorization_client_handler save_authorization_client, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting OpenID Connect client metadata fetch";

                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                bool authorization_service_error(false);

                pplx::cancellation_token_source cancellation_source;
                auto token = cancellation_source.get_token();
                pplx::task<void> request = pplx::task_from_result();

                bool registered(false);
                const auto client_metadata = nmos::experimental::get_client_metadata(authorization_state);
                const auto authorization_server_metadata = nmos::experimental::get_authorization_server_metadata(authorization_state);

                // is client already registered to the Authorization server
                if(client_metadata.is_null())
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Missing client_metadata from cache";
                    return false;
                }

                const auto& auth_version = version(nmos::experimental::fields::issuer(client_metadata));

                // See https://openid.net/specs/openid-connect-registration-1_0.html#RegistrationResponse
                // registration_access_token
                //     OPTIONAL. Registration Access Token that can be used at the Client Configuration Endpoint to perform subsequent operations upon the
                //               Client registration.
                // registration_client_uri
                //     OPTIONAL. Location of the Client Configuration Endpoint where the Registration Access Token can be used to perform subsequent operations
                //               upon the resulting Client registration.
                //               Implementations MUST either return both a Client Configuration Endpoint and a Registration Access Token or neither of them.
                if (!client_metadata.has_string_field(nmos::experimental::fields::registration_access_token))
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "No registration_access_token from client_metadata";
                    return false;
                }
                const auto& registration_access_token = nmos::experimental::fields::registration_access_token(client_metadata);
                if (!client_metadata.has_string_field(nmos::experimental::fields::registration_client_uri))
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "No registration_client_uri from client_metadata";
                    return false;
                }
                const auto& registration_client_uri = nmos::experimental::fields::registration_client_uri(client_metadata);
                const auto& issuer = nmos::experimental::fields::issuer(authorization_server_metadata);

                request = request_client_metadata_from_openid_connect(web::http::client::http_client(registration_client_uri, make_authorization_http_client_config(model.settings, load_ca_certificates, make_authorization_config_handler(authorization_server_metadata, client_metadata, gate), { registration_access_token }, gate)),
                    auth_version, gate, token).then([&model, &authorization_state, issuer, save_authorization_client, &gate](web::json::value client_metadata)
                {
                    auto lock = model.write_lock();

                    // check client_secret existence for confidential client
                    if (((nmos::experimental::fields::token_endpoint_auth_method(client_metadata) == web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_basic.name)
                        || (nmos::experimental::fields::token_endpoint_auth_method(client_metadata) == web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_post.name)
                        || (nmos::experimental::fields::token_endpoint_auth_method(client_metadata) == web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_jwt.name))
                        && (!client_metadata.has_string_field(nmos::experimental::fields::client_secret)))
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Missing client_secret";
                        throw authorization_exception();
                    }

                    // scope is optional, it may not be returned by the Authorization server, just insert it,
                    // as it is required for the authorization support
                    if (!client_metadata.has_field(nmos::experimental::fields::scope))
                    {
                        client_metadata[nmos::experimental::fields::scope] = web::json::value::string(make_scope(nmos::experimental::authorization_scopes::from_settings(model.settings)));
                    }
                    // grant_types is optional, it may not be returned by the Authorization server, just insert it,
                    // as it is required for the authorization support
                    if (!client_metadata.has_field(nmos::experimental::fields::grant_types))
                    {
                        client_metadata[nmos::experimental::fields::grant_types] = make_grant_types(grant_types_from_settings(model.settings));
                    }
                    // token_endpoint_auth_method is optional, it may not be returning by the Authorization server, just insert it,
                    // as it is required for the authorization support
                    if (!client_metadata.has_field(nmos::experimental::fields::token_endpoint_auth_method))
                    {
                        client_metadata[nmos::experimental::fields::token_endpoint_auth_method] = web::json::value::string(token_endpoint_auth_method_from_settings(model.settings).name);
                    }

                    // store client metadata to settings
                    // hmm, may store the only required fields
                    nmos::experimental::update_client_metadata(authorization_state, client_metadata);

                    // do callback to safely store the client metadata
                    // Client metadata SHOULD be stored by the client in a safe, permission-restricted, location in non-volatile memory in case of a device restart to prevent duplicate registrations.
                    // Client secrets SHOULD be encrypted before being stored to reduce the chance of client secret leaking.
                    // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.2._Behaviour_-_Clients.html#client-credentials
                    if (save_authorization_client)
                    {
                        save_authorization_client(web::json::value_of({
                            { nmos::experimental::fields::authorization_server_uri, issuer },
                            { nmos::experimental::fields::client_metadata, client_metadata }
                        }));
                    }

                }).then([&](pplx::task<void> finally)
                {
                    auto lock = model.write_lock(); // in order to update local state

                    try
                    {
                        finally.get();

                        registered = true;
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "OpenID Connect Authorization API client metadata retreieve HTTP error: " << e.what() << " [" << e.error_code() << "]";

                        authorization_service_error = true;
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "OpenID Connect Authorization API client metadata retreieve JSON error: " << e.what();

                        authorization_service_error = true;
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "OpenID Connect Authorization API client metadata retreieve error: " << e.what();

                        authorization_service_error = true;
                    }
                    catch (const authorization_exception&)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "OpenID Connect Authorization API client metadata retreieve error";

                        authorization_service_error = true;
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "OpenID Connect Authorization API client metadata retreieve unexpected unknown exception";

                        authorization_service_error = true;
                    }

                    model.notify();
                });

                // wait for the request because interactions with the Authorization API endpoint must be sequential
                condition.wait(lock, [&] { return shutdown || authorization_service_error || registered; });

                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                request.wait();
                return !authorization_service_error && registered;
            }

            // register client to the Authorization server
            bool client_registration(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::experimental::save_authorization_client_handler save_authorization_client, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting authorization client registration";

                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                bool authorization_service_error(false);

                pplx::cancellation_token_source cancellation_source;
                auto token = cancellation_source.get_token();
                pplx::task<void> request = pplx::task_from_result();

                bool registered(false);

                const auto auth_version = top_authorization_service(model.settings).first.first;

                // create client metadata from settings
                // see https://tools.ietf.org/html/rfc7591#section-2
                const auto client_name = model.settings.has_field(nmos::fields::label) ? nmos::fields::label(model.settings) : U("");
                const std::vector<web::uri> redirect_uris = { make_authorization_redirect_uri(model.settings) };
                const auto scopes = nmos::experimental::authorization_scopes::from_settings(model.settings);
                const auto grants = grant_types_from_settings(model.settings);

                std::set<web::http::oauth2::experimental::response_type> response_types;
                if (grants.end() != std::find_if(grants.begin(), grants.end(), [](const web::http::oauth2::experimental::grant_type& grant) { return web::http::oauth2::experimental::grant_types::authorization_code == grant; }))
                {
                    response_types.insert(web::http::oauth2::experimental::response_types::code);
                }
                if (grants.end() != std::find_if(grants.begin(), grants.end(), [](const web::http::oauth2::experimental::grant_type& grant) { return web::http::oauth2::experimental::grant_types::implicit == grant; }))
                {
                    response_types.insert(web::http::oauth2::experimental::response_types::token);
                }
                if (response_types.empty())
                {
                    response_types.insert(web::http::oauth2::experimental::response_types::none);
                }

                const auto token_endpoint_auth_method = token_endpoint_auth_method_from_settings(model.settings);
                const auto authorization_server_metadata = get_authorization_server_metadata(authorization_state);
                const auto& registration_endpoint = web::uri(nmos::experimental::fields::registration_endpoint(authorization_server_metadata));
                const auto& issuer = nmos::experimental::fields::issuer(authorization_server_metadata);
                const auto jwks_uri = make_jwks_uri(model.settings);
                const auto& initial_access_token = nmos::experimental::fields::initial_access_token(model.settings);

                request = request_client_registration(web::http::client::http_client(registration_endpoint, make_authorization_http_client_config(model.settings, load_ca_certificates, make_authorization_config_handler({}, {}, gate), { initial_access_token }, gate)),
                    client_name, redirect_uris, {}, response_types, scopes, grants, token_endpoint_auth_method, {}, jwks_uri, auth_version, gate, token).then([&model, &authorization_state, issuer, token_endpoint_auth_method, save_authorization_client, &gate](web::json::value client_metadata)
                {
                    auto lock = model.write_lock();

                    // check client_secret existence for confidential client
                    if (client_metadata.has_string_field(nmos::experimental::fields::token_endpoint_auth_method))
                    {
                        if (((nmos::experimental::fields::token_endpoint_auth_method(client_metadata) == web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_basic.name)
                            || (nmos::experimental::fields::token_endpoint_auth_method(client_metadata) == web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_post.name)
                            || (nmos::experimental::fields::token_endpoint_auth_method(client_metadata) == web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_jwt.name))
                            && (!client_metadata.has_string_field(nmos::experimental::fields::client_secret)))
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Missing client_secret";
                            throw authorization_exception();
                        }
                    }
                    else
                    {
                        if (((web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_basic == token_endpoint_auth_method)
                            || (web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_post == token_endpoint_auth_method)
                            || (web::http::oauth2::experimental::token_endpoint_auth_methods::client_secret_jwt == token_endpoint_auth_method))
                            && (!client_metadata.has_string_field(nmos::experimental::fields::client_secret)))
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Missing client_secret";
                            throw authorization_exception();
                        }
                    }

                    // scope is optional, it may not be returned by the Authorization server, just insert it,
                    // as it is required for the authorization support
                    if (!client_metadata.has_field(nmos::experimental::fields::scope))
                    {
                        client_metadata[nmos::experimental::fields::scope] = web::json::value::string(make_scope(nmos::experimental::authorization_scopes::from_settings(model.settings)));
                    }
                    // grant_types is optional, it may not be returned by the Authorization server, just insert it,
                    // as it is required for the authorization support
                    if (!client_metadata.has_field(nmos::experimental::fields::grant_types))
                    {
                        client_metadata[nmos::experimental::fields::grant_types] = make_grant_types(grant_types_from_settings(model.settings));
                    }
                    // token_endpoint_auth_method is optional, it may not be returning by the Authorization server, just insert it,
                    // as it is required for the authorization support
                    if (!client_metadata.has_field(nmos::experimental::fields::token_endpoint_auth_method))
                    {
                        client_metadata[nmos::experimental::fields::token_endpoint_auth_method] = web::json::value::string(token_endpoint_auth_method_from_settings(model.settings).name);
                    }

                    // store client metadata to settings
                    // hmm, may store the only required fields
                    nmos::experimental::update_client_metadata(authorization_state, client_metadata);

                    // hmm, do a callback allowing user to store the client credentials
                    // Client credentials SHOULD be stored by the client in a safe, permission-restricted, location in non-volatile memory in case of a device restart to prevent duplicate registrations. Client secrets SHOULD be encrypted before being stored to reduce the chance of client secret leaking.
                    // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.2._Behaviour_-_Clients.html#client-credentials
                    if (save_authorization_client)
                    {
                        save_authorization_client(web::json::value_of({
                            { nmos::experimental::fields::authorization_server_uri, issuer },
                            { nmos::experimental::fields::client_metadata, client_metadata }
                        }));
                    }

                }).then([&](pplx::task<void> finally)
                {
                    auto lock = model.write_lock(); // in order to update local state

                    try
                    {
                        finally.get();

                        registered = true;
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API client registration HTTP error: " << e.what() << " [" << e.error_code() << "]";

                        authorization_service_error = true;
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API client registration JSON error: " << e.what();

                        authorization_service_error = true;
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API client registration error: " << e.what();

                        authorization_service_error = true;
                    }
                    catch (const authorization_exception&)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API client registration error";

                        authorization_service_error = true;
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Authorization API client registration unexpected unknown exception";

                        authorization_service_error = true;
                    }

                    model.notify();
                });

                // wait for the request because interactions with the Authorization API endpoint must be sequential
                condition.wait(lock, [&] { return shutdown || authorization_service_error || registered; });

                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                request.wait();
                return !authorization_service_error && registered;
            }

            // start authorization code workflow
            // see https://tools.ietf.org/html/rfc8252#section-4.1
            bool authorization_code_flow(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::experimental::request_authorization_code_handler request_authorization_code, slog::base_gate& gate)
            {
                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                bool authorization_service_error(false);

                const auto& settings = model.settings;

                const auto authorization_server_metadata = get_authorization_server_metadata(authorization_state);
                const web::uri authorization_endpoint(nmos::experimental::fields::authorization_endpoint(authorization_server_metadata));
                const auto code_challenge_methods_supported(nmos::experimental::fields::code_challenge_methods_supported(authorization_server_metadata));
                const auto client_metadata = get_client_metadata(authorization_state);
                const auto client_id(nmos::experimental::fields::client_id(client_metadata));
                const web::uri redirct_uri(nmos::experimental::fields::redirect_uris(client_metadata).size() ? nmos::experimental::fields::redirect_uris(client_metadata).at(0).as_string() : U(""));
                const auto scopes = nmos::experimental::details::scopes(nmos::experimental::fields::scope(client_metadata));

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting authorization code flow for scope: '" << nmos::experimental::details::make_scope(scopes) << "'";

                auto access_token_received = false;
                auto authorization_flow = nmos::experimental::authorization_state::request_code;

                // start the authorization code grant workflow, the authorization URI is required to
                // be loaded in the web browser to kick start the authorization code grant workflow
                if (request_authorization_code)
                {
                    nmos::with_write_lock(authorization_state.mutex, [&]
                    {
                        authorization_state.authorization_flow = nmos::experimental::authorization_state::request_code;
                        request_authorization_code(make_authorization_code_uri(authorization_endpoint, client_id, redirct_uri, web::http::oauth2::experimental::response_types::code, scopes, code_challenge_methods_supported, authorization_state.state, authorization_state.code_verifier));
                    });

                    // wait for the access token
                    const auto& authorization_code_flow_max = nmos::experimental::fields::authorization_code_flow_max(settings);
                    if (authorization_code_flow_max > -1)
                    {
                        // wait access token with timeout
                        if (!model.wait_for(lock, std::chrono::seconds(authorization_code_flow_max), [&] {
                            authorization_flow = with_read_lock(authorization_state.mutex, [&] { return authorization_state.authorization_flow; });
                            return shutdown || nmos::experimental::authorization_state::failed == authorization_flow || nmos::experimental::authorization_state::access_token_received == authorization_flow; }))
                        {
                            // authorization code workflow timeout
                            authorization_service_error = true;
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "authorization code workflow timeout";
                        }
                        else if (nmos::experimental::authorization_state::access_token_received == authorization_flow)
                        {
                            // access token received
                            access_token_received = true;
                            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "access token received";
                        }
                        else
                        {
                            // authorization code workflow failure
                            authorization_service_error = true;
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "authorization code workflow failure";
                        }
                    }
                    else
                    {
                        // wait access token without timeout
                        condition.wait(lock, [&] {
                            authorization_flow = with_read_lock(authorization_state.mutex, [&] { return authorization_state.authorization_flow; });
                            return shutdown || nmos::experimental::authorization_state::failed == authorization_flow || nmos::experimental::authorization_state::access_token_received == authorization_flow; });

                        if (nmos::experimental::authorization_state::access_token_received == authorization_flow)
                        {
                            // access token received
                            access_token_received = true;
                            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "access token received";
                        }
                        else
                        {
                            // authorization code workflow failure
                            authorization_service_error = true;
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "authorization code workflow failure";
                        }
                    }
                }
                else
                {
                    // no handler to start the authorization code grant workflow
                    authorization_service_error = true;
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "no authorization code workflow handler";
                }

                model.notify();

                return !authorization_service_error && access_token_received;
            }

            // fetch the bearer access token for the required scope(s) to access the protected APIs
            // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.2._Behaviour_-_Clients.html#requesting-a-token
            // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.2._Behaviour_-_Clients.html#accessing-protected-resources
            // fetch the token issuer(authorization server)'s public keys fpr validating the incoming bearer access token
            // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.5._Behaviour_-_Resource_Servers.html#public-keys
            void authorization_operation(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, load_ca_certificates_handler load_ca_certificates, load_rsa_private_keys_handler load_rsa_private_keys, bool immediate_token_fetch, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting authorization operation: " << (immediate_token_fetch ? "immediate fetch token" : "fetch token at next interval");

                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                const auto authorization_server_metadata = get_authorization_server_metadata(authorization_state);
                const web::uri jwks_uri(nmos::experimental::fields::jwks_uri(authorization_server_metadata));
                const web::uri token_endpoint(nmos::experimental::fields::token_endpoint(authorization_server_metadata));
                const auto& authorization_flow = nmos::experimental::fields::authorization_flow(model.settings);
                const auto& grant = (web::http::oauth2::experimental::grant_types::client_credentials.name == authorization_flow) ? web::http::oauth2::experimental::grant_types::client_credentials : web::http::oauth2::experimental::grant_types::authorization_code;
                const auto authorization_version = top_authorization_service(model.settings).first.first;

                bool authorization_service_error(false);

                pplx::cancellation_token_source cancellation_source;

                auto pubkeys_requests(pplx::task_from_result());

                with_write_lock(authorization_state.mutex, [&]
                {
                    auto& pubkeys_state = authorization_state.pubkeys_state;
                    pubkeys_state.client.reset(new web::http::client::http_client{ jwks_uri, make_authorization_http_client_config(model.settings, load_ca_certificates, gate) });
                    pubkeys_state.version = authorization_version;
                    pubkeys_state.issuer = nmos::experimental::fields::issuer(authorization_server_metadata);
                });

                auto bearer_token_requests(pplx::task_from_result());
                web::http::oauth2::experimental::oauth2_token bearer_token;
                std::set<scope> scopes;
                const auto client_metadata = nmos::experimental::get_client_metadata(authorization_state);
                nmos::with_read_lock(authorization_state.mutex, [&]
                {
                    bearer_token = authorization_state.bearer_token.is_valid_access_token() ? authorization_state.bearer_token : web::http::oauth2::experimental::oauth2_token{};
                    scopes = nmos::experimental::details::scopes(client_metadata, nmos::experimental::authorization_scopes::from_settings(model.settings));
                });
                token_shared_state token_state(
                    grant,
                    bearer_token,
                    { token_endpoint, make_authorization_http_client_config(model.settings, load_ca_certificates, gate) },
                    authorization_version,
                    std::move(load_rsa_private_keys),
                    immediate_token_fetch
                );

                auto token = cancellation_source.get_token();

                // start a background task to fetch public keys from authorization server
                if (nmos::experimental::fields::server_authorization(model.settings))
                {
                    pubkeys_requests = do_public_keys_requests(model, authorization_state, authorization_state.pubkeys_state, authorization_service_error, gate, token);
                }

                // start a background task to fetch bearer access token from authorization server
                if (nmos::experimental::fields::client_authorization(model.settings) && scopes.size())
                {
                    bearer_token_requests = do_token_requests(model, authorization_state, token_state, authorization_service_error, gate, token);
                }

                // wait for the request because interactions with the Authorization API endpoint must be sequential
                condition.wait(lock, [&] { return shutdown || authorization_service_error; });

                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                pubkeys_requests.wait();
                bearer_token_requests.wait();
            }

            // make an asynchronously GET request over the Token Issuer to fetch issuer metadata
            bool request_token_issuer_metadata(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
            {
                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                bool authorization_service_error(false);

                bool metadata_received(false);

                pplx::cancellation_token_source cancellation_source;

                // wait for the thread to be interrupted because of no matching public keys from the received token or because the server is being shut down
                condition.wait(lock, [&] { return shutdown || nmos::with_read_lock(authorization_state.mutex, [&] { return authorization_state.fetch_token_issuer_pubkeys; }); });

                if (shutdown) return false;

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting authorization token issuer metadata fetch";

                const auto token_issuer = nmos::with_write_lock(authorization_state.mutex, [&]
                {
                    authorization_state.fetch_token_issuer_pubkeys = false;
                    return authorization_state.token_issuer;
                });
                if (token_issuer.is_empty())
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "No authorization token's issuer to fetch server metadata";
                    return false;
                }

                const auto client_metadata = nmos::experimental::get_client_metadata(authorization_state);
                const auto scopes = nmos::experimental::details::scopes(client_metadata, nmos::experimental::authorization_scopes::from_settings(model.settings));
                const auto grants = grant_types(client_metadata, grant_types_from_settings(model.settings));
                const auto token_endpoint_auth_method = nmos::experimental::details::token_endpoint_auth_method(client_metadata, token_endpoint_auth_method_from_settings(model.settings));

                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Using authorization token's issuer " << utility::us2s(token_issuer.to_string()) << " to fetch server metadata";

                web::http::client::http_client client(make_authorization_service_uri(token_issuer), make_authorization_http_client_config(model.settings, load_ca_certificates, gate));

                auto token = cancellation_source.get_token();

                auto request = details::request_authorization_server_metadata(client, scopes, grants, token_endpoint_auth_method, version(token_issuer), gate, token).then([&, token_issuer](web::json::value metadata)
                {
                    // cache the issuer metadata
                    nmos::experimental::update_authorization_server_metadata(authorization_state, token_issuer, metadata);

                }).then([&](pplx::task<void> finally)
                {
                    auto lock = model.write_lock(); // in order to update local state

                    try
                    {
                        finally.get();

                        metadata_received = true;
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API metadata request HTTP error: " << e.what() << " [" << e.error_code() << "]";

                        authorization_service_error = true;
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API metadata request JSON error: " << e.what();

                        authorization_service_error = true;
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API metadata request error: " << e.what();

                        authorization_service_error = true;
                    }
                    catch (const authorization_exception&)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Authorization API metadata request error";

                        authorization_service_error = true;
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Authorization API metadata request unexpected unknown exception";

                        authorization_service_error = true;
                    }
                });
                request.then([&]
                {
                    condition.notify_all();
                });

                // wait for the request because interactions with the Authorization API endpoint must be sequential
                condition.wait(lock, [&] { return shutdown || authorization_service_error || metadata_received; });

                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                request.wait();

                return !authorization_service_error && metadata_received;
            }

            // make an asynchronously GET request over the Token Issuer to fetch public keys
            void request_token_issuer_public_keys(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting authorization token issuer's public keys fetch";

                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                bool authorization_service_error(false);

                pplx::cancellation_token_source cancellation_source;

                const auto token_issuer = with_read_lock(authorization_state.mutex, [&] { return authorization_state.token_issuer; });
                const auto jwks_uri = nmos::experimental::fields::jwks_uri(get_authorization_server_metadata(authorization_state, token_issuer));

                auto authorization_version = version(token_issuer);
                pubkeys_shared_state pubkeys_state(
                    { jwks_uri, make_authorization_http_client_config(model.settings, load_ca_certificates, gate) },
                    authorization_version,
                    token_issuer,
                    true
                );

                // update the authorization_behaviour_thread's fetch public keys shared state, public keys are going to be fetched from this token issuer from now on
                with_write_lock(authorization_state.mutex, [&]
                {
                    auto& pubkeys_state = authorization_state.pubkeys_state;
                    pubkeys_state.client.reset(new web::http::client::http_client{ jwks_uri, make_authorization_http_client_config(model.settings, load_ca_certificates, gate) });
                    pubkeys_state.version = authorization_version;
                    pubkeys_state.issuer = token_issuer;
                });

                auto token = cancellation_source.get_token();

                // start a one-shot background task to fetch public keys from the token issuer
                auto pubkeys_requests = do_public_keys_requests(model, authorization_state, pubkeys_state, authorization_service_error, gate, token);

                // wait for the request because interactions with the Authorization API endpoint must be sequential
                condition.wait(lock, [&] { return shutdown || authorization_service_error || pubkeys_state.received; });

                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                pubkeys_requests.wait();
            }
        }
    }
}

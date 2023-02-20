#ifndef NMOS_AUTHORIZATION_OPERATION_H
#define NMOS_AUTHORIZATION_OPERATION_H

#include "nmos/authorization_behaviour.h"

namespace slog
{
    class base_gate;
}

namespace web
{
    namespace http
    {
        namespace oauth2
        {
            namespace experimental
            {
                struct token_endpoint_auth_method;
            }
        }
    }
}

namespace nmos
{
    struct base_model;

    namespace experimental
    {
        struct authorization_state;

        namespace details
        {
            // construct authorization client config based on settings
            // with the remaining options defaulted, e.g. authorization request timeout
            web::http::client::http_client_config make_authorization_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, authorization_config_handler make_authorization_config, const web::http::oauth2::experimental::oauth2_token& bearer_token, slog::base_gate& gate);
            inline web::http::client::http_client_config make_authorization_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
            {
                return make_authorization_http_client_config(settings, load_ca_certificates, {}, {}, gate);
            }

            // verify the redirect URI and make an asynchronously POST request on the Authorization API to exchange authorization code for bearer token
            // this function is based on the oauth2::token_from_redirected_uri
            pplx::task<web::http::oauth2::experimental::oauth2_token> request_token_from_redirected_uri(web::http::client::http_client client, const nmos::api_version& version, const web::uri& redirected_uri, const utility::string_t& response_type, const utility::string_t& client_id, const utility::string_t& client_secret, const utility::string_t& scope, const utility::string_t& redirect_uri, const utility::string_t& state, const utility::string_t& code_verifier, const web::http::oauth2::experimental::token_endpoint_auth_method& token_endpoint_auth_method, const utility::string_t& client_assertion, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());

            // make an asynchronously GET request on the Authorization API to fetch authorization server metadata
            bool request_authorization_server_metadata(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, bool& authorization_service_error, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);

            // make an asynchronously GET request on the OpenID Authorization API to fetch client metadata
            bool request_client_metadata_from_openid_connect(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::experimental::save_authorization_client_handler client_registered, slog::base_gate& gate);

            // make an asynchronously POST request on the Authorization API to register a client
            // see https://tools.ietf.org/html/rfc6749#section-2
            // see https://tools.ietf.org/html/rfc7591#section-3.1
            bool client_registration(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::experimental::save_authorization_client_handler client_registered, slog::base_gate& gate);

            // start authorization code workflow
            // see https://tools.ietf.org/html/rfc8252#section-4.1
            bool authorization_code_flow(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::experimental::request_authorization_code_handler request_authorization_code, slog::base_gate& gate);

            // The bearer token is used for accessing protected APIs
            // The pems are used for validating incoming access token
            // fetch the bearer access token for the required scope(s) to access the protected APIs
            // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.2._Behaviour_-_Clients.html#requesting-a-token
            // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.2._Behaviour_-_Clients.html#accessing-protected-resources
            // fetch the Token Issuer(authorization server)'s public keys fpr validating the incoming bearer access token
            // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.5._Behaviour_-_Resource_Servers.html#public-keys
            void authorization_operation(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, load_rsa_private_keys_handler load_rsa_private_keys, bool immediate_token_fetch, slog::base_gate& gate);

            // make an asynchronously GET request over the Token Issuer to fetch issuer metadata
            bool request_token_issuer_metadata(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);

            // make an asynchronously GET request over the Token Issuer to fetch public keys
            void request_token_issuer_public_keys(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);
        }
    }
}

#endif

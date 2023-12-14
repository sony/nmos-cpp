#ifndef NMOS_AUTHORIZATION_HANDLERS_H
#define NMOS_AUTHORIZATION_HANDLERS_H

#include <functional>
#include <cpprest/oauth2.h>
#include "cpprest/api_router.h"
#include "nmos/scope.h"
#include "nmos/settings.h"

namespace slog
{
    class base_gate;
}

namespace web
{
    class uri;

    namespace json
    {
        class value;
    }
}

namespace nmos
{
    struct base_model;

    namespace experimental
    {
        struct authorization_state;

        struct authorization_error
        {
            enum status_t
            {
                succeeded,
                without_authentication, // failure: access protected resource request without authentication
                insufficient_scope, // failure: access protected resource request requires higher privileges
                no_matching_keys, // failure: no matching keys for the token validation
                failed  // failure: access protected resource request with authentication but failed
            };

            authorization_error() : value(without_authentication) {}
            authorization_error(status_t value, const std::string& message = {}) : value(value), message(message) {}

            status_t value;
            std::string message;

            operator bool() const { return succeeded == value; }
        };

        namespace fields
        {
            // authorization_server_uri: the uri of the authorization server, where the client is registered
            const web::json::field_as_string_or authorization_server_uri{ U("authorization_server_uri"), U("") };

            // client_metadata: the registered client metadata
            // already defined in nmos/json_fields.h
            //const web::json::field_as_value client_metadata{ U("client_metadata") };
        }

        // callback to supply a list of authorization clients
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        // this callback should not throw exceptions
        // example JSON of the authorization client list
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
        typedef std::function<web::json::value()> load_authorization_clients_handler;

        // callback after authorization client has registered
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        // this callback should not throw exceptions
        // example JSON of the client_metadata
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
        typedef std::function<void(const web::json::value& client_metadata)> save_authorization_client_handler;

        // callback on requesting to start off the authorization code grant flow
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        // this callback should not throw exceptions
        typedef std::function<void(const web::uri& authorization_code_uri)> request_authorization_code_handler;

        // helper function to load from the authorization clients file
        web::json::value load_authorization_clients_file(const utility::string_t& filename, slog::base_gate& gate);

        // helper function to update the authorization clients file
        void update_authorization_clients_file(const utility::string_t& filename, const web::json::value& authorization_client, slog::base_gate& gate);

        // construct callback to load a table of authorization server uri vs authorization clients metadata from file based on settings seed_id
        load_authorization_clients_handler make_load_authorization_clients_handler(const nmos::settings& settings, slog::base_gate& gate);

        // construct callback to save authorization client metadata to file based on seed_id from settings
        save_authorization_client_handler make_save_authorization_client_handler(const nmos::settings& settings, slog::base_gate& gate);

        // construct callback to start the authorization code flow request on a browser
        request_authorization_code_handler make_request_authorization_code_handler(slog::base_gate& gate);

        // callback to validate OAuth 2.0 authorization access token
        // this callback should not throw exceptions
        typedef std::function <authorization_error(const utility::string_t& access_token)> validate_authorization_token_handler;
        // construct callback to validate OAuth 2.0 authorization access token
        validate_authorization_token_handler make_validate_authorization_token_handler(authorization_state& authorization_state, slog::base_gate& gate);

        // callback to return the OAuth 2.0 validation route handler
        // this callback is executed at the beginning while walking the supported API routes
        typedef std::function<web::http::experimental::listener::route_handler(const nmos::experimental::scope& scope)> validate_authorization_handler;
        // construct callback to validate OAuth 2.0 authorization
        validate_authorization_handler make_validate_authorization_handler(nmos::base_model& model, authorization_state& authorization_state, validate_authorization_token_handler access_token_validation, slog::base_gate& gate);

        // callback to return OAuth 2.0 authorization bearer token
        // this callback is execute while create http_client
        // this callback should not throw exceptions
        typedef std::function<web::http::oauth2::experimental::oauth2_token()> get_authorization_bearer_token_handler;
        // construct callback to retrieve OAuth 2.0 authorization bearer token
        get_authorization_bearer_token_handler make_get_authorization_bearer_token_handler(authorization_state& authorization_state, slog::base_gate& gate);
    }
}

#endif

#ifndef NMOS_AUTHORIZATION_STATE_H
#define NMOS_AUTHORIZATION_STATE_H

#include <memory>
#include "cpprest/http_client.h"
#include "cpprest/oauth2.h"
#include "nmos/api_version.h"
#include "nmos/issuers.h"
#include "nmos/mutex.h"
#include "nmos/random.h"

namespace nmos
{
    namespace experimental
    {
        struct pubkeys_shared_state
        {
            nmos::details::seed_generator seeder;
            std::default_random_engine engine;
            std::unique_ptr<web::http::client::http_client> client;
            nmos::api_version version;
            web::uri issuer;
            bool immediate; // true = do an immediate fetch; false = do time interval fetch
            bool received;

            pubkeys_shared_state()
                : engine(seeder)
                , immediate(true)
                , received(false) {}

            pubkeys_shared_state(web::http::client::http_client client, nmos::api_version version, web::uri issuer)//, bool one_shot = false)
                : engine(seeder)
                , client(std::unique_ptr<web::http::client::http_client>(new web::http::client::http_client(client)))
                , version(std::move(version))
                , issuer(std::move(issuer))
                , immediate(true)
                , received(false) {}
        };

        struct authorization_state
        {
            // mutex to be used to protect the members of the authorization_state from simultaneous access by multiple threads
            mutable nmos::mutex mutex;

            // authorization code flow settings
            utility::string_t state;
            utility::string_t code_verifier;

            enum authorization_flow_type
            {
                request_code,
                exchange_code_for_access_token,
                fetch_access_token,
                access_token_received,
                failed
            };
            // current status of the authorization flow
            authorization_flow_type authorization_flow;

            // fetch public keys from token issuer(Authorization server), in event when no matching keys in cache to validate token
            // it is used for triggering the authorization_token_issuer_thread to fetch the token issuer metadata follow by fetching the issuer public keys
            bool fetch_token_issuer_pubkeys;
            web::uri token_issuer;

            // map of issuer (authorization server) to jwt_validator set for access token validation
            nmos::experimental::issuers issuers;
            // currently connected authorization server
            web::uri authorization_server_uri;

            // OAuth 2.0 bearer token to access authorizaton protected APIs
            web::http::oauth2::experimental::oauth2_token bearer_token;

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }

            authorization_state()
                : state{}
                , code_verifier{}
                , authorization_flow(request_code)
                , fetch_token_issuer_pubkeys{ false }
                , token_issuer{}
                , authorization_server_uri{}
            {}
        };

        web::json::value get_authorization_server_metadata(const authorization_state& authorization_state, const web::uri& authorization_server_uri);
        web::json::value get_authorization_server_metadata(const authorization_state& authorization_state);

        web::json::value get_client_metadata(const authorization_state& authorization_state, const web::uri& authorization_server_uri);
        web::json::value get_client_metadata(const authorization_state& authorization_state);

        web::json::value get_jwks(const authorization_state& authorization_state, const web::uri& authorization_server_uri);
        web::json::value get_jwks(const authorization_state& authorization_state);

        void update_authorization_server_metadata(authorization_state& authorization_state, const web::uri& authorization_server_uri, const web::json::value& authorization_server_metadata);
        void update_authorization_server_metadata(authorization_state& authorization_state, const web::json::value& authorization_server_metadata);

        void update_client_metadata(authorization_state& authorization_state, const web::uri& authorization_server_uri, const web::json::value& client_metadata);
        void update_client_metadata(authorization_state& authorization_state, const web::json::value& client_metadata);

        void update_jwks(authorization_state& authorization_state, const web::uri& authorization_server_uri, const web::json::value& jwks, const nmos::experimental::jwt_validator& jwt_validator);
        void update_jwks(authorization_state& authorization_state, const web::json::value& jwks, const nmos::experimental::jwt_validator& jwt_validator);

        void erase_client_metadata(authorization_state& authorization_state, const web::uri& authorization_server_uri);
        void erase_client_metadata(authorization_state& authorization_state);

        void erase_jwks(authorization_state& authorization_state, const web::uri& authorization_server_uri);
        void erase_jwks(authorization_state& authorization_state);
    }
}

#endif

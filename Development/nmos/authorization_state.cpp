#include "nmos/authorization_state.h"

#include "nmos/json_fields.h"

namespace nmos
{
    namespace experimental
    {
        web::json::value get_authorization_server_metadata(const authorization_state& authorization_state, const web::uri& authorization_server_uri)
        {
            const auto& issuer = authorization_state.issuers.find(authorization_server_uri);
            if (authorization_state.issuers.end() != issuer)
            {
                return nmos::experimental::fields::authorization_server_metadata(issuer->second.settings);
            }
            return{};
        }
        web::json::value get_authorization_server_metadata(const authorization_state& authorization_state)
        {
            return get_authorization_server_metadata(authorization_state, authorization_state.authorization_server_uri);
        }

        web::json::value get_client_metadata(const authorization_state& authorization_state, const web::uri& authorization_server_uri)
        {
            const auto& issuer = authorization_state.issuers.find(authorization_server_uri);
            if (authorization_state.issuers.end() != issuer)
            {
                return nmos::experimental::fields::client_metadata(issuer->second.settings);
            }
            return{};
        }
        web::json::value get_client_metadata(const authorization_state& authorization_state)
        {
            return get_client_metadata(authorization_state, authorization_state.authorization_server_uri);
        }

        web::json::value get_jwks(const authorization_state& authorization_state, const web::uri& authorization_server_uri)
        {
            const auto& issuer = authorization_state.issuers.find(authorization_server_uri);
            if (authorization_state.issuers.end() != issuer)
            {
                return nmos::experimental::fields::jwks(issuer->second.settings);
            }
            return{};
        }
        web::json::value get_jwks(const authorization_state& authorization_state)
        {
            return get_jwks(authorization_state, authorization_state.authorization_server_uri);
        }

        void update_authorization_server_metadata(authorization_state& authorization_state, const web::uri& authorization_server_uri, const web::json::value& authorization_server_metadata)
        {
            auto issuer = authorization_state.issuers.find(authorization_server_uri);
            if (authorization_state.issuers.end() != issuer)
            {
                // update
                auto& settings = issuer->second.settings;
                settings[nmos::experimental::fields::authorization_server_metadata] = authorization_server_metadata;
            }
            else
            {
                // insert
                authorization_state.issuers.insert(std::make_pair<web::uri, nmos::experimental::issuer>(
                    authorization_server_uri.to_string(),
                    { web::json::value_of({
                        { nmos::experimental::fields::authorization_server_metadata, authorization_server_metadata },
                        { nmos::experimental::fields::jwks, {} },
                        { nmos::experimental::fields::client_metadata, {} }
                    }), nmos::experimental::jwt_validator{} }
                ));
            }
        }
        void update_authorization_server_metadata(authorization_state& authorization_state, const web::json::value& authorization_server_metadata)
        {
            update_authorization_server_metadata(authorization_state, authorization_state.authorization_server_uri, authorization_server_metadata);
        }

        void update_client_metadata(authorization_state& authorization_state, const web::uri& authorization_server_uri, const web::json::value& client_metadata)
        {
            auto issuer = authorization_state.issuers.find(authorization_server_uri);
            if (authorization_state.issuers.end() != issuer)
            {
                // update
                auto& settings = issuer->second.settings;
                settings[nmos::experimental::fields::client_metadata] = client_metadata;
            }
            else
            {
                // insert
                authorization_state.issuers.insert(std::make_pair<web::uri, nmos::experimental::issuer>(
                    authorization_server_uri.to_string(),
                    { web::json::value_of({
                        { nmos::experimental::fields::authorization_server_metadata, {} },
                        { nmos::experimental::fields::jwks, {} },
                        { nmos::experimental::fields::client_metadata, client_metadata }
                    }), nmos::experimental::jwt_validator{} }
                ));
            }
        }
        void update_client_metadata(authorization_state& authorization_state, const web::json::value& client_metadata)
        {
            update_client_metadata(authorization_state, authorization_state.authorization_server_uri, client_metadata);
        }

        void update_jwks(authorization_state& authorization_state, const web::uri& authorization_server_uri, const web::json::value& jwks, const nmos::experimental::jwt_validator& jwt_validator)
        {
            auto issuer = authorization_state.issuers.find(authorization_server_uri);
            if (authorization_state.issuers.end() != issuer)
            {
                // update
                auto& settings = issuer->second.settings;
                settings[nmos::experimental::fields::jwks] = jwks;
                issuer->second.jwt_validator = jwt_validator;
            }
            else
            {
                // insert
                authorization_state.issuers.insert(std::make_pair<web::uri, nmos::experimental::issuer>(
                    authorization_server_uri.to_string(),
                    { web::json::value_of({
                        { nmos::experimental::fields::authorization_server_metadata,{} },
                        { nmos::experimental::fields::jwks, jwks },
                        { nmos::experimental::fields::client_metadata,{} }
                }), jwt_validator }));
            }
        }
        void update_jwks(authorization_state& authorization_state, const web::json::value& jwks, const nmos::experimental::jwt_validator& jwt_validator)
        {
            update_jwks(authorization_state, authorization_state.authorization_server_uri, jwks, jwt_validator);
        }

        void erase_client_metadata(authorization_state& authorization_state, const web::uri& authorization_server_uri)
        {
            auto issuer = authorization_state.issuers.find(authorization_server_uri);
            if (authorization_state.issuers.end() != issuer)
            {
                // erase
                auto& settings = issuer->second.settings;
                settings[nmos::experimental::fields::client_metadata] = {};
            }
        }
        void erase_client_metadata(authorization_state& authorization_state)
        {
            erase_client_metadata(authorization_state, authorization_state.authorization_server_uri);
        }

        void erase_jwks(authorization_state& authorization_state, const web::uri& authorization_server_uri)
        {
            auto issuer = authorization_state.issuers.find(authorization_server_uri);
            if (authorization_state.issuers.end() != issuer)
            {
                // erase
                auto& settings = issuer->second.settings;
                settings[nmos::experimental::fields::jwks] = {};
                issuer->second.jwt_validator = {};
            }
        }
        void erase_jwks(authorization_state& authorization_state)
        {
            erase_jwks(authorization_state, authorization_state.authorization_server_uri);
        }
    }
}


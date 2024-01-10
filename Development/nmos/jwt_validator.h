#ifndef NMOS_JWT_VALIDATOR_H
#define NMOS_JWT_VALIDATOR_H

#include <cpprest/http_msg.h>
#include "cpprest/base_uri.h"

namespace web
{
    namespace json
    {
        class value;
    }
    namespace http
    {
        class http_request;
    }
}

namespace nmos
{
    namespace experimental
    {
        struct insufficient_scope_exception : std::runtime_error
        {
            insufficient_scope_exception(const std::string& message) : std::runtime_error(message) {}
        };

        struct no_matching_keys_exception : std::runtime_error
        {
            web::uri issuer;
            no_matching_keys_exception(const web::uri& issuer, const std::string& message)
                : std::runtime_error(message)
                , issuer(issuer) {}
        };

        struct scope;

        namespace details
        {
            class jwt_validator_impl;
        }

        // callback for JSON validating access token
        typedef std::function<void(const web::json::value& payload)> token_json_validator;

        class jwt_validator
        {
        public:
            jwt_validator() {}
            jwt_validator(const web::json::value& pub_keys, token_json_validator token_validation);

            // is JWT validator initialised
            bool is_initialized() const;

            // Token JSON validation
            // may throw
            void json_validation(const utility::string_t& token) const;

            // Basic token validation, including token schema validation and token issuer public keys validation
            // may throw
            void basic_validation(const utility::string_t& token) const;

            // Registered claims validation
            // may throw
            static void registered_claims_validation(const utility::string_t& token, const web::http::method& method, const web::uri& relative_uri, const scope& scope, const utility::string_t& audience);

            // Get token client Id
            // may throw
            static utility::string_t get_client_id(const utility::string_t& token);

            // Get token issuer
            // may throw
            static web::uri get_token_issuer(const utility::string_t& token);

        private:
            std::shared_ptr<details::jwt_validator_impl> impl;
        };
    }
}

#endif

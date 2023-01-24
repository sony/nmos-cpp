#ifndef NMOS_JWT_VALIDATOR_H
#define NMOS_JWT_VALIDATOR_H

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

        // callback for validating bearer_token
        typedef std::function<void(const web::json::value& payload)> token_validator;

        class jwt_validator
        {
        public:
            jwt_validator() {}
            jwt_validator(const web::json::value& pub_keys, token_validator token_validation);

            bool is_initialized() const;

            void validate_expiry(const utility::string_t& token) const;
            void validate(const utility::string_t& token, const web::http::http_request& request, const scope& scope, const utility::string_t& audience) const;

            static utility::string_t get_client_id(const utility::string_t& token);
            static web::uri get_token_issuer(const utility::string_t& token);

        private:
            std::shared_ptr<details::jwt_validator_impl> impl;
        };
    }
}

#endif

#ifndef NMOS_AUTHORIZATION_H
#define NMOS_AUTHORIZATION_H

#include <string>
#include "cpprest/basic_utils.h"
#include "cpprest/json_utils.h"
#include "nmos/issuers.h"

namespace web
{
    class uri;

    namespace http
    {
        class http_headers;
        class http_request;
    }
}

namespace slog
{
    class base_gate;
}

namespace nmos
{
    namespace experimental
    {
        struct scope;

        struct authorization_error
        {
            enum status_t
            {
                succeeded,
                without_authentication, // failure: access protected resource request without authentication
                insufficient_scope, // failure: access protected resource request higher privileges
                no_matching_keys, // failure: no matching keys for the token validation
                failed  // failure: access protected resource request with authentication but failed
            };

            authorization_error() : value(without_authentication) {}
            authorization_error(status_t value, const std::string& message = {}) : value(value), message(message) {}

            status_t value;
            std::string message;

            operator bool() const { return succeeded != value; }
        };

        bool is_token_expired(const utility::string_t& access_token, const issuers& issuers, const web::uri& expected_issuer, slog::base_gate& gate);

        utility::string_t get_client_id(const web::http::http_headers& headers, slog::base_gate& gate);

        authorization_error validate_authorization(const issuers& issuers, const web::http::http_request& request, const scope& scope, const utility::string_t& audience, const web::uri& auth_server, web::uri& token_issuer, slog::base_gate& gate);

        // RFC 6750 defines two methods of sending bearer access tokens which are applicable to WebSocket
        // Clients SHOULD use the "Authorization Request Header Field" method.
        // Clients MAY use "URI Query Parameter".
        // See https://tools.ietf.org/html/rfc6750#section-2
        authorization_error ws_validate_authorization(const issuers& issuers, const web::http::http_request& request, const scope& scope, const utility::string_t& audience, const web::uri& auth_server, web::uri& token_issuer, slog::base_gate& gate);
    }
}

#endif

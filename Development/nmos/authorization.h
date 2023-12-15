#ifndef NMOS_AUTHORIZATION_H
#define NMOS_AUTHORIZATION_H

#include "nmos/authorization_handlers.h" // for nmos::experimental::validate_authorization_token_handler, nmos::experimental::authorization_error, and nmos::experimental::scope
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
        bool is_access_token_expired(const utility::string_t& access_token, const issuers& issuers, const web::uri& expected_issuer, slog::base_gate& gate);

        utility::string_t get_client_id(const web::http::http_headers& headers, slog::base_gate& gate);

        authorization_error validate_authorization(const web::http::http_request& request, const scope& scope, const utility::string_t& audience, web::uri& token_issuer, validate_authorization_token_handler access_token_validation, slog::base_gate& gate);

        // RFC 6750 defines two methods of sending bearer access tokens which are applicable to WebSocket
        // Clients SHOULD use the "Authorization Request Header Field" method.
        // Clients MAY use "URI Query Parameter".
        // See https://tools.ietf.org/html/rfc6750#section-2
        authorization_error ws_validate_authorization(const web::http::http_request& request, const scope& scope, const utility::string_t& audience, web::uri& token_issuer, validate_authorization_token_handler access_token_validation, slog::base_gate& gate);
    }
}

#endif

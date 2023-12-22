#ifndef CPPREST_ACCESS_TOKEN_ERROR_H
#define CPPREST_ACCESS_TOKEN_ERROR_H

#include "nmos/string_enum.h"

namespace web
{
    namespace http
    {
        namespace oauth2
        {
            // experimental extension, for BCP-003-02 Authorization
            namespace experimental
            {
                // for redirect error
                // "If the resource owner denies the access request or if the request
                // fails for reasons other than a missing or invalid redirection URI,
                // the authorization server informs the client by adding the following
                //  parameters to the query component of the redirection URI using the
                //  "application/x-www-form-urlencoded" format"
                // see https://tools.ietf.org/html/rfc6749#section-4.1.2.1

                // for direct error:
                // If the access token request is invalid or unauthorized
                // "The authorization server responds with an HTTP 400 (Bad Request)
                // status code(unless specified otherwise) and includes the following
                // parameters with the response:"
                // and https://tools.ietf.org/html/rfc6749#section-5.2
                // "The parameters are included in the entity-body of the HTTP response
                // using the "application/json" media type"

                DEFINE_STRING_ENUM(access_token_error)
                namespace access_token_errors
                {
                    const access_token_error invalid_request{ U("invalid_request") }; // used for redirect error and direct error
                    const access_token_error unauthorized_client{ U("unauthorized_client") }; // used for redirect error and direct error
                    const access_token_error access_denied{ U("access_denied") }; // used for redirect error
                    const access_token_error unsupported_response_type{ U("unsupported_response_type") }; // used for redirect error
                    const access_token_error invalid_scope{ U("invalid_scope") }; // used for redirect error and direct error
                    const access_token_error server_error{ U("server_error") }; // used for redirect error
                    const access_token_error temporarily_unavailable{ U("temporarily_unavailable") }; // used for redirect error
                    const access_token_error invalid_client{ U("invalid_client") }; // used for direct error
                    const access_token_error invalid_grant{ U("invalid_grant") }; // used for direct error
                    const access_token_error unsupported_grant_type{ U("unsupported_grant_type") }; // used for direct error
                }

                inline access_token_error to_access_token_error(const utility::string_t& error)
                {
                    using namespace access_token_errors;
                    if (invalid_request.name == error) { return invalid_request; }
                    if (unauthorized_client.name == error) { return unauthorized_client; }
                    if (access_denied.name == error) { return access_denied; }
                    if (unsupported_response_type.name == error) { return unsupported_response_type; }
                    if (invalid_scope.name == error) { return invalid_scope; }
                    if (server_error.name == error) { return server_error; }
                    if (temporarily_unavailable.name == error) { return temporarily_unavailable; }
                    if (invalid_client.name == error) { return invalid_client; }
                    if (invalid_grant.name == error) { return invalid_grant; }
                    if (unsupported_grant_type.name == error) { return unsupported_grant_type; }
                    return{};
                }
            }
        }
    }
}

#endif

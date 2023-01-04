#ifndef CPPREST_RESOURCE_SERVER_ERROR_H
#define CPPREST_RESOURCE_SERVER_ERROR_H

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
                // "When a request fails, the resource server responds using the
                // appropriate HTTP status code (typically, 400, 401, 403, or 405) and
                // includes one of the following error codes in the response:"
                // see https://tools.ietf.org/html/rfc6750#section-3.1
                DEFINE_STRING_ENUM(resource_server_error)
                namespace resource_server_errors
                {
                    const resource_server_error invalid_request{ U("invalid_request") };
                    const resource_server_error invalid_token{ U("invalid_token") };
                    const resource_server_error insufficient_scope{ U("insufficient_scope") };
                }

                inline utility::string_t make_resource_server_error(const resource_server_error& error)
                {
                    return error.name;
                }

                inline resource_server_error parse_resource_server_error(const utility::string_t& error)
                {
                    if (resource_server_errors::invalid_request.name == error) { return resource_server_errors::invalid_request; }
                    if (resource_server_errors::invalid_token.name == error) { return resource_server_errors::invalid_token; }
                    if (resource_server_errors::insufficient_scope.name == error) { return resource_server_errors::insufficient_scope; }
                    return{};
                }
            }
        }
    }
}

#endif

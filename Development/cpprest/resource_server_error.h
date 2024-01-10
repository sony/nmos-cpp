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
            }
        }
    }
}

#endif

#ifndef CPPREST_CLIENT_TYPE_H
#define CPPREST_CLIENT_TYPE_H

#include "nmos/string_enum.h"

namespace web
{
    namespace http
    {
        namespace oauth2
        {
            // experimental extension, for BCP-003-02 Authorization
            // see https://tools.ietf.org/html/rfc6749#section-2.1
            namespace experimental
            {
                DEFINE_STRING_ENUM(client_type)
                namespace client_types
                {
                    const client_type confidential_client{ U("confidential_client") };
                    const client_type public_client{ U("public_client") };
                }
            }
        }
    }
}

#endif

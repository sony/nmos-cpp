#ifndef CPPREST_RESPONSE_TYPE_H
#define CPPREST_RESPONSE_TYPE_H

#include "nmos/string_enum.h"

namespace web
{
    namespace http
    {
        namespace oauth2
        {
            // experimental extension, for BCP-003-02 Authorization
            // see https://tools.ietf.org/html/rfc7591#section-2
            namespace experimental
            {
                DEFINE_STRING_ENUM(response_type)
                namespace response_types
                {
                    const response_type none{ U("none") };
                    const response_type code{ U("code") };
                    const response_type token{ U("token") };
                }
            }
        }
    }
}

#endif

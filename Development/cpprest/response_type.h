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

                inline utility::string_t make_response_type(const response_type& response_type)
                {
                    return response_type.name;
                }

                inline response_type parse_response_type(const utility::string_t& response_type)
                {
                    if (response_types::none.name == response_type) { return response_types::none; }
                    if (response_types::code.name == response_type) { return response_types::code; }
                    if (response_types::token.name == response_type) { return response_types::token; }
                    return{};
                }
            }
        }
    }
}

#endif

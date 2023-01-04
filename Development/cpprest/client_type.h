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

                inline utility::string_t make_client_type(const client_type& type)
                {
                    return type.name;
                }

                inline client_type parse_client_type(const utility::string_t& type)
                {
                    if (client_types::confidential_client.name == type) { return client_types::confidential_client; }
                    if (client_types::public_client.name == type) { return client_types::public_client; }
                    return{};
                }
            }
        }
    }
}

#endif

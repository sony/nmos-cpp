#ifndef CPPREST_TOKEN_ENDPOINT_AUTH_METHOD_H
#define CPPREST_TOKEN_ENDPOINT_AUTH_METHOD_H

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
                DEFINE_STRING_ENUM(token_endpoint_auth_method)
                namespace token_endpoint_auth_methods
                {
                    const token_endpoint_auth_method none{ U("none") };
                    const token_endpoint_auth_method client_secret_post{ U("client_secret_post") };
                    const token_endpoint_auth_method client_secret_basic{ U("client_secret_basic") };
                    // openid support
                    // see https://openid.net/specs/openid-connect-core-1_0.html#ClientAuthentication
                    const token_endpoint_auth_method private_key_jwt{ U("private_key_jwt") };
                    const token_endpoint_auth_method client_secret_jwt{ U("client_secret_jwt") };
                }

                inline utility::string_t make_token_endpoint_auth_method(const token_endpoint_auth_method& token_endpoint_auth_method)
                {
                    return token_endpoint_auth_method.name;
                }

                inline token_endpoint_auth_method parse_token_endpoint_auth_method(const utility::string_t& token_endpoint_auth_method)
                {
                    if (token_endpoint_auth_method == token_endpoint_auth_methods::client_secret_basic.name) { return token_endpoint_auth_methods::client_secret_basic; }
                    if (token_endpoint_auth_method == token_endpoint_auth_methods::client_secret_post.name) { return token_endpoint_auth_methods::client_secret_post; }
                    if (token_endpoint_auth_method == token_endpoint_auth_methods::none.name) { return token_endpoint_auth_methods::none; }
                    if (token_endpoint_auth_method == token_endpoint_auth_methods::private_key_jwt.name) { return token_endpoint_auth_methods::private_key_jwt; }
                    if (token_endpoint_auth_method == token_endpoint_auth_methods::client_secret_jwt.name) { return token_endpoint_auth_methods::client_secret_jwt; }
                    return {};
                }
            }
        }
    }
}

#endif

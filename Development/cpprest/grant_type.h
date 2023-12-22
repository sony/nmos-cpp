#ifndef CPPREST_GRANT_TYPE_H
#define CPPREST_GRANT_TYPE_H

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
                DEFINE_STRING_ENUM(grant_type)
                namespace grant_types
                {
                    const grant_type authorization_code{ U("authorization_code") };
                    const grant_type implicit{ U("implicit") };
                    const grant_type password{ U("password") };
                    const grant_type client_credentials{ U("client_credentials") };
                    const grant_type refresh_token{ U("refresh_token") };
                    const grant_type urn_ietf_params_oauth_grant_type_jwt_bearer{ U("urn:ietf:params:oauth:grant-type:jwt-bearer") };
                    const grant_type urn_ietf_params_oauth_grant_type_saml2_bearer{ U("urn:ietf:params:oauth:grant-type:saml2-bearer") };
                    const grant_type device_code{ U("urn:ietf:params:oauth:grant-type:device_code") };
                }

                inline grant_type to_grant_type(const utility::string_t& grant)
                {
                    if (grant_types::authorization_code.name == grant) { return grant_types::authorization_code; }
                    if (grant_types::implicit.name == grant) { return grant_types::implicit; }
                    if (grant_types::password.name == grant) { return grant_types::password; }
                    if (grant_types::client_credentials.name == grant) { return grant_types::client_credentials; }
                    if (grant_types::refresh_token.name == grant) { return grant_types::refresh_token; }
                    if (grant_types::urn_ietf_params_oauth_grant_type_jwt_bearer.name == grant) { return grant_types::urn_ietf_params_oauth_grant_type_jwt_bearer; }
                    if (grant_types::urn_ietf_params_oauth_grant_type_saml2_bearer.name == grant) { return grant_types::urn_ietf_params_oauth_grant_type_saml2_bearer; }
                    if (grant_types::device_code.name == grant) { return grant_types::device_code; }
                    return{};
                }
            }
        }
    }
}

#endif

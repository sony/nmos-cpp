#ifndef CPPREST_CODE_CHALLENGE_METHOD_H
#define CPPREST_CODE_CHALLENGE_METHOD_H

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
                DEFINE_STRING_ENUM(code_challenge_method)
                namespace code_challenge_methods
                {
                    const code_challenge_method S256{ U("S256") };
                    const code_challenge_method plain{ U("plain") };
                }
            }
        }
    }
}

#endif

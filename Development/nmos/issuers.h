#ifndef NMOS_ISSUERS_H
#define NMOS_ISSUERS_H

#include "cpprest/json.h"
#include "nmos/jwt_validator.h"

namespace nmos
{
    namespace experimental
    {
        struct issuer
        {
            web::json::value settings; // [U("authorization_server_metadata")], [U("jwks")], [U("client_metadata")],
                                       // where:
                                       //     "authorization_server_metadata": issuer (authorization server) metadata
                                       //     "jwks": issuer jwks
                                       //     "client_metadata": client (Node/Registry) metadata
            nmos::experimental::jwt_validator jwt_validator;
        };

        typedef std::map<web::uri, issuer> issuers;  // where uri: issuer (authorization server) uri
    }
}

#endif

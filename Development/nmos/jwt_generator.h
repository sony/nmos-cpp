#ifndef NMOS_JWT_GENERATOR_H
#define NMOS_JWT_GENERATOR_H

#include "cpprest/base_uri.h"

namespace nmos
{
    namespace experimental
    {
        class jwt_generator
        {
        public:
            static utility::string_t create_client_assertion(const utility::string_t& issuer, const utility::string_t& subject, const web::uri& audience, const std::chrono::seconds& token_lifetime, const utility::string_t& private_key);
        };
    }
}

#endif

#ifndef NMOS_JWK_UTILS_H
#define NMOS_JWK_UTILS_H

#include "cpprest/json_utils.h"
#include "jwk/algorithm.h"
#include "jwk/public_key_use.h"

namespace nmos
{
    namespace experimental
    {
        struct jwk_exception : std::runtime_error
        {
            jwk_exception(const std::string& message) : std::runtime_error(message) {}
        };

        // extract RSA public key from RSA private key
        utility::string_t rsa_public_key(const utility::string_t& rsa_private_key);

        // convert JSON Web Key to RSA public key
        utility::string_t jwk_to_rsa_public_key(const web::json::value& jwk);

        // convert RSA public key to JSON Web Key
        web::json::value rsa_public_key_to_jwk(const utility::string_t& rsa_public_key, const utility::string_t& keyid, const jwk::public_key_use& pubkey_use = jwk::public_key_uses::signing, const jwk::algorithm& alg = jwk::algorithms::RS256);

        // convert RSA private key to JSON Web Key
        web::json::value rsa_private_key_to_jwk(const utility::string_t& rsa_private_key, const utility::string_t& keyid, const jwk::public_key_use& pubkey_use = jwk::public_key_uses::signing, const jwk::algorithm& alg = jwk::algorithms::RS256);
    }
}

#endif

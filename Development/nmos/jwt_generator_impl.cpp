#include "nmos/jwt_generator.h"

#include "cpprest/basic_utils.h"
#include "jwt/nlohmann_traits.h"
#include "nmos/id.h"
#include "nmos/jwk_utils.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            class jwt_generator_impl
            {
            public:
                static utility::string_t create_client_assertion(const utility::string_t& issuer, const utility::string_t& subject, const web::uri& audience, const std::chrono::seconds& token_lifetime, const utility::string_t& public_key, const utility::string_t& private_key, const utility::string_t& keyid)
                {
                    using namespace jwt::experimental::details;

                    // use server private key to create client_assertion (JWT)
                    // where client_assertion MUST including iss, sub, aud, exp, and may including jti
                    // see https://tools.ietf.org/html/rfc7523#section-2.2
                    // see https://openid.net/specs/openid-connect-core-1_0.html#ClientAuthentication
                    return utility::s2us(jwt::create<nlohmann_traits>()
                        .set_issuer(utility::us2s(issuer))
                        .set_subject(utility::us2s(subject))
                        .set_audience(utility::us2s(audience.to_string()))
                        .set_issued_at(std::chrono::system_clock::now())
                        .set_expires_at(std::chrono::system_clock::now() + token_lifetime)
                        .set_id(utility::us2s(nmos::make_id()))
                        .set_key_id(utility::us2s(keyid))
                        .set_type("JWT")
                        .sign(jwt::algorithm::rs256(utility::us2s(public_key), utility::us2s(private_key))));
                }

                static utility::string_t create_client_assertion(const utility::string_t& issuer, const utility::string_t& subject, const web::uri& audience, const std::chrono::seconds& token_lifetime, const utility::string_t& private_key, const utility::string_t& keyid)
                {
                    return create_client_assertion(issuer, subject, audience, token_lifetime, rsa_public_key(private_key), private_key, keyid);
                }
            };
        }

        utility::string_t jwt_generator::create_client_assertion(const utility::string_t& issuer, const utility::string_t& subject, const web::uri& audience, const std::chrono::seconds& token_lifetime, const utility::string_t& private_key, const utility::string_t& keyid)
        {
            try
            {
                return details::jwt_generator_impl::create_client_assertion(issuer, subject, audience, token_lifetime, private_key, keyid);
            }
            catch (const jwt::error::signature_generation_exception& e)
            {
                throw std::invalid_argument(e.what());
            }
        }
    }
}

#include "nmos/jwt_validator.h"

#include <boost/algorithm/string.hpp>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include "cpprest/basic_utils.h"
#include "cpprest/http_msg.h"
#include "cpprest/json.h"
#include "cpprest/regex_utils.h"
#include "cpprest/uri_schemes.h"
#include "nmos/authorization_utils.h"
#include "nmos/json_fields.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            class jwt_validator_impl
            {
            public:
                explicit jwt_validator_impl(const web::json::value& pubkeys, token_validator token_validation)
                    : token_validation(token_validation)
                {
                    using namespace jwt::traits;

                    if (pubkeys.is_array())
                    {
                        // empty out all jwt validators
                        validators.clear();

                        // create jwt verifier for each public key

                        // preload JWT verifiers with authorization server publc keys (pems), should perform faster on token validation rather than load the public key then validation at runtime

                        // "The access token MUST be a JSON Web Signature (JWS) as defined by RFC 7515. JSON Web Algorithms (JWA) MUST NOT be used.
                        // The JWS MUST be signed with RSASSA-PKCS1-v1_5 using SHA-512, meaning the value of the alg field in the token's JOSE (JSON Object Signing and Encryption) header (see RFC 7515)
                        // MUST be set to RS512 as defined in RFC 7518."
                        // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.4._Behaviour_-_Access_Tokens.html#behaviour-access-tokens
                        for (const auto& pubkey : pubkeys.as_array())
                        {
                            const auto& jwk = pubkey.at(U("jwk"));

                            // Key Type (kty)
                            // see https://tools.ietf.org/html/rfc7517#section-4.1
                            if (U("RSA") != jwk.at(U("kty")).as_string()) continue;

                            // Public Key Use (use), optional!
                            // see https://tools.ietf.org/html/rfc7517#section-4.2
                            if (jwk.has_field(U("use")))
                            {
                                if (U("sig") != jwk.at(U("use")).as_string()) continue;
                            }

                            // Algorithm (alg), optional!
                            // see https://tools.ietf.org/html/rfc7517#section-4.4
                            if (jwk.has_field(U("alg")))
                            {
                                if (U("RS512") != jwk.at(U("alg")).as_string()) continue;
                            }

                            auto validator = jwt::verify<jwt::default_clock, nlohmann_json>({});
                            try
                            {
                                validator.allow_algorithm(jwt::algorithm::rs512(utility::us2s(pubkey.at(U("pem")).as_string())));
                                validators.push_back(validator);
                            }
                            catch (const jwt::error::rsa_exception&)
                            {
                                // hmm, maybe log the error?
                            }
                        }
                    }
                }

                void validate_expiry(const utility::string_t& token) const
                {
                    using namespace jwt::traits;

                    // verify JWT is well formed
                    auto decoded_token = jwt::decode<nlohmann_json>(utility::us2s(token));

                    for (const auto& validator : validators)
                    {
                        try
                        {
                            // verify the signature & some common claims, such as exp, iat, nbf etc
                            validator.verify(decoded_token);

                            // token not expired
                            return;
                        }
                        catch (const jwt::error::signature_verification_exception&)
                        {
                            // ignore, try next validator
                        }
                    }

                    // no public keys to validate access token
                    throw std::runtime_error("no public keys to validate access token expiry");
                }

                void validate(const utility::string_t& token, const web::http::http_request& req, const scope& scope, const utility::string_t& audience) const
                {
                    using namespace jwt::traits;

                    // verify JWT is well formed
                    auto decoded_token = jwt::decode<nlohmann_json>(utility::us2s(token));

                    // validate bearer token payload JSON
                    if (token_validation)
                    {
                        token_validation(web::json::value::parse(utility::s2us(decoded_token.get_payload())));
                    }

                    std::vector<std::string> errors;

                    if (validators.size())
                    {
                        const auto validate_scope = !scope.name.empty();

                        for (const auto& validator : validators)
                        {
                            try
                            {
                                // verify the signature & some common claims, such as exp, iat, nbf etc
                                validator.verify(decoded_token);

                                // common claims verified (i.e. validator/public key successfully verify the token's signature),
                                // from this point onwards any error detected will be treated as failure

                                // verify Registered Claims

                                // iss (Identifies principal that issued the JWT)
                                // The "iss" value is a case-sensitive string containing a StringOrURI value.
                                // see https://tools.ietf.org/html/rfc7519#section-4.1.1
                                // iss is not needed to validate as this token may be coming from an alternative Authorization server, which would have a different iss then the current in used Authorization server.

                                // sub (Identifies the subject of the JWT)
                                // hmm, not sure how to verify sub as it could be anything
                                // see https://tools.ietf.org/html/rfc7519#section-4.1.2

                                // aud (Identifies the recipients of the JWT)
                                // This claim MUST be a JSON array containing the fully resolved domain names of the intended recipients, or a domain name containing
                                // wild - card characters in order to target a subset of devices on a network. Such wild-carding of domain names is documented in RFC 4592.
                                // If aud claim does not match the fully resolved domain name of the resource server, the Resource Server MUST reject the token.
                                // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.4._Behaviour_-_Access_Tokens.html#aud
                                // see https://tools.ietf.org/html/rfc7519#section-4.1.3

                                auto verify_aud = [&decoded_token](const utility::string_t& audience_)
                                {
                                    auto strip_trailing_dot = [](const std::string& audience_) {
                                        auto audience = audience_;
                                        if (!audience.empty() && U('.') == audience.back())
                                        {
                                            audience.pop_back();
                                        }
                                        return audience;
                                    };

                                    auto audience = strip_trailing_dot(utility::us2s(audience_));
                                    std::vector<std::string> segments;
                                    boost::split(segments, audience, boost::is_any_of("."));

                                    const auto& auds = decoded_token.get_audience();
                                    for (const auto& aud_ : auds)
                                    {
                                        // strip the scheme (https://) if presented
                                        auto aud = strip_trailing_dot(aud_);
                                        web::http::uri aud_uri(utility::s2us(aud));
                                        if (!aud_uri.scheme().empty())
                                        {
                                            aud = utility::us2s(aud_uri.host());
                                        }

                                        // is the audience an exact match to the token audience
                                        if (audience == aud)
                                        {
                                            return true;
                                        }

                                        // do reverse segment matching between audience and token audience
                                        std::vector<std::string> aud_segments;
                                        boost::split(aud_segments, aud, boost::is_any_of("."));

                                        if (segments.size() >= aud_segments.size() && aud_segments.size())
                                        {
                                            // token audience got to be in wildcard domain name format, leftmost is a "*" charcater
                                            // if not it is not going to match
                                            // see https://tools.ietf.org/html/rfc4592#section-2.1.1
                                            if (aud_segments[0] != "*")
                                            {
                                                return false;
                                            }

                                            // token audience is in wildcard domain name format
                                            // let's do a segment to segment comparison between audience and token audience
                                            bool matched{ true };
                                            auto idx = aud_segments.size() - 1;
                                            for (auto it = aud_segments.rbegin(); it != aud_segments.rend() && matched; ++it)
                                            {
                                                if (idx && *it != segments[idx--])
                                                {
                                                    matched = false;
                                                }
                                            }
                                            if (matched)
                                            {
                                                return true;
                                            }
                                        }
                                    }
                                    return false;
                                };
                                if (!verify_aud(audience))
                                {
                                    throw insufficient_scope_exception(utility::us2s(audience) + " not found in audience");
                                }

                                // scope optional
                                // If scope claim does not contain the expected scope, the Resource Server reject the token.
                                // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.4._Behaviour_-_Access_Tokens.html#scope
                                auto verify_scope = [&decoded_token](const nmos::experimental::scope& scope)
                                {
                                    if (decoded_token.has_payload_claim(utility::us2s(nmos::experimental::fields::scope)))
                                    {
                                        const auto& scope_claim = decoded_token.get_payload_claim(utility::us2s(nmos::experimental::fields::scope));
                                        const auto scopes_set = scopes(utility::s2us(scope_claim.as_string()));
                                        return (scopes_set.end() != std::find(scopes_set.begin(), scopes_set.end(), scope));
                                    }
                                    return true;
                                };
                                if (validate_scope && !verify_scope(scope))
                                {
                                    throw insufficient_scope_exception(utility::us2s(scope.name) + " not found in " + utility::us2s(nmos::experimental::fields::scope));
                                }

                                // verify Private Claims

                                // x-nmos-* (Contains information particular to the NMOS API the token is intended for)
                                // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.4._Behaviour_-_Access_Tokens.html#x-nmos-
                                auto verify_x_nmos_scope_claim = [&decoded_token, req](const std::string& x_nmos_scope_claim_, const std::string& path)
                                {
                                    if (!decoded_token.has_payload_claim(x_nmos_scope_claim_)) { return false; }
                                    const auto x_nmos_scope_claim = decoded_token.get_payload_claim(x_nmos_scope_claim_).to_json();

                                    if (!x_nmos_scope_claim.is_null())
                                    {
                                        auto accessible = [&x_nmos_scope_claim, req, &path](const std::string& access_right)
                                        {
                                            if (x_nmos_scope_claim.contains(access_right))
                                            {
                                                auto accessible_paths = jwt::basic_claim<nlohmann_json>(x_nmos_scope_claim.at(access_right)).as_array();
                                                for (auto& accessible_path : accessible_paths)
                                                {
                                                    // construct path regex for regex comparison

                                                    auto acc_path = accessible_path.get<std::string>();
                                                    // replace any '*' => '.*'
                                                    boost::replace_all(acc_path, "*", ".*");
                                                    const bst::regex path_regex(acc_path);
                                                    if (bst::regex_match(path, path_regex))
                                                    {
                                                        return true;
                                                    }
                                                }
                                            }
                                            return false;
                                        };

                                        // write accessible
                                        if ((web::http::methods::POST == req.method())
                                            || (web::http::methods::PUT == req.method())
                                            || (web::http::methods::PATCH == req.method())
                                            || (web::http::methods::DEL == req.method()))
                                        {
                                            return accessible("write");
                                        }

                                        // read accessible
                                        if ((web::http::methods::OPTIONS == req.method())
                                            || (web::http::methods::GET == req.method())
                                            || (web::http::methods::HEAD == req.method()))
                                        {
                                            return accessible("read");
                                        }
                                    }
                                    return false;
                                };

                                // verify the relevant x-nmos-* private claim
                                if (validate_scope)
                                {
                                    const auto x_nmos_scope_claim = "x-nmos-" + utility::us2s(scope.name);

                                    // extract <path> from /x-nmos/<api name, the scope name>/<api version>/<path>
                                    auto extract_path = [req](const nmos::experimental::scope& scope)
                                    {
                                        const bst::regex search_regex("/x-nmos/" + utility::us2s(scope.name) + "/v[0-9]+\\.[0-9]");
                                        const auto request_uri = utility::us2s(req.request_uri().to_string());

                                        if (bst::regex_search(request_uri, search_regex))
                                        {
                                            auto path = bst::regex_replace(request_uri, search_regex, "");
                                            if (path.size() && ('/' == path[0]))
                                            {
                                                return path.erase(0, 1);
                                            }
                                            else
                                            {
                                                return std::string{};
                                            }
                                        }
                                        return std::string{};;
                                    };
                                    const auto path = extract_path(scope);

                                    if (path.empty())
                                    {
                                        // The token MUST include either an x-nmos-* claim matching the API name, a scope matching the API name or both in order to obtain 'read' permission.
                                        // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.5._Behaviour_-_Resource_Servers.html#path-validation

                                        // if scope claim is presented, it has already verified eariler
                                        if (!decoded_token.has_payload_claim(x_nmos_scope_claim) && !decoded_token.has_payload_claim(utility::us2s(nmos::experimental::fields::scope)))
                                        {
                                            // missing both x-nmos private claim and scope claim
                                            throw insufficient_scope_exception("missing claim x-nmos-" + utility::us2s(scope.name) + " and claim scope, " + utility::us2s(req.request_uri().to_string()) + " not accessible");
                                        }
                                    }
                                    else
                                    {
                                        // The token MUST include an x-nmos-* claim matching the API name and the path, in line with the method outlined in Tokens.
                                        // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.5._Behaviour_-_Resource_Servers.html#path-validation

                                        if (!verify_x_nmos_scope_claim(x_nmos_scope_claim, path))
                                        {
                                            throw insufficient_scope_exception("claim x-nmos-" + utility::us2s(scope.name) + " " + utility::us2s(req.request_uri().to_string()) + " not accessible");
                                        }
                                    }
                                }

                                // token validate successfully
                                return;
                            }
                            catch (const insufficient_scope_exception&)
                            {
                                throw;
                            }
                            catch (const jwt::error::token_verification_exception& e)
                            {
                                throw std::invalid_argument(e.what());
                            }
                            catch (const jwt::error::signature_verification_exception& e)
                            {
                                // ignore, try next validator
                                errors.push_back(e.what());
                            }
                        }
                    }

                    // reaching here, there must be no matching public key for the token

                    // "Where a Resource Server has no matching public key for a given token, it SHOULD attempt to obtain the missing public key via the the token iss
                    // claim as specified in RFC 8414 section 3. In cases where the Resource Server needs to fetch a public key from a remote Authorization Server it
                    // MAY temporarily respond with an HTTP 503 code in order to avoid blocking the incoming authorized request. When a HTTP 503 code is used, the Resource
                    // Server SHOULD include an HTTP Retry-After header to indicate when the client may retry its request.
                    // If the Resource Server fails to verify a token using all public keys available it MUST reject the token."
                    // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.5._Behaviour_-_Resource_Servers.html#public-keys

                    const auto token_issuer = web::uri{ utility::s2us(decoded_token.get_issuer()) };
                    // no matching public keys for the token, re-fetch public keys from token issuer
                    throw no_matching_keys_exception(token_issuer, format_errors(errors));
                }

                // may throw
                static utility::string_t get_client_id(const utility::string_t& token)
                {
                    using namespace jwt::traits;

                    // verify JWT is well formed
                    auto decoded_token = jwt::decode<nlohmann_json>(utility::us2s(token));
                    // token does not guarantee to have client_id
                    // see https://specs.amwa.tv/is-10/releases/v1.0.0/docs/4.4._Behaviour_-_Access_Tokens.html#client_id
                    if (decoded_token.has_payload_claim("client_id"))
                    {
                        const auto client_id = decoded_token.get_payload_claim("client_id");
                        return utility::s2us(client_id.as_string());
                    }
                    // azp is an OPTIONAL claim for OpenID Connect
                    // Authorized party - the party to which the ID Token was issued.If present, it MUST contain the OAuth 2.0 Client ID of this party.
                    // This Claim is only needed when the ID Token has a single audience value and that audience is different than the authorized party.
                    // It MAY be included even when the authorized party is the same as the sole audience.
                    // The azp value is a case sensitive string containing a StringOrURI value.
                    // see https://openid.net/specs/openid-connect-core-1_0.html#IDToken
                    else if (decoded_token.has_payload_claim("azp"))
                    {
                        const auto client_id = decoded_token.get_payload_claim("azp");
                        return utility::s2us(client_id.as_string());
                    }
                    return{};
                }

                // may throw
                static web::uri get_token_issuer(const utility::string_t& token)
                {
                    using namespace jwt::traits;

                    // verify JWT is well formed
                    auto decoded_token = jwt::decode<nlohmann_json>(utility::us2s(token));
                    return utility::s2us(decoded_token.get_issuer());
                }

            private:
                std::string format_errors(const std::vector<std::string>& errs) const
                {
                    std::string separator;
                    std::stringstream ss;
                    for (const auto& err : errs)
                    {
                        ss << separator << err;
                        separator = ", ";
                    }
                    return ss.str();
                }

            private:
                std::vector<jwt::verifier<jwt::default_clock, jwt::traits::nlohmann_json>> validators;
                token_validator token_validation;
            };
        }

        jwt_validator::jwt_validator(const web::json::value& pubkeys, token_validator token_validation)
            : impl(new details::jwt_validator_impl(pubkeys, token_validation))
        {
        }

        bool jwt_validator::is_initialized() const
        {
            return impl ? true : false;
        }

        void jwt_validator::validate_expiry(const utility::string_t& token) const
        {
            if (!impl) { throw std::runtime_error("JWT validator has not initiliased"); }

            impl->validate_expiry(token);
        }

        void jwt_validator::validate(const utility::string_t& token, const web::http::http_request& request, const scope& scope, const utility::string_t& audience) const
        {
            if (!impl) { throw std::runtime_error("JWT validator has not initiliased"); }

            impl->validate(token, request, scope, audience);
        }

        utility::string_t jwt_validator::get_client_id(const utility::string_t& token)
        {
            return details::jwt_validator_impl::get_client_id(token);
        }

        web::uri jwt_validator::get_token_issuer(const utility::string_t& token)
        {
            return details::jwt_validator_impl::get_token_issuer(token);
        }
    }
}

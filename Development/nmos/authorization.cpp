#include "nmos/authorization.h"

#include <boost/algorithm/string/predicate.hpp>
#include "nmos/authorization_utils.h"
#include "nmos/jwt_validator.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        struct without_authentication_exception : std::runtime_error
        {
            without_authentication_exception(const std::string& message) : std::runtime_error(message) {}
        };

        bool is_token_expired(const utility::string_t& access_token, const issuers& issuers, const web::uri& expected_issuer, slog::base_gate& gate)
        {
            if (access_token.empty())
            {
                // no access token, treat it as expired
                return true;
            }

            try
            {
                const auto& token_issuer = nmos::experimental::jwt_validator::token_issuer(access_token);

                // is token from expected issuer
                if (token_issuer == expected_issuer)
                {
                    // is token expired
                    const auto& issuer = issuers.find(token_issuer);
                    if (issuers.end() != issuer)
                    {
                        issuer->second.jwt_validator.validate_expiry(access_token);
                        return false;
                    }
                }
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "test token expiry: " << e.what();
            }

            // reaching here, token validation has failed, treat it as expired
            return true;
        }

        utility::string_t get_client_id(const web::http::http_headers& headers, slog::base_gate& gate)
        {
            try
            {
                const auto header = headers.find(web::http::header_names::authorization);
                if (headers.end() == header)
                {
                    throw without_authentication_exception{ "missing Authorization header" };
                }

                const auto& token = header->second;
                const utility::string_t scheme{ U("Bearer ") };
                if (!boost::algorithm::starts_with(token, scheme))
                {
                    throw without_authentication_exception{ "unsupported authentication scheme" };
                }

                const auto access_token = token.substr(scheme.length());
                return jwt_validator::client_id(access_token);
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "failed to get client_id from header: " << e.what();
            }
            return{};
        }

        authorization_error validate_authorization(const utility::string_t& access_token, const issuers& issuers, const web::http::http_request& request, const scope& scope, const utility::string_t& audience, const web::uri& auth_server, web::uri& token_issuer, slog::base_gate& gate)
        {
            if (access_token.empty())
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "missing access token";
                return{ authorization_error::without_authentication, "missing access token" };
            }

            if (issuers.empty())
            {
                try
                {
                    // record the unknown issuer of this access token, i.e. no public keys to validate the access token
                    // this will be used in the authorization_token_issuer_thread to fetch the missing public keys for token validation
                    token_issuer = nmos::experimental::jwt_validator::token_issuer(access_token);
#if defined (NDEBUG)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "no public keys to validate access token";
#else
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "no public keys to validate access token: " << access_token;
#endif
                    return{ authorization_error::no_matching_keys, "no public keys to validate access token" };
                }
                catch (const std::exception& e)
                {
#if defined (NDEBUG)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "invalid token issuer: " << e.what();
#else
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "invalid token issuer: " << e.what() << "; access_token: " << access_token;
#endif
                    return{ authorization_error::failed, e.what() };
                }
            }

            std::string error;
            for (auto issuer = issuers.begin(); issuer != issuers.end(); issuer++)
            {
                try
                {
                    // if jwt_validator has not already set up, treat it as no public keys to validate token
                    if (issuer->second.jwt_validator.is_initialized())
                    {
                        issuer->second.jwt_validator.validate(access_token, request, scope, audience, auth_server);
                        return{ authorization_error::succeeded };
                    }
                }
                catch (const no_matching_keys_exception& e)
                {
                    // validator failed to decode token due to no valid public keys, try next set of issuer's validator
                    // this will be used in the authorization_token_issuer_thread to fetch the missing public keys for token validation
                    token_issuer = e.issuer;
                    error = e.what();
#if defined (NDEBUG)
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << e.what() << " against " << utility::us2s(issuer->first.to_string()) << " public keys";
#else
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << e.what() << " against " << utility::us2s(issuer->first.to_string()) << " public keys; access_token: " << access_token;
#endif
                }
                catch (const insufficient_scope_exception& e)
                {
                    // validator can decode the token, but insufficient scope
#if !defined (NDEBUG)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << e.what() << "; access_token: " << access_token;
#endif
                    return{ authorization_error::insufficient_scope, e.what() };
                }
                catch (const std::exception& e)
                {
                    // validator can decode the token, with general failure
#if !defined (NDEBUG)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << e.what() << "; access_token: " << access_token;
#endif
                    return{ authorization_error::failed, e.what() };
                }
            }

            // reach here must be because there are no public keys to validate token
            return{ authorization_error::no_matching_keys, error };
        }

        authorization_error validate_authorization(const issuers& issuers, const web::http::http_request& request, const scope& scope, const utility::string_t& audience, const web::uri& auth_server, web::uri& token_issuer, slog::base_gate& gate)
        {
            try
            {
                const auto& headers = request.headers();

                const auto header = headers.find(web::http::header_names::authorization);
                if (headers.end() == header)
                {
                    throw without_authentication_exception{ "missing Authorization header" };
                }

                const auto& token = header->second;
                const utility::string_t scheme{ U("Bearer ") };
                if (!boost::algorithm::starts_with(token, scheme))
                {
                    throw without_authentication_exception{ "unsupported authentication scheme" };
                }

                const auto access_token = token.substr(scheme.length());
                return validate_authorization(access_token, issuers, request, scope, audience, auth_server, token_issuer, gate);
            }
            catch (const without_authentication_exception& e)
            {
                return{ authorization_error::without_authentication, e.what() };
            }
        }

        // RFC 6750 defines two methods of sending bearer access tokens which are applicable to WebSocket
        // Clients SHOULD use the "Authorization Request Header Field" method.
        // Clients MAY use "URI Query Parameter".
        // See https://tools.ietf.org/html/rfc6750#section-2
        authorization_error ws_validate_authorization(const issuers& issuers, const web::http::http_request& request, const scope& scope, const utility::string_t& audience, const web::uri& auth_server, web::uri& token_issuer, slog::base_gate& gate)
        {
            auto error = validate_authorization(issuers, request, scope, audience, auth_server, token_issuer, gate);

            if (error)
            {
                error = { authorization_error::without_authentication, "missing access token" };

                // test "URI Query Parameter"
                const auto& query = request.request_uri().query();
                if (!query.empty())
                {
                    auto querys = web::uri::split_query(query);
                    auto it = querys.find(U("access_token"));
                    if (querys.end() != it)
                    {
                        error = nmos::experimental::validate_authorization(it->second, issuers, request, scope, audience, auth_server, token_issuer, gate);
                    }
                }
            }
            return error;
        }
    }
}

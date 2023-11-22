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

        bool is_access_token_expired(const utility::string_t& access_token, const issuers& issuers, const web::uri& expected_issuer, slog::base_gate& gate)
        {
            if (access_token.empty())
            {
                // no access token, treat it as expired
                return true;
            }

            try
            {
                const auto& token_issuer = nmos::experimental::jwt_validator::get_token_issuer(access_token);

                // is token from the expected issuer
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
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Test token expiry error: " << e.what();
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
                return jwt_validator::get_client_id(access_token);
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Failed to get client_id from header: " << e.what();
            }
            return{};
        }

        namespace details
        {
            authorization_error validate_authorization(const utility::string_t& access_token, const web::http::http_request& request, const scope& scope, const utility::string_t& audience, web::uri& token_issuer, validate_authorization_token_handler access_token_validation, slog::base_gate& gate)
            {
                if (access_token.empty())
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Missing access token";
                    return{ authorization_error::without_authentication, "Missing access token" };
                }

                try
                {
                    // extract the token issuer from the token
                    token_issuer = nmos::experimental::jwt_validator::get_token_issuer(access_token);
                }
                catch (const std::exception& e)
                {
#if defined (NDEBUG)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unable to extract token issuer from access token: " << e.what();
#else
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unable to extract token issuer from access token: " << e.what() << "; access_token: " << access_token;
#endif
                    return{ authorization_error::failed, e.what() };
                }

                if (access_token_validation)
                {
                    return access_token_validation(access_token, request, scope, audience);
                }
                else
                {
                    std::string error{ "Access token validation callback is not set up to validate the access token" };
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << error;
                    return{ authorization_error::failed, error };
                }
            }
        }

        authorization_error validate_authorization(const web::http::http_request& request, const scope& scope, const utility::string_t& audience, web::uri& token_issuer, validate_authorization_token_handler access_token_validation, slog::base_gate& gate)
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
                return details::validate_authorization(access_token, request, scope, audience, token_issuer, access_token_validation, gate);
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
        authorization_error ws_validate_authorization(const web::http::http_request& request, const scope& scope, const utility::string_t& audience, web::uri& token_issuer, validate_authorization_token_handler access_token_validation, slog::base_gate& gate)
        {
            auto error = validate_authorization(request, scope, audience, token_issuer, access_token_validation, gate);

            if (error)
            {
                error = { authorization_error::without_authentication, "missing access token" };

                // test "URI Query Parameter"
                const auto& query = request.request_uri().query();
                if (!query.empty())
                {
                    auto querys = web::uri::split_query(query);
                    auto found = querys.find(U("access_token"));
                    if (querys.end() != found)
                    {
                        error = details::validate_authorization(found->second, request, scope, audience, token_issuer, access_token_validation, gate);
                    }
                }
            }
            return error;
        }
    }
}

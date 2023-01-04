#include "nmos/authorization_utils.h"

#include <boost/algorithm/string/classification.hpp> // for boost::is_any_of
#include <boost/algorithm/string/split.hpp> // for boost::split
#include "cpprest/client_type.h"
#include "cpprest/base_uri.h"
#include "nmos/authorization_scopes.h"
#include "nmos/is10_versions.h"
#include "nmos/json_fields.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            // get grant type set from json array
            std::set<web::http::oauth2::experimental::grant_type> grant_types(const web::json::array& grants)
            {
                return boost::copy_range<std::set<web::http::oauth2::experimental::grant_type>>(grants | boost::adaptors::transformed([](const web::json::value& v) { return web::http::oauth2::experimental::parse_grant(v.as_string()); }));
            }

            // get grant type set from settings
            std::set<web::http::oauth2::experimental::grant_type> grant_types_from_settings(const nmos::settings& settings)
            {
                const auto& authorization_flow = nmos::experimental::fields::authorization_flow(settings);
                return (web::http::oauth2::experimental::grant_types::client_credentials.name == authorization_flow) ? std::set<web::http::oauth2::experimental::grant_type>{ web::http::oauth2::experimental::grant_types::client_credentials } : std::set<web::http::oauth2::experimental::grant_type>{ web::http::oauth2::experimental::grant_types::authorization_code, web::http::oauth2::experimental::grant_types::refresh_token };
            }

            // get grant type set from client metadata if presented, otherwise return default grant types
            std::set<web::http::oauth2::experimental::grant_type> grant_types(const web::json::value& client_metadata, const std::set<web::http::oauth2::experimental::grant_type>& default_grant_types)
            {
                if (!client_metadata.is_null() && client_metadata.has_array_field(nmos::experimental::fields::grant_types))
                {
                    return details::grant_types(nmos::experimental::fields::grant_types(client_metadata));
                }
                return default_grant_types;
            }

            // get scope set from a spare delimiter scope string
            std::set<scope> scopes(const utility::string_t& scope)
            {
                std::vector<utility::string_t> tokens;
                boost::split(tokens, scope, boost::is_any_of(U(" ")));
                return boost::copy_range<std::set<nmos::experimental::scope>>(tokens | boost::adaptors::transformed([](const utility::string_t& v) { return parse_scope(v); }));
            }

            // get scope set from client metadata if presented, otherwise return default scope set
            std::set<scope> scopes(const web::json::value& client_metadata, const std::set<scope>& default_scopes)
            {
                if (!client_metadata.is_null() && client_metadata.has_string_field(nmos::experimental::fields::scope))
                {
                    return scopes(nmos::experimental::fields::scope(client_metadata));
                }
                return default_scopes;
            }

            // get token_endpoint_auth_method from settings
            web::http::oauth2::experimental::token_endpoint_auth_method token_endpoint_auth_method_from_settings(const nmos::settings& settings)
            {
                return web::http::oauth2::experimental::parse_token_endpoint_auth_method(nmos::experimental::fields::token_endpoint_auth_method(settings));
            }

            // get token_endpoint_auth_method from client metadata if presented, otherwise return default_token_endpoint_auth_method
            web::http::oauth2::experimental::token_endpoint_auth_method token_endpoint_auth_method(const web::json::value& client_metadata, const web::http::oauth2::experimental::token_endpoint_auth_method& default_token_endpoint_auth_method)
            {
                namespace token_endpoint_auth_methods = web::http::oauth2::experimental::token_endpoint_auth_methods;

                auto token_endpoint_auth_method = default_token_endpoint_auth_method;
                if (!client_metadata.is_null() && client_metadata.has_string_field(nmos::experimental::fields::token_endpoint_auth_method))
                {
                    token_endpoint_auth_method = web::http::oauth2::experimental::token_endpoint_auth_method{ nmos::experimental::fields::token_endpoint_auth_method(client_metadata) };
                }
                return token_endpoint_auth_method;
            }

            // get issuer version
            api_version version(const web::uri& issuer)
            {
                // issuer uri should be like "https://server.example.com/{version}
                api_version ver{ api_version{} };
                if (!issuer.is_path_empty())
                {
                    ver = parse_api_version(web::uri::split_path(issuer.path()).back());
                }
                return (api_version{} == ver) ? is10_versions::v1_0 : ver;
            }
        }
    }
}

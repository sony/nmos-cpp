#ifndef NMOS_AUTHORIZATION_UTILS_H
#define NMOS_AUTHORIZATION_UTILS_H

#include <set>
#include "cpprest/grant_type.h"
#include "cpprest/token_endpoint_auth_method.h"
#include "nmos/api_version.h"
#include "nmos/scope.h"
#include "nmos/settings.h" // just a forward declaration of nmos::settings

namespace web
{
    class uri;
}

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            // get client's grant types
            std::set<web::http::oauth2::experimental::grant_type> grant_types(const web::json::array& grants);
            std::set<web::http::oauth2::experimental::grant_type> grant_types_from_settings(const nmos::settings& settings);
            std::set<web::http::oauth2::experimental::grant_type> grant_types(const web::json::value& client_metadata, const std::set<web::http::oauth2::experimental::grant_type>& default_grant_types);
            // get client's scopes
            std::set<scope> scopes(const utility::string_t& scope);
            std::set<scope> scopes(const web::json::value& client_metadata, const std::set<scope>& default_scopes);
            // get client's token_endpoint_auth_method
            web::http::oauth2::experimental::token_endpoint_auth_method token_endpoint_auth_method_from_settings(const nmos::settings& settings);
            web::http::oauth2::experimental::token_endpoint_auth_method token_endpoint_auth_method(const web::json::value& client_metadata, const web::http::oauth2::experimental::token_endpoint_auth_method& default_token_endpoint_auth_method);
            // get issuer version
            api_version version(const web::uri& issuer);

            // is subsets found in given set
            template <typename T>
            inline bool find_all(const std::set<T>& sub, const std::set<T>& full)
            {
                if (sub.size() == 0 || full.size() == 0) { return false; }

                for (auto s : sub)
                {
                    bool found{ false };
                    for (auto f : full)
                    {
                        if (f == s)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found) { return false; }
                }
                return true;
            }
        }
    }
}

#endif

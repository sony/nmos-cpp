#ifndef NMOS_AUTHORIZATION_SCOPES_H
#define NMOS_AUTHORIZATION_SCOPES_H

#include <set>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/scope.h"
#include "nmos/settings.h"

namespace nmos
{
    namespace experimental
    {
        namespace authorization_scopes
        {
            // get scope set from settings
            inline std::set<scope> from_settings(const nmos::settings& settings)
            {
                return settings.has_field(nmos::experimental::fields::authorization_scopes)
                    ? boost::copy_range<std::set<scope>>(nmos::experimental::fields::authorization_scopes(settings) | boost::adaptors::transformed([](const web::json::value& v) { return nmos::experimental::parse_scope(v.as_string()); }))
                    : std::set<scope>{};
            }
        }
    }
}

#endif

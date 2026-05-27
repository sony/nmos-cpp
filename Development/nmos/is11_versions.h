#ifndef NMOS_IS11_VERSIONS_H
#define NMOS_IS11_VERSIONS_H

#include <set>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/api_version.h"
#include "nmos/settings.h"

namespace nmos
{
    namespace is11_versions
    {
        const api_version v1_0{ 1, 0 };

        const std::set<api_version> all{ nmos::is11_versions::v1_0 };

        inline std::set<api_version> from_settings(const nmos::settings& settings)
        {
            return settings.has_field(nmos::fields::is11_versions)
                ? boost::copy_range<std::set<api_version>>(nmos::fields::is11_versions(settings) | boost::adaptors::transformed([](const web::json::value& v) { return nmos::parse_api_version(v.as_string()); }))
                : nmos::is11_versions::all;
        }
    }
}

#endif

#ifndef NMOS_IS12_VERSIONS_H
#define NMOS_IS12_VERSIONS_H

#include <set>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/api_version.h"
#include "nmos/settings.h"

namespace nmos
{
    namespace is12_versions
    {
        const api_version v1_0{ 1, 0 };

        const std::set<api_version> all{ nmos::is12_versions::v1_0 };

        inline std::set<api_version> from_settings(const nmos::settings& settings)
        {
            return settings.has_field(nmos::fields::is12_versions)
                ? boost::copy_range<std::set<api_version>>(nmos::fields::is12_versions(settings) | boost::adaptors::transformed([](const web::json::value& v) { return nmos::parse_api_version(v.as_string()); }))
                : nmos::is12_versions::all;
        }
    }
}

#endif

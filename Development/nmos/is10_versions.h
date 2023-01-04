#ifndef NMOS_IS10_VERSIONS_H
#define NMOS_IS10_VERSIONS_H

#include <set>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/api_version.h"
#include "nmos/settings.h"

namespace nmos
{
    namespace is10_versions
    {
        const api_version v1_0{ 1, 0 };

        const std::set<api_version> all{ nmos::is10_versions::v1_0 };

        inline std::set<api_version> from_settings(const nmos::settings& settings)
        {
            return settings.has_field(nmos::fields::is10_versions)
                ? boost::copy_range<std::set<api_version>>(nmos::fields::is10_versions(settings) | boost::adaptors::transformed([](const web::json::value& v) { return nmos::parse_api_version(v.as_string()); }))
                : nmos::is10_versions::all;
        }
    }
}

#endif

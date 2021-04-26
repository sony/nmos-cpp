#ifndef NMOS_IS04_VERSIONS_H
#define NMOS_IS04_VERSIONS_H

#include <set>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/api_version.h"
#include "nmos/settings.h"

namespace nmos
{
    namespace is04_versions
    {
        const api_version v1_0{ 1, 0 };
        const api_version v1_1{ 1, 1 };
        const api_version v1_2{ 1, 2 };
        const api_version v1_3{ 1, 3 };

        const std::set<api_version> all{ nmos::is04_versions::v1_0, nmos::is04_versions::v1_1, nmos::is04_versions::v1_2, nmos::is04_versions::v1_3 };

        inline std::set<api_version> from_settings(const nmos::settings& settings)
        {
            return settings.has_field(nmos::fields::is04_versions)
                ? boost::copy_range<std::set<api_version>>(nmos::fields::is04_versions(settings) | boost::adaptors::transformed([](const web::json::value& v) { return nmos::parse_api_version(v.as_string()); }))
                : nmos::is04_versions::all;
        }
    }
}

#endif

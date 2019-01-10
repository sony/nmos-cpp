#ifndef NMOS_IS04_VERSIONS_H
#define NMOS_IS04_VERSIONS_H

#include <set>
#include "nmos/api_version.h"

namespace nmos
{
    namespace is04_versions
    {
        const api_version v1_0{ 1, 0 };
        const api_version v1_1{ 1, 1 };
        const api_version v1_2{ 1, 2 };

        const std::set<api_version> all{ nmos::is04_versions::v1_0, nmos::is04_versions::v1_1, nmos::is04_versions::v1_2 };
    }
}

#endif

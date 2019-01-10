#ifndef NMOS_IS05_VERSIONS_H
#define NMOS_IS05_VERSIONS_H

#include <set>
#include "nmos/api_version.h"

namespace nmos
{
    namespace is05_versions
    {
        const api_version v1_0{ 1, 0 };

        const std::set<api_version> all{ nmos::is05_versions::v1_0 };
    }
}

#endif

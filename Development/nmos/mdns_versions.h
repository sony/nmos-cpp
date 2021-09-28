#ifndef NMOS_MDNS_VERSIONS_H
#define NMOS_MDNS_VERSIONS_H

#include <set>
#include "nmos/api_version.h"

namespace nmos
{
    namespace experimental
    {
        namespace mdns_versions
        {
            const api_version v1_0{ 1, 0 };
            const api_version v1_1{ 1, 1 };

            const std::set<api_version> all{ nmos::experimental::mdns_versions::v1_0, nmos::experimental::mdns_versions::v1_1 };
        }
    }
}

#endif

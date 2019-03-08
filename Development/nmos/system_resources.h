#ifndef NMOS_SYSTEM_RESOURCES_H
#define NMOS_SYSTEM_RESOURCES_H

#include "nmos/id.h"
#include "nmos/settings.h"

// System API implementation
// See JT-NM TR-1001-1:2018 Annex A
namespace nmos
{
    struct resource;

    nmos::resource make_system_global(const nmos::id& id, const nmos::settings& settings);

    namespace experimental
    {
        // assign a system global configuration resource according to the settings
        void assign_system_global_resource(nmos::resource& resource, const nmos::settings& settings);
    }
}

#endif

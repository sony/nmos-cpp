#ifndef NMOS_SYSTEM_RESOURCES_H
#define NMOS_SYSTEM_RESOURCES_H

#include "nmos/id.h"
#include "nmos/settings.h"

namespace nmos
{
    struct resource;

    // System API global configuration resource
    // See https://github.com/AMWA-TV/nmos-system/blob/v1.0/APIs/schemas/global.json
    nmos::resource make_system_global(const nmos::id& id, const nmos::settings& settings);

    web::json::value make_system_global_data(const nmos::id& id, const nmos::settings& settings);
    std::pair<nmos::id, nmos::settings> parse_system_global_data(const web::json::value& system_global);

    namespace experimental
    {
        // assign a system global configuration resource according to the settings
        void assign_system_global_resource(nmos::resource& resource, const nmos::settings& settings);
    }
}

#endif

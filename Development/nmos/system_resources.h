#ifndef NMOS_SYSTEM_RESOURCES_H
#define NMOS_SYSTEM_RESOURCES_H

#include "nmos/id.h"
#include "nmos/settings.h"

namespace nmos
{
    struct resource;

    // System API global configuration resource
    // See https://specs.amwa.tv/is-09/releases/v1.0.0/APIs/schemas/with-refs/global.html
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

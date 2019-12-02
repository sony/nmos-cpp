#ifndef NMOS_NODE_RESOURCE_H
#define NMOS_NODE_RESOURCE_H

#include "nmos/id.h"
#include "nmos/settings.h"

namespace web
{
    namespace hosts
    {
        namespace experimental
        {
            struct host_interface;
        }
    }
}

namespace nmos
{
    struct clock_name;
    struct resource;

    // See  https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/schemas/node.json
    nmos::resource make_node(const nmos::id& id, const web::json::value& clocks, const web::json::value& interfaces, const nmos::settings& settings);
    nmos::resource make_node(const nmos::id& id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/clock_internal.json
    web::json::value make_internal_clock(const nmos::clock_name& clock_name);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/clock_ptp.json
    web::json::value make_ptp_clock(const nmos::clock_name& clock_name, bool traceable, const utility::string_t& gmid, bool locked);
}

#endif

#ifndef NMOS_NODE_RESOURCE_H
#define NMOS_NODE_RESOURCE_H

#include "nmos/id.h"
#include "nmos/settings.h"

namespace nmos
{
    struct resource;

    // See  https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/schemas/node.json
    nmos::resource make_node(const nmos::id& id, const nmos::settings& settings);
}

#endif

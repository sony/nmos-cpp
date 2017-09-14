#ifndef NMOS_NODE_RESOURCES_H
#define NMOS_NODE_RESOURCES_H

#include "nmos/resources.h"
#include "nmos/settings.h"

// This currently serves as an example of the resources that an NMOS node would construct
namespace nmos
{
    namespace experimental
    {
        void make_node_resources(nmos::resources& resources, const nmos::settings& settings);
    }
}

#endif

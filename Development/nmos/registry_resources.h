#ifndef NMOS_REGISTRY_RESOURCES_H
#define NMOS_REGISTRY_RESOURCES_H

#include "nmos/resources.h"
#include "nmos/settings.h"

// This currently serves as an example of the resources that an NMOS registry should construct for itself
namespace nmos
{
    namespace experimental
    {
        // insert a node resource according to the settings; return an iterator to the inserted node resource,
        // or to a resource that prevented the insertion, and a bool denoting whether the insertion took place
        std::pair<resources::iterator, bool> insert_registry_resources(nmos::resources& resources, const nmos::settings& settings);
    }
}

#endif

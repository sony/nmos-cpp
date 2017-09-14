#ifndef NMOS_SERVER_RESOURCES_H
#define NMOS_SERVER_RESOURCES_H

#include "nmos/resources.h"
#include "nmos/settings.h"

// This currently serves as an example of the resources that an NMOS server (registry) should construct for itself
namespace nmos
{
    namespace experimental
    {
        void make_server_resources(nmos::resources& resources, const nmos::settings& settings);
    }
}

#endif

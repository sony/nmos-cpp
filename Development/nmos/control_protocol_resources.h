#ifndef NMOS_CONTROL_PROTOCOL_RESOURCES_H
#define NMOS_CONTROL_PROTOCOL_RESOURCES_H

#include <map>
#include "nmos/control_protocol_resource.h" // for details::nc_oid definition
#include "nmos/settings.h"

namespace nmos
{
    struct resource;

    nmos::resource make_device_manager(details::nc_oid oid, nmos::resource& root_block, const nmos::settings& settings);

    nmos::resource make_class_manager(details::nc_oid oid, nmos::resource& root_block);

    nmos::resource make_root_block();
}

#endif

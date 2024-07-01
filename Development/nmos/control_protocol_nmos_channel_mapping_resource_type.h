#ifndef NMOS_CONTROL_PROTOCOL_NMOS_CHANNEL_MAPPING_RESOURCE_TYPE_H
#define NMOS_CONTROL_PROTOCOL_NMOS_CHANNEL_MAPPING_RESOURCE_TYPE_H

#include "cpprest/basic_utils.h"
#include "nmos/string_enum.h"

namespace nmos
{
    // resourceType
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointresourcenmoschannelmapping
    DEFINE_STRING_ENUM(ncp_nmos_channel_mapping_resource_type)
    namespace ncp_nmos_channel_mapping_resource_types
    {
        const ncp_nmos_channel_mapping_resource_type input{ U("input") };
        const ncp_nmos_channel_mapping_resource_type output{ U("output") };
    }
}

#endif

#ifndef NMOS_CONTROL_PROTOCOL_NMOS_RESOURCE_TYPE_H
#define NMOS_CONTROL_PROTOCOL_NMOS_RESOURCE_TYPE_H

#include "cpprest/basic_utils.h"
#include "nmos/string_enum.h"

namespace nmos
{
    // resourceType
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointresourcenmos
    DEFINE_STRING_ENUM(ncp_touchpoint_resource_type)
    namespace ncp_touchpoint_resource_types
    {
        const ncp_touchpoint_resource_type node{ U("node") };
        const ncp_touchpoint_resource_type device{ U("device") };
        const ncp_touchpoint_resource_type source{ U("source") };
        const ncp_touchpoint_resource_type flow{ U("flow") };
        const ncp_touchpoint_resource_type sender{ U("sender") };
        const ncp_touchpoint_resource_type receiver{ U("receiver") };
    }
}

#endif

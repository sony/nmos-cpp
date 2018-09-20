#ifndef NMOS_DEVICE_TYPE_H
#define NMOS_DEVICE_TYPE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Device types
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.1.%20APIs%20-%20Common%20Keys.md#type-devices
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/device.json
    DEFINE_STRING_ENUM(device_type)
    namespace device_types
    {
        const device_type generic{ U("urn:x-nmos:device:generic") };
        const device_type pipeline{ U("urn:x-nmos:device:pipeline") };
    }
}

#endif

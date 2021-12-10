#ifndef NMOS_DEVICE_TYPE_H
#define NMOS_DEVICE_TYPE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Device types
    // See https://specs.amwa.tv/is-04/releases/v1.2.0/docs/2.1._APIs_-_Common_Keys.html#type-devices
    // and https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/device.html
    DEFINE_STRING_ENUM(device_type)
    namespace device_types
    {
        const device_type generic{ U("urn:x-nmos:device:generic") };
        const device_type pipeline{ U("urn:x-nmos:device:pipeline") };
    }
}

#endif

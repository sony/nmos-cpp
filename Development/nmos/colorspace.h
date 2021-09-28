#ifndef NMOS_COLORSPACE_H
#define NMOS_COLORSPACE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Colorspace (used in video flows)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video.json
    DEFINE_STRING_ENUM(colorspace)
    namespace colorspaces
    {
        const colorspace BT601{ U("BT601") };
        const colorspace BT709{ U("BT709") };
        const colorspace BT2020{ U("BT2020") };
        const colorspace BT2100{ U("BT2100") };
    }
}

#endif

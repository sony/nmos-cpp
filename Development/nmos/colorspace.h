#ifndef NMOS_COLORSPACE_H
#define NMOS_COLORSPACE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Colorspace (used in video flows)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2.0/APIs/schemas/flow_video.json
    // and https://github.com/AMWA-TV/nmos-parameter-registers/tree/main/flow-attributes#colorspace
    DEFINE_STRING_ENUM(colorspace)
    namespace colorspaces
    {
        // Recommendation ITU-R BT.601-7
        const colorspace BT601{ U("BT601") };
        // Recommendation ITU-R BT.709-6
        const colorspace BT709{ U("BT709") };
        // Recommendation ITU-R BT.2020-2
        const colorspace BT2020{ U("BT2020") };
        // Recommendation ITU-R BT.2100 Table 2 titled "System colorimetry"
        const colorspace BT2100{ U("BT2100") };

        // Since IS-04 v1.3, colorspace values may be defined in the Flow Attributes register of the NMOS Parameter Registers

        // SMPTE ST 2065-1 Academy Color Encoding Specification (ACES)
        const colorspace ST2065_1{ U("ST2065-1") };
        // SMPTE ST 2065-3 Academy Density Exchange Encoding (ADX)
        const colorspace ST2065_3{ U("ST2065-3") };
        // ISO 11664-1 CIE 1931 standard colorimetric system
        const colorspace XYZ{ U("XYZ") };
    }
}

#endif

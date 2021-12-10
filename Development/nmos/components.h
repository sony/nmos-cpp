#ifndef NMOS_COMPONENTS_H
#define NMOS_COMPONENTS_H

#include "cpprest/json.h"
#include "nmos/string_enum.h"

namespace nmos
{
    // Components (for raw video flows since IS-04 v1.1, extended to coded video Flows since v1.3 by the entry in the Flow Attributes register)
    // See https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/flow_video_raw.html
    // and https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/#components
    DEFINE_STRING_ENUM(component_name)
    namespace component_names
    {
        const component_name Y{ U("Y") };
        const component_name Cb{ U("Cb") };
        const component_name Cr{ U("Cr") };
        const component_name I{ U("I") };
        const component_name Ct{ U("Ct") };
        const component_name Cp{ U("Cp") };
        const component_name A{ U("A") };
        const component_name R{ U("R") };
        const component_name G{ U("G") };
        const component_name B{ U("B") };
        const component_name DepthMap{ U("DepthMap") };

        // Since IS-04 v1.3, component names may be defined in the Flow Attributes register of the NMOS Parameter Registers
        // The following values support CLYCbCr, XYZ, and KEY signal formats, see sdp::samplings

        const component_name Yc{ U("Yc") };
        const component_name Cbc{ U("Cbc") };
        const component_name Crc{ U("Crc") };
        const component_name X{ U("X") };
        const component_name Z{ U("Z") };
        const component_name Key{ U("Key") };
    }

    web::json::value make_component(const nmos::component_name& name, unsigned int width, unsigned int height, unsigned int bit_depth);

    enum chroma_subsampling : int { YCbCr422, RGB444 };
    // deprecated, see overload with sdp::sampling in nmos/sdp_utils.h
    web::json::value make_components(chroma_subsampling chroma_subsampling = YCbCr422, unsigned int frame_width = 1920, unsigned int frame_height = 1080, unsigned int bit_depth = 10);
}

#endif

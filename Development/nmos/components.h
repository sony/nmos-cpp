#ifndef NMOS_COMPONENTS_H
#define NMOS_COMPONENTS_H

#include "cpprest/json.h"
#include "nmos/string_enum.h"

namespace nmos
{
    // Components (used in raw video flows)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video_raw.json
    // and https://github.com/AMWA-TV/nmos-parameter-registers/tree/main/flow-attributes#components
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
        // Experimental extension, to support CLYCbCr, XYZ, and KEY signal formats
        const component_name Yc{ U("Yc") };
        const component_name Cbc{ U("Cbc") };
        const component_name Crc{ U("Crc") };
        const component_name X{ U("X") };
        const component_name Z{ U("Z") };
        const component_name Key{ U("Key") };
    }

    web::json::value make_component(const nmos::component_name& name, unsigned int width, unsigned int height, unsigned int bit_depth);

    enum chroma_subsampling : int { YCbCr422, RGB444 };
    web::json::value make_components(chroma_subsampling chroma_subsampling = YCbCr422, unsigned int frame_width = 1920, unsigned int frame_height = 1080, unsigned int bit_depth = 10);
}

#endif

#ifndef NMOS_INTERLACE_MODE_H
#define NMOS_INTERLACE_MODE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Interlace modes (used in video flows)
    // See https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/flow_video.html
    DEFINE_STRING_ENUM(interlace_mode)
    namespace interlace_modes
    {
        const interlace_mode none{};

        const interlace_mode progressive{ U("progressive") };
        const interlace_mode interlaced_tff{ U("interlaced_tff") };
        const interlace_mode interlaced_bff{ U("interlaced_bff") };
        const interlace_mode interlaced_psf{ U("interlaced_psf") };
    }
}

#endif

#ifndef NMOS_INTERLACE_MODE_H
#define NMOS_INTERLACE_MODE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Interlace modes (used in video flows)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video.json
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

#ifndef NMOS_MXL_H
#define NMOS_MXL_H

#include "nmos/media_type.h"

namespace nmos
{
    namespace media_types
    {
        // MXL video media types

        const media_type video_v210{ U("video/v210") };
        const media_type video_v210a{ U("video/v210a") };

        // MXL audio media types

        const media_type audio_float32{ U("audio/float32") };

        // MXL data media types

        //const media_type video_smpte291{ U("video/smpte291") };
    }
}

#endif
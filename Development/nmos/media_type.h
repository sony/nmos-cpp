#ifndef NMOS_MEDIA_TYPE_H
#define NMOS_MEDIA_TYPE_H

#include "cpprest/basic_utils.h"
#include "nmos/string_enum.h"

namespace nmos
{
    // Media types (used in flows and receivers)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video.json
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_audio_raw.json
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_video.json
    // etc.
    DEFINE_STRING_ENUM(media_type)
    namespace media_types
    {
        // Video media types

        const media_type video_raw{ U("video/raw") };

        // Audio media types

        inline media_type audio_L(unsigned int bit_depth)
        {
            return media_type{ U("audio/L") + utility::ostringstreamed(bit_depth) };
        }

        const media_type audio_L24 = audio_L(24);

        // Data media types

        const media_type video_smpte291{ U("video/smpte291") };

        const media_type application_json{ U("application/json") };

        // Mux media types

        // See http://www.videoservicesforum.org/download/technical_recommendations/VSF_TR-04_2015-11-12.pdf
        const media_type video_SMPTE2022_6{ U("video/SMPTE2022-6") };
    }
}

#endif

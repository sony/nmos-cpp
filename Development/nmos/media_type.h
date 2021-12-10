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

        // Uncompressed Video
        // See https://tools.ietf.org/html/rfc4175#section-6
        const media_type video_raw{ U("video/raw") };

        // JPEG XS
        // See https://tools.ietf.org/html/draft-ietf-payload-rtp-jpegxs-09#section-6
        const media_type video_jxsv{ U("video/jxsv") };

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

        // See SMPTE ST 2022-8:2019
        const media_type video_SMPTE2022_6{ U("video/SMPTE2022-6") };

        // Additional media types for NMOS responses

        const media_type application_sdp{ U("application/sdp") };

        // experimental extension, to support HTML rendering of NMOS responses
        const media_type text_html{ U("text/html") };

        // experimental extension, to support JSON rendering in NMOS responses
        const media_type application_schema_json{ U("application/schema+json") };
        const media_type application_sdp_json{ U("application/sdp+json") };
    }
}

#endif

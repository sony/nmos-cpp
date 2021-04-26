#ifndef NMOS_FORMAT_H
#define NMOS_FORMAT_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Formats (used in sources, flows and receivers)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.1.%20APIs%20-%20Common%20Keys.md#format
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_generic.json
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_audio.json
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video.json
    // etc.
    DEFINE_STRING_ENUM(format)
    namespace formats
    {
        const format video{ U("urn:x-nmos:format:video") };
        const format audio{ U("urn:x-nmos:format:audio") };
        const format data{ U("urn:x-nmos:format:data") };
        const format mux{ U("urn:x-nmos:format:mux") };
    }
}

#endif

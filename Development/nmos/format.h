#ifndef NMOS_FORMAT_H
#define NMOS_FORMAT_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Formats (used in sources, flows and receivers)
    // See https://specs.amwa.tv/is-04/releases/v1.2.0/docs/2.1._APIs_-_Common_Keys.html#format
    // and https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/source_generic.html
    // and https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/source_audio.html
    // and https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/flow_video.html
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

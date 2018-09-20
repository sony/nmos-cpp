#ifndef NMOS_TRANSPORT_H
#define NMOS_TRANSPORT_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Transports (used in senders and receivers)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.1.%20APIs%20-%20Common%20Keys.md#transport
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/sender.json
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_core.json
    DEFINE_STRING_ENUM(transport)
    namespace transports
    {
        const transport rtp{ U("urn:x-nmos:transport:rtp") };
        const transport rtp_ucast{ U("urn:x-nmos:transport:rtp.ucast") };
        const transport rtp_mcast{ U("urn:x-nmos:transport:rtp.mcast") };
        const transport dash{ U("urn:x-nmos:transport:dash") };
    }
}

#endif

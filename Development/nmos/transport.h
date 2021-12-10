#ifndef NMOS_TRANSPORT_H
#define NMOS_TRANSPORT_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Transports (used in senders and receivers)
    // See https://specs.amwa.tv/is-04/releases/v1.2.0/docs/2.1._APIs_-_Common_Keys.html#transport
    // and https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/sender.html
    // and https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/receiver_core.html
    // and experimentally, for IS-04 v1.3, IS-05 v1.1, IS-07 v1.0
    // also https://github.com/AMWA-TV/nmos-parameter-registers/pull/6
    DEFINE_STRING_ENUM(transport)
    namespace transports
    {
        const transport rtp{ U("urn:x-nmos:transport:rtp") };
        const transport rtp_ucast{ U("urn:x-nmos:transport:rtp.ucast") };
        const transport rtp_mcast{ U("urn:x-nmos:transport:rtp.mcast") };
        const transport dash{ U("urn:x-nmos:transport:dash") };

        const transport mqtt{ U("urn:x-nmos:transport:mqtt") };
        const transport websocket{ U("urn:x-nmos:transport:websocket") };
    }

    // "Subclassifications are defined as the portion of the URN which follows the first occurrence of a '.', but prior to any '/' character."
    // "Versions are defined as the portion of the URN which follows the first occurrence of a '/'."
    // See https://specs.amwa.tv/is-04/releases/v1.2.2/docs/2.1._APIs_-_Common_Keys.html#use-of-urns
    inline nmos::transport transport_base(const nmos::transport& transport)
    {
        return nmos::transport{ transport.name.substr(0, transport.name.find_first_of(U("./"))) };
    }
}

#endif

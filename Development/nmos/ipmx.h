#ifndef NMOS_IPMX_H
#define NMOS_IPMX_H

#include "cpprest/details/basic_types.h"

namespace nmos
{
    namespace ipmx
    {
        // Default multicast IPv4 address of a media stream of an IPMX Sender, 239.S.C.D, given the IPv4 address A.B.C.D
        // of its media network port and the stream number S, which must be greater than 0 and less than 128
        // See VSF TR-10-9:2025 Section 17.1
        utility::string_t make_default_multicast_address_v4(const utility::string_t& interface_address, int stream_number);
    }
}

#endif

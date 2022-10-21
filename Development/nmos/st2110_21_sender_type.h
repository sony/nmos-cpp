#ifndef NMOS_ST2110_21_SENDER_TYPE_H
#define NMOS_ST2110_21_SENDER_TYPE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // ST 2110-21 Sender Type
    // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/sender-attributes/#st-2110-21-sender-type
    DEFINE_STRING_ENUM(st2110_21_sender_type)
    namespace st2110_21_sender_types
    {
        // Narrow Senders (Type N)
        const st2110_21_sender_type type_N{ U("2110TPN") };
        // Narrow Linear Senders (Type NL)
        const st2110_21_sender_type type_NL{ U("2110TPNL") };
        // Wide Senders (Type W)
        const st2110_21_sender_type type_W{ U("2110TPW") };
    }
}

#endif

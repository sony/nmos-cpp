#ifndef NMOS_CLOCK_REF_TYPE_H
#define NMOS_CLOCK_REF_TYPE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Clock reference type
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/clock_internal.json
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/clock_ptp.json
    DEFINE_STRING_ENUM(clock_ref_type)
    namespace clock_ref_types
    {
        const clock_ref_type internal{ U("internal") };
        const clock_ref_type ptp{ U("ptp") };
    }
}

#endif

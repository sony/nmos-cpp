#ifndef NMOS_CLOCK_REF_TYPE_H
#define NMOS_CLOCK_REF_TYPE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Clock reference type
    // See https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/clock_internal.html
    // and https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/clock_ptp.html
    DEFINE_STRING_ENUM(clock_ref_type)
    namespace clock_ref_types
    {
        const clock_ref_type internal{ U("internal") };
        const clock_ref_type ptp{ U("ptp") };
    }
}

#endif

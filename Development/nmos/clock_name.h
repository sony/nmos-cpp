#ifndef NMOS_CLOCK_NAME_H
#define NMOS_CLOCK_NAME_H

#include "cpprest/basic_utils.h"
#include "nmos/string_enum.h"

namespace nmos
{
    // Clock name
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/clock_internal.json
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/clock_ptp.json
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_core.json
    DEFINE_STRING_ENUM(clock_name)
    namespace clock_names
    {
        const clock_name clk0{ U("clk0") };

        inline const clock_name clk(unsigned int n)
        {
            return clock_name{ U("clk") + utility::ostringstreamed(n) };
        }
    }
}

#endif

#ifndef NMOS_CLOCK_NAME_H
#define NMOS_CLOCK_NAME_H

#include "cpprest/basic_utils.h"
#include "nmos/string_enum.h"

namespace nmos
{
    // Clock name
    // See https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/clock_internal.html
    // and https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/clock_ptp.html
    // and https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/source_core.html
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

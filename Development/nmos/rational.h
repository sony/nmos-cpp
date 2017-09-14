#ifndef NMOS_RATIONAL_H
#define NMOS_RATIONAL_H

#include "cpprest/json_utils.h"

namespace nmos
{
    // A sub-object representing a rational is used in several of the NMOS IS-04 schemas
    // although unfortunately there are differences in the minimum and default specified
    inline web::json::value make_rational(int64_t numerator = 0, int64_t denominator = 1)
    {
        return web::json::value_of({
            { U("numerator"), numerator },
            { U("denominator"), denominator }
        });
    }
}

#endif

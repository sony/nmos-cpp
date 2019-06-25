#ifndef NMOS_RATIONAL_H
#define NMOS_RATIONAL_H

#include <boost/rational.hpp>
#include "cpprest/json_ops.h"

namespace nmos
{
    // A sub-object representing a rational is used in several of the NMOS IS-04 schemas
    // although unfortunately there are differences in the minimum and default specified
    // (uint64_t ought to work, but I can't be bothered to fix the resulting compile warnings...)
    typedef boost::rational<int64_t> rational; // defaults to { 0, 1 }

    inline web::json::value make_rational(const rational& value = {})
    {
        return web::json::value_of({
            { U("numerator"), value.numerator() },
            { U("denominator"), value.denominator() }
        });
    }

    inline web::json::value make_rational(int64_t numerator, int64_t denominator)
    {
        return make_rational({ numerator, denominator });
    }
}

#endif

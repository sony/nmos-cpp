#ifndef NMOS_RATIONAL_H
#define NMOS_RATIONAL_H

#include <boost/rational.hpp>
#include "cpprest/json.h"

namespace nmos
{
    // A sub-object representing a rational is used in several of the NMOS IS-04 schemas
    // although unfortunately there are differences in the minimum and default specified
    // (uint64_t ought to work, but I can't be bothered to fix the resulting compile warnings...)
    typedef boost::rational<int64_t> rational; // defaults to { 0, 1 }

    namespace rationals
    {
        const rational rate60{ 60, 1 };
        const rational rate59_94{ 60000, 1001 };
        const rational rate50{ 50, 1 };
        const rational rate30{ 30, 1 };
        const rational rate29_97{ 30000, 1001 };
        const rational rate25{ 25, 1 };
        const rational rate24{ 24, 1 };
        const rational rate23_98{ 24000, 1001 };
    }
    namespace rates = rationals;

    web::json::value make_rational(const rational& value = {});

    inline web::json::value make_rational(int64_t numerator, int64_t denominator)
    {
        return make_rational({ numerator, denominator });
    }

    rational parse_rational(const web::json::value& value);
}

#endif

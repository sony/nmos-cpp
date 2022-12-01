#include "nmos/rational.h"
#include "nmos/json_fields.h"

namespace nmos
{
    web::json::value make_rational(const rational& value)
    {
        return web::json::value_of({
            { nmos::fields::numerator, value.numerator() },
            { nmos::fields::denominator, value.denominator() }
        });
    }

    bool is_rational(const web::json::value& value)
    {
        // denominator has a default of 1 so may be omitted
        return value.has_integer_field(nmos::fields::numerator);
    }

    rational parse_rational(const web::json::value& value)
    {
        return{
            nmos::fields::numerator(value),
            nmos::fields::denominator(value)
        };
    }
}

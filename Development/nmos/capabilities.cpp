#include "nmos/capabilities.h"

#include <boost/range/adaptor/transformed.hpp>
#include "nmos/json_fields.h"

namespace nmos
{
    web::json::value make_caps_string_constraint(const std::vector<utility::string_t>& enum_values)
    {
        using web::json::value_of;
        using web::json::value_from_elements;
        return value_of({
            { !enum_values.empty() ? nmos::fields::constraint_enum.key : U(""), value_from_elements(enum_values) }
        });
    }

    web::json::value make_caps_integer_constraint(const std::vector<int64_t>& enum_values, int64_t minimum, int64_t maximum)
    {
        using web::json::value_of;
        using web::json::value_from_elements;
        return value_of({
            { !enum_values.empty() ? nmos::fields::constraint_enum.key : U(""), value_from_elements(enum_values) },
            { minimum != (std::numeric_limits<int64_t>::max)() ? nmos::fields::constraint_minimum.key : U(""), minimum },
            { maximum != (std::numeric_limits<int64_t>::min)() ? nmos::fields::constraint_maximum.key : U(""), maximum }
        });
    }

    web::json::value make_caps_number_constraint(const std::vector<double>& enum_values, double minimum, double maximum)
    {
        using web::json::value_of;
        using web::json::value_from_elements;
        return value_of({
            { !enum_values.empty() ? nmos::fields::constraint_enum.key : U(""), value_from_elements(enum_values) },
            { minimum != (std::numeric_limits<double>::max)() ? nmos::fields::constraint_minimum.key : U(""), minimum },
            { maximum != (std::numeric_limits<double>::min)() ? nmos::fields::constraint_maximum.key : U(""), maximum }
        });
    }

    web::json::value make_caps_boolean_constraint(const std::vector<bool>& enum_values)
    {
        using web::json::value_of;
        using web::json::value_from_elements;
        return value_of({
            { !enum_values.empty() ? nmos::fields::constraint_enum.key : U(""), value_from_elements(enum_values) }
        });
    }

    web::json::value make_caps_rational_constraint(const std::vector<nmos::rational>& enum_values, const nmos::rational& minimum, const nmos::rational& maximum)
    {
        using web::json::value_of;
        using web::json::value_from_elements;
        return value_of({
            { !enum_values.empty() ? nmos::fields::constraint_enum.key : U(""), value_from_elements(enum_values | boost::adaptors::transformed([](const nmos::rational& r) { return make_rational(r); })) },
            { minimum != (std::numeric_limits<int64_t>::max)() ? nmos::fields::constraint_minimum.key : U(""), make_rational(minimum) },
            { maximum != 0 ? nmos::fields::constraint_maximum.key : U(""), make_rational(maximum) }
        });
    }
}

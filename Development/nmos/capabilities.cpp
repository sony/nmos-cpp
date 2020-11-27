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
            { minimum != no_minimum<int64_t>() ? nmos::fields::constraint_minimum.key : U(""), minimum },
            { maximum != no_maximum<int64_t>() ? nmos::fields::constraint_maximum.key : U(""), maximum }
        });
    }

    web::json::value make_caps_number_constraint(const std::vector<double>& enum_values, double minimum, double maximum)
    {
        using web::json::value_of;
        using web::json::value_from_elements;
        return value_of({
            { !enum_values.empty() ? nmos::fields::constraint_enum.key : U(""), value_from_elements(enum_values) },
            { minimum != no_minimum<double>() ? nmos::fields::constraint_minimum.key : U(""), minimum },
            { maximum != no_maximum<double>() ? nmos::fields::constraint_maximum.key : U(""), maximum }
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
            { minimum != no_minimum<nmos::rational>() ? nmos::fields::constraint_minimum.key : U(""), make_rational(minimum) },
            { maximum != no_maximum<nmos::rational>() ? nmos::fields::constraint_maximum.key : U(""), make_rational(maximum) }
        });
    }

    namespace details
    {
        // cf. nmos::details::make_constraints_schema in nmos/connection_api.cpp 
        template <typename T, typename Parse>
        bool match_constraint(const T& value, const web::json::value& constraint, Parse parse)
        {
            if (constraint.has_field(nmos::fields::constraint_enum))
            {
                const auto& enum_values = nmos::fields::constraint_enum(constraint).as_array();
                if (enum_values.end() == std::find_if(enum_values.begin(), enum_values.end(), [&parse, &value](const web::json::value& enum_value)
                {
                    return parse(enum_value) == value;
                }))
                {
                    return false;
                }
            }
            if (constraint.has_field(nmos::fields::constraint_minimum))
            {
                const auto& minimum = nmos::fields::constraint_minimum(constraint);
                if (parse(minimum) > value)
                {
                    return false;
                }
            }
            if (constraint.has_field(nmos::fields::constraint_maximum))
            {
                const auto& maximum = nmos::fields::constraint_maximum(constraint);
                if (parse(maximum) < value)
                {
                    return false;
                }
            }
            return true;
        }
    }

    bool match_string_constraint(const utility::string_t& value, const web::json::value& constraint)
    {
        return details::match_constraint(value, constraint, [](const web::json::value& enum_value)
        {
            return enum_value.as_string();
        });
    }

    bool match_integer_constraint(int64_t value, const web::json::value& constraint)
    {
        return details::match_constraint(value, constraint, [&value](const web::json::value& enum_value)
        {
            return enum_value.as_integer();
        });
    }

    bool match_number_constraint(double value, const web::json::value& constraint)
    {
        return details::match_constraint(value, constraint, [&value](const web::json::value& enum_value)
        {
            return enum_value.as_double();
        });
    }

    bool match_boolean_constraint(bool value, const web::json::value& constraint)
    {
        return details::match_constraint(value, constraint, [&value](const web::json::value& enum_value)
        {
            return enum_value.as_bool();
        });
    }

    bool match_rational_constraint(const nmos::rational& value, const web::json::value& constraint)
    {
        return details::match_constraint(value, constraint, [&value](const web::json::value& enum_value)
        {
            return nmos::parse_rational(enum_value);
        });
    }
}

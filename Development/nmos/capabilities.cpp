#include "nmos/capabilities.h"

#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/regex_utils.h"
#include "nmos/json_fields.h"

namespace nmos
{
    web::json::value make_caps_string_constraint(const std::vector<utility::string_t>& enum_values, const utility::string_t& pattern)
    {
        using web::json::value_of;
        using web::json::value_from_elements;
        return value_of({
            { !enum_values.empty() ? nmos::fields::constraint_enum.key : U(""), value_from_elements(enum_values) },
            { !pattern.empty() ? nmos::fields::constraint_pattern.key : U(""), pattern }
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
        template <typename T, typename Parse = web::json::details::value_as<T>>
        bool match_enum_constraint(const T& value, const web::json::value& constraint, Parse parse = {})
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
            return true;
        }

        template <typename T, typename Parse = web::json::details::value_as<T>>
        bool match_minimum_maximum_constraint(const T& value, const web::json::value& constraint, Parse parse = {})
        {
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

        bool match_pattern_constraint(const utility::string_t& value, const web::json::value& constraint)
        {
            if (constraint.has_field(nmos::fields::constraint_pattern))
            {
                const utility::regex_t regex(nmos::fields::constraint_pattern(constraint));
                utility::smatch_t match;
                // throws bst::regex_error if pattern is invalid
                if (!bst::regex_search(value, match, regex))
                {
                    return false;
                }
            }
            return true;
        }
    }

    bool match_string_constraint(const utility::string_t& value, const web::json::value& constraint)
    {
        return details::match_enum_constraint(value, constraint)
            && details::match_pattern_constraint(value, constraint);
    }

    bool match_integer_constraint(int64_t value, const web::json::value& constraint)
    {
        return details::match_enum_constraint(value, constraint)
            && details::match_minimum_maximum_constraint(value, constraint);
    }

    bool match_number_constraint(double value, const web::json::value& constraint)
    {
        return details::match_enum_constraint(value, constraint)
            && details::match_minimum_maximum_constraint(value, constraint);
    }

    bool match_boolean_constraint(bool value, const web::json::value& constraint)
    {
        return details::match_enum_constraint(value, constraint);
    }

    bool match_rational_constraint(const nmos::rational& value, const web::json::value& constraint)
    {
        return details::match_enum_constraint(value, constraint, &nmos::parse_rational)
            && details::match_minimum_maximum_constraint(value, constraint, &nmos::parse_rational);
    }

    bool match_constraint(const web::json::value& value, const web::json::value& constraint)
    {
        if (value.is_string())
        {
            return match_string_constraint(value.as_string(), constraint);
        }
        else if (value.is_integer())
        {
            return match_integer_constraint(value.as_number().to_int64(), constraint);
        }
        else if (value.is_double())
        {
            return match_number_constraint(value.as_double(), constraint);
        }
        else if (value.is_boolean())
        {
            return match_boolean_constraint(value.as_bool(), constraint);
        }
        else if (nmos::is_rational(value))
        {
            return match_rational_constraint(nmos::parse_rational(value), constraint);
        }
        else
        {
            throw web::json::json_exception("not a valid constraint target type");
        }
    }
}

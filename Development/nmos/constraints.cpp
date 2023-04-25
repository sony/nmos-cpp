#include "nmos/constraints.h"

#include <algorithm>
#include <set>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/basic_utils.h" // for utility::us2s
#include "cpprest/json_utils.h"
#include "nmos/capabilities.h"
#include "nmos/json_fields.h"
#include "nmos/rational.h"

namespace nmos
{
    namespace experimental
    {
        namespace detail
        {
            bool constraint_value_less(const web::json::value& lhs, const web::json::value& rhs)
            {
                if (nmos::is_rational(lhs) && nmos::is_rational(rhs))
                {
                    if (nmos::parse_rational(lhs) < nmos::parse_rational(rhs))
                    {
                        return true;
                    }
                    return false;
                }
                return lhs < rhs;
            }
        }

        web::json::value get_intersection(const web::json::array& lhs_, const web::json::array& rhs_)
        {
            std::set<web::json::value, decltype(&detail::constraint_value_less)> lhs(lhs_.begin(), lhs_.end(), &detail::constraint_value_less);
            std::set<web::json::value, decltype(&detail::constraint_value_less)> rhs(rhs_.begin(), rhs_.end(), &detail::constraint_value_less);

            std::vector<web::json::value> v(std::min(lhs.size(), rhs.size()));

            const auto it = std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), v.begin(), detail::constraint_value_less);
            v.resize(it - v.begin());

            return web::json::value_from_elements(v);
        }

        web::json::value get_intersection(const web::json::array& enumeration, const web::json::value& constraint)
        {
            return web::json::value_from_elements(enumeration | boost::adaptors::filtered([&constraint](const web::json::value& enum_value)
            {
                return match_constraint(enum_value, constraint);
            }));
        }

        web::json::value get_constraint_intersection(const web::json::value& lhs, const web::json::value& rhs)
        {
            web::json::value result;

            // "enum"
            if (lhs.has_field(nmos::fields::constraint_enum) && rhs.has_field(nmos::fields::constraint_enum))
            {
                result[nmos::fields::constraint_enum] = get_intersection(nmos::fields::constraint_enum(lhs).as_array(), nmos::fields::constraint_enum(rhs).as_array());
            }
            else if (lhs.has_field(nmos::fields::constraint_enum))
            {
                result[nmos::fields::constraint_enum] = nmos::fields::constraint_enum(lhs);
            }
            else if (rhs.has_field(nmos::fields::constraint_enum))
            {
                result[nmos::fields::constraint_enum] = nmos::fields::constraint_enum(rhs);
            }

            // "minimum"
            if (lhs.has_field(nmos::fields::constraint_minimum) && rhs.has_field(nmos::fields::constraint_minimum))
            {
                result[nmos::fields::constraint_minimum] = std::max(nmos::fields::constraint_minimum(lhs), nmos::fields::constraint_minimum(rhs), detail::constraint_value_less);
            }
            else if (lhs.has_field(nmos::fields::constraint_minimum))
            {
                result[nmos::fields::constraint_minimum] = nmos::fields::constraint_minimum(lhs);
            }
            else if (rhs.has_field(nmos::fields::constraint_minimum))
            {
                result[nmos::fields::constraint_minimum] = nmos::fields::constraint_minimum(rhs);
            }

            // "maximum"
            if (lhs.has_field(nmos::fields::constraint_maximum) && rhs.has_field(nmos::fields::constraint_maximum))
            {
                result[nmos::fields::constraint_maximum] = std::min(nmos::fields::constraint_maximum(lhs), nmos::fields::constraint_maximum(rhs), detail::constraint_value_less);
            }
            else if (lhs.has_field(nmos::fields::constraint_maximum))
            {
                result[nmos::fields::constraint_maximum] = nmos::fields::constraint_maximum(lhs);
            }
            else if (rhs.has_field(nmos::fields::constraint_maximum))
            {
                result[nmos::fields::constraint_maximum] = nmos::fields::constraint_maximum(rhs);
            }

            // "min" > "max"
            if (result.has_field(nmos::fields::constraint_minimum) && result.has_field(nmos::fields::constraint_maximum))
            {
                if (detail::constraint_value_less(nmos::fields::constraint_maximum(result), nmos::fields::constraint_minimum(result)))
                {
                    return web::json::value::null();
                }
            }

            // "enum" matches "min"/"max"
            if (result.has_field(nmos::fields::constraint_enum) && (result.has_field(nmos::fields::constraint_minimum) || result.has_field(nmos::fields::constraint_maximum)))
            {
                result[nmos::fields::constraint_enum] = get_intersection(nmos::fields::constraint_enum(result).as_array(), result);
                for (const auto& keyword : { nmos::fields::constraint_minimum, nmos::fields::constraint_maximum })
                {
                    if (result.has_field(keyword))
                    {
                        result.erase(keyword);
                    }
                }
            }

            // "enum" is empty
            if (result.has_field(nmos::fields::constraint_enum) && web::json::empty(nmos::fields::constraint_enum(result)))
            {
                return web::json::value::null();
            }

            return result;
        }

        web::json::value get_constraint_set_intersection(const web::json::value& lhs, const web::json::value& rhs, bool merge_left_to_right)
        {
            using web::json::value;
            using web::json::value_from_elements;

            const auto& lhs_param_constraints = lhs.as_object();
            auto rhs_param_constraints = rhs.as_object();

            if (merge_left_to_right)
            {
                rhs_param_constraints = lhs_param_constraints;
                std::for_each(rhs.as_object().begin(), rhs.as_object().end(), [&rhs_param_constraints](const std::pair<utility::string_t, value>& rhs_constraint)
                {
                    rhs_param_constraints[rhs_constraint.first] = rhs_constraint.second;
                });
            }

            const auto common_param_constraints = get_intersection(
                value_from_elements(lhs_param_constraints | boost::adaptors::filtered([](const std::pair<utility::string_t, value>& constraint) { return !boost::algorithm::starts_with(constraint.first, U("urn:x-nmos:cap:meta:")); }) | boost::adaptors::transformed([](const std::pair<utility::string_t, value>& constraint) { return constraint.first; })).as_array(),
                value_from_elements(rhs_param_constraints | boost::adaptors::filtered([](const std::pair<utility::string_t, value>& constraint) { return !boost::algorithm::starts_with(constraint.first, U("urn:x-nmos:cap:meta:")); }) | boost::adaptors::transformed([](const std::pair<utility::string_t, value>& constraint) { return constraint.first; })).as_array()
            ).as_array();

            if (0 == common_param_constraints.size())
            {
                return value::null();
            }

            try
            {
                value result;

                std::for_each(common_param_constraints.begin(), common_param_constraints.end(), [&result, &lhs_param_constraints, &rhs_param_constraints](const web::json::value& constraint_name_)
                {
                    const auto& constraint_name = constraint_name_.as_string();
                    const web::json::value intersection = get_constraint_intersection(lhs_param_constraints.at(constraint_name), rhs_param_constraints.at(constraint_name));
                    if (intersection.is_null())
                    {
                        throw std::runtime_error(utility::us2s(constraint_name) + " gives empty intersection");
                    }
                    result[constraint_name] = intersection;
                });

                return result;
            }
            catch (const std::runtime_error& e)
            {
                return value::null();
            }
        }

        // Constraint B is a subconstraint of Constraint A if:
        // 1. Constraint B has enum keyword when Constraint A has it and enumerated values of Constraint B are a subset of enumerated values of Constraint A
        // 2. Constraint B has enum or minimum keyword when Constraint A has minimum keyword and allowed values of Constraint B are greater than minimum value of Constraint A
        // 3. Constraint B has enum or maximum keyword when Constraint A has maximum keyword and allowed values of Constraint B are less than maximum value of Constraint A
        bool is_subconstraint(const web::json::value& constraint, const web::json::value& subconstraint)
        {
            // subconstraint should have enum if constraint has enum
            if (constraint.has_field(nmos::fields::constraint_enum) && !subconstraint.has_field(nmos::fields::constraint_enum))
            {
                return false;
            }
            // subconstraint should have minimum or enum if constraint has minimum
            if (constraint.has_field(nmos::fields::constraint_minimum) && !subconstraint.has_field(nmos::fields::constraint_enum) && !subconstraint.has_field(nmos::fields::constraint_minimum))
            {
                return false;
            }
            // subconstraint should have maximum or enum if constraint has maximum
            if (constraint.has_field(nmos::fields::constraint_maximum) && !subconstraint.has_field(nmos::fields::constraint_enum) && !subconstraint.has_field(nmos::fields::constraint_maximum))
            {
                return false;
            }
            if (constraint.has_field(nmos::fields::constraint_minimum) && subconstraint.has_field(nmos::fields::constraint_minimum))
            {
                const auto& constraint_min = nmos::fields::constraint_minimum(constraint);
                const auto& subconstraint_min = nmos::fields::constraint_minimum(subconstraint);

                if (detail::constraint_value_less(subconstraint_min, constraint_min))
                {
                    return false;
                }
            }
            if (constraint.has_field(nmos::fields::constraint_maximum) && subconstraint.has_field(nmos::fields::constraint_maximum))
            {
                const auto& constraint_max = nmos::fields::constraint_maximum(constraint);
                const auto& subconstraint_max = nmos::fields::constraint_maximum(subconstraint);

                if (detail::constraint_value_less(constraint_max, subconstraint_max))
                {
                    return false;
                }
            }
            // subconstraint enum values should match constraint
            if (subconstraint.has_field(nmos::fields::constraint_enum))
            {
                const auto& subconstraint_enum_values = nmos::fields::constraint_enum(subconstraint).as_array();
                return subconstraint_enum_values.end() == std::find_if_not(subconstraint_enum_values.begin(), subconstraint_enum_values.end(), [&constraint](const web::json::value& enum_value)
                {
                    return match_constraint(enum_value, constraint);
                });
            }
            return true;
        }

        // Constraint Set B is a subset of Constraint Set A if all Parameter Constraints of Constraint Set A are present in Constraint Set B, and for each Parameter Constraint
        // that is present in both, the Parameter Constraint of Constraint Set B is a subconstraint of the Parameter Constraint of Constraint Set A.
        bool is_constraint_subset(const web::json::value& constraint_set, const web::json::value& constraint_subset)
        {
            using web::json::value;

            const auto& param_constraints_set = constraint_set.as_object();
            const auto& param_constraints_subset = constraint_subset.as_object();

            return param_constraints_set.end() == std::find_if_not(param_constraints_set.begin(), param_constraints_set.end(), [&param_constraints_subset](const std::pair<utility::string_t, value>& constraint)
            {
                if (boost::algorithm::starts_with(constraint.first, U("urn:x-nmos:cap:meta:"))) return true;

                const auto& subconstraint = param_constraints_subset.find(constraint.first);
                return param_constraints_subset.end() != subconstraint && is_subconstraint(constraint.second, subconstraint->second);
            });
        }
    }
}

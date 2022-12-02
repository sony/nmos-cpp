#include "nmos/constraints.h"

#include <map>
#include <boost/algorithm/string/predicate.hpp>
#include "nmos/capabilities.h"
#include "nmos/json_fields.h"
#include "nmos/rational.h"

namespace nmos
{
    namespace experimental
    {
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

                if (nmos::is_rational(constraint_min))
                {
                    if (nmos::parse_rational(constraint_min) > nmos::parse_rational(subconstraint_min))
                    {
                        return false;
                    }
                }
                else if (constraint_min.is_integer() && subconstraint_min.is_integer())
                {
                    if (constraint_min.as_number().to_int64() > subconstraint_min.as_number().to_int64())
                    {
                        return false;
                    }
                }
                else if (constraint_min.as_double() > subconstraint_min.as_double())
                {
                    return false;
                }
            }
            if (constraint.has_field(nmos::fields::constraint_maximum) && subconstraint.has_field(nmos::fields::constraint_maximum))
            {
                const auto& constraint_max = nmos::fields::constraint_maximum(constraint);
                const auto& subconstraint_max = nmos::fields::constraint_maximum(subconstraint);

                if (nmos::is_rational(constraint_max))
                {
                    if (nmos::parse_rational(constraint_max) < nmos::parse_rational(subconstraint_max))
                    {
                        return false;
                    }
                }
                else if (constraint_max.is_integer() && subconstraint_max.is_integer())
                {
                    if (constraint_max.as_number().to_int64() < subconstraint_max.as_number().to_int64())
                    {
                        return false;
                    }
                }
                else if (constraint_max.as_double() < subconstraint_max.as_double())
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

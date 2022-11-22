#include "nmos/constraints.h"

#include <map>
#include <boost/algorithm/string/predicate.hpp>
#include "nmos/capabilities.h"
#include "nmos/json_fields.h"
#include "nmos/sdp_utils.h"
#include "nmos/video_jxsv.h"

namespace nmos
{
    namespace experimental
    {
        typedef std::map<utility::string_t, std::function<bool(const web::json::value& resource, const web::json::value& con)>> constraints_matcher;

        // NMOS Parameter Registers - Capabilities register
        // See https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md
        static const std::map<utility::string_t, std::function<bool(const web::json::value& source, const web::json::value& con)>> source_constraints_matcher
        {
            // Audio Constraints

            { nmos::caps::format::channel_count, [](const web::json::value& source, const web::json::value& con) { return nmos::match_integer_constraint((uint32_t)nmos::fields::channels(source).size(), con); } },
        };

        static const std::map<utility::string_t, std::function<bool(const web::json::value& flow, const web::json::value& con)>> flow_constraints_matcher
        {
            // General Constraints

            { nmos::caps::format::media_type, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_string_constraint(nmos::fields::media_type(flow), con); } },
            { nmos::caps::format::grain_rate, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_rational_constraint(nmos::parse_rational(nmos::fields::grain_rate(flow)), con); } },

            // Video Constraints

            { nmos::caps::format::frame_height, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_integer_constraint(nmos::fields::frame_height(flow), con); } },
            { nmos::caps::format::frame_width, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_integer_constraint(nmos::fields::frame_width(flow), con); } },
            { nmos::caps::format::color_sampling, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_string_constraint(nmos::details::make_sampling(nmos::fields::components(flow)).name, con); } },
            { nmos::caps::format::interlace_mode, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_string_constraint(nmos::fields::interlace_mode(flow), con); } },
            { nmos::caps::format::colorspace, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_string_constraint(nmos::fields::colorspace(flow), con); } },
            { nmos::caps::format::transfer_characteristic, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_string_constraint(nmos::fields::transfer_characteristic(flow), con); } },
            { nmos::caps::format::component_depth, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_integer_constraint(nmos::fields::bit_depth(nmos::fields::components(flow).at(0)), con); } },

            // Audio Constraints

            { nmos::caps::format::sample_rate, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_rational_constraint(nmos::parse_rational(nmos::fields::sample_rate(flow)), con); } },
            { nmos::caps::format::sample_depth, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_integer_constraint(nmos::fields::bit_depth(flow), con); } },
        };

        static const std::map<utility::string_t, std::function<bool(const web::json::value& sender, const web::json::value& con)>> sender_constraints_matcher
        {
            { nmos::caps::transport::packet_transmission_mode, [](const web::json::value& sender, const web::json::value& con) { return nmos::match_string_constraint(nmos::fields::packet_transmission_mode(sender), con); } },
            { nmos::caps::transport::st2110_21_sender_type, [](const web::json::value& sender, const web::json::value& con) { return nmos::match_string_constraint(nmos::fields::st2110_21_sender_type(sender), con); } },
            { nmos::caps::transport::bit_rate, [](const web::json::value& sender, const web::json::value& con) { return nmos::match_integer_constraint(nmos::fields::bit_rate(sender), con); } },
        };

        bool match_resource_parameters_constraint_set(const web::json::value& resource, const constraints_matcher& matcher, const web::json::value& constraint_set)
        {
            if (!nmos::caps::meta::enabled(constraint_set)) return true;

            const auto& constraints = constraint_set.as_object();
            return constraints.end() == std::find_if(constraints.begin(), constraints.end(), [&](const std::pair<utility::string_t, web::json::value>& constraint)
            {
                const auto& found = matcher.find(constraint.first);
                return matcher.end() != found && !found->second(resource, constraint.second);
            });
        }

        bool match_source_parameters_constraint_set(const web::json::value& source, const web::json::value& constraint_set)
        {
            return match_resource_parameters_constraint_set(source, source_constraints_matcher, constraint_set);
        }

        bool match_flow_parameters_constraint_set(const web::json::value& flow, const web::json::value& constraint_set)
        {
            return match_resource_parameters_constraint_set(flow, flow_constraints_matcher, constraint_set);
        }

        bool match_sender_parameters_constraint_set(const web::json::value& sender, const web::json::value& constraint_set)
        {
            return match_resource_parameters_constraint_set(sender, sender_constraints_matcher, constraint_set);
        }

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
                if (constraint.at(U("minimum")).has_field(nmos::fields::numerator))
                {
                    if (nmos::parse_rational(constraint.at(U("minimum"))) > nmos::parse_rational(subconstraint.at(U("minimum"))))
                    {
                        return false;
                    }
                }
                else if (nmos::fields::constraint_minimum(constraint) > nmos::fields::constraint_minimum(subconstraint))
                {
                    return false;
                }
            }
            if (constraint.has_field(nmos::fields::constraint_maximum) && subconstraint.has_field(nmos::fields::constraint_maximum))
            {
                if (constraint.at(U("maximum")).has_field(nmos::fields::numerator))
                {
                    if (nmos::parse_rational(constraint.at(U("maximum"))) < nmos::parse_rational(subconstraint.at(U("maximum"))))
                    {
                        return false;
                    }
                }
                else if (nmos::fields::constraint_maximum(constraint) < nmos::fields::constraint_maximum(subconstraint))
                {
                    return false;
                }
            }
            // subconstraint enum values should match constraint
            if (subconstraint.has_field(nmos::fields::constraint_enum))
            {
                const auto& subconstraint_enum_values = nmos::fields::constraint_enum(subconstraint).as_array();
                if (subconstraint_enum_values.end() == std::find_if(subconstraint_enum_values.begin(), subconstraint_enum_values.end(), [&constraint](const web::json::value& enum_value)
                {
                    return match_constraint(enum_value, constraint);
                }))
                {
                    return false;
                }
            }
            return true;
        }

        // Constraint Set B is a subset of Constraint Set A if all of Parameter Constraints of Constraint Set B, except for meta, are present in Constraint Set A and each Parameter Constraint of Constraint Set B, except for meta, is a subconstraint of the according Parameter Constraint of Constraint Set A.
        // Constraint B is a subconstraint of a Constraint A if:

        // 1. Constraint B has enum keyword when Constraint A has it and enum of Constraint B is a subset of enum of Constraint A
        // 2. Constraint B has enum or minimum keyword when Constraint A has minimum keyword and allowed values for Constraint B are less than allowed values for Constraint A
        // 3. Constraint B has enum or maximum keyword when Constraint A has maximum keyword and allowed values for Constraint B are greater than allowed values for Constraint A
        bool is_constraint_subset(const web::json::value& constraint_set, const web::json::value& constraint_subset)
        {
            using web::json::value;

            const auto& param_constraints_set = constraint_set.as_object();
            const auto& param_constraints_subset = constraint_subset.as_object();

            return param_constraints_subset.end() == std::find_if(param_constraints_subset.begin(), param_constraints_subset.end(), [&](const std::pair<utility::string_t, value>& subconstraint)
            {
                if (boost::algorithm::istarts_with(subconstraint.first, U("urn:x-nmos:cap:meta:"))) return false;

                const auto& constraint = param_constraints_set.find(subconstraint.first);
                return param_constraints_set.end() == constraint || !is_subconstraint(constraint->second, subconstraint.second);
            });
        }
    }
}

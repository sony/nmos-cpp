#ifndef NMOS_STREAMCOMPATIBILITY_VALIDATION_H
#define NMOS_STREAMCOMPATIBILITY_VALIDATION_H

#include <functional>
#include <map>
#include "nmos/capabilities.h"
#include "nmos/connection_api.h"
#include "nmos/resource.h"
#include "nmos/sdp_utils.h"
#include "nmos/streamcompatibility_state.h"
#include "nmos/video_jxsv.h"

namespace web
{
    namespace json
    {
        class array;
        class value;
    }
}

namespace nmos
{
    struct node_model;

    namespace experimental
    {
        namespace details
        {
            typedef std::function<std::pair<nmos::sender_state, utility::string_t>(const web::json::value& transport_file, const nmos::resource& sender, const nmos::resource& flow, const nmos::resource& source, const web::json::array& constraint_sets)> streamcompatibility_sender_validator;
            typedef std::function<bool(const nmos::resource& resource, const web::json::value& constraint_set)> resource_constraints_matcher;
            typedef std::function<bool(const std::pair<utility::string_t, utility::string_t>& transport_file, const web::json::array& constraint_sets)> transport_file_constraint_sets_matcher;
        }

        typedef std::map<utility::string_t, std::function<bool(const web::json::value& resource, const web::json::value& con)>> parameter_constraints;

        // NMOS Parameter Registers - Capabilities register
        // See https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md
        const std::map<utility::string_t, std::function<bool(const web::json::value& source, const web::json::value& con)>> source_parameter_constraints
        {
            // Audio Constraints

            { nmos::caps::format::channel_count, [](const web::json::value& source, const web::json::value& con) { return nmos::match_integer_constraint((uint32_t)nmos::fields::channels(source).size(), con); } }
        };

        const std::map<utility::string_t, std::function<bool(const web::json::value& flow, const web::json::value& con)>> flow_parameter_constraints
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
            { nmos::caps::format::sample_depth, [](const web::json::value& flow, const web::json::value& con) { return nmos::match_integer_constraint(nmos::fields::bit_depth(flow), con); } }
        };

        const std::map<utility::string_t, std::function<bool(const web::json::value& sender, const web::json::value& con)>> sender_parameter_constraints
        {
            { nmos::caps::transport::packet_transmission_mode, [](const web::json::value& sender, const web::json::value& con) { return nmos::match_string_constraint(nmos::fields::packet_transmission_mode(sender), con); } },
            { nmos::caps::transport::st2110_21_sender_type, [](const web::json::value& sender, const web::json::value& con) { return nmos::match_string_constraint(nmos::fields::st2110_21_sender_type(sender), con); } },
            { nmos::caps::transport::bit_rate, [](const web::json::value& sender, const web::json::value& con) { return nmos::match_integer_constraint(nmos::fields::bit_rate(sender), con); } }
        };

        bool match_resource_parameters_constraint_set(const parameter_constraints& constraints, const web::json::value& resource, const web::json::value& constraint_set);

        // "At any time if State of an active Sender becomes active_constraints_violation, the Sender MUST become inactive.
        // An inactive Sender in this state MUST NOT allow activations.
        // At any time if State of an active Receiver becomes non_compliant_stream, the Receiver SHOULD become inactive.
        // An inactive Receiver in this state SHOULD NOT allow activations."
        // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#preventing-restrictions-violation
        nmos::details::connection_resource_patch_validator make_connection_streamcompatibility_validator(nmos::node_model& model);

        details::streamcompatibility_sender_validator make_streamcompatibility_sender_resources_validator(const details::resource_constraints_matcher& resource_matcher, const details::transport_file_constraint_sets_matcher& transport_file_matcher);

        details::streamcompatibility_sender_validator make_streamcompatibility_sender_resources_validator();
    }
}

#endif
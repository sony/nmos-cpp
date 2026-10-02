#ifndef NMOS_STREAMCOMPATIBILITY_VALIDATION_H
#define NMOS_STREAMCOMPATIBILITY_VALIDATION_H

#include <functional>
#include <map>
#include "nmos/capabilities.h"
#include "nmos/connection_api.h"
#include "nmos/media_type.h"
#include "nmos/resource.h"
#include "nmos/sdp_utils.h"
#include "nmos/streamcompatibility_state.h"

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
            // Sender validation with its associated resources, returns sender's "state" and "debug" values
            // It is used by the stream compatibility behaviour thread
            // It may throw exception, which will be logged
            typedef std::function<std::pair<nmos::sender_state, utility::string_t>(const nmos::resource& source, const nmos::resource& flow, const nmos::resource& sender, const nmos::resource& connection_sender, const web::json::array& constraint_sets)> streamcompatibility_sender_validator;

            // Receiver validation with its transport file, returns receiver's "state" and "debug" values
            // It is used by the stream compatibility behaviour thread
            // It may throw exception, which will be logged
            typedef std::function<std::pair<nmos::receiver_state, utility::string_t>(const nmos::resource& receiver, const web::json::value& transport_file)> streamcompatibility_receiver_validator;

            // Check the specified receiver against the specified transport file
            typedef std::function<void(const nmos::resource& receiver, const utility::string_t& transportfile_type, const utility::string_t& transportfile_data)> transport_file_validator;

            // Check the specified resource against the specified constraint sets
            typedef std::function<bool(const nmos::resource& resource, const web::json::value& constraint_set)> resource_constraints_matcher;

            // Check the specified transport file against the specified constraint sets
            typedef std::function<bool(const std::pair<utility::string_t, utility::string_t>& transport_file, const web::json::array& constraint_sets)> transport_file_constraint_sets_matcher;

            // Check the specified sdp parameters against the specified constraint sets
            typedef std::function<bool(const web::json::array& constraint_sets, const sdp_parameters& sdp_params)> sdp_constraint_sets_matcher;

            // Validate the specified transport file for the specified receiver using the specified validator
            void validate_rtp_transport_file(nmos::details::sdp_parameters_validator validate_sdp_parameters, const nmos::resource& receiver, const utility::string_t& transport_file_type, const utility::string_t& transport_file_data);
        }

        // Validate the specified transport file for the specified receiver using the default validator
        // (this is the default transport file validator)
        void validate_rtp_transport_file(const nmos::resource& receiver, const utility::string_t& transport_file_type, const utility::string_t& transport_file_data);

        typedef std::map<utility::string_t, std::function<bool(const web::json::value& resource, const web::json::value& constraint)>> parameter_constraints;

        // NMOS Parameter Registers - Capabilities register
        // See https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md

        const parameter_constraints source_parameter_constraints
        {
            // Audio Constraints

            // urn:x-nmos:cap:format:channel_count - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#channel-count
            { nmos::caps::format::channel_count, [](const web::json::value& source, const web::json::value& constraint) { return nmos::match_integer_constraint((uint32_t)nmos::fields::channels(source).size(), constraint); } }
        };

        const parameter_constraints flow_parameter_constraints
        {
            // General Constraints

            // urn:x-nmos:cap:format:media_type - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#media-type
            { nmos::caps::format::media_type, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_string_constraint(nmos::fields::media_type(flow), constraint); } },
            // urn:x-nmos:cap:format:grain_rate - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#grain-rate
            { nmos::caps::format::grain_rate, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_rational_constraint(nmos::parse_rational(nmos::fields::grain_rate(flow)), constraint); } },

            // Video Constraints

            // urn:x-nmos:cap:format:frame_height - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#frame-height
            { nmos::caps::format::frame_height, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_integer_constraint(nmos::fields::frame_height(flow), constraint); } },
            // urn:x-nmos:cap:format:frame_width - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#frame-width
            { nmos::caps::format::frame_width, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_integer_constraint(nmos::fields::frame_width(flow), constraint); } },
            // urn:x-nmos:cap:format:color_sampling - https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#color-sampling
            { nmos::caps::format::color_sampling, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_string_constraint(nmos::details::make_sampling(nmos::fields::components(flow)).name, constraint); } },
            // urn:x-nmos:cap:format:interlace_mode - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#interlace-mode
            { nmos::caps::format::interlace_mode, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_string_constraint(nmos::fields::interlace_mode(flow), constraint); } },
            // urn:x-nmos:cap:format:colorspace - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#colorspace
            { nmos::caps::format::colorspace, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_string_constraint(nmos::fields::colorspace(flow), constraint); } },
            // transfer_characteristic - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#transfer-characteristic
            { nmos::caps::format::transfer_characteristic, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_string_constraint(nmos::fields::transfer_characteristic(flow), constraint); } },
            // urn:x-nmos:cap:format:component_depth - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#component-depth
            { nmos::caps::format::component_depth, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_integer_constraint(nmos::fields::bit_depth(nmos::fields::components(flow).at(0)), constraint); } },

            // Audio Constraints

            // urn:x-nmos:cap:format:sample_rate - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#sample-rate
            { nmos::caps::format::sample_rate, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_rational_constraint(nmos::parse_rational(nmos::fields::sample_rate(flow)), constraint); } },
            // urn:x-nmos:cap:format:sample_depth - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#sample-depth
            { nmos::caps::format::sample_depth, [](const web::json::value& flow, const web::json::value& constraint) { return nmos::match_integer_constraint(nmos::fields::bit_depth(flow), constraint); } }
        };

        const parameter_constraints sender_parameter_constraints
        {
            // urn:x-nmos:cap:transport:st2110_21_sender_type - see https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/capabilities/README.md#st-2110-21-sender-type
            { nmos::caps::transport::st2110_21_sender_type, [](const web::json::value& sender, const web::json::value& constraint) { return nmos::match_string_constraint(nmos::fields::st2110_21_sender_type(sender), constraint); } }
        };

        namespace detail
        {
            bool match_resource_parameters_constraint_set(const parameter_constraints& constraints, const web::json::value& resource, const web::json::value& constraint_set);
        }

        // "At any time if State of an active Sender becomes active_constraints_violation, the Sender MUST become inactive.
        // An inactive Sender in this state MUST NOT allow activations.
        // At any time if State of an active Receiver becomes non_compliant_stream, the Receiver SHOULD become inactive.
        // An inactive Receiver in this state SHOULD NOT allow activations."
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Behaviour_-_Server_Side.html#preventing-restrictions-violation
        nmos::details::connection_resource_patch_validator make_connection_streamcompatibility_validator(nmos::node_model& model);

        details::streamcompatibility_sender_validator make_streamcompatibility_sender_resources_validator(const details::resource_constraints_matcher& resource_matcher, const details::transport_file_constraint_sets_matcher& transport_file_matcher);
        details::streamcompatibility_receiver_validator make_streamcompatibility_receiver_validator(const details::transport_file_validator& validate_transport_file);
        bool match_resource_parameters_constraint_set(const nmos::resource& resource, const web::json::value& constraint_set);
        details::transport_file_constraint_sets_matcher make_streamcompatibility_sdp_constraint_sets_matcher(const details::sdp_constraint_sets_matcher& match_sdp_parameters_constraint_sets);
    }
}

#endif

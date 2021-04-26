#ifndef NMOS_CHANNELMAPPING_API_H
#define NMOS_CHANNELMAPPING_API_H

#include "cpprest/api_router.h"
#include "nmos/channelmapping_resources.h"
#include "nmos/resources.h"

namespace slog
{
    class base_gate;
}

// Channel Mapping API implementation
// See https://github.com/AMWA-TV/nmos-audio-channel-mapping/blob/v1.0.x/APIs/ChannelMappingAPI.raml
namespace nmos
{
    struct node_model;

    // Channel Mapping API callbacks

    namespace details
    {
        // a channelmapping_output_map_validator can be used to perform any final validation of the specified merged output map value for the specified channelmapping_resource
        // that cannot be expressed by the schemas or /caps endpoints
        // it may throw web::json::json_exception, which will be mapped to a 400 Bad Request status code with NMOS error "debug" information including the exception message
        typedef std::function<void(const nmos::resource& channelmapping_output, const web::json::value& output_map, slog::base_gate& gate)> channelmapping_output_map_validator;
    }

    // Channel Mapping API factory functions

    // callbacks from this function are called with the model locked, and may read but should not write directly to the model
    web::http::experimental::listener::api_router make_channelmapping_api(nmos::node_model& model, details::channelmapping_output_map_validator validate_merged, slog::base_gate& gate);

    inline web::http::experimental::listener::api_router make_channelmapping_api(nmos::node_model& model, slog::base_gate& gate)
    {
        return make_channelmapping_api(model, {}, gate);
    }

    // Functions for interaction between the Channel Mapping API implementation and the channel mapping activation thread

    // Activate an IS-08 output by transitioning the 'staged' action into the active map
    void set_channelmapping_output_active(nmos::resource& channelmapping_output, const nmos::tai& activation_time);

    // Clear any pending activation of an IS-08 output
    // (This function should not be called after nmos::set_channelmapping_output_active.)
    void set_channelmapping_output_not_pending(nmos::resource& channelmapping_output);

    // Update the IS-04 source or device after the active map is changed in any way
    // (This function should be called after nmos::set_channelmapping_output_active.)
    void set_resource_version(nmos::resource& node_resource, const nmos::tai& activation_time);

    // Channel Mapping API implementation details
    namespace details
    {
        typedef std::pair<web::http::status_code, web::json::value> channelmapping_activation_post_response;

        channelmapping_activation_post_response validate_output_caps(const nmos::resources& channelmapping_resources, const nmos::resource& output, const web::json::value& output_action);
        channelmapping_activation_post_response validate_output_not_locked(const nmos::resource& output);
        channelmapping_activation_post_response validate_input_caps(const nmos::resources& channelmapping_resources, const nmos::channelmapping_id& output_id, const web::json::value& active_map);
    }
}

#endif

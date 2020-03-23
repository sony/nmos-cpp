#ifndef NMOS_CONNECTION_API_H
#define NMOS_CONNECTION_API_H

#include "cpprest/api_router.h"
#include "nmos/id.h"

namespace slog
{
    class base_gate;
}

// Connection API implementation
// See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/ConnectionAPI.raml
namespace nmos
{
    struct api_version;
    struct node_model;
    struct resource;
    struct tai;
    struct type;

    // Connection API callbacks

    // a transport_file_parser validates the specified transport file type/data for the specified (IS-04/IS-05) resource/connection_resource and returns a transport_params array to be merged
    // or may throw std::runtime_error, which will be mapped to a 500 Internal Error status code with NMOS error "debug" information including the exception message
    // (the default transport file parser, nmos::parse_rtp_transport_file, only supports RTP transport via the default SDP parser)
    typedef std::function<web::json::value(const nmos::resource& resource, const nmos::resource& connection_resource, const utility::string_t& transportfile_type, const utility::string_t& transportfile_data, slog::base_gate& gate)> transport_file_parser;

    namespace details
    {
        // a connection_resource_patch_validator can be used to perform any final validation of the specified merged /staged value for the specified (IS-04/IS-05) resource/connection_resource
        // that cannot be expressed by the schemas or /constraints endpoint
        // it may throw web::json::json_exception, which will be mapped to a 400 Bad Request status code with NMOS error "debug" information including the exception message
        typedef std::function<void(const nmos::resource& resource, const nmos::resource& connection_resource, const web::json::value& endpoint_staged, slog::base_gate& gate)> connection_resource_patch_validator;
    }

    // Connection API factory functions

    // callbacks from this function are called with the model locked, and may read but should not write directly to the model
    web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, transport_file_parser parse_transport_file, details::connection_resource_patch_validator validate_merged, slog::base_gate& gate);

    inline web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, transport_file_parser parse_transport_file, slog::base_gate& gate)
    {
        return make_connection_api(model, std::move(parse_transport_file), {}, gate);
    }

    web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, slog::base_gate& gate);

    // Connection API implementation details shared with the Node API /receivers/{receiverId}/target endpoint
    // and experimental Manifest API for Node API "manifest_href"
    namespace details
    {
        void handle_connection_resource_patch(web::http::http_response res, nmos::node_model& model, const nmos::api_version& version, const std::pair<nmos::id, nmos::type>& id_type, const web::json::value& patch, transport_file_parser parse_transport_file, connection_resource_patch_validator validate_merged, slog::base_gate& gate);
        void handle_connection_resource_transportfile(web::http::http_response res, const nmos::node_model& model, const nmos::api_version& version, const std::pair<nmos::id, nmos::type>& id_type, const utility::string_t& accept, slog::base_gate& gate);
    }

    // Functions for interaction between the Connection API implementation and the connection activation thread

    // Activate an IS-05 sender or receiver by transitioning the 'staged' settings into the 'active' resource
    void set_connection_resource_active(nmos::resource& connection_resource, std::function<void(web::json::value& endpoint_active)> resolve_auto, const nmos::tai& activation_time);

    // Clear any pending activation of an IS-05 sender or receiver
    // (This function should not be called after nmos::set_connection_resource_active.)
    void set_connection_resource_not_pending(nmos::resource& connection_resource);

    // Update the IS-04 sender or receiver after the active connection is changed in any way
    // (This function should be called after nmos::set_connection_resource_active.)
    void set_resource_subscription(nmos::resource& node_resource, bool active, const nmos::id& connected_id, const nmos::tai& activation_time);

    // Helper functions for the Connection API callbacks

    // Validate and parse the specified transport file for the specified receiver
    // (this is the default transport file parser)
    web::json::value parse_rtp_transport_file(const nmos::resource& receiver, const nmos::resource& connection_receiver, const utility::string_t& transport_file_type, const utility::string_t& transport_file_data, slog::base_gate& gate);

    // "On activation all instances of "auto" must be resolved into the actual values that will be used by the sender, unless there is an error condition."
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/ConnectionAPI.raml#L300-L301
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_rtp.json
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_rtp.json
    // "In many cases this is a simple operation, and the behaviour is very clearly defined in the relevant transport parameter schemas.
    // For example a port number may be offset from the RTP port number by a pre-determined value. The specification makes suggestions
    // of a sensible default value for "auto" to resolve to, but the Sender or Receiver may choose any value permitted by the schema
    // and constraints."
    // This function implements those sensible defaults for the RTP transport type.
    // "In some cases the behaviour is more complex, and may be determined by the vendor."
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#use-of-auto
    // This function therefore does not select a value for e.g. sender "source_ip" or receiver "interface_ip".
    void resolve_rtp_auto(const nmos::type& type, web::json::value& transport_params, int auto_rtp_port = 5004);

    namespace details
    {
        template <typename AutoFun>
        inline void resolve_auto(web::json::value& params, const utility::string_t& key, AutoFun auto_fun)
        {
            if (!params.has_field(key)) return;
            auto& param = params.at(key);
            if (param.is_string() && U("auto") == param.as_string())
            {
                param = auto_fun();
            }
        }
    }
}

#endif

#ifndef NMOS_CONNECTION_API_H
#define NMOS_CONNECTION_API_H

#include "cpprest/api_router.h"
#include "nmos/id.h"

namespace slog
{
    class base_gate;
}

// Connection API implementation
// See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml
namespace nmos
{
    struct api_version;
    struct node_model;
    struct resource;
    struct tai;
    struct type;

    web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, slog::base_gate& gate);

    namespace details
    {
        void handle_connection_resource_patch(web::http::http_response res, nmos::node_model& model, const nmos::api_version& version, const std::pair<nmos::id, nmos::type>& id_type, const web::json::value& patch, slog::base_gate& gate);
    }

    // Activate an IS-05 sender or receiver by transitioning the 'staged' settings into the 'active' resource
    void set_connection_resource_active(nmos::resource& connection_resource, std::function<void(web::json::value&)> resolve_auto, const nmos::tai& activation_time);

    // Clear any pending activation of an IS-05 sender or receiver
    // (This function should not be called after nmos::set_connection_resource_active.)
    void set_connection_resource_not_pending(nmos::resource& connection_resource);

    // Update the IS-04 sender or receiver after the active connection is changed in any way
    // (This function should be called after nmos::set_connection_resource_active.)
    void set_resource_subscription(nmos::resource& node_resource, bool active, const nmos::id& connected_id, const nmos::tai& activation_time);

    // Validate and parse the specified transport file for the specified receiver
    web::json::value parse_rtp_transport_file(const nmos::resource& receiver, const nmos::resource& connection_receiver, const utility::string_t& transport_file_type, const utility::string_t& transport_file_data, slog::base_gate& gate);

    // "On activation all instances of "auto" should be resolved into the actual values that will be used"
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/APIs/ConnectionAPI.raml#L280-L281
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/APIs/schemas/sender_transport_params_rtp.json
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/APIs/schemas/receiver_transport_params_rtp.json
    // "In many cases this is a simple operation, and the behaviour is very clearly defined in the relevant transport parameter schemas.
    // For example a port number may be offset from the RTP port number by a pre-determined value. The specification makes suggestions
    // of a sensible default value for "auto" to resolve to, but the Sender or Receiver may choose any value permitted by the schema
    // and constraints."
    // This function implements those sensible defaults for the RTP transport type.
    // "In some cases the behaviour is more complex, and may be determined by the vendor."
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
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

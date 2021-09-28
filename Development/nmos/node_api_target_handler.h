#ifndef NMOS_NODE_API_TARGET_HANDLER_H
#define NMOS_NODE_API_TARGET_HANDLER_H

#include "nmos/certificate_handlers.h"
#include "nmos/connection_api.h"

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace nmos
{
    struct node_model;

    // handler for the Node API /receivers/{receiverId}/target endpoint
    typedef std::function<pplx::task<void>(const nmos::id& receiver_id, const web::json::value& sender_data, slog::base_gate& gate)> node_api_target_handler;

    // implement the Node API /receivers/{receiverId}/target endpoint using the Connection API implementation with the specified handlers
    // (the /target endpoint is only required to support RTP transport, other transport types use the Connection API)
    node_api_target_handler make_node_api_target_handler(nmos::node_model& model, load_ca_certificates_handler load_ca_certificates, transport_file_parser parse_transport_file, details::connection_resource_patch_validator validate_merged);

    inline node_api_target_handler make_node_api_target_handler(nmos::node_model& model, transport_file_parser parse_transport_file, details::connection_resource_patch_validator validate_merged)
    {
        return make_node_api_target_handler(model, {}, std::move(parse_transport_file), std::move(validate_merged));
    }

    inline node_api_target_handler make_node_api_target_handler(nmos::node_model& model, transport_file_parser parse_transport_file)
    {
        return make_node_api_target_handler(model, std::move(parse_transport_file), {});
    }

    node_api_target_handler make_node_api_target_handler(nmos::node_model& model);
}

#endif

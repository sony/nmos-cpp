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

    // implement the Node API /receivers/{receiverId}/target endpoint using the Connection API implementation with the specified transport file parser and the specified validator
    // (the /target endpoint is only required to support RTP transport, other transport types use the Connection API)
    node_api_target_handler make_node_api_target_handler(nmos::node_model& model, load_cacerts_handler load_cacerts, transport_file_parser parse_transport_file, details::connection_resource_patch_validator validate_merged);
}

#endif

#ifndef NMOS_NODE_API_H
#define NMOS_NODE_API_H

#include "cpprest/api_router.h"
#include "nmos/node_api_target_handler.h"

// Node API implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/NodeAPI.raml
namespace nmos
{
    struct model;

    web::http::experimental::listener::api_router make_node_api(const nmos::model& model, node_api_target_handler target_handler, slog::base_gate& gate);
}

#endif

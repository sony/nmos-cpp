#ifndef NMOS_NODE_API_H
#define NMOS_NODE_API_H

#include "cpprest/api_router.h"
#include "nmos/node_api_target_handler.h"

// Node API implementation
// See https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/NodeAPI.html
namespace nmos
{
    struct model;

    web::http::experimental::listener::api_router make_node_api(const nmos::model& model, node_api_target_handler target_handler, slog::base_gate& gate);
}

#endif

#ifndef NMOS_NODE_API_H
#define NMOS_NODE_API_H

#include "cpprest/api_router.h"
#include "nmos/node_api_target_handler.h"

// Node API implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/NodeAPI.raml
namespace nmos
{
    web::http::experimental::listener::api_router make_node_api(const nmos::resources& resources, nmos::mutex& mutex, node_api_target_handler target_handler, slog::base_gate& gate);
}

#endif

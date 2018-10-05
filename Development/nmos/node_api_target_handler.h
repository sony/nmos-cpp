#ifndef NMOS_NODE_API_TARGET_HANDLER_H
#define NMOS_NODE_API_TARGET_HANDLER_H

#include <functional>
#include "pplx/pplxtasks.h"
#include "nmos/id.h"

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;

    // handler for the Node API /receivers/{receiverId}/target endpoint
    typedef std::function<pplx::task<void>(const nmos::id&, const web::json::value&)> node_api_target_handler;

    node_api_target_handler make_node_api_target_handler(nmos::node_model& model, slog::base_gate& gate);
}

#endif

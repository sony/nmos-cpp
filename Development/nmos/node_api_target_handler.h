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
    struct model;

    // handler for the Node API /receivers/{receiverId}/target endpoiint
    typedef std::function<pplx::task<void>(const nmos::id&, const web::json::value&)> node_api_target_handler;

    // function to connect or disconnect (when sdp is empty) the specified receiver
    typedef std::function<pplx::task<void>(const nmos::id& receiver_id, const utility::string_t& sdp)> connect_function;

    node_api_target_handler make_node_api_target_handler(nmos::model& model, connect_function connect, slog::base_gate& gate);
}

#endif

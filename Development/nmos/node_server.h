#ifndef NMOS_NODE_SERVER_H
#define NMOS_NODE_SERVER_H

#include "nmos/connection_api.h"
#include "nmos/connection_activation.h"

namespace nmos
{
    struct node_model;

    struct server;

    namespace experimental
    {
        struct log_model;

        // Construct a server instance for an NMOS Node, implementing the IS-04 Node API, IS-05 Connection API, IS-07 Events API
        // and the experimental Logging API and Settings API, according to the specified data models and callbacks
        nmos::server make_node_server(nmos::node_model& node_model, nmos::transport_file_parser parse_transport_file, nmos::details::connection_resource_patch_validator validate_merged, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::experimental::log_model& log_model, slog::base_gate& gate);
        nmos::server make_node_server(nmos::node_model& node_model, nmos::transport_file_parser parse_transport_file, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::experimental::log_model& log_model, slog::base_gate& gate);
        nmos::server make_node_server(nmos::node_model& node_model, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::experimental::log_model& log_model, slog::base_gate& gate);
    }
}

#endif

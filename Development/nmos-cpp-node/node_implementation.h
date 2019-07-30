#ifndef NMOS_CPP_NODE_NODE_IMPLEMENTATION_H
#define NMOS_CPP_NODE_NODE_IMPLEMENTATION_H

#include "nmos/connection_activation.h"
#include "nmos/connection_api.h"
#include "nmos/events_ws_client.h" // just a forward declaration of nmos::events_ws_message_handler
#include "nmos/resources.h" // just a forward declaration of nmos::resources
#include "nmos/settings.h" // just a forward declaration of nmos::settings

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;
}

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// starts background tasks to emit regular events from the temperature event source and then waits for shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate);

// Example Connection API callback to parse "transport_file" during a PATCH /staged request
nmos::transport_file_parser make_node_implementation_transport_file_parser();

// Example Connection API callback to to perform application-specific validation of the merged /staged endpoint during a PATCH /staged request
nmos::details::connection_resource_patch_validator make_node_implementation_patch_validator();

// Example Connection API activation callback to resolve "auto" values when /staged is transitioned to /active
nmos::connection_resource_auto_resolver make_node_implementation_auto_resolver(const nmos::settings& settings);

// Example Connection API activation callback to update senders' /transportfile endpoint - captures node_resources by reference!
nmos::connection_sender_transportfile_setter make_node_implementation_transportfile_setter(const nmos::resources& node_resources, const nmos::settings& settings);

// Example Connection API activation callback to perform application-specific operations to complete activation
nmos::connection_activation_handler make_node_implementation_activation_handler(nmos::node_model& model, slog::base_gate& gate);

// Example Events WebSocket API client message handler
nmos::events_ws_message_handler make_node_implementation_events_ws_message_handler(const nmos::node_model& model, slog::base_gate& gate);

#endif

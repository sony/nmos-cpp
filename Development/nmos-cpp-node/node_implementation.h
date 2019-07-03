#ifndef NMOS_CPP_NODE_NODE_IMPLEMENTATION_H
#define NMOS_CPP_NODE_NODE_IMPLEMENTATION_H

#include "nmos/connection_activation.h"
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

// Example connection activation callback
nmos::connection_resource_auto_resolver make_node_implementation_auto_resolver(const nmos::settings& settings);

// Example connection activation callback - captures node_resources by reference!
nmos::connection_sender_transportfile_setter make_node_implementation_transportfile_setter(const nmos::resources& node_resources, const nmos::settings& settings);

#endif

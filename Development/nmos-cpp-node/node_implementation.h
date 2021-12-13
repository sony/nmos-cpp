#ifndef NMOS_CPP_NODE_NODE_IMPLEMENTATION_H
#define NMOS_CPP_NODE_NODE_IMPLEMENTATION_H

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;

    namespace experimental
    {
        struct node_implementation;
    }
}

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// starts background tasks to emit regular events from the temperature event source, and then waits for shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate);

// This constructs all the callbacks used to integrate the example device-specific underlying implementation
// into the server instance for the NMOS Node.
nmos::experimental::node_implementation make_node_implementation(nmos::node_model& model, slog::base_gate& gate);

#endif

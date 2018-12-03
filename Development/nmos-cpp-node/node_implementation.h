#ifndef NMOS_CPP_NODE_NODE_IMPLEMENTATION_H
#define NMOS_CPP_NODE_NODE_IMPLEMENTATION_H

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
// and then waits for sender/receiver activations or shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate);

#endif

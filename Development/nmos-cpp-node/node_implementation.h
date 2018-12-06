#ifndef NMOS_CPP_NODE_NODE_IMPLEMENTATION_H
#define NMOS_CPP_NODE_NODE_IMPLEMENTATION_H

#include "nmos/id.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;
}

// This function exists so that the main() does not have to be included and collide with the apps main()
int node_main_thread(int argc, char* argv[]);

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// and then waits for sender/receiver activations or shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate);

// sample implementation function to create initial resources
void node_initial_resources(nmos::node_model& model, const utility::string_t& seed_id, slog::base_gate& gate, nmos::id node_id = nmos::id());

#endif

#ifndef NMOS_NODE_RESOURCES_THREAD_H
#define NMOS_NODE_RESOURCES_THREAD_H

namespace slog
{
    class base_gate;
}

// This currently serves as an example of the resources that an NMOS node would construct
// and how to handle sender/receiver activations
namespace nmos
{
    struct node_model;

    namespace experimental
    {
        // insert a node resource, and sub-resources, according to the settings, and then wait for shutdown
        void node_resources_thread(nmos::node_model& model, slog::base_gate& gate);
    }
}

#endif

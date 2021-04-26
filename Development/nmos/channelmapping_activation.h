#ifndef NMOS_CHANNELMAPPING_ACTIVATION_H
#define NMOS_CHANNELMAPPING_ACTIVATION_H

#include <functional>

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;
    struct resource;

    // a channelmapping_activation_handler is a notification that the active map for the specified IS-08 output has changed
    // this callback should not throw exceptions, as the active map will already have been changed and those changes will not be rolled back
    typedef std::function<void(const nmos::resource& channelmapping_output)> channelmapping_activation_handler;

    // callbacks from this function are called with the model locked, and may read but should not write directly to the model
    void channelmapping_activation_thread(nmos::node_model& model, channelmapping_activation_handler channelmapping_activated, slog::base_gate& gate);
}

#endif

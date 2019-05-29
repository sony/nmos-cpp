#ifndef NMOS_CPP_NODE_NODE_IMPLEMENTATION_H
#define NMOS_CPP_NODE_NODE_IMPLEMENTATION_H

#include "nmos/id.h"
#include "nmos/resource.h"
#include "nmos/sdp_utils.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;
    
    namespace experimental
    {
        // a set of call-back functions into the application. 
        struct app_hooks
        {
            // Function to initialize the application providing the model and gate.
            // typically only called once
            // Initial resources can be created here
            std::function<void(node_model& model, slog::base_gate& gate)> initialize;

            // Function to check if the application has "work" to do
            // Work can be the creation, deletion, or modification of various resources
            // Returns true to indicate that there is work to be done.
            std::function<bool()> check_for_work;

            // Function to perform the work that the application has queued up
            // Work can be the creation, deletion, or modification of various resources
            // The app only changes resources within the context of these callbacks
            // Returns true if model.notify() should be called
            std::function<bool()> process_work;

            // Function to tell the app about a resource activation
            // The app may need to do its own processing with respect to making and breaking connections
            // or changing resources
            // Returns true if model.notify() should be called
            std::function<bool(const resource& resource)> resource_activation;

            // Function to retrieve the base_sdp_params for a particular sender
            // Called when senders are being activated
            // The app can either retrieve the params saved statically at init time or get
            // the params more dynamically if the params could actually change in the app
            std::function<sdp_parameters(const nmos::resource& resource)> get_base_sdp_params;

            // Function to resolve the use of "auto" in various transport params to real values in
            // both senders and receivers.
            // The function could call get_defaults_for_autos which can either retrieve static or dynamic values
            std::function<void(resource& connection_resource, const resource& node_resource, web::json::value& endpoint_active)>  resolve_auto;
        };
    }
}

// This function exists so that the main() does not have to be included and collide with the apps main()
int node_main_thread(int argc, char* argv[], const nmos::experimental::app_hooks& app_hooks);

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// and then waits for sender/receiver activations or shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate, const nmos::experimental::app_hooks& app_hooks);

// as part of activation, the sender /transportfile should be updated based on the active transport parameters
void node_set_connection_sender_transportfile(nmos::resource& connection_sender, const nmos::sdp_parameters& sdp_params);



#endif

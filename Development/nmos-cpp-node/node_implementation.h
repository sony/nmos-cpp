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
            std::function<bool(const id& resource_id)> resource_activation;
#if NEVER
            // Function to set the base_sdp_params for a particular sender
            // If the app has more than one sender, this function may be needed
            // The function is only called when creating resources. If the app has some
            // other way to save the base_sdp_params, then this function does not need to be called
            std::function<void(const nmos::resource& resource, const nmos::sdp_parameters& sdp_params)>  set_base_sdp_params;
#endif
            // Function to retrieve the base_sdp_params for a particular sender
            // Called when senders are being activated
            // The app can either retrieve the params saved statically at init time or get
            // the params more dynamically if the params could actually change in the app
            std::function<sdp_parameters(const nmos::resource& resource)> get_base_sdp_params;

            // Function to resolve the use of "auto" in various transport params to real values in
            // both senders and receivers.
            // The function could call get_defaults_for_autos which can either retrieve static or dynamic values
            std::function<void(resource& connection_resource, web::json::value& endpoint_active, const app_hooks& app_hooks)>  resolve_auto;
#if NEVER
            // Function to save the default values for "auto" resolution
            // the value is an array of values. typically an array of 1 or 2 elements
            // Each element of the array has different values saved depending on whether the resource is
            // a sender or receiver. 
            // Sender: source_ip and destination_ip
            // Receiver: interface_ip
            std::function<void(const nmos::id& resource_id, const web::json::value& defaults_for_autos)> set_defaults_for_autos;
#endif
            // Function to retrieve the default values for "auto" resolution for a sender or receiver
            // The app can get previously saved values or get values based on dynamic conditions.
            std::function<web::json::value(const nmos::id& resource_id)> get_defaults_for_autos;
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

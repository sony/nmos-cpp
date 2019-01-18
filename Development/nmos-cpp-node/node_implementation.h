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
        struct app_hooks
        {
            std::function<bool(node_model& model, slog::base_gate& gate)>               initialize;
            std::function<bool()>                                                       check_for_work;
            std::function<bool()>                                                       process_work;
            std::function<bool(const id& resource_id)>                                  resource_activation;

            std::function<void(const nmos::resource& resource, const nmos::sdp_parameters& sdp_params)>  set_base_sdp_params;
            std::function<sdp_parameters(const nmos::resource& resource)>                              get_base_sdp_params;

            std::function<void(resource& connection_resource, web::json::value& endpoint_active, const app_hooks& app_hooks)>  resolve_auto;
            
            std::function<void(const nmos::id& resource_id, const web::json::value& defaults_for_autos)>    set_defaults_for_autos;
            std::function<web::json::value(const nmos::id& resource_id)>                                    get_defaults_for_autos;
        };
    }
}

// This function exists so that the main() does not have to be included and collide with the apps main()
int node_main_thread(int argc, char* argv[], const nmos::experimental::app_hooks& app_hooks);

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// and then waits for sender/receiver activations or shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate, const nmos::experimental::app_hooks& app_hooks);

// sample implementation function to create initial resources
// a map from sender_id to base sdp_parameters is returned
void node_initial_resources(nmos::node_model& model, slog::base_gate& gate, 
    const nmos::experimental::app_hooks& app_hooks,
    nmos::id node_id = nmos::id());

#endif

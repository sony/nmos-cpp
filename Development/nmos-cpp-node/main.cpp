#include <fstream>
#include <iostream>
#include "cpprest/host_utils.h"
#include "nmos/admin_ui.h"
#include "nmos/api_utils.h"
#include "nmos/connection_api.h"
#include "nmos/logging_api.h"
#include "nmos/model.h"
#include "nmos/node_api.h"
#include "nmos/node_behaviour.h"
#include "nmos/node_resources.h"
#include "nmos/process_utils.h"
#include "nmos/settings_api.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h"
#include "main_gate.h"
#include "node_implementation.h"

int main(int argc, char* argv[])
{
    std::map<nmos::id, nmos::sdp_parameters> map_sender_sdp_params;
    std::map<nmos::id, web::json::value> map_resource_id_default_autos;
    
    nmos::experimental::app_hooks app_hooks = 
    {
        // app initialize
        [&](nmos::node_model& model, slog::base_gate& gate) -> bool
        {
            ::node_initial_resources (model, gate, app_hooks);
            return false;
        },
        // app check_for_work
        []() -> bool
        {
            // By default, there is no work
            return false;
        },
        // app process_work
        []() -> bool
        {
            // By default, notify does not need to be set
            return false;
        },
        // app resource_activation
        [](const nmos::id& resource_id) -> bool
        {
            // By default, notify does not need to be set
            return false;
        },
        // set_base_sdp_params
        [&](const nmos::resource& resource, const nmos::sdp_parameters& sdp_params) -> void
        {
            // For senders, use the returned map of sdp_params to retrieve the base sdp
            if (resource.type == nmos::types::sender)
            {
                map_sender_sdp_params[resource.id] = sdp_params;
            }
            return;
        },
        // get_base_sdp_params
        [&](const nmos::resource& resource) -> nmos::sdp_parameters
        {
            // For senders, use the returned map of sdp_params to retrieve the base sdp
            if (resource.type == nmos::types::sender)
            {
                if (map_sender_sdp_params.count(resource.id))
                {
                    return map_sender_sdp_params[resource.id];
                }
            }
            return nmos::sdp_parameters();
        },
        // resolve_auto
        [](nmos::resource& connection_resource, web::json::value& endpoint_active, const nmos::experimental::app_hooks& app_hooks) -> void
        {
            auto type = connection_resource.type;

            auto& transport_params = endpoint_active[nmos::fields::transport_params];
            
            web::json::value default_autos = app_hooks.get_defaults_for_autos (connection_resource.id);

            if (default_autos.is_null() == false)
            {
                // "In some cases the behaviour is more complex, and may be determined by the vendor."
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
                if (nmos::types::sender == type)
                {
                    nmos::details::resolve_auto(transport_params[0], nmos::fields::source_ip, [&default_autos] { return default_autos[0][nmos::fields::source_ip]; });
                    nmos::details::resolve_auto(transport_params[0], nmos::fields::destination_ip, [&default_autos] { return default_autos[0][nmos::fields::destination_ip]; });
                    if (transport_params.size() > 1)
                    {
                        nmos::details::resolve_auto(transport_params[1], nmos::fields::source_ip, [&default_autos] { return default_autos[1][nmos::fields::source_ip]; });
                        nmos::details::resolve_auto(transport_params[1], nmos::fields::destination_ip, [&default_autos] { return default_autos[1][nmos::fields::destination_ip]; });
                    }
                }
                else // if (nmos::types::receiver == type)
                {
                    nmos::details::resolve_auto(transport_params[0], nmos::fields::interface_ip, [&default_autos] { return default_autos[0][nmos::fields::interface_ip]; });
                    if (transport_params.size() > 1)
                    {
                        nmos::details::resolve_auto(transport_params[1], nmos::fields::interface_ip, [&default_autos] { return default_autos[1][nmos::fields::interface_ip]; });
                    }
                }
            }

            nmos::resolve_auto(type, transport_params);
        },
        // set_defaults_for_autos
        [&map_resource_id_default_autos](const nmos::id& resource_id, const web::json::value& defaults_for_autos) -> void
        {
            map_resource_id_default_autos[resource_id] = defaults_for_autos;
        },
        // get_defaults_for_autos
        [&map_resource_id_default_autos](const nmos::id& resource_id) -> web::json::value
        {
            if (map_resource_id_default_autos.count(resource_id))
            {
                return map_resource_id_default_autos[resource_id];
            }
            return web::json::value();
        }
    };
    // Call the main thread with default functions for all the application hooks
    return node_main_thread(argc, argv, app_hooks);
        

}

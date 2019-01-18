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
#include "nmos/transport.h"
#include "nmos/channels.h"
#include "nmos/node_resource.h"
#include "nmos/group_hint.h"

// Sample implementation function to create initial resources
// Can be called with an already made node_id if the app is generating its own
void node_initial_resources(nmos::node_model& model, slog::base_gate& gate,
    const nmos::experimental::app_hooks& app_hooks,
    std::function<void(const nmos::id& resource_id, const web::json::value& defaults_for_autos)> set_defaults_for_autos,
    std::function<void(const nmos::resource& resource, const nmos::sdp_parameters& sdp_params)>  set_base_sdp_params,
    nmos::id node_id = nmos::id());

int main(int argc, char* argv[])
{
    std::map<nmos::id, nmos::sdp_parameters> map_sender_sdp_params;
    std::map<nmos::id, web::json::value> map_resource_id_default_autos;
    
    nmos::experimental::app_hooks app_hooks = 
    {
        // app initialize
        // save model and gate if needed
        // create initial resources or wait to add resources dynamically
        [&](nmos::node_model& model, slog::base_gate& gate) -> void
        {
            ::node_initial_resources (model, gate, app_hooks,
                // set_defaults_for_autos
                // Function to save the default "auto" values for a resource
                // This function is called from node_initial_resources for the sample app but may not be called
                // for a real app
                [&map_resource_id_default_autos](const nmos::id& resource_id, const web::json::value& defaults_for_autos) -> void
                {
                    map_resource_id_default_autos[resource_id] = defaults_for_autos;
                },
                // set_base_sdp_params
                // Function to save base_sdp_params for senders
                // Called when resources are being created
                // In this sample app, save the params in a map for later retrieval
                [&map_sender_sdp_params](const nmos::resource& resource, const nmos::sdp_parameters& sdp_params) -> void
                {
                    // For senders, use the returned map of sdp_params to retrieve the base sdp
                    if (resource.type == nmos::types::sender)
                    {
                        map_sender_sdp_params[resource.id] = sdp_params;
                    }
                    return;
                });

        },
        // app check_for_work
        // if there is work to do, return true
        []() -> bool
        {
            // In this sample app, there is no work
            return false;
        },
        // app process_work
        // Process any work that needs to be done
        // Return true if model.notify() needs to be called
        // This function could be called even when check_for_work has returned false
        // so don't assume there is work to do
        []() -> bool
        {
            // In this sample app, there is no work and notify does not need to be set
            return false;
        },
        // app resource_activation
        // Deal with a resource which has been activated
        // This could involve making or breaking connections or modifying app resources.
        // Return true if model.notify() needs to be called (i.e. the app makes resource changes)
        [](const nmos::id& resource_id) -> bool
        {
            // In this sample app, notify does not need to be set
            return false;
        },
#if NEVER
        // set_base_sdp_params
        // Function to save base_sdp_params for senders
        // Called when resources are being created
        // In this sample app, save the params in a map for later retrieval
        [&](const nmos::resource& resource, const nmos::sdp_parameters& sdp_params) -> void
        {
            // For senders, use the returned map of sdp_params to retrieve the base sdp
            if (resource.type == nmos::types::sender)
            {
                map_sender_sdp_params[resource.id] = sdp_params;
            }
            return;
        },
#endif
        // get_base_sdp_params
        // Function to get the base_sdp_params for senders
        // Called when the base_sdp_params for senders are needed (during activation)
        // In this sample app, just retrieve the params saved in a map
        // In a real app, getting the params may be dynamic
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
        // Function to resolve the "auto" values in the transport params.
        // The "auto" values need to be turned into real values
        // In the sample app, when senders/resources are created, the "real" values are
        // stored in a map and a call to "get_defaults_for_autos" retrieves those saved values
        // A real app may get these values in a different way that allows resources to change
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
#if NEVER
        // set_defaults_for_autos
        // Function to save the default "auto" values for a resource
        // This function is called from node_initial_resources for the sample app but may not be called
        // for a real app
        [&map_resource_id_default_autos](const nmos::id& resource_id, const web::json::value& defaults_for_autos) -> void
        {
            map_resource_id_default_autos[resource_id] = defaults_for_autos;
        },
#endif
        // get_defaults_for_autos
        // Function to retrieve the default "auto" values for a resource
        // This function is called from the app_hook.resolve_auto
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


// sample function which creates initial video and audio senders and receivers
void node_initial_resources(nmos::node_model& model, slog::base_gate& gate,
    const nmos::experimental::app_hooks& app_hooks,
    std::function<void(const nmos::id& resource_id, const web::json::value& defaults_for_autos)> set_defaults_for_autos,
    std::function<void(const nmos::resource& resource, const nmos::sdp_parameters& sdp_params)>  set_base_sdp_params,
    nmos::id existing_node_id)
{
    using web::json::value;
    using web::json::value_of;

    const auto seed_id = nmos::with_read_lock(model.mutex, [&] { return nmos::experimental::fields::seed_id(model.settings); });

    auto node_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/self"));
    auto device_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/device/0"));
    auto source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/0"));
    auto flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/0"));
    auto sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/0"));
    auto receiver_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/receiver/0"));

    auto audio_source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/audio_0"));
    auto audio_flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/audio_0"));
    auto audio_sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/audio_0"));
    auto audio_receiver_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/receiver/audio_0"));

    auto lock = model.write_lock(); // in order to update the resources

    const auto insert_resource_after = [&model, &lock](unsigned int milliseconds, nmos::resources& resources, nmos::resource resource, slog::base_gate& gate)
    {
        if (!nmos::details::wait_for(model.shutdown_condition, lock, std::chrono::milliseconds(milliseconds), [&] { return model.shutdown; }))
        {
            const std::pair<nmos::id, nmos::type> id_type{ resource.id, resource.type };
            const bool success = insert_resource(resources, std::move(resource)).second;

            if (success)
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Updated model with " << id_type;
            else
                slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Model update error: " << id_type;

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying node behaviour thread"; // and anyone else who cares...
            model.notify();
        }
    };

    // any delay between updates to the model resources is unnecessary
    // this just serves as a slightly more realistic example!
    const unsigned int delay_millis{ 10 };

    // make example node or use node_id as provided by caller
    if (existing_node_id.size() == 0)
    {
        auto node = nmos::make_node(node_id, model.settings);
        // add one example network interface
        node.data[U("interfaces")] = value_of({ value_of({ { U("chassis_id"), value::null() },{ U("port_id"), U("ff-ff-ff-ff-ff-ff") },{ U("name"), U("example") } }) });
        insert_resource_after(delay_millis, model.node_resources, std::move(node), gate);
    }
    else
    {
        node_id = existing_node_id;
    }

    // example device
    insert_resource_after(delay_millis, model.node_resources, nmos::make_device(device_id, node_id, { sender_id, audio_sender_id }, { receiver_id, audio_receiver_id }, model.settings), gate);

    // example video source, flow and sender
    {
        auto source = nmos::make_video_source(source_id, device_id, { 25, 1 }, model.settings);

        auto flow = nmos::make_raw_video_flow(flow_id, source_id, device_id, model.settings);
        // add example network interface binding for both primary and secondary

        auto sender = nmos::make_sender(sender_id, flow_id, device_id, { U("example"), U("example") }, model.settings);
        // add example "natural grouping" hint
        web::json::push_back(sender.data[U("tags")][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), U("sender 0") }));

        nmos::sdp_parameters sdp_params = nmos::make_sdp_parameters(source.data, flow.data, sender.data, { U("PRIMARY"), U("SECONDARY") });

        std::vector<value> vec_auto_defaults;
        vec_auto_defaults.push_back(value_of({
            { nmos::fields::source_ip, value::string(U("192.168.240.1")) },
            { nmos::fields::destination_ip, value::string(U("239.255.240.1")) }
            }));
        vec_auto_defaults.push_back(value_of({
            { nmos::fields::source_ip, value::string(U("192.168.240.2")) },
            { nmos::fields::destination_ip, value::string(U("239.255.240.2")) }
            }));

        auto sender_auto_defaults = value::array(vec_auto_defaults);

        set_defaults_for_autos (sender_id, sender_auto_defaults);

        auto connection_sender = nmos::make_connection_sender(sender_id, true);
        app_hooks.resolve_auto(connection_sender, connection_sender.data[nmos::fields::endpoint_active], app_hooks);
        node_set_connection_sender_transportfile(connection_sender, sdp_params);

        set_base_sdp_params (connection_sender, sdp_params);

        insert_resource_after(delay_millis, model.node_resources, std::move(source), gate);
        insert_resource_after(delay_millis, model.node_resources, std::move(flow), gate);
        insert_resource_after(delay_millis, model.node_resources, std::move(sender), gate);
        insert_resource_after(delay_millis, model.connection_resources, std::move(connection_sender), gate);
    }

    // example video receiver
    {
        // add example network interface binding for both primary and secondary
        auto receiver = nmos::make_video_receiver(receiver_id, device_id, nmos::transports::rtp_mcast, { U("example"), U("example") }, model.settings);
        // add example "natural grouping" hint
        web::json::push_back(receiver.data[U("tags")][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), U("receiver 0") }));

        std::vector<value> vec_auto_defaults;
        vec_auto_defaults.push_back(value_of({
            { nmos::fields::interface_ip, value::string(U("192.168.242.1")) }
            }));
        vec_auto_defaults.push_back(value_of({
            { nmos::fields::interface_ip, value::string(U("192.168.240.2")) }
            }));

        auto receiver_auto_defaults = value::array(vec_auto_defaults);
        set_defaults_for_autos (receiver_id, receiver_auto_defaults);

        auto connection_receiver = nmos::make_connection_receiver(receiver_id, true);
        app_hooks.resolve_auto(connection_receiver, connection_receiver.data[nmos::fields::endpoint_active], app_hooks);

        insert_resource_after(delay_millis, model.node_resources, std::move(receiver), gate);
        insert_resource_after(delay_millis, model.connection_resources, std::move(connection_receiver), gate);
    }


    // example audio source, flow and sender
    {
        nmos::rational sample_rate = { 48000, 1 };
        std::vector<nmos::channel> channels;
        nmos::channel left = { utility::s2us("Left"), nmos::channel_symbols::L };
        nmos::channel right = { utility::s2us("Right"), nmos::channel_symbols::R };
        channels.push_back(left);
        channels.push_back(right);

        auto source = nmos::make_audio_source(audio_source_id, device_id, sample_rate, channels, model.settings);
        auto flow = nmos::make_raw_audio_flow(audio_flow_id, audio_source_id, device_id, sample_rate, 24, model.settings);

        // add example network interface binding for both primary and secondary

        auto sender = nmos::make_sender(audio_sender_id, audio_flow_id, device_id, { U("example"), U("example") }, model.settings);
        // add example "natural grouping" hint
        web::json::push_back(sender.data[U("tags")][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), U("sender 0") }));

        nmos::sdp_parameters sdp_params = nmos::make_sdp_parameters(source.data, flow.data, sender.data, { U("PRIMARY") });

        std::vector<value> vec_auto_defaults;
        vec_auto_defaults.push_back(value_of({
            { nmos::fields::source_ip, value::string(U("192.168.240.3")) },
            { nmos::fields::destination_ip, value::string(U("239.255.240.3")) }
            }));

        auto sender_auto_defaults = value::array(vec_auto_defaults);
        set_defaults_for_autos (audio_sender_id, sender_auto_defaults);

        auto connection_sender = nmos::make_connection_sender(audio_sender_id, false);
        app_hooks.resolve_auto(connection_sender, connection_sender.data[nmos::fields::endpoint_active], app_hooks);
        node_set_connection_sender_transportfile(connection_sender, sdp_params);

        set_base_sdp_params (connection_sender, sdp_params);

        insert_resource_after(delay_millis, model.node_resources, std::move(source), gate);
        insert_resource_after(delay_millis, model.node_resources, std::move(flow), gate);
        insert_resource_after(delay_millis, model.node_resources, std::move(sender), gate);
        insert_resource_after(delay_millis, model.connection_resources, std::move(connection_sender), gate);
    }

    // example audio receiver
    {
        // add example network interface binding for both primary and secondary
        auto receiver = nmos::make_audio_receiver(audio_receiver_id, device_id, nmos::transports::rtp_mcast, {}, 24, model.settings);
        // add example "natural grouping" hint
        web::json::push_back(receiver.data[U("tags")][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), U("receiver 0") }));

        std::vector<value> vec_auto_defaults;
        vec_auto_defaults.push_back(value_of({
            { nmos::fields::interface_ip, value::string(U("192.168.242.3")) }
            }));

        auto receiver_auto_defaults = value::array(vec_auto_defaults);
        set_defaults_for_autos (audio_receiver_id, receiver_auto_defaults);

        auto connection_receiver = nmos::make_connection_receiver(audio_receiver_id, false);
        app_hooks.resolve_auto(connection_receiver, connection_receiver.data[nmos::fields::endpoint_active], app_hooks);

        insert_resource_after(delay_millis, model.node_resources, std::move(receiver), gate);
        insert_resource_after(delay_millis, model.connection_resources, std::move(connection_receiver), gate);
    }
}
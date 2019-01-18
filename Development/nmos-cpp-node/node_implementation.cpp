#include "node_implementation.h"

#include "detail/for_each_reversed.h"
#include "nmos/activation_mode.h"
#include "nmos/connection_api.h"
#include "nmos/group_hint.h"
#include "nmos/model.h"
#include "nmos/node_resource.h"
#include "nmos/node_resources.h"
#include "nmos/sdp_utils.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h"

#include "sdp/sdp.h"



// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// and then waits for sender/receiver activations or shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate, const nmos::experimental::app_hooks& app_hooks)
{
    using web::json::value;
    using web::json::value_of;
   
    // Initialize the app and create initial resources
    app_hooks.initialize(model, gate);

    auto lock = model.write_lock(); // in order to update the resources

    auto most_recent_update = nmos::tai_min();
    auto earliest_scheduled_activation = (nmos::tai_clock::time_point::max)();

    for (;;)
    {
        // wait for the thread to be interrupted because there may be new scheduled activations, or immediate activations to process
        // or because the server is being shut down
        // or because it's time for the next scheduled activation
        // or because the application has work to do (presumably nmos related)
        model.wait_until(lock, earliest_scheduled_activation, [&] { 
            return model.shutdown || 
                most_recent_update < nmos::most_recent_update(model.connection_resources) ||
                app_hooks.check_for_work(); 
        });
        if (model.shutdown) break;
        
        bool notify = false;
        
        // Have the application process any work that it has (might not have any work)
        // set the notify flag if the app says it is needed
        if (app_hooks.process_work())
            notify = true;
        
        auto& by_updated = model.connection_resources.get<nmos::tags::updated>();

        // go through all connection resources
        // process any immediate activations
        // process any scheduled activations whose requested_time has passed
        // identify the next scheduled activation

        const auto now = nmos::tai_clock::now();

        earliest_scheduled_activation = (nmos::tai_clock::time_point::max)();

        // since modify reorders the resource in this index, use for_each_reversed
        detail::for_each_reversed(by_updated.begin(), by_updated.end(), [&](const nmos::resource& resource)
        {
            if (!resource.has_data()) return;

            const std::pair<nmos::id, nmos::type> id_type{ resource.id, resource.type };

            auto& staged = nmos::fields::endpoint_staged(resource.data);
            auto& staged_activation = nmos::fields::activation(staged);
            auto& staged_mode_or_null = nmos::fields::mode(staged_activation);

            if (staged_mode_or_null.is_null()) return;

            const nmos::activation_mode staged_mode{ staged_mode_or_null.as_string() };

            if (nmos::activation_modes::activate_scheduled_absolute == staged_mode ||
                nmos::activation_modes::activate_scheduled_relative == staged_mode)
            {
                auto& staged_activation_time = nmos::fields::activation_time(staged_activation);
                const auto scheduled_activation = nmos::time_point_from_tai(nmos::parse_version(staged_activation_time.as_string()));

                if (scheduled_activation < now)
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Processing scheduled activation for " << id_type;
                }
                else
                {
                    if (scheduled_activation < earliest_scheduled_activation)
                    {
                        earliest_scheduled_activation = scheduled_activation;
                    }

                    return;
                }
            }
            else if (nmos::activation_modes::activate_immediate == staged_mode)
            {
                // check for cancelled in-flight immediate activation
                if (nmos::fields::requested_time(staged_activation).is_null()) return;
                // check for processed in-flight immediate activation
                if (!nmos::fields::activation_time(staged_activation).is_null()) return;

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Processing immediate activation for " << id_type;
            }
            else
            {
                slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected activation mode for " << id_type;
                return;
            }

            const auto activation_time = nmos::tai_now();

            bool active = false;
            nmos::id connected_id;

            // Update the IS-05 connection resource

            nmos::modify_resource(model.connection_resources, resource.id, [&](nmos::resource& connection_resource)
            {
                const auto& type = connection_resource.type;
                nmos::set_connection_resource_active(connection_resource, [&](web::json::value& endpoint_active)
                {
                    app_hooks.resolve_auto(connection_resource, endpoint_active, app_hooks);
                    active = nmos::fields::master_enable(endpoint_active);
                    // Senders indicate the connected receiver_id, receivers indicate the connected sender_id
                    auto& connected_id_or_null = nmos::types::sender == type ? nmos::fields::receiver_id(endpoint_active) : nmos::fields::sender_id(endpoint_active);
                    if (!connected_id_or_null.is_null()) connected_id = connected_id_or_null.as_string();
                }, activation_time);

                // If this is a sender, then update the transport file
                if (nmos::types::sender == type) 
                    node_set_connection_sender_transportfile(connection_resource, app_hooks.get_base_sdp_params(resource));
            });

            // Update the IS-04 resource

            nmos::modify_resource(model.node_resources, resource.id, [&activation_time, &active, &connected_id](nmos::resource& resource)
            {
                nmos::set_resource_subscription(resource, active, connected_id, activation_time);
            });

            notify = true;
            
            // Tell the application that the resource has been updated
            app_hooks.resource_activation(resource.id);
        });

        if ((nmos::tai_clock::time_point::max)() != earliest_scheduled_activation)
        {
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Next scheduled activation is at " << nmos::make_version(nmos::tai_from_time_point(earliest_scheduled_activation))
                << " in about " << std::fixed << std::setprecision(3) << std::chrono::duration_cast<std::chrono::duration<double>>(earliest_scheduled_activation - now).count() << " seconds time";
        }

        if (notify)
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying node behaviour thread"; // and anyone else who cares...
            model.notify();
        }

        most_recent_update = nmos::most_recent_update(model.connection_resources);
    }
}

// as part of activation, the sender /transportfile should be updated based on the active transport parameters
void node_set_connection_sender_transportfile(nmos::resource& connection_sender, const nmos::sdp_parameters& sdp_params)
{
    auto& transport_params = connection_sender.data[nmos::fields::endpoint_active][nmos::fields::transport_params];
    auto session_description = nmos::make_session_description(sdp_params, transport_params);
    auto sdp = utility::s2us(sdp::make_session_description(session_description));
    connection_sender.data[nmos::fields::endpoint_transportfile] = nmos::make_connection_sender_transportfile(sdp);
}
#if NEVER

// sample function which creates initial video and audio senders and receivers
void node_initial_resources(nmos::node_model& model, slog::base_gate& gate, 
    const nmos::experimental::app_hooks& app_hooks, 
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
            } ));
        vec_auto_defaults.push_back(value_of({ 
            { nmos::fields::source_ip, value::string(U("192.168.240.2")) },
            { nmos::fields::destination_ip, value::string(U("239.255.240.2")) }
            }));
        
        auto sender_auto_defaults = value::array(vec_auto_defaults);

        app_hooks.set_defaults_for_autos (sender_id, sender_auto_defaults);

        auto connection_sender = nmos::make_connection_sender(sender_id, true);
        app_hooks.resolve_auto(connection_sender, connection_sender.data[nmos::fields::endpoint_active], app_hooks);
        node_set_connection_sender_transportfile(connection_sender, sdp_params);

        app_hooks.set_base_sdp_params (connection_sender, sdp_params);
        
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
        app_hooks.set_defaults_for_autos (receiver_id, receiver_auto_defaults);

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
        app_hooks.set_defaults_for_autos (audio_sender_id, sender_auto_defaults);

        auto connection_sender = nmos::make_connection_sender(audio_sender_id, false);
        app_hooks.resolve_auto(connection_sender, connection_sender.data[nmos::fields::endpoint_active], app_hooks);
        node_set_connection_sender_transportfile(connection_sender, sdp_params);

        app_hooks.set_base_sdp_params (connection_sender, sdp_params);

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
        app_hooks.set_defaults_for_autos (audio_receiver_id, receiver_auto_defaults);

        auto connection_receiver = nmos::make_connection_receiver(audio_receiver_id, false);
        app_hooks.resolve_auto(connection_receiver, connection_receiver.data[nmos::fields::endpoint_active], app_hooks);

        insert_resource_after(delay_millis, model.node_resources, std::move(receiver), gate);
        insert_resource_after(delay_millis, model.connection_resources, std::move(connection_receiver), gate);
    }
}
#endif
#include "node_implementation.h"

#include "pplx/pplx_utils.h" // for pplx::complete_after, etc.
#include "nmos/connection_api.h" // for nmos::resolve_rtp_auto, etc.
#include "nmos/connection_resources.h"
#include "nmos/events_resources.h"
#include "nmos/group_hint.h"
#include "nmos/media_type.h"
#include "nmos/model.h"
#include "nmos/node_resource.h"
#include "nmos/node_resources.h"
#include "nmos/random.h"
#include "nmos/sdp_utils.h"
#include "nmos/slog.h"
#include "nmos/transport.h"
#include "sdp/sdp.h"

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// starts background tasks to emit regular events from the temperature event source and then waits for shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate)
{
    using web::json::value;
    using web::json::value_of;

    auto lock = model.write_lock(); // in order to update the resources

    const auto seed_id = nmos::experimental::fields::seed_id(model.settings);
    auto node_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/self"));
    auto device_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/device/0"));
    auto source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/0"));
    auto flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/0"));
    auto sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/0"));
    auto receiver_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/receiver/0"));
    auto temperature_source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/1"));
    auto temperature_flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/1"));
    auto temperature_ws_sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/1"));

    // any delay between updates to the model resources is unnecessary
    // this just serves as a slightly more realistic example!
    const unsigned int delay_millis{ 10 };

    // it is important that the model be locked before inserting, updating or deleting a resource
    // and that the the node behaviour thread be notified after doing so
    const auto insert_resource_after = [&model, &lock](unsigned int milliseconds, nmos::resources& resources, nmos::resource&& resource, slog::base_gate& gate)
    {
        if (nmos::details::wait_for(model.shutdown_condition, lock, std::chrono::milliseconds(milliseconds), [&] { return model.shutdown; })) return false;

        const std::pair<nmos::id, nmos::type> id_type{ resource.id, resource.type };
        const bool success = insert_resource(resources, std::move(resource)).second;

        if (success)
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Updated model with " << id_type;
        else
            slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Model update error: " << id_type;

        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying node behaviour thread"; // and anyone else who cares...
        model.notify();

        return success;
    };

    const auto resolve_auto = make_node_implementation_auto_resolver(model.settings);
    const auto set_transportfile = make_node_implementation_transportfile_setter(model.node_resources, model.settings);

    // example node
    {
        auto node = nmos::make_node(node_id, model.settings);
        // add one example network interface
        node.data[U("interfaces")] = value_of({
            value_of({
                { U("chassis_id"), value::null() },
                { U("port_id"), U("ff-ff-ff-ff-ff-ff") },
                { U("name"), U("example") }
            })
        });
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(node), gate)) return;
    }

    // example device
    {
        const auto senders = 0 <= nmos::fields::events_port(model.settings)
            ? std::vector<nmos::id>{ sender_id, temperature_ws_sender_id }
            : std::vector<nmos::id>{ sender_id };
        const auto receivers = std::vector<nmos::id>{ receiver_id };
        if (!insert_resource_after(delay_millis, model.node_resources, nmos::make_device(device_id, node_id, senders, receivers, model.settings), gate)) return;
    }

    // example source, flow and sender
    {
        auto source = nmos::make_video_source(source_id, device_id, { 25, 1 }, model.settings);

        auto flow = nmos::make_raw_video_flow(flow_id, source_id, device_id, model.settings);

        // set_transportfile needs to find the matching source and flow for the sender, so insert these first
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(source), gate)) return;
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(flow), gate)) return;

        // add example network interface binding for both primary and secondary
        auto sender = nmos::make_sender(sender_id, flow_id, device_id, { U("example"), U("example") }, model.settings);
        // add example "natural grouping" hint
        web::json::push_back(sender.data[U("tags")][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), U("sender 0") }));

        auto connection_sender = nmos::make_connection_rtp_sender(sender_id, true);
        resolve_auto(sender, connection_sender, connection_sender.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);
        set_transportfile(sender, connection_sender, connection_sender.data[nmos::fields::endpoint_transportfile]);

        if (!insert_resource_after(delay_millis, model.node_resources, std::move(sender), gate)) return;
        if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_sender), gate)) return;
    }

    // example receiver
    {
        // add example network interface binding for both primary and secondary
        auto receiver = nmos::make_video_receiver(receiver_id, device_id, nmos::transports::rtp_mcast, { U("example"), U("example") }, model.settings);
        // add example "natural grouping" hint
        web::json::push_back(receiver.data[U("tags")][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), U("receiver 0") }));

        auto connection_receiver = nmos::make_connection_rtp_receiver(receiver_id, true);
        resolve_auto(receiver, connection_receiver, connection_receiver.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

        if (!insert_resource_after(delay_millis, model.node_resources, std::move(receiver), gate)) return;
        if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_receiver), gate)) return;
    }

    // example temperature event source, sender, flow
    if (0 <= nmos::fields::events_port(model.settings))
    {
        auto temperature_source = nmos::make_data_source(temperature_source_id, device_id, { 1, 1 }, nmos::event_types::measurement(nmos::event_types::number, U("temperature"), U("C")), model.settings);

        // see https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#231-measurements
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/examples/eventsapi-v1.0-type-number-measurement-get-200.json
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/examples/eventsapi-v1.0-state-number-rational-get-200.json
        auto events_temperature_type = nmos::make_events_number_type({ -200, 10 }, { 1000, 10 }, { 1, 10 }, U("C"));
        auto events_temperature_state = nmos::make_events_number_state(temperature_source_id, { 201, 10 });
        auto events_temperature_source = nmos::make_events_source(temperature_source_id, events_temperature_state, events_temperature_type);

        auto temperature_flow = nmos::make_data_flow(temperature_flow_id, temperature_source_id, device_id, nmos::media_types::application_json, model.settings);
        auto temperature_ws_sender = nmos::make_sender(temperature_ws_sender_id, temperature_flow_id, nmos::transports::websocket, device_id, {}, { U("example") }, model.settings);
        auto connection_temperature_ws_sender = nmos::make_connection_events_websocket_sender(temperature_ws_sender_id, device_id, temperature_source_id, model.settings);
        resolve_auto(temperature_ws_sender, connection_temperature_ws_sender, connection_temperature_ws_sender.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

        if (!insert_resource_after(delay_millis, model.node_resources, std::move(temperature_source), gate)) return;
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(temperature_flow), gate)) return;
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(temperature_ws_sender), gate)) return;
        if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_temperature_ws_sender), gate)) return;
        if (!insert_resource_after(delay_millis, model.events_resources, std::move(events_temperature_source), gate)) return;
    }

    // start background tasks to intermittently update the state of the temperature event source, to cause events to be emitted to connected receivers

    nmos::details::seed_generator temperature_interval_seeder;
    std::shared_ptr<std::default_random_engine> temperature_interval_engine(new std::default_random_engine(temperature_interval_seeder));

    auto cancellation_source = pplx::cancellation_token_source();
    auto token = cancellation_source.get_token();
    auto temperature_events = pplx::do_while([&model, temperature_source_id, temperature_interval_engine, token]
    {
        const auto temp_interval = std::uniform_real_distribution<>(0.5, 5.0)(*temperature_interval_engine);
        return pplx::complete_after(std::chrono::milliseconds(std::chrono::milliseconds::rep(1000 * temp_interval)), token).then([&model, temperature_source_id]
        {
            auto lock = model.write_lock();

            modify_resource(model.events_resources, temperature_source_id, [](nmos::resource& resource)
            {
                // make example temperature data ... \/\/\/\/ ... around 200
                auto value = 175.0 + std::abs(nmos::tai_now().seconds % 100 - 50);
                // i.e. 17.5-22.5 C
                nmos::fields::endpoint_state(resource.data) = nmos::make_events_number_state(resource.id, { value, 10 });
            });

            model.notify();

            return true;
        });
    }, token);

    // wait for the thread to be interrupted because the server is being shut down
    model.shutdown_condition.wait(lock, [&] { return model.shutdown; });

    cancellation_source.cancel();
    // wait without the lock since it is also used by the background tasks
    nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
    temperature_events.wait();
}

nmos::connection_resource_auto_resolver make_node_implementation_auto_resolver(const nmos::settings& settings)
{
    using web::json::value;

    const auto seed_id = nmos::experimental::fields::seed_id(settings);
    auto device_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/device/0"));
    auto sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/0"));
    auto receiver_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/receiver/0"));
    auto temperature_ws_sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/1"));
    auto temperature_ws_sender_uri = nmos::make_events_ws_api_connection_uri(device_id, settings);

    // although which properties may need to be defaulted depends on the resource type,
    // the default value will almost always be different for each resource
    return [device_id, sender_id, receiver_id, temperature_ws_sender_id, temperature_ws_sender_uri](const nmos::resource& resource, const nmos::resource& connection_resource, value& transport_params)
    {
        const std::pair<nmos::id, nmos::type> id_type{ connection_resource.id, connection_resource.type };

        // "In some cases the behaviour is more complex, and may be determined by the vendor."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
        if (sender_id == id_type.first)
        {
            nmos::resolve_rtp_auto(id_type.second, transport_params);
            nmos::details::resolve_auto(transport_params[0], nmos::fields::source_ip, [] { return value::string(U("192.168.255.0")); });
            nmos::details::resolve_auto(transport_params[1], nmos::fields::source_ip, [] { return value::string(U("192.168.255.1")); });
            nmos::details::resolve_auto(transport_params[0], nmos::fields::destination_ip, [] { return value::string(U("239.255.255.0")); });
            nmos::details::resolve_auto(transport_params[1], nmos::fields::destination_ip, [] { return value::string(U("239.255.255.1")); });
        }
        else if (receiver_id == id_type.first)
        {
            nmos::resolve_rtp_auto(id_type.second, transport_params);
            nmos::details::resolve_auto(transport_params[0], nmos::fields::interface_ip, [] { return value::string(U("192.168.255.2")); });
            nmos::details::resolve_auto(transport_params[1], nmos::fields::interface_ip, [] { return value::string(U("192.168.255.3")); });
        }
        else if (temperature_ws_sender_id == id_type.first)
        {
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_uri, [&] { return value::string(temperature_ws_sender_uri.to_string()); });
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_authorization, [&] { return value::boolean(false); });
        }
    };
}

nmos::connection_sender_transportfile_setter make_node_implementation_transportfile_setter(const nmos::resources& node_resources, const nmos::settings& settings)
{
    using web::json::value;

    const auto seed_id = nmos::experimental::fields::seed_id(settings);
    auto source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/0"));
    auto flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/0"));
    auto sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/0"));

    // as part of activation, the example sender /transportfile should be updated based on the active transport parameters
    return [&node_resources, source_id, flow_id, sender_id](const nmos::resource& sender, const nmos::resource& connection_sender, value& endpoint_transportfile)
    {
        if (sender_id == connection_sender.id)
        {
            auto source = nmos::find_resource(node_resources, { source_id, nmos::types::source });
            auto flow = nmos::find_resource(node_resources, { flow_id, nmos::types::flow });
            if (node_resources.end() == source || node_resources.end() == flow)
            {
                throw std::logic_error("matching IS-04 source or flow not found");
            }

            auto sdp_params = nmos::make_sdp_parameters(source->data, flow->data, sender.data, { U("PRIMARY"), U("SECONDARY") });
            auto& transport_params = nmos::fields::transport_params(nmos::fields::endpoint_active(connection_sender.data));
            auto session_description = nmos::make_session_description(sdp_params, transport_params);
            auto sdp = utility::s2us(sdp::make_session_description(session_description));
            endpoint_transportfile = nmos::make_connection_rtp_sender_transportfile(sdp);
        }
    };
}

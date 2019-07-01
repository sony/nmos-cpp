#include "node_implementation.h"

#include "pplx/pplx_utils.h" // for pplx::complete_after, etc.
#include "detail/for_each_reversed.h"
#include "nmos/activation_mode.h"
#include "nmos/connection_api.h"
#include "nmos/connection_resources.h"
#include "nmos/events_resources.h"
#include "nmos/group_hint.h"
#include "nmos/media_type.h"
#include "nmos/model.h"
#include "nmos/node_resource.h"
#include "nmos/node_resources.h"
#include "nmos/sdp_utils.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h"
#include "nmos/transport.h"
#include "sdp/sdp.h"

// a connection_resource_auto_resolver overwrites every instance of "auto" in the specified transport_params array for the specified (IS-04/IS-05) sender/connection_sender or receiver/connection_receiver
typedef std::function<void(const nmos::resource&, const nmos::resource&, web::json::value&)> connection_resource_auto_resolver;

// a connection_sender_transportfile_setter updates the specified /transportfile endpoint for the specified (IS-04/IS-05) sender/connection_sender
typedef std::function<void(const nmos::resource&, const nmos::resource&, web::json::value&)> connection_sender_transportfile_setter;

void connection_activation_thread(nmos::node_model& model, connection_resource_auto_resolver resolve_auto, connection_sender_transportfile_setter set_transportfile, slog::base_gate& gate);

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// and then waits for sender/receiver activations or shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate)
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
    auto temperature_source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/1"));
    auto temperature_flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/1"));
    auto temperature_ws_sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/1"));

    // example sender session description params
    nmos::sdp_parameters sdp_params;

    auto lock = model.write_lock(); // in order to update the resources

    // any delay between updates to the model resources is unnecessary
    // this just serves as a slightly more realistic example!
    const unsigned int delay_millis{ 10 };

    const auto insert_resource_after = [&model, &lock](unsigned int milliseconds, nmos::resources& resources, nmos::resource&& resource, slog::base_gate& gate)
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

    // although which properties may need to be defaulted depends on the resource type,
    // the default value will almost always be different for each resource
    const auto resolve_auto = [&](const std::pair<nmos::id, nmos::type>& id_type, value& transport_params)
    {
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
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_uri, [&] { return value::string(nmos::make_events_ws_api_connection_uri(device_id, model.settings).to_string()); });
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_authorization, [&] { return value::boolean(false); });
        }
    };

    // as part of activation, the example sender /transportfile should be updated based on the active transport parameters
    const auto set_transportfile = [&](const nmos::id& id, const web::json::value& transport_params, web::json::value& endpoint_transportfile)
    {
        if (sender_id == id)
        {
            auto session_description = nmos::make_session_description(sdp_params, transport_params);
            auto sdp = utility::s2us(sdp::make_session_description(session_description));
            endpoint_transportfile = nmos::make_connection_rtp_sender_transportfile(sdp);
        }
    };

    // example node
    {
        auto node = nmos::make_node(node_id, model.settings);
        // add one example network interface
        node.data[U("interfaces")] = value_of({ value_of({ { U("chassis_id"), value::null() }, { U("port_id"), U("ff-ff-ff-ff-ff-ff") }, { U("name"), U("example") } }) });
        insert_resource_after(delay_millis, model.node_resources, std::move(node), gate);
    }

    // example device
    {
        const auto senders = 0 <= nmos::fields::events_port(model.settings)
            ? std::vector<nmos::id>{ sender_id, temperature_ws_sender_id }
            : std::vector<nmos::id>{ sender_id };
        const auto receivers = std::vector<nmos::id>{ receiver_id };
        insert_resource_after(delay_millis, model.node_resources, nmos::make_device(device_id, node_id, senders, receivers, model.settings), gate);
    }

    // example source, flow and sender
    {
        auto source = nmos::make_video_source(source_id, device_id, { 25, 1 }, model.settings);

        auto flow = nmos::make_raw_video_flow(flow_id, source_id, device_id, model.settings);
        // add example network interface binding for both primary and secondary

        auto sender = nmos::make_sender(sender_id, flow_id, device_id, { U("example"), U("example") }, model.settings);
        // add example "natural grouping" hint
        web::json::push_back(sender.data[U("tags")][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), U("sender 0") }));

        sdp_params = nmos::make_sdp_parameters(source.data, flow.data, sender.data, { U("PRIMARY"), U("SECONDARY") });

        auto connection_sender = nmos::make_connection_rtp_sender(sender_id, true);
        resolve_auto({ connection_sender.id, connection_sender.type }, connection_sender.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);
        set_transportfile(connection_sender.id, nmos::fields::transport_params(nmos::fields::endpoint_active(connection_sender.data)), connection_sender.data[nmos::fields::endpoint_transportfile]);

        insert_resource_after(delay_millis, model.node_resources, std::move(source), gate);
        insert_resource_after(delay_millis, model.node_resources, std::move(flow), gate);
        insert_resource_after(delay_millis, model.node_resources, std::move(sender), gate);
        insert_resource_after(delay_millis, model.connection_resources, std::move(connection_sender), gate);
    }

    // example receiver
    {
        // add example network interface binding for both primary and secondary
        auto receiver = nmos::make_video_receiver(receiver_id, device_id, nmos::transports::rtp_mcast, { U("example"), U("example") }, model.settings);
        // add example "natural grouping" hint
        web::json::push_back(receiver.data[U("tags")][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), U("receiver 0") }));

        auto connection_receiver = nmos::make_connection_rtp_receiver(receiver_id, true);
        resolve_auto({ connection_receiver.id, connection_receiver.type }, connection_receiver.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

        insert_resource_after(delay_millis, model.node_resources, std::move(receiver), gate);
        insert_resource_after(delay_millis, model.connection_resources, std::move(connection_receiver), gate);
    }

    // example temperature source, sender, flow
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
        resolve_auto({ connection_temperature_ws_sender.id, connection_temperature_ws_sender.type }, connection_temperature_ws_sender.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

        insert_resource_after(delay_millis, model.node_resources, std::move(temperature_source), gate);
        insert_resource_after(delay_millis, model.node_resources, std::move(temperature_flow), gate);
        insert_resource_after(delay_millis, model.node_resources, std::move(temperature_ws_sender), gate);
        insert_resource_after(delay_millis, model.connection_resources, std::move(connection_temperature_ws_sender), gate);
        insert_resource_after(delay_millis, model.events_resources, std::move(events_temperature_source), gate);
    }

    auto cancellation_source = pplx::cancellation_token_source();
    auto token = cancellation_source.get_token();
    auto temperature_events = pplx::do_while([&]
    {
        return pplx::complete_after(std::chrono::seconds(1), token).then([&]
        {
            auto lock = model.write_lock();

            modify_resource(model.events_resources, temperature_source_id, [&](nmos::resource& resource)
            {
                // make example temperature data ... \/\/\/\/ ... around 200
                auto value = 175.0 + std::abs(nmos::tai_now().seconds % 100 - 50);
                // i.e. 17.5-22.5 C
                nmos::fields::endpoint_state(resource.data) = nmos::make_events_number_state(temperature_source_id, { value, 10 });
            });

            model.notify();

            return true;
        });
    }, token);

    {
        nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };

        connection_activation_thread(model, [&resolve_auto](const nmos::resource& resource, const nmos::resource& connection_resource, web::json::value& transport_params)
        {
            resolve_auto({ connection_resource.id, connection_resource.type }, transport_params);
        }, [&set_transportfile](const nmos::resource& sender, const nmos::resource& connection_sender, web::json::value& endpoint_transportfile)
        {
            set_transportfile(connection_sender.id, nmos::fields::transport_params(nmos::fields::endpoint_active(connection_sender.data)), endpoint_transportfile);
        }, gate);
    }

    cancellation_source.cancel();
    // wait without the lock since it is also used by the background tasks
    nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
    temperature_events.wait();
}

void connection_activation_thread(nmos::node_model& model, connection_resource_auto_resolver resolve_auto, connection_sender_transportfile_setter set_transportfile, slog::base_gate& gate)
{
    auto lock = model.write_lock(); // in order to update the resources

    auto most_recent_update = nmos::tai_min();
    auto earliest_scheduled_activation = (nmos::tai_clock::time_point::max)();

    for (;;)
    {
        // wait for the thread to be interrupted because there may be new scheduled activations, or immediate activations to process
        // or because the server is being shut down
        // or because it's time for the next scheduled activation
        model.wait_until(lock, earliest_scheduled_activation, [&] { return model.shutdown || most_recent_update < nmos::most_recent_update(model.connection_resources); });
        if (model.shutdown) break;

        auto& by_updated = model.connection_resources.get<nmos::tags::updated>();

        // go through all connection resources
        // process any immediate activations
        // process any scheduled activations whose requested_time has passed
        // identify the next scheduled activation

        const auto now = nmos::tai_clock::now();

        earliest_scheduled_activation = (nmos::tai_clock::time_point::max)();

        bool notify = false;

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
                const std::pair<nmos::id, nmos::type> id_type{ connection_resource.id, connection_resource.type };
                auto matching_resource = find_resource(model.node_resources, id_type);
                if (model.node_resources.end() == matching_resource)
                {
                    throw std::logic_error("matching IS-04 resource not found");
                }

                nmos::set_connection_resource_active(connection_resource, [&](web::json::value& endpoint_active)
                {
                    resolve_auto(*matching_resource, connection_resource, nmos::fields::transport_params(endpoint_active));
                    active = nmos::fields::master_enable(endpoint_active);
                    // Senders indicate the connected receiver_id, receivers indicate the connected sender_id
                    auto& connected_id_or_null = nmos::types::sender == id_type.second ? nmos::fields::receiver_id(endpoint_active) : nmos::fields::sender_id(endpoint_active);
                    if (!connected_id_or_null.is_null()) connected_id = connected_id_or_null.as_string();
                }, activation_time);

                set_transportfile(*matching_resource, connection_resource, connection_resource.data[nmos::fields::endpoint_transportfile]);
            });

            // Update the IS-04 resource

            nmos::modify_resource(model.node_resources, resource.id, [&activation_time, &active, &connected_id](nmos::resource& resource)
            {
                nmos::set_resource_subscription(resource, active, connected_id, activation_time);
            });

            notify = true;
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

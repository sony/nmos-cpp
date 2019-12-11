#include "node_implementation.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_first_of.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/irange.hpp>
#include "pplx/pplx_utils.h" // for pplx::complete_after, etc.
#include "cpprest/host_utils.h"
#ifdef HAVE_LLDP
#include "lldp/lldp_manager.h"
#endif
#include "nmos/clock_name.h"
#include "nmos/colorspace.h"
#include "nmos/components.h" // for nmos::chroma_subsampling
#include "nmos/connection_resources.h"
#include "nmos/connection_events_activation.h"
#include "nmos/events_resources.h"
#include "nmos/group_hint.h"
#include "nmos/interlace_mode.h"
#ifdef HAVE_LLDP
#include "nmos/lldp_manager.h"
#endif
#include "nmos/media_type.h"
#include "nmos/model.h"
#include "nmos/node_interfaces.h"
#include "nmos/node_resource.h"
#include "nmos/node_resources.h"
#include "nmos/random.h"
#include "nmos/sdp_utils.h"
#include "nmos/slog.h"
#include "nmos/transfer_characteristic.h"
#include "nmos/transport.h"
#include "sdp/sdp.h"

// custom logging category for the example node implementation thread
const nmos::category node_implementation_category{ "node_implementation" };

// custom setting for the example node to provide for very basic testing of a node with many sub-resources
const web::json::field_as_integer_or node_implementation_field_how_many{ U("how_many"), 1 };

// generate repeatable ids for a range of resources
template <typename Suffixes>
std::vector<nmos::id> make_repeatable_ids(const nmos::id& seed_id, const utility::string_t& name_prefix, Suffixes name_suffixes);

// add a helpful suffix to the label of a sub-resource for the example node
template <typename Suffix>
void set_node_implementation_label(nmos::resource& resource, Suffix suffix);

// add an example "natural grouping" hint to a sender or receiver
template <typename Suffix>
void insert_node_implementation_group_hint(nmos::resource& resource, Suffix suffix);

// specific event types used by the example node
const auto temperature_Celsius = nmos::event_types::measurement(nmos::event_types::number, U("temperature"), U("C"));
const auto temperature_wildcard = nmos::event_types::measurement(nmos::event_types::number, U("temperature"), nmos::event_types::wildcard);

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// starts background tasks to emit regular events from the temperature event source and then waits for shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate_)
{
    nmos::details::omanip_gate gate{ gate_, nmos::stash_category(node_implementation_category) };

    using web::json::value;
    using web::json::value_of;

    auto lock = model.write_lock(); // in order to update the resources

    const auto seed_id = nmos::experimental::fields::seed_id(model.settings);
    const auto node_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/self"));
    const auto device_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/device/0"));
    const auto how_many = node_implementation_field_how_many(model.settings);
    const auto suffixes = boost::irange(0, how_many * 2, 2);
    const auto source_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/source/"), suffixes);
    const auto flow_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/flow/"), suffixes);
    const auto sender_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/sender/"), suffixes);
    const auto receiver_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/receiver/"), suffixes);
    const auto temperature_suffixes = boost::irange(1, how_many * 2 + 1, 2);
    const auto temperature_source_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/source/"), temperature_suffixes);
    const auto temperature_flow_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/flow/"), temperature_suffixes);
    const auto temperature_ws_sender_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/sender/"), temperature_suffixes);
    const auto temperature_ws_receiver_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/receiver/"), temperature_suffixes);

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

    const auto clocks = web::json::value_of({ nmos::make_internal_clock(nmos::clock_names::clk0) });
    // filter network interfaces to those that correspond to the specified host_addresses
    const auto host_interfaces = boost::copy_range<std::vector<web::hosts::experimental::host_interface>>(web::hosts::experimental::host_interfaces() | boost::adaptors::filtered([&](const web::hosts::experimental::host_interface& interface)
    {
        return interface.addresses.end() != boost::range::find_first_of(interface.addresses, nmos::fields::host_addresses(model.settings), [](const utility::string_t& interface_address, const web::json::value& host_address)
        {
            return interface_address == host_address.as_string();
        });
    }));
    const auto interfaces = nmos::experimental::node_interfaces(host_interfaces);

    // example node
    {
        auto node = nmos::make_node(node_id, clocks, nmos::make_node_interfaces(interfaces), model.settings);
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(node), gate)) return;
    }

#ifdef HAVE_LLDP
    // LLDP manager for advertising server identity, capabilities, and discovering neighbours on a local area network
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting to configure LLDP";
    auto lldp_manager = nmos::experimental::make_lldp_manager(model, interfaces, true, gate);
    // hm, open may potentially throw?
    lldp::lldp_manager_guard lldp_manager_guard(lldp_manager);
#endif

    // prepare interface bindings for all senders and receivers
    const auto host_interface_ = boost::range::find_if(host_interfaces, [&](const web::hosts::experimental::host_interface& interface)
    {
        return interface.addresses.end() != boost::range::find(interface.addresses, nmos::fields::host_address(model.settings));
    });
    if (host_interfaces.end() == host_interface_)
    {
        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "No network interface corresponding to host_address?";
        return;
    }
    const auto& host_interface = *host_interface_;
    // hmm, should probably add a custom setting to control the primary and secondary interfaces for the example node's senders and receivers
    const auto& primary_interface = host_interfaces.front();
    const auto& secondary_interface = host_interfaces.back();

    // example device
    {
        std::vector<nmos::id> senders(sender_ids);
        if (0 <= nmos::fields::events_port(model.settings)) boost::range::push_back(senders, temperature_ws_sender_ids);
        std::vector<nmos::id> receivers(receiver_ids);
        boost::range::push_back(receivers, temperature_ws_receiver_ids);
        if (!insert_resource_after(delay_millis, model.node_resources, nmos::make_device(device_id, node_id, senders, receivers, model.settings), gate)) return;
    }

    // example sources, flows and senders
    for (int i = 0; i < how_many; ++i)
    {
        const auto& suffix = suffixes[i];
        const auto& source_id = source_ids[i];
        const auto& flow_id = flow_ids[i];
        const auto& sender_id = sender_ids[i];

        auto source = nmos::make_video_source(source_id, device_id, nmos::clock_names::clk0, nmos::rates::rate25, model.settings);
        set_node_implementation_label(source, suffix);

        // note, nmos::make_raw_video_flow(flow_id, source_id, device_id, model.settings) would basically use the following default values
        auto flow = nmos::make_raw_video_flow(
            flow_id, source_id, device_id,
            nmos::rates::rate25,
            1920, 1080, nmos::interlace_modes::interlaced_bff,
            nmos::colorspaces::BT709, nmos::transfer_characteristics::SDR, nmos::chroma_subsampling::YCbCr422, 10,
            model.settings
        );
        set_node_implementation_label(flow, suffix);

        // set_transportfile needs to find the matching source and flow for the sender, so insert these first
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(source), gate)) return;
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(flow), gate)) return;

        auto sender = nmos::make_sender(sender_id, flow_id, device_id, { primary_interface.name, secondary_interface.name }, model.settings);
        set_node_implementation_label(sender, suffix);
        insert_node_implementation_group_hint(sender, suffix);

        auto connection_sender = nmos::make_connection_rtp_sender(sender_id, true);
        // add some example constraints; these should be completed fully!
        connection_sender.data[nmos::fields::endpoint_constraints][0][nmos::fields::source_ip] = value_of({
            { nmos::fields::constraint_enum, web::json::value_from_elements(primary_interface.addresses) }
        });
        connection_sender.data[nmos::fields::endpoint_constraints][1][nmos::fields::source_ip] = value_of({
            { nmos::fields::constraint_enum, web::json::value_from_elements(secondary_interface.addresses) }
        });

        // initialize this sender enabled, just to enable the IS-05-01 test suite to run immediately
        connection_sender.data[nmos::fields::endpoint_active][nmos::fields::master_enable] = connection_sender.data[nmos::fields::endpoint_staged][nmos::fields::master_enable] = value::boolean(true);
        resolve_auto(sender, connection_sender, connection_sender.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);
        set_transportfile(sender, connection_sender, connection_sender.data[nmos::fields::endpoint_transportfile]);
        nmos::set_resource_subscription(sender, nmos::fields::master_enable(connection_sender.data[nmos::fields::endpoint_active]), {}, nmos::tai_now());

        if (!insert_resource_after(delay_millis, model.node_resources, std::move(sender), gate)) return;
        if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_sender), gate)) return;
    }

    // example receivers
    for (int i = 0; i < how_many; ++i)
    {
        const auto& suffix = suffixes[i];
        const auto& receiver_id = receiver_ids[i];

        auto receiver = nmos::make_video_receiver(receiver_id, device_id, nmos::transports::rtp_mcast, { primary_interface.name, secondary_interface.name }, model.settings);
        set_node_implementation_label(receiver, suffix);
        insert_node_implementation_group_hint(receiver, suffix);

        auto connection_receiver = nmos::make_connection_rtp_receiver(receiver_id, true);
        // add some example constraints; these should be completed fully!
        connection_receiver.data[nmos::fields::endpoint_constraints][0][nmos::fields::interface_ip] = value_of({
            { nmos::fields::constraint_enum, web::json::value_from_elements(primary_interface.addresses) }
        });
        connection_receiver.data[nmos::fields::endpoint_constraints][1][nmos::fields::interface_ip] = value_of({
            { nmos::fields::constraint_enum, web::json::value_from_elements(secondary_interface.addresses) }
        });

        resolve_auto(receiver, connection_receiver, connection_receiver.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

        if (!insert_resource_after(delay_millis, model.node_resources, std::move(receiver), gate)) return;
        if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_receiver), gate)) return;
    }

    // example temperature event source, sender, flow
    for (int i = 0; 0 <= nmos::fields::events_port(model.settings) && i < how_many; ++i)
    {
        const auto& temperature_suffix = temperature_suffixes[i];
        const auto& temperature_source_id = temperature_source_ids[i];
        const auto& temperature_flow_id = temperature_flow_ids[i];
        const auto& temperature_ws_sender_id = temperature_ws_sender_ids[i];

        // grain_rate is not set because temperature events are aperiodic
        auto temperature_source = nmos::make_data_source(temperature_source_id, device_id, {}, temperature_Celsius, model.settings);
        set_node_implementation_label(temperature_source, temperature_suffixes[i]);

        // see https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#231-measurements
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/examples/eventsapi-v1.0-type-number-measurement-get-200.json
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/examples/eventsapi-v1.0-state-number-rational-get-200.json
        auto events_temperature_type = nmos::make_events_number_type({ -200, 10 }, { 1000, 10 }, { 1, 10 }, U("C"));
        auto events_temperature_state = nmos::make_events_number_state(temperature_source_id, { 201, 10 });
        auto events_temperature_source = nmos::make_events_source(temperature_source_id, events_temperature_state, events_temperature_type);

        auto temperature_flow = nmos::make_data_flow(temperature_flow_id, temperature_source_id, device_id, nmos::media_types::application_json, model.settings);
        set_node_implementation_label(temperature_flow, temperature_suffix);

        auto temperature_ws_sender = nmos::make_sender(temperature_ws_sender_id, temperature_flow_id, nmos::transports::websocket, device_id, {}, { host_interface.name }, model.settings);
        set_node_implementation_label(temperature_ws_sender, temperature_suffix);
        insert_node_implementation_group_hint(temperature_ws_sender, temperature_suffix);

        auto connection_temperature_ws_sender = nmos::make_connection_events_websocket_sender(temperature_ws_sender_id, device_id, temperature_source_id, model.settings);
        resolve_auto(temperature_ws_sender, connection_temperature_ws_sender, connection_temperature_ws_sender.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

        if (!insert_resource_after(delay_millis, model.node_resources, std::move(temperature_source), gate)) return;
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(temperature_flow), gate)) return;
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(temperature_ws_sender), gate)) return;
        if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_temperature_ws_sender), gate)) return;
        if (!insert_resource_after(delay_millis, model.events_resources, std::move(events_temperature_source), gate)) return;
    }

    // example temperature event receiver
    for (int i = 0; i < how_many; ++i)
    {
        const auto& temperature_suffix = temperature_suffixes[i];
        const auto& temperature_ws_receiver_id = temperature_ws_receiver_ids[i];

        auto temperature_ws_receiver = nmos::make_data_receiver(temperature_ws_receiver_id, device_id, nmos::transports::websocket, { host_interface.name }, nmos::media_types::application_json, { temperature_wildcard }, model.settings);
        set_node_implementation_label(temperature_ws_receiver, temperature_suffix);
        insert_node_implementation_group_hint(temperature_ws_receiver, temperature_suffix);

        auto connection_temperature_ws_receiver = nmos::make_connection_events_websocket_receiver(temperature_ws_receiver_id, model.settings);
        resolve_auto(temperature_ws_receiver, connection_temperature_ws_receiver, connection_temperature_ws_receiver.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

        if (!insert_resource_after(delay_millis, model.node_resources, std::move(temperature_ws_receiver), gate)) return;
        if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_temperature_ws_receiver), gate)) return;
    }

    // start background tasks to intermittently update the state of the temperature event source, to cause events to be emitted to connected receivers

    nmos::details::seed_generator temperature_interval_seeder;
    std::shared_ptr<std::default_random_engine> temperature_interval_engine(new std::default_random_engine(temperature_interval_seeder));

    auto cancellation_source = pplx::cancellation_token_source();
    auto token = cancellation_source.get_token();
    auto temperature_events = pplx::do_while([&model, temperature_source_ids, temperature_interval_engine, &gate, token]
    {
        const auto temp_interval = std::uniform_real_distribution<>(0.5, 5.0)(*temperature_interval_engine);
        return pplx::complete_after(std::chrono::milliseconds(std::chrono::milliseconds::rep(1000 * temp_interval)), token).then([&model, temperature_source_ids, &gate]
        {
            auto lock = model.write_lock();

            // make example temperature data ... \/\/\/\/ ... around 200
            const nmos::events_number value(175.0 + std::abs(nmos::tai_now().seconds % 100 - 50), 10);
            // i.e. 17.5-22.5 C

            for (const auto& temperature_source_id : temperature_source_ids)
            {
                modify_resource(model.events_resources, temperature_source_id, [&value](nmos::resource& resource)
                {
                    nmos::fields::endpoint_state(resource.data) = nmos::make_events_number_state(resource.id, value, temperature_Celsius);
                });
            }

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Temperature updated: " << value.scaled_value() << " (" << temperature_Celsius.name << ")";

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

// Example Connection API callback to parse "transport_file" during a PATCH /staged request
nmos::transport_file_parser make_node_implementation_transport_file_parser()
{
    // this example uses the default transport file parser explicitly
    // (if this callback is specified, an 'empty' std::function is not allowed)
    return &nmos::parse_rtp_transport_file;
}

// Example Connection API callback to to perform application-specific validation of the merged /staged endpoint during a PATCH /staged request
nmos::details::connection_resource_patch_validator make_node_implementation_patch_validator()
{
    // this example uses an 'empty' std::function because it does not need to do any validation
    // beyond what is expressed by the schemas and /constraints endpoint
    return{};
}

// Example Connection API activation callback to resolve "auto" values when /staged is transitioned to /active
nmos::connection_resource_auto_resolver make_node_implementation_auto_resolver(const nmos::settings& settings)
{
    using web::json::value;

    const auto seed_id = nmos::experimental::fields::seed_id(settings);
    const auto device_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/device/0"));
    const auto how_many = node_implementation_field_how_many(settings);
    const auto suffixes = boost::irange(0, how_many * 2, 2);
    const auto sender_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/sender/"), suffixes);
    const auto receiver_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/receiver/"), suffixes);
    const auto temperature_suffixes = boost::irange(1, how_many * 2 + 1, 2);
    const auto temperature_ws_sender_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/sender/"), temperature_suffixes);
    const auto temperature_ws_sender_uri = nmos::make_events_ws_api_connection_uri(device_id, settings);
    const auto temperature_ws_receiver_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/receiver/"), temperature_suffixes);

    // although which properties may need to be defaulted depends on the resource type,
    // the default value will almost always be different for each resource
    return [sender_ids, receiver_ids, temperature_ws_sender_ids, temperature_ws_sender_uri, temperature_ws_receiver_ids](const nmos::resource& resource, const nmos::resource& connection_resource, value& transport_params)
    {
        const std::pair<nmos::id, nmos::type> id_type{ connection_resource.id, connection_resource.type };
        // this code relies on the specific constraints added by nmos_implementation_thread
        const auto& constraints = nmos::fields::endpoint_constraints(connection_resource.data);

        // "In some cases the behaviour is more complex, and may be determined by the vendor."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
        if (sender_ids.end() != boost::range::find(sender_ids, id_type.first))
        {
            nmos::details::resolve_auto(transport_params[0], nmos::fields::source_ip, [&] { return web::json::front(nmos::fields::constraint_enum(constraints.at(0).at(nmos::fields::source_ip))); });
            nmos::details::resolve_auto(transport_params[1], nmos::fields::source_ip, [&] { return web::json::back(nmos::fields::constraint_enum(constraints.at(1).at(nmos::fields::source_ip))); });
            nmos::details::resolve_auto(transport_params[0], nmos::fields::destination_ip, [] { return value::string(U("239.255.255.0")); });
            nmos::details::resolve_auto(transport_params[1], nmos::fields::destination_ip, [] { return value::string(U("239.255.255.1")); });
            // lastly, apply the specification defaults for any properties not handled above
            nmos::resolve_rtp_auto(id_type.second, transport_params);
        }
        else if (receiver_ids.end() != boost::range::find(receiver_ids, id_type.first))
        {
            nmos::details::resolve_auto(transport_params[0], nmos::fields::interface_ip, [&] { return web::json::front(nmos::fields::constraint_enum(constraints.at(0).at(nmos::fields::interface_ip))); });
            nmos::details::resolve_auto(transport_params[1], nmos::fields::interface_ip, [&] { return web::json::back(nmos::fields::constraint_enum(constraints.at(1).at(nmos::fields::interface_ip))); });
            // lastly, apply the specification defaults for any properties not handled above
            nmos::resolve_rtp_auto(id_type.second, transport_params);
        }
        else if (temperature_ws_sender_ids.end() != boost::range::find(temperature_ws_sender_ids, id_type.first))
        {
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_uri, [&] { return value::string(temperature_ws_sender_uri.to_string()); });
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_authorization, [&] { return value::boolean(false); });
        }
        else if (temperature_ws_receiver_ids.end() != boost::range::find(temperature_ws_receiver_ids, id_type.first))
        {
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_authorization, [&] { return value::boolean(false); });
        }
    };
}

// Example Connection API activation callback to update senders' /transportfile endpoint - captures node_resources by reference!
nmos::connection_sender_transportfile_setter make_node_implementation_transportfile_setter(const nmos::resources& node_resources, const nmos::settings& settings)
{
    using web::json::value;

    const auto seed_id = nmos::experimental::fields::seed_id(settings);
    const auto node_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/self"));
    const auto how_many = node_implementation_field_how_many(settings);
    const auto suffixes = boost::irange(0, how_many * 2, 2);
    const auto source_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/source/"), suffixes);
    const auto flow_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/flow/"), suffixes);
    const auto sender_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/sender/"), suffixes);

    // as part of activation, the example sender /transportfile should be updated based on the active transport parameters
    return [&node_resources, node_id, source_ids, flow_ids, sender_ids](const nmos::resource& sender, const nmos::resource& connection_sender, value& endpoint_transportfile)
    {
        const uint32_t i = boost::range::find(sender_ids, connection_sender.id) - sender_ids.begin();
        if (sender_ids.size() != i)
        {
            const auto& source_id = source_ids[i];
            const auto& flow_id = flow_ids[i];

            // note, model mutex is already locked by the calling thread, so access to node_resources is OK...
            auto node = nmos::find_resource(node_resources, { node_id, nmos::types::node });
            auto source = nmos::find_resource(node_resources, { source_id, nmos::types::source });
            auto flow = nmos::find_resource(node_resources, { flow_id, nmos::types::flow });
            if (node_resources.end() == node || node_resources.end() == source || node_resources.end() == flow)
            {
                throw std::logic_error("matching IS-04 node, source or flow not found");
            }

            auto sdp_params = nmos::make_sdp_parameters(node->data, source->data, flow->data, sender.data, { U("PRIMARY"), U("SECONDARY") });
            auto& transport_params = nmos::fields::transport_params(nmos::fields::endpoint_active(connection_sender.data));
            auto session_description = nmos::make_session_description(sdp_params, transport_params);
            auto sdp = utility::s2us(sdp::make_session_description(session_description));
            endpoint_transportfile = nmos::make_connection_rtp_sender_transportfile(sdp);
        }
    };
}

// Example Connection API activation callback to perform application-specific operations to complete activation
nmos::connection_activation_handler make_node_implementation_activation_handler(nmos::node_model& model, slog::base_gate& gate)
{
    // this example uses this callback to (un)subscribe a IS-07 Events WebSocket receiver when it is activated
    // and, in addition to the message handler, specifies the optional close handler in order that any subsequent
    // connection errors are reflected into the /active endpoint by setting master_enable to false
    auto handle_events_ws_message = make_node_implementation_events_ws_message_handler(model, gate);
    auto handle_close = nmos::experimental::make_events_ws_close_handler(model, gate);
    return nmos::make_connection_events_websocket_activation_handler(handle_events_ws_message, handle_close, model.settings, gate);
}

// Example Events WebSocket API client message handler
nmos::events_ws_message_handler make_node_implementation_events_ws_message_handler(const nmos::node_model& model, slog::base_gate& gate)
{
    const auto seed_id = nmos::experimental::fields::seed_id(model.settings);
    const auto how_many = node_implementation_field_how_many(model.settings);
    const auto temperature_suffixes = boost::irange(1, how_many * 2 + 1, 2);
    const auto temperature_ws_receiver_ids = make_repeatable_ids(seed_id, U("/x-nmos/node/receiver/"), temperature_suffixes);

    // the message handler will be used for all Events WebSocket connections, and each connection may potentially
    // have subscriptions to a number of sources, for multiple receivers, so this example uses a handler adaptor
    // that enables simple processing of "state" messages (events) per receiver
    return nmos::experimental::make_events_ws_message_handler(model, [temperature_ws_receiver_ids, &gate](const nmos::resource& receiver, const nmos::resource& connection_receiver, const web::json::value& message)
    {
        const uint32_t i = boost::range::find(temperature_ws_receiver_ids, connection_receiver.id) - temperature_ws_receiver_ids.begin();
        if (temperature_ws_receiver_ids.size() != i)
        {
            const auto event_type = nmos::event_type(nmos::fields::state_event_type(message));
            const auto& payload = nmos::fields::state_payload(message);
            const nmos::events_number value(nmos::fields::payload_number_value(payload).to_double(), nmos::fields::payload_number_scale(payload));

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Temperature received: " << value.scaled_value() << " (" << event_type.name << ")";
        }
    }, gate);
}

// generate repeatable ids for a range of resources
template <typename Suffixes>
std::vector<nmos::id> make_repeatable_ids(const nmos::id& seed_id, const utility::string_t& name_prefix, Suffixes name_suffixes)
{
    typedef decltype(*std::begin(name_suffixes)) value_type;
    return boost::copy_range<std::vector<nmos::id>>(name_suffixes | boost::adaptors::transformed([&](const value_type& name_suffix)
    {
        return nmos::make_repeatable_id(seed_id, name_prefix + utility::conversions::details::to_string_t(name_suffix));
    }));
}

// add a helpful suffix to the label of a sub-resource for the example node
template <typename Suffix>
void set_node_implementation_label(nmos::resource& resource, Suffix suffix)
{
    using web::json::value;

    auto label = nmos::fields::label(resource.data);
    if (!label.empty()) label += U('/');
    label += resource.type.name + U('/') + utility::conversions::details::to_string_t(suffix);
    resource.data[nmos::fields::label] = resource.data[nmos::fields::description] = value::string(label);
}

// add an example "natural grouping" hint to a sender or receiver
template <typename Suffix>
void insert_node_implementation_group_hint(nmos::resource& resource, Suffix suffix)
{
    web::json::push_back(resource.data[nmos::fields::tags][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), resource.type.name + U(' ') + utility::conversions::details::to_string_t(suffix) }));
}

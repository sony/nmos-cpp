#include "node_implementation.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_first_of.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/join.hpp>
#include "pplx/pplx_utils.h" // for pplx::complete_after, etc.
#include "cpprest/host_utils.h"
#ifdef HAVE_LLDP
#include "lldp/lldp_manager.h"
#endif
#include "nmos/activation_mode.h"
#include "nmos/capabilities.h"
#include "nmos/channels.h"
#include "nmos/channelmapping_resources.h"
#include "nmos/clock_name.h"
#include "nmos/colorspace.h"
#include "nmos/configuration_handlers.h"
#include "nmos/configuration_methods.h"
#include "nmos/configuration_resources.h"
#include "nmos/configuration_utils.h"
#include "nmos/connection_resources.h"
#include "nmos/connection_events_activation.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/events_resources.h"
#include "nmos/format.h"
#include "nmos/group_hint.h"
#include "nmos/interlace_mode.h"
#include "nmos/is12_versions.h"  // for IS-12 gain control
#ifdef HAVE_LLDP
#include "nmos/lldp_manager.h"
#endif
#include "nmos/media_type.h"
#include "nmos/model.h"
#include "nmos/node_interfaces.h"
#include "nmos/node_resource.h"
#include "nmos/node_resources.h"
#include "nmos/node_server.h"
#include "nmos/random.h"
#include "nmos/sdp_utils.h"
#include "nmos/slog.h"
#include "nmos/st2110_21_sender_type.h"
#include "nmos/system_resources.h"
#include "nmos/transfer_characteristic.h"
#include "nmos/transport.h"
#include "nmos/video_jxsv.h"
#include "sdp/sdp.h"

// example node implementation details
namespace impl
{
    // custom logging category for the example node implementation thread
    namespace categories
    {
        const nmos::category node_implementation{ "node_implementation" };
    }

    // custom settings for the example node implementation
    namespace fields
    {
        // node_tags, device_tags: used in resource tags fields
        // "Each tag has a single key, but MAY have multiple values."
        // See https://specs.amwa.tv/is-04/releases/v1.3.2/docs/APIs_-_Common_Keys.html#tags
        // {
        //     "tag_1": [ "tag_1_value_1", "tag_1_value_2" ],
        //     "tag_2": [ "tag_2_value_1" ]
        // }
        const web::json::field_as_value_or node_tags{ U("node_tags"), web::json::value::object() };
        const web::json::field_as_value_or device_tags{ U("device_tags"), web::json::value::object() };

        // how_many: provides for very basic testing of a node with many sub-resources of each type
        const web::json::field_as_integer_or how_many{ U("how_many"), 1 };

        // activate_senders: controls whether to activate senders on start up (true, default) or not (false)
        const web::json::field_as_bool_or activate_senders{ U("activate_senders"), true };

        // senders, receivers: controls which kinds of sender and receiver are instantiated by the example node
        // the values must be an array of unique strings identifying the kinds of 'port', like ["v", "a", "d"], see impl::ports
        // when omitted, all ports are instantiated
        const web::json::field_as_value_or senders{ U("senders"), {} };
        const web::json::field_as_value_or receivers{ U("receivers"), {} };

        // frame_rate: controls the grain_rate of video, audio and ancillary data sources and flows
        // and the equivalent parameter constraint on video receivers
        // the value must be an object like { "numerator": 25, "denominator": 1 }
        // hm, unfortunately can't use nmos::make_rational(nmos::rates::rate25) during static initialization
        const web::json::field_as_value_or frame_rate{ U("frame_rate"), web::json::value_of({
            { nmos::fields::numerator, 25 },
            { nmos::fields::denominator, 1 }
        }) };

        // frame_width, frame_height: control the frame_width and frame_height of video flows
        const web::json::field_as_integer_or frame_width{ U("frame_width"), 1920 };
        const web::json::field_as_integer_or frame_height{ U("frame_height"), 1080 };

        // interlace_mode: controls the interlace_mode of video flows, see nmos::interlace_mode
        // when omitted, a default of "progressive" or "interlaced_tff" is used based on the frame_rate, etc.
        const web::json::field_as_string interlace_mode{ U("interlace_mode") };

        // colorspace: controls the colorspace of video flows, see nmos::colorspace
        const web::json::field_as_string_or colorspace{ U("colorspace"), U("BT709") };

        // transfer_characteristic: controls the transfer characteristic system of video flows, see nmos::transfer_characteristic
        const web::json::field_as_string_or transfer_characteristic{ U("transfer_characteristic"), U("SDR") };

        // color_sampling: controls the color (sub-)sampling mode of video flows, see sdp::sampling
        const web::json::field_as_string_or color_sampling{ U("color_sampling"), U("YCbCr-4:2:2") };

        // component_depth: controls the bits per component sample of video flows
        const web::json::field_as_integer_or component_depth{ U("component_depth"), 10 };

        // video_type: media type of video flows, e.g. "video/raw" or "video/jxsv", see nmos::media_types
        const web::json::field_as_string_or video_type{ U("video_type"), U("video/raw") };

        // channel_count: controls the number of channels in audio sources
        const web::json::field_as_integer_or channel_count{ U("channel_count"), 4 };

        // smpte2022_7: controls whether senders and receivers have one leg (false) or two legs (true, default)
        const web::json::field_as_bool_or smpte2022_7{ U("smpte2022_7"), true };

        // simulate_status_monitor_activity: when true status monitor statuses will change randomly after activation
        const web::json::field_as_bool_or simulate_status_monitor_activity{U("simulate_status_monitor_activity"), true};
    }

    nmos::interlace_mode get_interlace_mode(const nmos::settings& settings);

    // the different kinds of 'port' (standing for the format/media type/event type) implemented by the example node
    // each 'port' of the example node has a source, flow, sender and/or compatible receiver
    DEFINE_STRING_ENUM(port)
    namespace ports
    {
        // video/raw, video/jxsv, etc.
        const port video{ U("v") };
        // audio/L24
        const port audio{ U("a") };
        // video/smpte291
        const port data{ U("d") };
        // video/SMPTE2022-6
        const port mux{ U("m") };

        // example measurement event
        const port temperature{ U("t") };
        // example boolean event
        const port burn{ U("b") };
        // example string event
        const port nonsense{ U("s") };
        // example number/enum event
        const port catcall{ U("c") };

        const std::vector<port> rtp{ video, audio, data, mux };
        const std::vector<port> ws{ temperature, burn, nonsense, catcall };
        const std::vector<port> all{ boost::copy_range<std::vector<port>>(boost::range::join(rtp, ws)) };
    }

    bool is_rtp_port(const port& port);
    bool is_ws_port(const port& port);
    std::vector<port> parse_ports(const web::json::value& value);

    const std::vector<nmos::channel> channels_repeat{
        { U("Left Channel"), nmos::channel_symbols::L },
        { U("Right Channel"), nmos::channel_symbols::R },
        { U("Center Channel"), nmos::channel_symbols::C },
        { U("Low Frequency Effects Channel"), nmos::channel_symbols::LFE }
    };

    // find interface with the specified address
    std::vector<web::hosts::experimental::host_interface>::const_iterator find_interface(const std::vector<web::hosts::experimental::host_interface>& interfaces, const utility::string_t& address);

    // generate repeatable ids for the example node's resources
    nmos::id make_id(const nmos::id& seed_id, const nmos::type& type, const port& port = {}, int index = 0);
    std::vector<nmos::id> make_ids(const nmos::id& seed_id, const nmos::type& type, const port& port, int how_many = 1);
    std::vector<nmos::id> make_ids(const nmos::id& seed_id, const nmos::type& type, const std::vector<port>& ports, int how_many = 1);
    std::vector<nmos::id> make_ids(const nmos::id& seed_id, const std::vector<nmos::type>& types, const std::vector<port>& ports, int how_many = 1);

    // generate a repeatable source-specific multicast address for each leg of a sender
    utility::string_t make_source_specific_multicast_address_v4(const nmos::id& id, int leg = 0);

    // add a selection of parents to a source or flow
    void insert_parents(nmos::resource& resource, const nmos::id& seed_id, const port& port, int index);

    // add a helpful suffix to the label of a sub-resource for the example node
    void set_label_description(nmos::resource& resource, const port& port, int index);

    // add an example "natural grouping" hint to a sender or receiver
    void insert_group_hint(nmos::resource& resource, const port& port, int index);

    // specific event types used by the example node
    const auto temperature_Celsius = nmos::event_types::measurement(U("temperature"), U("C"));
    const auto temperature_wildcard = nmos::event_types::measurement(U("temperature"), nmos::event_types::wildcard);
    const auto catcall = nmos::event_types::named_enum(nmos::event_types::number, U("caterwaul"));

    // packet counters for the network interface controller
    struct nic_packet_counter
    {
        nmos::nc::counter lost_packet_counter;
        nmos::nc::counter late_packet_counter;
    };
    std::vector<nic_packet_counter> nic_packet_counters;


    // Example of audio sender control class descriptors. Used below for setting up a sender control class
    void
    make_audio_sender_descriptors(nmos::experimental::control_protocol_state & control_protocol_state,
                                  const web::json::value & sender_data, nmos::nc_class_id snd_class_id) {
        using web::json::value;
        using web::json::value_of;

        // Datatype description for an object within an object
        auto make_object_object_datatype = [](const std::vector<std::tuple<utility::string_t,utility::string_t,utility::string_t>> & obj_desc,
                                              const utility::string_t & descriptor, const utility::string_t & type_name) {
            using web::json::value;
            auto fields = value::array();
            for ( auto obj : obj_desc ) {
                auto data = nmos::nc::details::make_field_descriptor(std::get<0>(obj), std::get<1>(obj), std::get<2>(obj), true, false, value::null());
                web::json::push_back(fields, std::move(data));
            }
            auto ret = nmos::nc::details::make_datatype_descriptor_struct(descriptor, type_name, fields, value::null());
            return ret;
        };

        // Datatype description for an array within an object
        auto make_object_array_datatype = [](const utility::string_t & obj_descriptor, const utility::string_t & obj_type_name,
                                             const utility::string_t & descriptor, const utility::string_t & type_name) {
            using web::json::value;
            auto fields = value::array();
            auto data = nmos::nc::details::make_field_descriptor(obj_descriptor, obj_type_name, U("NcString"), false, true, value::null());
            web::json::push_back(fields, std::move(data));
            auto ret = nmos::nc::details::make_datatype_descriptor_struct(descriptor, type_name, fields, value::null());
            return ret;
        };
        std::vector<std::tuple<utility::string_t,utility::string_t,utility::string_t>> subs_array {
            {U("active"), U("active"), U("NcBoolean")},
            {U("receiver_id"), U("receiver_id"), U("NcUuid")}
        };
        auto subs_data_type = make_object_object_datatype(subs_array, U("SenderObjectType"), U("SenderObjectType"));
        control_protocol_state.insert(std::move(nmos::experimental::datatype_descriptor{ subs_data_type }));

        auto caps_data_type  = make_object_array_datatype(U("Media Type Array"), U("media_types"), U("SenderCapsType"), U("SenderCapsType"));
        control_protocol_state.insert(std::move(nmos::experimental::datatype_descriptor{ caps_data_type }));

        auto tags_data_type  = make_object_array_datatype(U("Hints Type Array"), U("urn:x-nmos:tag:grouphint/v1.0"), U("SenderTagsType"), U("SenderTagsType"));
        control_protocol_state.insert(std::move(nmos::experimental::datatype_descriptor{ tags_data_type }));

        auto array_data_type = nmos::nc::details::make_datatype_typedef(U("Array Type"), U("NcArray"), true, U("NcString"), value::null());
        control_protocol_state.insert(std::move(nmos::experimental::datatype_descriptor{ array_data_type }));

        const web::json::field_as_value  caps_property{ U("caps") };
        const web::json::field_as_string description_property{ U("description") };
        const web::json::field_as_string device_id_property{ U("device_id") };
        const web::json::field_as_string flow_id_property{ U("flow_id") };
        const web::json::field_as_string uuid_property{ U("resource_id") };
        const web::json::field_as_array  interface_bindings_property{ U("interface_bindings") };
        const web::json::field_as_string label_property{ U("label") };
        const web::json::field_as_string manifest_href_property{ U("manifest_href") };
        const web::json::field_as_array  subscription_property{ U("subscription") };
        const web::json::field_as_value  tags_property{ U("tags") };
        const web::json::field_as_string transport_property{ U("transport") };
        const web::json::field<nmos::tai> version_property{ U("version") };

        // Define property descriptors: required fields
        std::vector<web::json::value> snd_control_property_descriptors = {
            nmos::experimental::make_control_class_property_descriptor(U("description"), { 3, 1 }, description_property, U("NcString")),
            nmos::experimental::make_control_class_property_descriptor(U("resource_id"), { 3, 2 }, uuid_property, U("NcUuid")),
            nmos::experimental::make_control_class_property_descriptor(U("label"), { 3, 3 }, label_property, U("NcString")),
            nmos::experimental::make_control_class_property_descriptor(U("tags"), { 3, 4 }, tags_property, U("SenderTagsType")),
            nmos::experimental::make_control_class_property_descriptor(U("version"), { 3, 5 }, version_property, U("NcVersionCode")),
        };
        // Property descriptor for optional fields
     snd_control_property_descriptors.push_back(nmos::experimental::make_control_class_property_descriptor(U("caps"), { 3, 6 }, caps_property, U("SenderCapsType"), true, true, false, false, web::json::value::null()));
     snd_control_property_descriptors.push_back(nmos::experimental::make_control_class_property_descriptor(U("description"), { 3, 7 }, device_id_property, U("NcUuid"), true, true, false, false, web::json::value::null()));
     snd_control_property_descriptors.push_back(nmos::experimental::make_control_class_property_descriptor(U("flow_id"), { 3, 8 }, flow_id_property, U("NcUuid"), true, true, false, false, web::json::value::null()));
     snd_control_property_descriptors.push_back(nmos::experimental::make_control_class_property_descriptor(U("interface_bindings"), { 3, 9 }, interface_bindings_property, U("Array")));
  snd_control_property_descriptors.push_back(nmos::experimental::make_control_class_property_descriptor(U("manifest_href"), { 3, 10 }, manifest_href_property, U("NcUri"), true, true, false, false, web::json::value::null()));
 snd_control_property_descriptors.push_back(nmos::experimental::make_control_class_property_descriptor(U("subscription"), { 3, 11 }, subscription_property, U("SenderObjectType"), true, true, false, false, web::json::value::null()));
  snd_control_property_descriptors.push_back(nmos::experimental::make_control_class_property_descriptor(U("transport"), { 3, 12 }, transport_property, U("NcString"), true, true, false, false, web::json::value::null()));

        // method and event descriptors are defined by defaults in the function prototype, so only need to pass the property descriptors
        utility::string_t descriptor_description = utility::string_t(U("AudioSenderControl")) + utility::string_t(U(" control class descriptor"));
        auto sender_control_class_descriptor =
            nmos::experimental::make_control_class_descriptor(descriptor_description,
                                                              snd_class_id, U("AudioSenderControl"),
                                                              snd_control_property_descriptors);

        // Insert class descriptor into the nmos-cpp framework
        control_protocol_state.insert(std::move(sender_control_class_descriptor));
    }

    // Example of an audio sender control class  
    static nmos::control_protocol_resource
    make_audio_sender_control(const nmos::nc_class_id & snd_class_id,
                              const web::json::value & sender_data,
                              const nmos::nc_oid & oid,
                              const nmos::nc_oid & parent_oid,
                              const web::json::value touchpoint,
                              slog::base_gate & gate) {
        using web::json::value;
        using web::json::value_of;

        const web::json::field_as_value  caps_property{ U("caps") };
        const web::json::field_as_string description_property{ U("description") };
        const web::json::field_as_string device_id_property{ U("device_id") };
        const web::json::field_as_string flow_id_property{ U("flow_id") };
        const web::json::field_as_string uuid_property{ U("resource_id") };
        const web::json::field_as_array  interface_bindings_property{ U("interface_bindings") };
        const web::json::field_as_string label_property{ U("label") };
        const web::json::field_as_string manifest_href_property{ U("manifest_href") };
        const web::json::field_as_array  subscription_property{ U("subscription") };
        const web::json::field_as_value  tags_property{ U("tags") };
        const web::json::field_as_string transport_property{ U("transport") };
        const web::json::field<nmos::tai> version_property{ U("version") };

        auto snd_name = U("AudioSenderControl");
        // define a function for instantiating object instances of the class
        auto data = nmos::nc::details::make_worker(snd_class_id, oid,
                                                   true,
                                                   parent_oid,
                                                   snd_name,
                                                   web::json::value(snd_name),
                                                   U("Sender resource data"),
                                                   touchpoint,
                                                   web::json::value::null(),
                                                   true);

        // Required
        data[description_property] = sender_data.at(U("description"));
        data[uuid_property]        = sender_data.at(U("id"));
        data[label_property]       = sender_data.at(U("label"));
        data[tags_property]        = sender_data.at(U("tags"));
        data[version_property]     = sender_data.at(U("version"));
        // Optional
        data[caps_property]               = sender_data.has_field(U("caps")) ? sender_data.at(U("caps")) : web::json::value::null();
        data[device_id_property]          = sender_data.has_field(U("device_id")) ? sender_data.at(U("device_id")) : web::json::value::null();
        data[flow_id_property]            = sender_data.has_field(U("flow_id")) ? sender_data.at(U("flow_id")) : web::json::value::null();
        data[interface_bindings_property] = sender_data.has_field(U("interface_bindings")) ? sender_data.at(U("interface_bindings"))  : web::json::value::null();
        data[manifest_href_property]      = sender_data.has_field(U("manifest_href")) ? sender_data.at(U("manifest_href")) : web::json::value::null();
        data[subscription_property]       = sender_data.has_field(U("subscription")) ? sender_data.at(U("subscription")) : web::json::value::null();
        data[transport_property]          = sender_data.has_field(U("transport")) ? sender_data.at(U("transport")) : web::json::value::null();

        auto snd_control_resource = nmos::control_protocol_resource{ nmos::is12_versions::v1_0, nmos::types::nc_worker, std::move(data), true };
        return snd_control_resource;
    }

}

// forward declarations for node_implementation_thread
void node_implementation_init(nmos::node_model& model, nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);
void node_implementation_run(nmos::node_model& model, nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);
nmos::connection_resource_auto_resolver make_node_implementation_auto_resolver(const nmos::settings& settings);
nmos::connection_sender_transportfile_setter make_node_implementation_transportfile_setter(const nmos::resources& node_resources, const nmos::settings& settings);

struct node_implementation_init_exception {};

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// starts background tasks to emit regular events from the temperature event source, and then waits for shutdown.
void node_implementation_thread(nmos::node_model& model, nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate_)
{
    nmos::details::omanip_gate gate{ gate_, nmos::stash_category(impl::categories::node_implementation) };

    try
    {
        node_implementation_init(model, control_protocol_state, gate);
        node_implementation_run(model, control_protocol_state, gate);
    }
    catch (const node_implementation_init_exception&)
    {
        // node_implementation_init writes the log message
    }
    catch (const web::json::json_exception& e)
    {
        // most likely from incorrect value types in the command line settings
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();
    }
    catch (const std::system_error& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "System error: " << e.what() << " [" << e.code() << "]";
    }
    catch (const std::runtime_error& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error: " << e.what();
    }
    catch (const std::exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what();
    }
    catch (...)
    {
        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception";
    }
}

void node_implementation_init(nmos::node_model& model, nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
{
    using web::json::value;
    using web::json::value_from_elements;
    using web::json::value_of;

    auto lock = model.write_lock(); // in order to update the resources

    const auto seed_id = nmos::experimental::fields::seed_id(model.settings);
    const auto node_id = impl::make_id(seed_id, nmos::types::node);
    const auto device_id = impl::make_id(seed_id, nmos::types::device);
    const auto how_many = impl::fields::how_many(model.settings);
    const auto sender_ports = impl::parse_ports(impl::fields::senders(model.settings));
    const auto rtp_sender_ports = boost::copy_range<std::vector<impl::port>>(sender_ports | boost::adaptors::filtered(impl::is_rtp_port));
    const auto ws_sender_ports = boost::copy_range<std::vector<impl::port>>(sender_ports | boost::adaptors::filtered(impl::is_ws_port));
    const auto receiver_ports = impl::parse_ports(impl::fields::receivers(model.settings));
    const auto rtp_receiver_ports = boost::copy_range<std::vector<impl::port>>(receiver_ports | boost::adaptors::filtered(impl::is_rtp_port));
    const auto ws_receiver_ports = boost::copy_range<std::vector<impl::port>>(receiver_ports | boost::adaptors::filtered(impl::is_ws_port));
    const auto frame_rate = nmos::parse_rational(impl::fields::frame_rate(model.settings));
    const auto frame_width = impl::fields::frame_width(model.settings);
    const auto frame_height = impl::fields::frame_height(model.settings);
    const auto interlace_mode = impl::get_interlace_mode(model.settings);
    const auto colorspace = nmos::colorspace{ impl::fields::colorspace(model.settings) };
    const auto transfer_characteristic = nmos::transfer_characteristic{ impl::fields::transfer_characteristic(model.settings) };
    const auto sampling = sdp::sampling{ impl::fields::color_sampling(model.settings) };
    const auto bit_depth = impl::fields::component_depth(model.settings);
    const auto video_type = nmos::media_type{ impl::fields::video_type(model.settings) };
    const auto channel_count = impl::fields::channel_count(model.settings);
    const auto smpte2022_7 = impl::fields::smpte2022_7(model.settings);

    // for now, some typical values for video/jxsv, based on VSF TR-08:2022
    // see https://vsf.tv/download/technical_recommendations/VSF_TR-08_2022-04-20.pdf
    const auto profile = nmos::profiles::High444_12;
    const auto level = nmos::get_video_jxsv_level(frame_rate, frame_width, frame_height);
    const auto sublevel = nmos::sublevels::Sublev3bpp;
    const auto max_bits_per_pixel = 4.0; // min coding efficiency
    const auto bits_per_pixel = 2.0;
    const auto transport_bit_rate_factor = 1.05;

    // any delay between updates to the model resources is unnecessary unless for debugging purposes
    const unsigned int delay_millis{ 0 };

    // it is important that the model be locked before inserting, updating or deleting a resource
    // and that the the node behaviour thread be notified after doing so
    const auto insert_resource_after = [&model, &lock](unsigned int milliseconds, nmos::resources& resources, nmos::resource&& resource, slog::base_gate& gate)
    {
        if (nmos::details::wait_for(model.shutdown_condition, lock, bst::chrono::milliseconds(milliseconds), [&] { return model.shutdown; })) return false;

        const auto is_control_protocol_resource = [&resource]() { return nmos::types::all_nc.end() != std::find(nmos::types::all_nc.begin(), nmos::types::all_nc.end(), resource.type); };
        const std::pair<nmos::id, nmos::type> id_type{ resource.id, resource.type };
        const bool success = is_control_protocol_resource() ? nmos::nc::insert_resource(resources, std::move(resource)).second : insert_resource(resources, std::move(resource)).second;

        if (success)
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Updated model with " << id_type;
        else
            slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Model update error: " << id_type;

        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying node behaviour thread"; // and anyone else who cares...
        model.notify();

        return success;
    };

    // it is important that the model be locked before inserting, updating or deleting a resource
    // and that the node behaviour thread be notified after doing so
    const auto insert_root_after = [&model, insert_resource_after](unsigned int milliseconds, nmos::control_protocol_resource& root, slog::base_gate& gate)
    {
        std::function<void(nmos::resources& resources, nmos::control_protocol_resource& resource)> insert_resources;

        insert_resources = [&milliseconds, insert_resource_after, &insert_resources, &gate](nmos::resources& resources, nmos::control_protocol_resource& resource)
        {
            for (auto& resource_ : resource.resources)
            {
                insert_resources(resources, resource_);
                if (!insert_resource_after(milliseconds, resources, std::move(resource_), gate)) throw node_implementation_init_exception();
            }
            resource.resources.clear();
        };

        auto& resources = model.control_protocol_resources;

        insert_resources(resources, root);
        if (!insert_resource_after(milliseconds, resources, std::move(root), gate)) throw node_implementation_init_exception();
    };

    const auto resolve_auto = make_node_implementation_auto_resolver(model.settings);
    const auto set_transportfile = make_node_implementation_transportfile_setter(model.node_resources, model.settings);

    const auto clocks = web::json::value_of({ nmos::make_internal_clock(nmos::clock_names::clk0) });
    // filter network interfaces to those that correspond to the specified host_addresses
    const auto host_interfaces = nmos::get_host_interfaces(model.settings);
    const auto interfaces = nmos::experimental::node_interfaces(host_interfaces);

    // example node
    {
        auto node = nmos::make_node(node_id, clocks, nmos::make_node_interfaces(interfaces), model.settings);
        node.data[nmos::fields::tags] = impl::fields::node_tags(model.settings);
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(node), gate)) throw node_implementation_init_exception();
    }

#ifdef HAVE_LLDP
    // LLDP manager for advertising server identity, capabilities, and discovering neighbors on a local area network
    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting to configure LLDP";
    auto lldp_manager = nmos::experimental::make_lldp_manager(model, interfaces, true, gate);
    // hm, open may potentially throw?
    lldp::lldp_manager_guard lldp_manager_guard(lldp_manager);
#endif

    // prepare interface bindings for all senders and receivers
    const auto& host_address = nmos::fields::host_address(model.settings);
    // the interface corresponding to the host address is used for the example node's WebSocket senders and receivers
    const auto host_interface_ = impl::find_interface(host_interfaces, host_address);
    if (host_interfaces.end() == host_interface_)
    {
        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "No network interface corresponding to host_address?";
        throw node_implementation_init_exception();
    }
    const auto& host_interface = *host_interface_;
    // hmm, should probably add a custom setting to control the primary and secondary interfaces for the example node's RTP senders and receivers
    // rather than just picking the one(s) corresponding to the first and last of the specified host addresses
    const auto& primary_address = model.settings.has_field(nmos::fields::host_addresses) ? web::json::front(nmos::fields::host_addresses(model.settings)).as_string() : host_address;
    const auto& secondary_address = model.settings.has_field(nmos::fields::host_addresses) ? web::json::back(nmos::fields::host_addresses(model.settings)).as_string() : host_address;
    const auto primary_interface_ = impl::find_interface(host_interfaces, primary_address);
    const auto secondary_interface_ = impl::find_interface(host_interfaces, secondary_address);
    if (host_interfaces.end() == primary_interface_ || host_interfaces.end() == secondary_interface_)
    {
        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "No network interface corresponding to one of the host_addresses?";
        throw node_implementation_init_exception();
    }
    const auto& primary_interface = *primary_interface_;
    const auto& secondary_interface = *secondary_interface_;
    const auto interface_names = smpte2022_7
        ? std::vector<utility::string_t>{ primary_interface.name, secondary_interface.name }
        : std::vector<utility::string_t>{ primary_interface.name };

    // example device
    {
        auto sender_ids = impl::make_ids(seed_id, nmos::types::sender, rtp_sender_ports, how_many);
        if (0 <= nmos::fields::events_port(model.settings)) boost::range::push_back(sender_ids, impl::make_ids(seed_id, nmos::types::sender, ws_sender_ports, how_many));
        auto receiver_ids = impl::make_ids(seed_id, nmos::types::receiver, receiver_ports, how_many);
        auto device = nmos::make_device(device_id, node_id, sender_ids, receiver_ids, model.settings);
        device.data[nmos::fields::tags] = impl::fields::device_tags(model.settings);
        if (!insert_resource_after(delay_millis, model.node_resources, std::move(device), gate)) throw node_implementation_init_exception();
    }

    web::json::value audio_sender_for_control_protocol;
    // example sources, flows and senders
    for (int index = 0; index < how_many; ++index)
    {
        for (const auto& port : rtp_sender_ports)
        {
            const auto source_id = impl::make_id(seed_id, nmos::types::source, port, index);
            const auto flow_id = impl::make_id(seed_id, nmos::types::flow, port, index);
            const auto sender_id = impl::make_id(seed_id, nmos::types::sender, port, index);

            nmos::resource source;
            if (impl::ports::video == port)
            {
                source = nmos::make_video_source(source_id, device_id, nmos::clock_names::clk0, frame_rate, model.settings);
            }
            else if (impl::ports::audio == port)
            {
                const auto channels = boost::copy_range<std::vector<nmos::channel>>(boost::irange(0, channel_count) | boost::adaptors::transformed([&](const int& index)
                {
                    return impl::channels_repeat[index % (int)impl::channels_repeat.size()];
                }));

                source = nmos::make_audio_source(source_id, device_id, nmos::clock_names::clk0, frame_rate, channels, model.settings);
            }
            else if (impl::ports::data == port)
            {
                source = nmos::make_data_source(source_id, device_id, nmos::clock_names::clk0, frame_rate, model.settings);
            }
            else if (impl::ports::mux == port)
            {
                source = nmos::make_mux_source(source_id, device_id, nmos::clock_names::clk0, frame_rate, model.settings);
            }
            impl::insert_parents(source, seed_id, port, index);
            impl::set_label_description(source, port, index);

            nmos::resource flow;
            if (impl::ports::video == port)
            {
                if (nmos::media_types::video_raw == video_type)
                {
                    flow = nmos::make_raw_video_flow(
                        flow_id, source_id, device_id,
                        frame_rate,
                        frame_width, frame_height, interlace_mode,
                        colorspace, transfer_characteristic, sampling, bit_depth,
                        model.settings
                    );
                }
                else if (nmos::media_types::video_jxsv == video_type)
                {
                    flow = nmos::make_video_jxsv_flow(
                        flow_id, source_id, device_id,
                        frame_rate,
                        frame_width, frame_height, interlace_mode,
                        colorspace, transfer_characteristic, sampling, bit_depth,
                        profile, level, sublevel, bits_per_pixel,
                        model.settings
                    );
                }
                else
                {
                    flow = nmos::make_coded_video_flow(
                        flow_id, source_id, device_id,
                        frame_rate,
                        frame_width, frame_height, interlace_mode,
                        colorspace, transfer_characteristic, sampling, bit_depth,
                        video_type,
                        model.settings
                    );
                }
            }
            else if (impl::ports::audio == port)
            {
                flow = nmos::make_raw_audio_flow(flow_id, source_id, device_id, 48000, 24, model.settings);
                // add optional grain_rate
                flow.data[nmos::fields::grain_rate] = nmos::make_rational(frame_rate);
            }
            else if (impl::ports::data == port)
            {
                nmos::did_sdid timecode{ 0x60, 0x60 };
                flow = nmos::make_sdianc_data_flow(flow_id, source_id, device_id, { timecode }, model.settings);
                // add optional grain_rate
                flow.data[nmos::fields::grain_rate] = nmos::make_rational(frame_rate);
            }
            else if (impl::ports::mux == port)
            {
                flow = nmos::make_mux_flow(flow_id, source_id, device_id, model.settings);
                // add optional grain_rate
                flow.data[nmos::fields::grain_rate] = nmos::make_rational(frame_rate);
            }
            impl::insert_parents(flow, seed_id, port, index);
            impl::set_label_description(flow, port, index);

            // set_transportfile needs to find the matching source and flow for the sender, so insert these first
            if (!insert_resource_after(delay_millis, model.node_resources, std::move(source), gate)) throw node_implementation_init_exception();
            if (!insert_resource_after(delay_millis, model.node_resources, std::move(flow), gate)) throw node_implementation_init_exception();

            const auto manifest_href = nmos::experimental::make_manifest_api_manifest(sender_id, model.settings);
            auto sender = nmos::make_sender(sender_id, flow_id, nmos::transports::rtp, device_id, manifest_href.to_string(), interface_names, model.settings);
            // hm, could add nmos::make_video_jxsv_sender to encapsulate this?
            if (impl::ports::video == port && nmos::media_types::video_jxsv == video_type)
            {
                // additional attributes required by BCP-006-01
                // see https://specs.amwa.tv/bcp-006-01/branches/v1.0-dev/docs/NMOS_With_JPEG_XS.html#senders
                const auto format_bit_rate = nmos::get_video_jxsv_bit_rate(frame_rate, frame_width, frame_height, bits_per_pixel);
                // round to nearest Megabit/second per examples in VSF TR-08:2022
                const auto transport_bit_rate = uint64_t(transport_bit_rate_factor * format_bit_rate / 1e3 + 0.5) * 1000;
                sender.data[nmos::fields::bit_rate] = value(transport_bit_rate);
                sender.data[nmos::fields::st2110_21_sender_type] = value(nmos::st2110_21_sender_types::type_N.name);
            }
            else if (impl::ports::audio == port) {
                audio_sender_for_control_protocol = sender.data;
            }
            impl::set_label_description(sender, port, index);
            impl::insert_group_hint(sender, port, index);

            auto connection_sender = nmos::make_connection_rtp_sender(sender_id, smpte2022_7);
            // add some example constraints; these should be completed fully!
            connection_sender.data[nmos::fields::endpoint_constraints][0][nmos::fields::source_ip] = value_of({
                { nmos::fields::constraint_enum, value_from_elements(primary_interface.addresses) }
            });
            if (smpte2022_7) connection_sender.data[nmos::fields::endpoint_constraints][1][nmos::fields::source_ip] = value_of({
                { nmos::fields::constraint_enum, value_from_elements(secondary_interface.addresses) }
            });

            if (impl::fields::activate_senders(model.settings))
            {
                // initialize this sender with a scheduled activation, e.g. to enable the IS-05-01 test suite to run immediately
                auto& staged = connection_sender.data[nmos::fields::endpoint_staged];
                staged[nmos::fields::master_enable] = value::boolean(true);
                staged[nmos::fields::activation] = value_of({
                    { nmos::fields::mode, nmos::activation_modes::activate_scheduled_relative.name },
                    { nmos::fields::requested_time, U("0:0") },
                    { nmos::fields::activation_time, nmos::make_version() }
                });
            }

            if (!insert_resource_after(delay_millis, model.node_resources, std::move(sender), gate)) throw node_implementation_init_exception();
            if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_sender), gate)) throw node_implementation_init_exception();
        }
    }

    // example receivers
    for (int index = 0; index < how_many; ++index)
    {
        for (const auto& port : rtp_receiver_ports)
        {
            const auto receiver_id = impl::make_id(seed_id, nmos::types::receiver, port, index);

            nmos::resource receiver;
            if (impl::ports::video == port)
            {
                receiver = nmos::make_receiver(receiver_id, device_id, nmos::transports::rtp, interface_names, nmos::formats::video, { video_type }, model.settings);
                // add an example constraint set; these should be completed fully!
                if (nmos::media_types::video_raw == video_type)
                {
                    const auto interlace_modes = nmos::interlace_modes::progressive != interlace_mode
                        ? std::vector<utility::string_t>{ nmos::interlace_modes::interlaced_bff.name, nmos::interlace_modes::interlaced_tff.name, nmos::interlace_modes::interlaced_psf.name }
                        : std::vector<utility::string_t>{ nmos::interlace_modes::progressive.name };
                    receiver.data[nmos::fields::caps][nmos::fields::constraint_sets] = value_of({
                        value_of({
                            { nmos::caps::format::grain_rate, nmos::make_caps_rational_constraint({ frame_rate }) },
                            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({ frame_width }) },
                            { nmos::caps::format::frame_height, nmos::make_caps_integer_constraint({ frame_height }) },
                            { nmos::caps::format::interlace_mode, nmos::make_caps_string_constraint(interlace_modes) },
                            { nmos::caps::format::color_sampling, nmos::make_caps_string_constraint({ sampling.name }) }
                        })
                    });
                }
                else if (nmos::media_types::video_jxsv == video_type)
                {
                    // some of the parameter constraints recommended by BCP-006-01
                    // see https://specs.amwa.tv/bcp-006-01/branches/v1.0-dev/docs/NMOS_With_JPEG_XS.html#receivers
                    const auto max_format_bit_rate = nmos::get_video_jxsv_bit_rate(frame_rate, frame_width, frame_height, max_bits_per_pixel);
                    // round to nearest Megabit/second per examples in VSF TR-08:2022
                    const auto max_transport_bit_rate = uint64_t(transport_bit_rate_factor * max_format_bit_rate / 1e3 + 0.5) * 1000;

                    receiver.data[nmos::fields::caps][nmos::fields::constraint_sets] = value_of({
                        value_of({
                            { nmos::caps::format::profile, nmos::make_caps_string_constraint({ profile.name }) },
                            { nmos::caps::format::level, nmos::make_caps_string_constraint({ level.name }) },
                            { nmos::caps::format::sublevel, nmos::make_caps_string_constraint({ nmos::sublevels::Sublev3bpp.name, nmos::sublevels::Sublev4bpp.name }) },
                            { nmos::caps::format::bit_rate, nmos::make_caps_integer_constraint({}, nmos::no_minimum<int64_t>(), (int64_t)max_format_bit_rate) },
                            { nmos::caps::transport::bit_rate, nmos::make_caps_integer_constraint({}, nmos::no_minimum<int64_t>(), (int64_t)max_transport_bit_rate) },
                            { nmos::caps::transport::packet_transmission_mode, nmos::make_caps_string_constraint({ nmos::packet_transmission_modes::codestream.name }) }
                        })
                    });
                }
                receiver.data[nmos::fields::version] = receiver.data[nmos::fields::caps][nmos::fields::version] = value(nmos::make_version());
            }
            else if (impl::ports::audio == port)
            {
                receiver = nmos::make_audio_receiver(receiver_id, device_id, nmos::transports::rtp, interface_names, 24, model.settings);
                // add some example constraint sets; these should be completed fully!
                receiver.data[nmos::fields::caps][nmos::fields::constraint_sets] = value_of({
                    value_of({
                        { nmos::caps::format::channel_count, nmos::make_caps_integer_constraint({}, 1, channel_count) },
                        { nmos::caps::format::sample_rate, nmos::make_caps_rational_constraint({ { 48000, 1 } }) },
                        { nmos::caps::format::sample_depth, nmos::make_caps_integer_constraint({ 16, 24 }) },
                        { nmos::caps::transport::packet_time, nmos::make_caps_number_constraint({ 0.125 }) }
                    }),
                    value_of({
                        { nmos::caps::meta::preference, -1 },
                        { nmos::caps::format::channel_count, nmos::make_caps_integer_constraint({}, 1, (std::min)(8, channel_count)) },
                        { nmos::caps::format::sample_rate, nmos::make_caps_rational_constraint({ { 48000, 1 } }) },
                        { nmos::caps::format::sample_depth, nmos::make_caps_integer_constraint({ 16, 24 }) },
                        { nmos::caps::transport::packet_time, nmos::make_caps_number_constraint({ 1 }) }
                    })
                });
                receiver.data[nmos::fields::version] = receiver.data[nmos::fields::caps][nmos::fields::version] = value(nmos::make_version());
            }
            else if (impl::ports::data == port)
            {
                receiver = nmos::make_sdianc_data_receiver(receiver_id, device_id, nmos::transports::rtp, interface_names, model.settings);
                // add an example constraint set; these should be completed fully!
                receiver.data[nmos::fields::caps][nmos::fields::constraint_sets] = value_of({
                    value_of({
                        { nmos::caps::format::grain_rate, nmos::make_caps_rational_constraint({ frame_rate }) }
                    })
                });
                receiver.data[nmos::fields::version] = receiver.data[nmos::fields::caps][nmos::fields::version] = value(nmos::make_version());
            }
            else if (impl::ports::mux == port)
            {
                receiver = nmos::make_mux_receiver(receiver_id, device_id, nmos::transports::rtp, interface_names, model.settings);
                // add an example constraint set; these should be completed fully!
                receiver.data[nmos::fields::caps][nmos::fields::constraint_sets] = value_of({
                    value_of({
                        { nmos::caps::format::grain_rate, nmos::make_caps_rational_constraint({ frame_rate }) }
                    })
                });
                receiver.data[nmos::fields::version] = receiver.data[nmos::fields::caps][nmos::fields::version] = value(nmos::make_version());
            }
            impl::set_label_description(receiver, port, index);
            impl::insert_group_hint(receiver, port, index);

            auto connection_receiver = nmos::make_connection_rtp_receiver(receiver_id, smpte2022_7);
            // add some example constraints; these should be completed fully!
            connection_receiver.data[nmos::fields::endpoint_constraints][0][nmos::fields::interface_ip] = value_of({
                { nmos::fields::constraint_enum, value_from_elements(primary_interface.addresses) }
            });
            if (smpte2022_7) connection_receiver.data[nmos::fields::endpoint_constraints][1][nmos::fields::interface_ip] = value_of({
                { nmos::fields::constraint_enum, value_from_elements(secondary_interface.addresses) }
            });

            resolve_auto(receiver, connection_receiver, connection_receiver.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

            if (!insert_resource_after(delay_millis, model.node_resources, std::move(receiver), gate)) throw node_implementation_init_exception();
            if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_receiver), gate)) throw node_implementation_init_exception();
        }
    }

    // example event sources, flows and senders
    for (int index = 0; 0 <= nmos::fields::events_port(model.settings) && index < how_many; ++index)
    {
        for (const auto& port : ws_sender_ports)
        {
            const auto source_id = impl::make_id(seed_id, nmos::types::source, port, index);
            const auto flow_id = impl::make_id(seed_id, nmos::types::flow, port, index);
            const auto sender_id = impl::make_id(seed_id, nmos::types::sender, port, index);

            nmos::event_type event_type;
            web::json::value events_type;
            web::json::value events_state;
            if (impl::ports::temperature == port)
            {
                event_type = impl::temperature_Celsius;

                // see https://specs.amwa.tv/is-07/releases/v1.0.1/docs/3.0._Event_types.html#231-measurements
                // and https://specs.amwa.tv/is-07/releases/v1.0.1/examples/eventsapi-type-number-measurement-get-200.html
                // and https://specs.amwa.tv/is-07/releases/v1.0.1/examples/eventsapi-state-number-measurement-get-200.html
                events_type = nmos::make_events_number_type({ -200, 10 }, { 1000, 10 }, { 1, 10 }, U("C"));
                events_state = nmos::make_events_number_state({ source_id, flow_id }, { 201, 10 }, event_type);
            }
            else if (impl::ports::burn == port)
            {
                event_type = nmos::event_types::boolean;

                // see https://specs.amwa.tv/is-07/releases/v1.0.1/docs/3.0._Event_types.html#21-boolean
                events_type = nmos::make_events_boolean_type();
                events_state = nmos::make_events_boolean_state({ source_id, flow_id }, false);
            }
            else if (impl::ports::nonsense == port)
            {
                event_type = nmos::event_types::string;

                // see https://specs.amwa.tv/is-07/releases/v1.0.1/docs/3.0._Event_types.html#22-string
                // and of course, https://en.wikipedia.org/wiki/Metasyntactic_variable
                events_type = nmos::make_events_string_type(0, 0, U("^foo|bar|baz|qu+x$"));
                events_state = nmos::make_events_string_state({ source_id, flow_id }, U("foo"));
            }
            else if (impl::ports::catcall == port)
            {
                event_type = impl::catcall;

                // see https://specs.amwa.tv/is-07/releases/v1.0.1/docs/3.0._Event_types.html#3-enum
                events_type = nmos::make_events_number_enum_type({
                    { 1, { U("meow"), U("chatty") } },
                    { 2, { U("purr"), U("happy") } },
                    { 4, { U("hiss"), U("afraid") } },
                    { 8, { U("yowl"), U("sonorous") } }
                });
                events_state = nmos::make_events_number_state({ source_id, flow_id }, 1, event_type);
            }

            // grain_rate is not set because these events are aperiodic
            auto source = nmos::make_data_source(source_id, device_id, {}, event_type, model.settings);
            impl::set_label_description(source, port, index);

            auto events_source = nmos::make_events_source(source_id, events_state, events_type);

            auto flow = nmos::make_json_data_flow(flow_id, source_id, device_id, event_type, model.settings);
            impl::set_label_description(flow, port, index);

            auto sender = nmos::make_sender(sender_id, flow_id, nmos::transports::websocket, device_id, {}, { host_interface.name }, model.settings);
            impl::set_label_description(sender, port, index);
            impl::insert_group_hint(sender, port, index);

            // initialize this sender enabled, just to enable the IS-07-02 test suite to run immediately
            auto connection_sender = nmos::make_connection_events_websocket_sender(sender_id, device_id, source_id, model.settings);
            connection_sender.data[nmos::fields::endpoint_active][nmos::fields::master_enable] = connection_sender.data[nmos::fields::endpoint_staged][nmos::fields::master_enable] = value::boolean(true);
            resolve_auto(sender, connection_sender, connection_sender.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);
            nmos::set_resource_subscription(sender, nmos::fields::master_enable(connection_sender.data[nmos::fields::endpoint_active]), {}, nmos::tai_now());

            if (!insert_resource_after(delay_millis, model.node_resources, std::move(source), gate)) throw node_implementation_init_exception();
            if (!insert_resource_after(delay_millis, model.node_resources, std::move(flow), gate)) throw node_implementation_init_exception();
            if (!insert_resource_after(delay_millis, model.node_resources, std::move(sender), gate)) throw node_implementation_init_exception();
            if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_sender), gate)) throw node_implementation_init_exception();
            if (!insert_resource_after(delay_millis, model.events_resources, std::move(events_source), gate)) throw node_implementation_init_exception();
        }
    }

    // example event receivers
    for (int index = 0; index < how_many; ++index)
    {
        for (const auto& port : ws_receiver_ports)
        {
            const auto receiver_id = impl::make_id(seed_id, nmos::types::receiver, port, index);

            nmos::event_type event_type;
            if (impl::ports::temperature == port)
            {
                // accept e.g. "number/temperature/F" or "number/temperature/K" as well as "number/temperature/C"
                event_type = impl::temperature_wildcard;
            }
            else if (impl::ports::burn == port)
            {
                // accept any boolean
                event_type = nmos::event_types::wildcard(nmos::event_types::boolean);
            }
            else if (impl::ports::nonsense == port)
            {
                // accept any string
                event_type = nmos::event_types::wildcard(nmos::event_types::string);
            }
            else if (impl::ports::catcall == port)
            {
                // accept only a catcall
                event_type = impl::catcall;
            }

            auto receiver = nmos::make_data_receiver(receiver_id, device_id, nmos::transports::websocket, { host_interface.name }, nmos::media_types::application_json, { event_type }, model.settings);
            impl::set_label_description(receiver, port, index);
            impl::insert_group_hint(receiver, port, index);

            auto connection_receiver = nmos::make_connection_events_websocket_receiver(receiver_id, model.settings);
            resolve_auto(receiver, connection_receiver, connection_receiver.data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

            if (!insert_resource_after(delay_millis, model.node_resources, std::move(receiver), gate)) throw node_implementation_init_exception();
            if (!insert_resource_after(delay_millis, model.connection_resources, std::move(connection_receiver), gate)) throw node_implementation_init_exception();
        }
    }

    // example channelmapping resources demonstrating a range of input/output capabilities
    // see https://github.com/sony/nmos-cpp/issues/111#issuecomment-740613137

    // example audio inputs
    const bool channelmapping_receivers = 0 <= nmos::fields::channelmapping_port(model.settings) && rtp_receiver_ports.end() != boost::range::find(rtp_receiver_ports, impl::ports::audio);
    for (int index = 0; channelmapping_receivers && index < how_many; ++index)
    {
        const auto stri = utility::conversions::details::to_string_t(index);

        const auto id = U("input") + stri;

        const auto name = U("IP Input ") + stri;
        const auto description = U("SMPTE 2110-30 IP Input ") + stri;

        const auto receiver_id = impl::make_id(seed_id, nmos::types::receiver, impl::ports::audio, index);
        const auto parent = std::pair<nmos::id, nmos::type>(receiver_id, nmos::types::receiver);

        const auto channel_labels = boost::copy_range<std::vector<utility::string_t>>(boost::irange(0, channel_count) | boost::adaptors::transformed([&](const int& index)
        {
            return impl::channels_repeat[index % (int)impl::channels_repeat.size()].label;
        }));

        // use default input capabilities to indicate no constraints
        auto channelmapping_input = nmos::make_channelmapping_input(id, name, description, parent, channel_labels);
        if (!insert_resource_after(delay_millis, model.channelmapping_resources, std::move(channelmapping_input), gate)) throw node_implementation_init_exception();
    }

    // example audio outputs
    const bool channelmapping_senders = 0 <= nmos::fields::channelmapping_port(model.settings) && rtp_sender_ports.end() != boost::range::find(rtp_sender_ports, impl::ports::audio);
    for (int index = 0; channelmapping_senders && index < how_many; ++index)
    {
        const auto stri = utility::conversions::details::to_string_t(index);

        const auto id = U("output") + stri;

        const auto name = U("IP Output ") + stri;
        const auto description = U("SMPTE 2110-30 IP Output ") + stri;

        const auto source_id = impl::make_id(seed_id, nmos::types::source, impl::ports::audio, index);

        const auto channel_labels = boost::copy_range<std::vector<utility::string_t>>(boost::irange(0, channel_count) | boost::adaptors::transformed([&](const int& index)
        {
            return impl::channels_repeat[index % (int)impl::channels_repeat.size()].label;
        }));

        // omit routable inputs to indicate no restrictions
        auto channelmapping_output = nmos::make_channelmapping_output(id, name, description, source_id, channel_labels);
        if (!insert_resource_after(delay_millis, model.channelmapping_resources, std::move(channelmapping_output), gate)) throw node_implementation_init_exception();
    }

    const int input_block_size = 8;
    const int input_block_count = 8;

    // example non-IP audio input
    if (0 <= nmos::fields::channelmapping_port(model.settings))
    {
        const auto id = U("inputA");

        const auto name = U("MADI Input A");
        const auto description = U("MADI Input A");

        // non-IP audio inputs have no parent
        const auto parent = std::pair<nmos::id, nmos::type>();

        const auto channel_labels = boost::copy_range<std::vector<utility::string_t>>(boost::irange(0, input_block_size * input_block_count) | boost::adaptors::transformed([](const int& index)
        {
            return nmos::channel_symbols::Undefined(1 + index).name;
        }));

        // some example constraints; this input's channels can only be used in blocks and the channels cannot be reordered within each block
        const auto reordering = false;
        const auto block_size = input_block_size;

        auto channelmapping_input = nmos::make_channelmapping_input(id, name, description, parent, channel_labels, reordering, block_size);
        if (!insert_resource_after(delay_millis, model.channelmapping_resources, std::move(channelmapping_input), gate)) throw node_implementation_init_exception();
    }

    // example outputs to some audio gizmo
    if (0 <= nmos::fields::channelmapping_port(model.settings))
    {
        const auto id = U("outputX");

        const auto name = U("Gizmo Output X");
        const auto description = U("Gizmo Output X");

        const auto source_id = impl::make_id(seed_id, nmos::types::source, impl::ports::audio, how_many);

        const auto channel_labels = boost::copy_range<std::vector<utility::string_t>>(boost::irange(0, input_block_size) | boost::adaptors::transformed([](const int& index)
        {
            return nmos::channel_symbols::Undefined(1 + index).name;
        }));

        // some example constraints; only allow inputs from the example non-IP audio input
        auto routable_inputs = std::vector<nmos::channelmapping_id>{ U("inputA") };
        // do not allow unrouted channels

        // start with a valid active map
        auto active_map = boost::copy_range<std::vector<std::pair<nmos::channelmapping_id, uint32_t>>>(boost::irange(0, input_block_size) | boost::adaptors::transformed([](const int& index)
        {
            return std::pair<nmos::channelmapping_id, uint32_t>{ U("inputA"), index };
        }));

        auto channelmapping_output = nmos::make_channelmapping_output(id, name, description, source_id, channel_labels, routable_inputs, active_map);
        if (!insert_resource_after(delay_millis, model.channelmapping_resources, std::move(channelmapping_output), gate)) throw node_implementation_init_exception();
    }

    // example source for some audio gizmo
    if (0 <= nmos::fields::channelmapping_port(model.settings))
    {
        const auto source_id = impl::make_id(seed_id, nmos::types::source, impl::ports::audio, how_many);

        const auto channels = boost::copy_range<std::vector<nmos::channel>>(boost::irange(0, input_block_size) | boost::adaptors::transformed([](const int& index)
        {
            return nmos::channel{ {}, nmos::channel_symbols::Undefined(1 + index) };
        }));

        auto source = nmos::make_audio_source(source_id, device_id, nmos::clock_names::clk0, frame_rate, channels, model.settings);
        impl::set_label_description(source, impl::ports::audio, how_many);

        if (!insert_resource_after(delay_millis, model.node_resources, std::move(source), gate)) throw node_implementation_init_exception();
    }

    // example inputs from some audio gizmo
    if (0 <= nmos::fields::channelmapping_port(model.settings))
    {
        const auto id = U("inputX");

        const auto name = U("Gizmo Input X");
        const auto description = U("Gizmo Input X");

        // the audio gizmo is re-entrant
        const auto source_id = impl::make_id(seed_id, nmos::types::source, impl::ports::audio, how_many);
        const auto parent = std::pair<nmos::id, nmos::type>(source_id, nmos::types::source);

        const auto channel_labels = boost::copy_range<std::vector<utility::string_t>>(boost::irange(0, input_block_size) | boost::adaptors::transformed([](const int& index)
        {
            return nmos::channel_symbols::Undefined(1 + index).name;
        }));

        // this input is weird, it is block-based but allows reordering of channels within a block
        const auto reordering = true;
        const auto block_size = 2;

        auto channelmapping_input = nmos::make_channelmapping_input(id, name, description, parent, channel_labels, reordering, block_size);
        if (!insert_resource_after(delay_millis, model.channelmapping_resources, std::move(channelmapping_input), gate)) throw node_implementation_init_exception();
    }

    // example non-ST 2110-30 audio output
    if (0 <= nmos::fields::channelmapping_port(model.settings))
    {
        const auto id = U("outputB");

        const auto name = U("AES Output B");
        const auto description = U("AES Output B");

        // non-IP audio outputs have no sourceid
        const auto source_id = nmos::id();

        const auto channel_labels = boost::copy_range<std::vector<utility::string_t>>(nmos::channel_symbols::ST | boost::adaptors::transformed([](const nmos::channel_symbol& symbol)
        {
            return symbol.name;
        }));

        // allow inputs from the audio gizmo
        auto routable_inputs = std::vector<nmos::channelmapping_id>{ U("inputX") };
        // allow unrouted channels
        routable_inputs.push_back({});

        auto channelmapping_output = nmos::make_channelmapping_output(id, name, description, source_id, channel_labels, routable_inputs);
        if (!insert_resource_after(delay_millis, model.channelmapping_resources, std::move(channelmapping_output), gate)) throw node_implementation_init_exception();
    }

    // examples of using IS-12 control protocol
    // they are based on the NC-DEVICE-MOCK
    // See https://specs.amwa.tv/nmos-device-control-mock/#about-nc-device-mock
    // See https://github.com/AMWA-TV/nmos-device-control-mock/blob/main/code/src/NCModel/Features.ts
    if (0 <= nmos::fields::control_protocol_ws_port(model.settings))
    {
        // example to create a non-standard Gain control class
        const auto gain_control_class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 1 });
        const web::json::field_as_number gain_value{ U("gainValue") };
        {
            // Gain control class property descriptors
            std::vector<web::json::value> gain_control_property_descriptors = { nmos::experimental::make_control_class_property_descriptor(U("Gain value"), { 3, 1 }, gain_value, U("NcFloat32")) };

            // create Gain control class descriptor
            auto gain_control_class_descriptor = nmos::experimental::make_control_class_descriptor(U("Gain control class descriptor"), gain_control_class_id, U("GainControl"), gain_control_property_descriptors);

            // insert Gain control class descriptor to global state, which will be used by the control_protocol_ws_message_handler to process incoming ws message
            control_protocol_state.insert(gain_control_class_descriptor);
        }
        // helper function to create Gain control instance
        auto make_gain_control = [&gain_value, &gain_control_class_id](nmos::nc_oid oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, float gain)
        {
            auto data = nmos::nc::details::make_worker(gain_control_class_id, oid, true, owner, role, value::string(user_label), description, touchpoints, runtime_property_constraints, true);
            data[gain_value] = value::number(gain);

            return nmos::control_protocol_resource{ nmos::is12_versions::v1_0, nmos::types::nc_worker, std::move(data), true };
        };

        // example to create a non-standard Example control class
        const auto example_control_class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 2 });
        const web::json::field_as_number enum_property{ U("enumProperty") };
        const web::json::field_as_string string_property{ U("stringProperty") };
        const web::json::field_as_number number_property{ U("numberProperty") };
        const web::json::field_as_number deprecated_number_property{ U("deprecatedNumberProperty") };
        const web::json::field_as_bool boolean_property{ U("booleanProperty") };
        const web::json::field_as_value object_property{ U("objectProperty") };
        const web::json::field_as_number method_no_args_count{ U("methodNoArgsCount") };
        const web::json::field_as_number method_simple_args_count{ U("methodSimpleArgsCount") };
        const web::json::field_as_number method_object_arg_count{ U("methodObjectArgCount") };
        const web::json::field_as_array string_sequence{ U("stringSequence") };
        const web::json::field_as_array boolean_sequence{ U("booleanSequence") };
        const web::json::field_as_array enum_sequence{ U("enumSequence") };
        const web::json::field_as_array number_sequence{ U("numberSequence") };
        const web::json::field_as_array object_sequence{ U("objectSequence") };
        const web::json::field_as_number enum_arg{ U("enumArg") };
        const web::json::field_as_string string_arg{ U("stringArg") };
        const web::json::field_as_number number_arg{ U("numberArg") };
        const web::json::field_as_bool boolean_arg{ U("booleanArg") };
        const web::json::field_as_value obj_arg{ U("objArg") };
        enum example_enum
        {
            Undefined = 0,
            Alpha = 1,
            Beta = 2,
            Gamma = 3
        };
        {
            // following constraints are used for the example control class level 0 datatype, level 1 property constraints and the method parameters constraints
            auto make_string_example_argument_constraints = []() {return nmos::nc::details::make_parameter_constraints_string(10, U("^[a-z]+$")); };
            auto make_number_example_argument_constraints = []() {return nmos::nc::details::make_parameter_constraints_number(0, 1000, 1); };

            // Example control class property descriptors
            std::vector<web::json::value> example_control_property_descriptors = {
                nmos::experimental::make_control_class_property_descriptor(U("Example enum property"), { 3, 1 }, enum_property, U("ExampleEnum")),
                // create "Example string property" with level 1: property constraints, See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
                // use nmos::nc::details::make_parameter_constraints_string to create property constraints
                nmos::experimental::make_control_class_property_descriptor(U("Example string property"), { 3, 2 }, string_property, U("NcString"), false, false, false, false, make_string_example_argument_constraints()),
                // create "Example numeric property" with level 1: property constraints, See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
                // use nmos::nc::details::make_parameter_constraints_number to create property constraints
                nmos::experimental::make_control_class_property_descriptor(U("Example numeric property"), { 3, 3 }, number_property, U("NcUint64"), false, false, false, false, make_number_example_argument_constraints()),
                nmos::experimental::make_control_class_property_descriptor(U("Example deprecated numeric property"), { 3, 4 }, deprecated_number_property, U("NcUint64"), false, false, false, true, make_number_example_argument_constraints()),
                nmos::experimental::make_control_class_property_descriptor(U("Example boolean property"), { 3, 5 }, boolean_property, U("NcBoolean")),
                nmos::experimental::make_control_class_property_descriptor(U("Example object property"), { 3, 6 }, object_property, U("ExampleDataType")),
                nmos::experimental::make_control_class_property_descriptor(U("Example method no args invoke counter"), { 3, 7 }, method_no_args_count, U("NcUint64"), true),
                nmos::experimental::make_control_class_property_descriptor(U("Example method simple args invoke counter"), { 3, 8 }, method_simple_args_count, U("NcUint64"), true),
                nmos::experimental::make_control_class_property_descriptor(U("Example method obj arg invoke counter"), { 3, 9 }, method_object_arg_count, U("NcUint64"), true),
                // create "Example sequence string property" with level 1: property constraints, See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
                // use nmos::nc::details::make_parameter_constraints_string to create sequence property constraints
                nmos::experimental::make_control_class_property_descriptor(U("Example string sequence property"), { 3, 10 }, string_sequence, U("NcString"), false, false, true, false, make_string_example_argument_constraints()),
                nmos::experimental::make_control_class_property_descriptor(U("Example boolean sequence property"), { 3, 11 }, boolean_sequence, U("NcBoolean"), false, false, true),
                nmos::experimental::make_control_class_property_descriptor(U("Example enum sequence property"), { 3, 12 }, enum_sequence, U("ExampleEnum"), false, false, true),
                // create "Example sequence numeric property" with level 1: property constraints, See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
                // use nmos::nc::details::make_parameter_constraints_number to create sequence property constraints
                nmos::experimental::make_control_class_property_descriptor(U("Example number sequence property"), { 3, 13 }, number_sequence, U("NcUint64"), false, false, true, false, make_number_example_argument_constraints()),
                nmos::experimental::make_control_class_property_descriptor(U("Example object sequence property"), { 3, 14 }, object_sequence, U("ExampleDataType"), false, false, true)
            };

            auto example_method_with_no_args = [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
            {
                // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Executing the example method with no arguments";

                return nmos::nc::details::make_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::nc_method_status::ok });
            };
            auto example_method_with_simple_args = [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
            {
                // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...
                // and the method parameters constraints have already been validated by the outer function

                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Executing the example method with simple arguments: " << arguments.serialize();

                return nmos::nc::details::make_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::nc_method_status::ok });
            };
            auto example_method_with_object_args = [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
            {
                // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...
                // and the method parameters constraints have already been validated by the outer function

                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Executing the example method with object argument: " << arguments.serialize();

                return nmos::nc::details::make_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::nc_method_status::ok });
            };
            // Example control class method descriptors
            std::vector<nmos::experimental::method> example_control_method_descriptors =
            {
                { nmos::experimental::make_control_class_method_descriptor(U("Example method with no arguments"), { 3, 1 }, U("MethodNoArgs"), U("NcMethodResult"), {}, false, example_method_with_no_args) },
                { nmos::experimental::make_control_class_method_descriptor(U("Example deprecated method with no arguments"), { 3, 2 }, U("MethodNoArgs"), U("NcMethodResult"), {}, true, example_method_with_no_args) },
                { nmos::experimental::make_control_class_method_descriptor(U("Example method with simple arguments"), { 3, 3 }, U("MethodSimpleArgs"), U("NcMethodResult"),
                     {
                        nmos::experimental::make_control_class_method_parameter_descriptor(U("Enum example argument"), enum_arg, U("ExampleEnum")),
                        nmos::experimental::make_control_class_method_parameter_descriptor(U("String example argument"), string_arg, U("NcString"), false, false, make_string_example_argument_constraints()), // e.g. include method property constraints
                        nmos::experimental::make_control_class_method_parameter_descriptor(U("Number example argument"), number_arg, U("NcUint64"), false, false, make_number_example_argument_constraints()), // e.g. include method property constraints
                        nmos::experimental::make_control_class_method_parameter_descriptor(U("Boolean example argument"), boolean_arg, U("NcBoolean"))
                    },
                    false, example_method_with_simple_args)
                },
                { nmos::experimental::make_control_class_method_descriptor(U("Example method with object argument"), { 3, 4 }, U("MethodObjectArg"), U("NcMethodResult"),
                    {
                        nmos::experimental::make_control_class_method_parameter_descriptor(U("Object example argument"), obj_arg, U("ExampleDataType"))
                    },
                    false, example_method_with_object_args)
                }
            };

            // create Example control class descriptor
            auto example_control_class_descriptor = nmos::experimental::make_control_class_descriptor(U("Example control class descriptor"), example_control_class_id, U("ExampleControl"), example_control_property_descriptors, example_control_method_descriptors);

            // insert Example control class descriptor to global state, which will be used by the control_protocol_ws_message_handler to process incoming ws message
            control_protocol_state.insert(example_control_class_descriptor);

            // create/insert Example datatypes to global state, which will be used by the control_protocol_ws_message_handler to process incoming ws message
            auto make_example_enum_datatype = [&]()
            {
                using web::json::value;

                auto items = value::array();
                web::json::push_back(items, nmos::nc::details::make_enum_item_descriptor(U("Undefined"), U("Undefined"), example_enum::Undefined));
                web::json::push_back(items, nmos::nc::details::make_enum_item_descriptor(U("Alpha"), U("Alpha"), example_enum::Alpha));
                web::json::push_back(items, nmos::nc::details::make_enum_item_descriptor(U("Beta"), U("Beta"), example_enum::Beta));
                web::json::push_back(items, nmos::nc::details::make_enum_item_descriptor(U("Gamma"), U("Gamma"), example_enum::Gamma));
                return nmos::nc::details::make_datatype_descriptor_enum(U("Example enum datatype"), U("ExampleEnum"), items, value::null());
            };
            auto make_example_datatype_datatype = [&]()
            {
                using web::json::value;

                auto fields = value::array();
                web::json::push_back(fields, nmos::nc::details::make_field_descriptor(U("Enum property example"), enum_property, U("ExampleEnum"), false, false, value::null()));
                {
                    // level 0: datatype constraints, See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
                    // use nmos::nc::details::make_parameter_constraints_string to create datatype constraints
                    value datatype_constraints = make_string_example_argument_constraints();
                    web::json::push_back(fields, nmos::nc::details::make_field_descriptor(U("String property example"), string_property, U("NcString"), false, false, datatype_constraints));
                }
                {
                    // level 0: datatype constraints, See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
                    // use nmos::nc::details::make_parameter_constraints_number to create datatype constraints
                    value datatype_constraints = make_number_example_argument_constraints();
                    web::json::push_back(fields, nmos::nc::details::make_field_descriptor(U("Number property example"), number_property, U("NcUint64"), false, false, datatype_constraints));
                }
                web::json::push_back(fields, nmos::nc::details::make_field_descriptor(U("Boolean property example"), boolean_property, U("NcBoolean"), false, false, value::null()));
                return nmos::nc::details::make_datatype_descriptor_struct(U("Example data type"), U("ExampleDataType"), fields, value::null());
            };
            control_protocol_state.insert(nmos::experimental::datatype_descriptor{ make_example_enum_datatype() });
            control_protocol_state.insert(nmos::experimental::datatype_descriptor{ make_example_datatype_datatype() });
        }
        // helper function to create Example datatype
        auto make_example_datatype = [&](example_enum enum_property_, const utility::string_t& string_property_, uint64_t number_property_, bool boolean_property_)
        {
            using web::json::value_of;

            return value_of({
                { enum_property, enum_property_ },
                { string_property, string_property_ },
                { number_property, number_property_ },
                { boolean_property, boolean_property_ }
            });
        };
        // helper function to create Example control instance
        auto make_example_control = [&](nmos::nc_oid oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const value& touchpoints,
            const value& runtime_property_constraints,  // level 2: runtime constraints. See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
                                                        // use of make_property_constraints_string and make_property_constraints_number to create runtime constraints
            example_enum enum_property_,
            const utility::string_t& string_property_,
            uint64_t number_property_,
            uint64_t deprecated_number_property_,
            bool boolean_property_,
            const value& object_property_,
            uint64_t method_no_args_count_,
            uint64_t method_simple_args_count_,
            uint64_t method_object_arg_count_,
            std::vector<utility::string_t> string_sequence_,
            std::vector<bool> boolean_sequence_,
            std::vector<example_enum> enum_sequence_,
            std::vector<uint64_t> number_sequence_,
            std::vector<value> object_sequence_)
        {
            auto data = nmos::nc::details::make_worker(example_control_class_id, oid, true, owner, role, value::string(user_label), description, touchpoints, runtime_property_constraints, true);
            data[enum_property] = value::number(enum_property_);
            data[string_property] = value::string(string_property_);
            data[number_property] = value::number(number_property_);
            data[deprecated_number_property] = value::number(deprecated_number_property_);
            data[boolean_property] = value::boolean(boolean_property_);
            data[object_property] = object_property_;
            data[method_no_args_count] = value::number(method_no_args_count_);
            data[method_simple_args_count] = value::number(method_simple_args_count_);
            data[method_object_arg_count] = value::number(method_object_arg_count_);
            {
                value sequence;
                for (const auto& value_ : string_sequence_) { web::json::push_back(sequence, value::string(value_)); }
                data[string_sequence] = sequence;
            }
            {
                value sequence;
                for (const auto& value_ : boolean_sequence_) { web::json::push_back(sequence, value::boolean(value_)); }
                data[boolean_sequence] = sequence;
            }
            {
                value sequence;
                for (const auto& value_ : enum_sequence_) { web::json::push_back(sequence, value_); }
                data[enum_sequence] = sequence;
            }
            {
                value sequence;
                for (const auto& value_ : number_sequence_) { web::json::push_back(sequence, value_); }
                data[number_sequence] = sequence;
            }
            {
                value sequence;
                for (const auto& value_ : object_sequence_) { web::json::push_back(sequence, value_); }
                data[object_sequence] = sequence;
            }

            return nmos::control_protocol_resource{ nmos::is12_versions::v1_0, nmos::types::nc_worker, std::move(data), true };
        };

        // example to create a non-standard Temperature Sensor control class
        const auto temperature_sensor_control_class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 3 }); // hmm, maybe pull in out to impl namespace
        const web::json::field_as_number temperature{ U("temperature") }; // hmm, maybe pull in out to impl namespace
        const web::json::field_as_string unit{ U("uint") }; // hmm, maybe pull in out to impl namespace
        {
            // Temperature Sensor control class property descriptors
            std::vector<web::json::value> temperature_sensor_property_descriptors = {
                nmos::experimental::make_control_class_property_descriptor(U("Temperature"), { 3, 1 }, temperature, U("NcFloat32"), true), // hmm, maybe pull in out to impl namespace
                nmos::experimental::make_control_class_property_descriptor(U("Unit"), { 3, 2 }, unit, U("NcString"), true)
            };

            // create Temperature Sensor control class descriptor
            auto temperature_sensor_control_class_descriptor = nmos::experimental::make_control_class_descriptor(U("Temperature Sensor control class descriptor"), temperature_sensor_control_class_id, U("TemperatureSensor"), temperature_sensor_property_descriptors);

            // insert Temperature Sensor control class descriptor to global state, which will be used by the control_protocol_ws_message_handler to process incoming ws message
            control_protocol_state.insert(temperature_sensor_control_class_descriptor);
        }
        // helper function to create Temperature Sensor control instance
        auto make_temperature_sensor = [&temperature, &unit, temperature_sensor_control_class_id](nmos::nc_oid oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, float temperature_, const utility::string_t& unit_)
        {
            auto data = nmos::nc::details::make_worker(temperature_sensor_control_class_id, oid, true, owner, role, value::string(user_label), description, touchpoints, runtime_property_constraints, true);
            data[temperature] = value::number(temperature_);
            data[unit] = value::string(unit_);

            return nmos::control_protocol_resource{ nmos::is12_versions::v1_0, nmos::types::nc_worker, std::move(data), true };
        };

        // audio sender control descriptors
        auto audio_sender_class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 5 });
        impl::make_audio_sender_descriptors(control_protocol_state, audio_sender_for_control_protocol, audio_sender_class_id);

        // example root block
        auto root_block = nmos::make_root_block();

        nmos::nc_oid oid = nmos::root_block_oid;

        // example device manager
        auto device_manager = nmos::make_device_manager(++oid, model.settings);

        // example class manager
        auto class_manager = nmos::make_class_manager(++oid, control_protocol_state);

        // example bulk properties manager
        auto bulk_properties_manager = nmos::make_bulk_properties_manager(++oid);

        // example sender control
        auto sender_control_block = impl::make_audio_sender_control(audio_sender_class_id,
                                                                    audio_sender_for_control_protocol,
                                                                    ++oid, nmos::root_block_oid,
                                                                    value_of({ { nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::sender, audio_sender_for_control_protocol.at(U("id")).as_string()}) } }),
                                                                    gate);

        // example stereo gain
        const auto stereo_gain_oid = ++oid;
        auto stereo_gain = nmos::make_block(stereo_gain_oid, nmos::root_block_oid, U("stereo-gain"), U("Stereo gain"), U("Stereo gain block"));

        // example channel gain
        const auto channel_gain_oid = ++oid;
        auto channel_gain = nmos::make_block(channel_gain_oid, stereo_gain_oid, U("channel-gain"), U("Channel gain"), U("Channel gain block"));
        // example left/right gains
        auto left_gain = make_gain_control(++oid, channel_gain_oid, U("left-gain"), U("Left gain"), U("Left channel gain"), value::null(), value::null(), 0.0);
        auto right_gain = make_gain_control(++oid, channel_gain_oid, U("right-gain"), U("Right gain"), U("Right channel gain"), value::null(), value::null(), 0.0);

        // add left-gain and right-gain to channel gain
        nmos::nc::push_back(channel_gain, left_gain);
        nmos::nc::push_back(channel_gain, right_gain);

        // example master-gain
        auto master_gain = make_gain_control(++oid, stereo_gain_oid, U("master-gain"), U("Master gain"), U("Master gain block"), value::null(), value::null(), 0.0);
        // add channel-gain and master-gain to stereo-gain
        nmos::nc::push_back(stereo_gain, channel_gain);
        nmos::nc::push_back(stereo_gain, master_gain);

        // example example-control
        auto example_control = make_example_control(++oid, nmos::root_block_oid, U("ExampleControl"), U("Example control worker"), U("Example control worker"),
            value::null(),
            // specify the level 2: runtime constraints, see https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
            // use of make_property_constraints_string and make_property_constraints_number to create runtime constraints
            value_of({
                { nmos::nc::details::make_property_constraints_string({3, 2}, 5, U("^[a-z]+$")) },
                { nmos::nc::details::make_property_constraints_number({3, 3}, 10, 100, 2) }
            }),
            example_enum::Undefined,
            U("test"),
            30,
            10,
            false,
            make_example_datatype(example_enum::Undefined, U("default"), 5, false),
            0,
            0,
            0,
            { U("red"), U("blue"), U("green") },
            { true, false },
            { example_enum::Alpha, example_enum::Gamma },
            { 0, 50, 80 },
            { make_example_datatype(example_enum::Alpha, U("example"), 50, false), make_example_datatype(example_enum::Gamma, U("different"), 75, true) }
        );

        // making an object rebuildable allows read only properties to be modified by the Configuration API in Rebuild mode
        nmos::make_rebuildable(example_control);

        const auto receivers_block_oid = ++oid;
        auto receivers_block = nmos::make_block(receivers_block_oid, nmos::root_block_oid, U("receivers"), U("Receiver Monitors"), U("Receiver Monitors"));
        // making a block rebuildable allows block members to be added or removed by the Configuration API in Rebuild mode
        nmos::make_rebuildable(receivers_block);
        // restrict the allowed classes for members of this block
        nmos::set_block_allowed_member_classes(receivers_block, {nmos::nc_receiver_monitor_class_id});

        // example receiver-monitor(s)
        {
            int count = 0;
            for (int index = 0; index < how_many; ++index)
            {
                for (const auto& port : rtp_receiver_ports)
                {
                    const auto receiver_id = impl::make_id(seed_id, nmos::types::receiver, port, index);

                    utility::ostringstream_t role;
                    role << U("receiver-monitor-") << ++count;
                    const auto& receiver = nmos::find_resource(model.node_resources, receiver_id);
                    auto receiver_monitor = nmos::make_receiver_monitor(++oid, true, receivers_block_oid, role.str(), nmos::fields::label(receiver->data), nmos::fields::description(receiver->data), value_of({ { nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, receiver_id}) } }));
                    // optionally indicate dependencies within the device model
                    nmos::set_object_dependency_paths(receiver_monitor, {{U("root"), U("receivers")}});
                    // add receiver-monitor to receivers-block
                    nmos::nc::push_back(receivers_block, receiver_monitor);
                }
            }
        }

        // example sender-monitor(s)
        {
            int count = 0;
            for (int index = 0; index < how_many; ++index)
            {
                for (const auto& port : rtp_sender_ports)
                {
                    const auto sender_id = impl::make_id(seed_id, nmos::types::sender, port, index);

                    utility::ostringstream_t role;
                    role << U("sender-monitor-") << ++count;
                    const auto& sender = nmos::find_resource(model.node_resources, sender_id);
                    const auto sender_monitor = nmos::make_sender_monitor(++oid, true, nmos::root_block_oid, role.str(), nmos::fields::label(sender->data), nmos::fields::description(sender->data), value_of({ { nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::sender, sender_id}) } }));

                    // add sender-monitor to root-block
                    nmos::nc::push_back(root_block, sender_monitor);
                }
            }
        }

        // example temperature-sensor
        const auto temperature_sensor = make_temperature_sensor(++oid, nmos::root_block_oid, U("temperature-sensor"), U("Temperature Sensor"), U("Temperature Sensor block"), value::null(), value::null(), 0.0, U("Celsius"));

        // add receivers-block to root-block
        nmos::nc::push_back(root_block, receivers_block);
        // add temperature-sensor to root-block
        nmos::nc::push_back(root_block, temperature_sensor);
        // add example-control to root-block
        nmos::nc::push_back(root_block, example_control);
        // add stereo-gain to root-block
        nmos::nc::push_back(root_block, stereo_gain);
        // add class-manager to root-block
        nmos::nc::push_back(root_block, class_manager);
        // add device-manager to root-block
        nmos::nc::push_back(root_block, device_manager);
        // add bulk-properties-manager to root-block
        nmos::nc::push_back(root_block, bulk_properties_manager);

        // insert control protocol resources to model
        insert_root_after(delay_millis, root_block, gate);
    }

    // create the list of network interface controller packet counters
    impl::nic_packet_counters = boost::copy_range<std::vector<impl::nic_packet_counter>>(host_interfaces | boost::adaptors::transformed([](const web::hosts::experimental::host_interface& interface)
    {
        return impl::nic_packet_counter{
            {interface.name, 0, U("total number of lost pockets")},
            {interface.name, 0, U("total number of late pockets")}
        };
    }));
}

void node_implementation_run(nmos::node_model& model, nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
{
    auto lock = model.read_lock();

    const auto seed_id = nmos::experimental::fields::seed_id(model.settings);
    const auto how_many = impl::fields::how_many(model.settings);
    const auto sender_ports = impl::parse_ports(impl::fields::senders(model.settings));
    const auto rtp_sender_ports = boost::copy_range<std::vector<impl::port>>(sender_ports | boost::adaptors::filtered(impl::is_rtp_port));
    const auto ws_sender_ports = boost::copy_range<std::vector<impl::port>>(sender_ports | boost::adaptors::filtered(impl::is_ws_port));
    const auto rtp_receiver_ports = boost::copy_range<std::vector<impl::port>>(impl::parse_ports(impl::fields::receivers(model.settings)) | boost::adaptors::filtered(impl::is_rtp_port));
    const auto simulate_status_monitor_activity = impl::fields::simulate_status_monitor_activity(model.settings);

    auto& control_protocol_resources = model.control_protocol_resources;

    auto get_control_protocol_property = nmos::make_get_control_protocol_property_handler(control_protocol_resources, control_protocol_state, gate);
    auto set_control_protocol_property = nmos::make_set_control_protocol_property_handler(control_protocol_resources, control_protocol_state, gate);

    auto set_receiver_monitor_link_status = nmos::make_set_receiver_monitor_link_status_handler(control_protocol_resources, control_protocol_state, gate);
    auto set_receiver_monitor_connection_status = nmos::make_set_receiver_monitor_connection_status_handler(control_protocol_resources, control_protocol_state, gate);
    auto set_receiver_monitor_external_synchronization_status = nmos::make_set_receiver_monitor_external_synchronization_status_handler(control_protocol_resources, control_protocol_state, gate);
    auto set_receiver_monitor_stream_status = nmos::make_set_receiver_monitor_stream_status_handler(control_protocol_resources, control_protocol_state, gate);
    auto set_receiver_monitor_synchronization_source_id = nmos::make_set_receiver_monitor_synchronization_source_id_handler(control_protocol_resources, control_protocol_state, gate);

    auto set_sender_monitor_link_status = nmos::make_set_sender_monitor_link_status_handler(control_protocol_resources, control_protocol_state, gate);
    auto set_sender_monitor_transmission_status = nmos::make_set_sender_monitor_transmission_status_handler(control_protocol_resources, control_protocol_state, gate);
    auto set_sender_monitor_external_synchronization_status = nmos::make_set_sender_monitor_external_synchronization_status_handler(control_protocol_resources, control_protocol_state, gate);
    auto set_sender_monitor_essence_status = nmos::make_set_sender_monitor_essence_status_handler(control_protocol_resources, control_protocol_state, gate);
    auto set_sender_monitor_synchronization_source_id = nmos::make_set_sender_monitor_synchronization_source_id_handler(control_protocol_resources, control_protocol_state, gate);

    // start background tasks to intermittently update the state of the event sources, to cause events to be emitted to connected receivers

    nmos::details::seed_generator events_seeder;
    std::shared_ptr<std::default_random_engine> events_engine(new std::default_random_engine(events_seeder));

    auto cancellation_source = pplx::cancellation_token_source();

    auto token = cancellation_source.get_token();
    auto events = pplx::do_while([&model, seed_id, how_many, simulate_status_monitor_activity, ws_sender_ports, rtp_receiver_ports, rtp_sender_ports, get_control_protocol_property, set_receiver_monitor_link_status, set_receiver_monitor_connection_status, set_receiver_monitor_external_synchronization_status, set_receiver_monitor_stream_status, set_receiver_monitor_synchronization_source_id, set_sender_monitor_link_status, set_sender_monitor_transmission_status, set_sender_monitor_external_synchronization_status, set_sender_monitor_essence_status, set_sender_monitor_synchronization_source_id, set_control_protocol_property, events_engine, &gate, token]
    {
        const auto event_interval = std::uniform_real_distribution<>(0.5, 5.0)(*events_engine);
        return pplx::complete_after(std::chrono::milliseconds(std::chrono::milliseconds::rep(1000 * event_interval)), token).then([&model, seed_id, how_many, simulate_status_monitor_activity, ws_sender_ports, rtp_receiver_ports, rtp_sender_ports, get_control_protocol_property, set_receiver_monitor_link_status, set_receiver_monitor_connection_status, set_receiver_monitor_external_synchronization_status, set_receiver_monitor_stream_status, set_receiver_monitor_synchronization_source_id, set_sender_monitor_link_status, set_sender_monitor_transmission_status, set_sender_monitor_external_synchronization_status, set_sender_monitor_essence_status, set_sender_monitor_synchronization_source_id, set_control_protocol_property, events_engine, &gate]
        {
            auto lock = model.write_lock();

            // make example temperature data ... \/\/\/\/ ... around 200
            const nmos::events_number temp(175.0 + std::abs(nmos::tai_now().seconds % 100 - 50), 10);
            // i.e. 17.5-22.5 C

            for (int index = 0; 0 <= nmos::fields::events_port(model.settings) && index < how_many; ++index)
            {
                for (const auto& port : ws_sender_ports)
                {
                    const auto source_id = impl::make_id(seed_id, nmos::types::source, port, index);
                    const auto flow_id = impl::make_id(seed_id, nmos::types::flow, port, index);

                    modify_resource(model.events_resources, source_id, [&](nmos::resource& resource)
                    {
                        if (impl::ports::temperature == port)
                        {
                            nmos::fields::endpoint_state(resource.data) = nmos::make_events_number_state({ source_id, flow_id }, temp, impl::temperature_Celsius);
                        }
                        else if (impl::ports::burn == port)
                        {
                            nmos::fields::endpoint_state(resource.data) = nmos::make_events_boolean_state({ source_id, flow_id }, temp.scaled_value() > 20.0);
                        }
                        else if (impl::ports::nonsense == port)
                        {
                            const auto nonsenses = { U("foo"), U("bar"), U("baz"), U("qux"), U("quux"), U("quuux") };
                            const auto& nonsense = *(nonsenses.begin() + (std::min)(std::geometric_distribution<size_t>()(*events_engine), nonsenses.size() - 1));
                            nmos::fields::endpoint_state(resource.data) = nmos::make_events_string_state({ source_id, flow_id }, nonsense);
                        }
                        else if (impl::ports::catcall == port)
                        {
                            const auto catcalls = { 1, 2, 4, 8 };
                            const auto& catcall = *(catcalls.begin() + (std::min)(std::geometric_distribution<size_t>()(*events_engine), catcalls.size() - 1));
                            nmos::fields::endpoint_state(resource.data) = nmos::make_events_number_state({ source_id, flow_id }, catcall, impl::catcall);
                        }
                    });
                }
            }

            // update temperature sensor
            {
                const auto temperature_sensor_control_class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 3 }); // hmm, maybe pull out temperature_sensor_control_class_id to impl namespace
                const auto temperature_value_property_id = nmos::nc_property_id({3, 1});

                auto& resources = model.control_protocol_resources;

                auto found = nmos::find_resource_if(resources, nmos::types::nc_worker, [&temperature_sensor_control_class_id](const nmos::resource& resource)
                {
                    return temperature_sensor_control_class_id == nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(resource.data));
                });

                if (resources.end() != found)
                {
                    set_control_protocol_property(nmos::fields::nc::oid(found->data), temperature_value_property_id, web::json::value(temp.scaled_value()));
                }
            }

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Temperature updated: " << temp.scaled_value() << " (" << impl::temperature_Celsius.name << ")";

            // example to increment nic packet counters
            for (auto& counter : impl::nic_packet_counters)
            {
                if (counter.lost_packet_counter.value < std::numeric_limits<uint64_t>::max()) ++counter.lost_packet_counter.value;
                if (counter.late_packet_counter.value < std::numeric_limits<uint64_t>::max()) ++counter.late_packet_counter.value;
            }

            // example setting receiver monitor statuses
            if (simulate_status_monitor_activity) {
                auto& resources = model.control_protocol_resources;
                for (int index = 0; index < how_many; ++index)
                {
                    for (const auto& port : rtp_receiver_ports)
                    {
                        const auto receiver_id = impl::make_id(seed_id, nmos::types::receiver, port, index);

                        auto receiver_monitor = nmos::nc::find_resource(resources, nmos::types::nc_status_monitor, receiver_id);
                        if (resources.end() != receiver_monitor)
                        {
                            const auto& oid = nmos::fields::nc::oid(receiver_monitor->data);

                            auto overall_status = get_control_protocol_property(oid, nmos::nc_status_monitor_overall_status_property_id);

                            switch (rand() % 3)
                            {
                                case 0:
                                {
                                    // Change link status
                                    const auto status = nmos::nc_link_status::status(nmos::nc_link_status::all_up + rand() % 3);
                                    const auto status_message = status > nmos::nc_link_status::all_up ? U("NIC1, NIC2 are down") : U("");
                                    set_receiver_monitor_link_status(oid, status, status_message);
                                    break;
                                }
                                case 1:
                                {
                                    // Change connection status
                                    if (overall_status.as_integer() != nmos::nc_overall_status::inactive)
                                    {
                                        const auto connection_status = nmos::nc_connection_status::status(nmos::nc_connection_status::healthy + rand() % 3);
                                        const auto connection_status_message = connection_status > nmos::nc_connection_status::healthy ? U("Packet loss detected") : U("");

                                        set_receiver_monitor_connection_status(oid, connection_status, connection_status_message);
                                    }
                                    break;
                                }
                                case 2:
                                {
                                    // Change synchronization status
                                    const auto status = nmos::nc_synchronization_status::status(nmos::nc_synchronization_status::not_used + rand() % 4);
                                    const auto status_message = status > nmos::nc_synchronization_status::healthy ? U("Source change from: 00:0c:ec:ff:fe:0a:2b:a1 on NIC1") : U("");
                                    set_receiver_monitor_external_synchronization_status(oid, status, status_message);
                                    // update receiver monitor synchronization source id if in-used
                                    if (nmos::nc_synchronization_status::not_used != status)
                                    {
                                        if (nmos::nc_synchronization_status::healthy == status) set_receiver_monitor_synchronization_source_id(oid, bst::optional<utility::string_t>{ U("internal") });
                                        else set_receiver_monitor_synchronization_source_id(oid, {});
                                    }
                                    break;
                                }
                                case 3:
                                {
                                    // Change stream status
                                    if (overall_status.as_integer() != nmos::nc_overall_status::inactive)
                                    {
                                        const auto status = nmos::nc_stream_status::status(nmos::nc_stream_status::healthy + rand() % 3);
                                        const auto status_message = status > nmos::nc_stream_status::status::healthy ? U("Unexpected stream format") : U("");

                                        set_receiver_monitor_stream_status(oid, status, status_message);
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                    }
                }
            }
            // example setting sender monitor statuses
            if (simulate_status_monitor_activity) {
                auto& resources = model.control_protocol_resources;
                for (int index = 0; index < how_many; ++index)
                {
                    for (const auto& port : rtp_sender_ports)
                    {
                        const auto sender_id = impl::make_id(seed_id, nmos::types::sender, port, index);

                        auto sender_monitor = nmos::nc::find_resource(resources, nmos::types::nc_status_monitor, sender_id);
                        if (resources.end() != sender_monitor)
                        {
                            const auto& oid = nmos::fields::nc::oid(sender_monitor->data);

                            auto overall_status = get_control_protocol_property(oid, nmos::nc_status_monitor_overall_status_property_id);

                            switch (rand() % 3)
                            {
                                case 0:
                                {
                                    // Change link status
                                    const auto status = nmos::nc_link_status::status(nmos::nc_link_status::all_up + rand() % 3);
                                    const auto status_message = status > nmos::nc_link_status::all_up ? U("NIC1, NIC2 are down") : U("");
                                    set_sender_monitor_link_status(oid, status, status_message);
                                    break;
                                }
                                case 1:
                                {
                                    // Change transmission status
                                    if (overall_status.as_integer() != nmos::nc_overall_status::inactive)
                                    {
                                        const auto transmission_status = nmos::nc_transmission_status::status(nmos::nc_transmission_status::healthy + rand() % 3);
                                        const auto transmission_status_message = transmission_status > nmos::nc_transmission_status::healthy ? U("Transmission errors detected") : U("");

                                        set_sender_monitor_transmission_status(oid, transmission_status, transmission_status_message);
                                    }
                                    break;
                                }
                                case 2:
                                {
                                    // Change synchronization status
                                    const auto status = nmos::nc_synchronization_status::status(nmos::nc_synchronization_status::not_used + rand() % 4);
                                    const auto status_message = status > nmos::nc_synchronization_status::healthy ? U("Source change from: 00:0c:ec:ff:fe:0a:2b:a1 on NIC1") : U("");
                                    set_sender_monitor_external_synchronization_status(oid, status, status_message);
                                    // update sender monitor synchronization source id if in-used
                                    if (nmos::nc_synchronization_status::not_used != status)
                                    {
                                        if (nmos::nc_synchronization_status::healthy == status) set_sender_monitor_synchronization_source_id(oid, bst::optional<utility::string_t>{ U("internal") });
                                        else set_sender_monitor_synchronization_source_id(oid, {});
                                    }
                                    break;
                                }
                                case 3:
                                {
                                    // Change essence status
                                    if (overall_status.as_integer() != nmos::nc_overall_status::inactive)
                                    {
                                        const auto status = nmos::nc_essence_status::status(nmos::nc_stream_status::healthy + rand() % 3);
                                        const auto status_message = status > nmos::nc_essence_status::status::healthy ? U("No valid input signal on input SDI1") : U("");

                                        set_sender_monitor_essence_status(oid, status, status_message);
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                    }
                }
            }
            model.notify();

            return true;
        });
    }, token);

    // wait for the thread to be interrupted because the server is being shut down
    model.shutdown_condition.wait(lock, [&] { return model.shutdown; });

    cancellation_source.cancel();
    // wait without the lock since it is also used by the background tasks
    nmos::details::reverse_lock_guard<nmos::read_lock> unlock{ lock };

    events.wait();
}

// Example System API node behaviour callback to perform application-specific operations when the global configuration resource changes
nmos::system_global_handler make_node_implementation_system_global_handler(nmos::node_model& model, slog::base_gate& gate)
{
    // this example uses the callback to update the settings
    // (an 'empty' std::function disables System API node behaviour)
    return [&](const web::uri& system_uri, const web::json::value& system_global)
    {
        if (!system_uri.is_empty())
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "New system global configuration discovered from the System API at: " << system_uri.to_string();

            // although this example immediately updates the settings, the effect is not propagated
            // in either Registration API behaviour or the senders' /transportfile endpoints until
            // an update to these is forced by other circumstances

            auto system_global_settings = nmos::parse_system_global_data(system_global).second;
            web::json::merge_patch(model.settings, system_global_settings, true);
        }
        else
        {
            slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "System global configuration is not discoverable";
        }
    };
}

// Example Registration API node behaviour callback to perform application-specific operations when the current Registration API changes
nmos::registration_handler make_node_implementation_registration_handler(slog::base_gate& gate)
{
    return [&](const web::uri& registration_uri)
    {
        if (!registration_uri.is_empty())
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Started registered operation with Registration API at: " << registration_uri.to_string();
        }
        else
        {
            slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Stopped registered operation";
        }
    };
}

// Example Connection API callback to parse "transport_file" during a PATCH /staged request
nmos::transport_file_parser make_node_implementation_transport_file_parser()
{
    // this example uses a custom transport file parser to handle video/jxsv in addition to the core media types
    // otherwise, it could simply return &nmos::parse_rtp_transport_file
    // (if this callback is specified, an 'empty' std::function is not allowed)
    return [](const nmos::resource& receiver, const nmos::resource& connection_receiver, const utility::string_t& transport_file_type, const utility::string_t& transport_file_data, slog::base_gate& gate)
    {
        const auto validate_sdp_parameters = [](const web::json::value& receiver, const nmos::sdp_parameters& sdp_params)
        {
            if (nmos::media_types::video_jxsv == nmos::get_media_type(sdp_params))
            {
                nmos::validate_video_jxsv_sdp_parameters(receiver, sdp_params);
            }
            else
            {
                // validate core media types, i.e., "video/raw", "audio/L", "video/smpte291" and "video/SMPTE2022-6"
                nmos::validate_sdp_parameters(receiver, sdp_params);
            }
        };
        return nmos::details::parse_rtp_transport_file(validate_sdp_parameters, receiver, connection_receiver, transport_file_type, transport_file_data, gate);
    };
}

// Example Connection API callback to perform application-specific validation of the merged /staged endpoint during a PATCH /staged request
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
    const auto device_id = impl::make_id(seed_id, nmos::types::device);
    const auto how_many = impl::fields::how_many(settings);
    const auto rtp_sender_ports = boost::copy_range<std::vector<impl::port>>(impl::parse_ports(impl::fields::senders(settings)) | boost::adaptors::filtered(impl::is_rtp_port));
    const auto rtp_sender_ids = impl::make_ids(seed_id, nmos::types::sender, rtp_sender_ports, how_many);
    const auto ws_sender_ports = boost::copy_range<std::vector<impl::port>>(impl::parse_ports(impl::fields::senders(settings)) | boost::adaptors::filtered(impl::is_ws_port));
    const auto ws_sender_ids = impl::make_ids(seed_id, nmos::types::sender, ws_sender_ports, how_many);
    const auto ws_sender_uri = nmos::make_events_ws_api_connection_uri(device_id, settings);
    const auto rtp_receiver_ports = boost::copy_range<std::vector<impl::port>>(impl::parse_ports(impl::fields::receivers(settings)) | boost::adaptors::filtered(impl::is_rtp_port));
    const auto rtp_receiver_ids = impl::make_ids(seed_id, nmos::types::receiver, rtp_receiver_ports, how_many);
    const auto ws_receiver_ports = boost::copy_range<std::vector<impl::port>>(impl::parse_ports(impl::fields::receivers(settings)) | boost::adaptors::filtered(impl::is_ws_port));
    const auto ws_receiver_ids = impl::make_ids(seed_id, nmos::types::receiver, ws_receiver_ports, how_many);

    // although which properties may need to be defaulted depends on the resource type,
    // the default value will almost always be different for each resource
    return [rtp_sender_ids, rtp_receiver_ids, ws_sender_ids, ws_sender_uri, ws_receiver_ids](const nmos::resource& resource, const nmos::resource& connection_resource, value& transport_params)
    {
        const std::pair<nmos::id, nmos::type> id_type{ connection_resource.id, connection_resource.type };
        // this code relies on the specific constraints added by node_implementation_thread
        const auto& constraints = nmos::fields::endpoint_constraints(connection_resource.data);

        // "In some cases the behaviour is more complex, and may be determined by the vendor."
        // See https://specs.amwa.tv/is-05/releases/v1.0.0/docs/2.2._APIs_-_Server_Side_Implementation.html#use-of-auto
        if (rtp_sender_ids.end() != boost::range::find(rtp_sender_ids, id_type.first))
        {
            const bool smpte2022_7 = 1 < transport_params.size();
            nmos::details::resolve_auto(transport_params[0], nmos::fields::source_ip, [&] { return web::json::front(nmos::fields::constraint_enum(constraints.at(0).at(nmos::fields::source_ip))); });
            if (smpte2022_7) nmos::details::resolve_auto(transport_params[1], nmos::fields::source_ip, [&] { return web::json::back(nmos::fields::constraint_enum(constraints.at(1).at(nmos::fields::source_ip))); });
            nmos::details::resolve_auto(transport_params[0], nmos::fields::destination_ip, [&] { return value::string(impl::make_source_specific_multicast_address_v4(id_type.first, 0)); });
            if (smpte2022_7) nmos::details::resolve_auto(transport_params[1], nmos::fields::destination_ip, [&] { return value::string(impl::make_source_specific_multicast_address_v4(id_type.first, 1)); });
            // lastly, apply the specification defaults for any properties not handled above
            nmos::resolve_rtp_auto(id_type.second, transport_params);
        }
        else if (rtp_receiver_ids.end() != boost::range::find(rtp_receiver_ids, id_type.first))
        {
            const bool smpte2022_7 = 1 < transport_params.size();
            nmos::details::resolve_auto(transport_params[0], nmos::fields::interface_ip, [&] { return web::json::front(nmos::fields::constraint_enum(constraints.at(0).at(nmos::fields::interface_ip))); });
            if (smpte2022_7) nmos::details::resolve_auto(transport_params[1], nmos::fields::interface_ip, [&] { return web::json::back(nmos::fields::constraint_enum(constraints.at(1).at(nmos::fields::interface_ip))); });
            // lastly, apply the specification defaults for any properties not handled above
            nmos::resolve_rtp_auto(id_type.second, transport_params);
        }
        else if (ws_sender_ids.end() != boost::range::find(ws_sender_ids, id_type.first))
        {
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_uri, [&] { return value::string(ws_sender_uri.to_string()); });
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_authorization, [&] { return value::boolean(false); });
        }
        else if (ws_receiver_ids.end() != boost::range::find(ws_receiver_ids, id_type.first))
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
    const auto node_id = impl::make_id(seed_id, nmos::types::node);
    const auto how_many = impl::fields::how_many(settings);
    const auto sender_ports = impl::parse_ports(impl::fields::senders(settings));
    const auto rtp_sender_ports = boost::copy_range<std::vector<impl::port>>(sender_ports | boost::adaptors::filtered(impl::is_rtp_port));
    const auto rtp_source_ids = impl::make_ids(seed_id, nmos::types::source, rtp_sender_ports, how_many);
    const auto rtp_flow_ids = impl::make_ids(seed_id, nmos::types::flow, rtp_sender_ports, how_many);
    const auto rtp_sender_ids = impl::make_ids(seed_id, nmos::types::sender, rtp_sender_ports, how_many);

    // as part of activation, the example sender /transportfile should be updated based on the active transport parameters
    return [&node_resources, node_id, rtp_source_ids, rtp_flow_ids, rtp_sender_ids](const nmos::resource& sender, const nmos::resource& connection_sender, value& endpoint_transportfile)
    {
        const auto found = boost::range::find(rtp_sender_ids, connection_sender.id);
        if (rtp_sender_ids.end() != found)
        {
            const auto index = int(found - rtp_sender_ids.begin());
            const auto source_id = rtp_source_ids.at(index);
            const auto flow_id = rtp_flow_ids.at(index);

            // note, model mutex is already locked by the calling thread, so access to node_resources is OK...
            auto node = nmos::find_resource(node_resources, { node_id, nmos::types::node });
            auto source = nmos::find_resource(node_resources, { source_id, nmos::types::source });
            auto flow = nmos::find_resource(node_resources, { flow_id, nmos::types::flow });
            if (node_resources.end() == node || node_resources.end() == source || node_resources.end() == flow)
            {
                throw std::logic_error("matching IS-04 node, source or flow not found");
            }

            // the nmos::make_sdp_parameters overload from the IS-04 resources provides a high-level interface
            // for common "video/raw", "audio/L", "video/smpte291" and "video/SMPTE2022-6" use cases
            //auto sdp_params = nmos::make_sdp_parameters(node->data, source->data, flow->data, sender.data, { U("PRIMARY"), U("SECONDARY") });

            // nmos::make_{video,audio,data,mux}_sdp_parameters provide a little more flexibility for those four media types
            // and the combination of nmos::make_{video_raw,audio_L,video_smpte291,video_SMPTE2022_6}_parameters
            // with the related make_sdp_parameters overloads provides the most flexible and extensible approach
            auto sdp_params = [&]
            {
                const std::vector<utility::string_t> mids{ U("PRIMARY"), U("SECONDARY") };
                const nmos::format format{ nmos::fields::format(flow->data) };
                if (nmos::formats::video == format)
                {
                    const nmos::media_type video_type{ nmos::fields::media_type(flow->data) };
                    if (nmos::media_types::video_raw == video_type)
                    {
                        return nmos::make_video_sdp_parameters(node->data, source->data, flow->data, sender.data, nmos::details::payload_type_video_default, mids, {}, sdp::type_parameters::type_N);
                    }
                    else if (nmos::media_types::video_jxsv == video_type)
                    {
                        const auto params = nmos::make_video_jxsv_parameters(node->data, source->data, flow->data, sender.data);
                        const auto ts_refclk = nmos::details::make_ts_refclk(node->data, source->data, sender.data, {});
                        return nmos::make_sdp_parameters(nmos::fields::label(sender.data), params, nmos::details::payload_type_video_default, mids, ts_refclk);
                    }
                    else
                    {
                        throw std::logic_error("unexpected flow media_type");
                    }
                }
                else if (nmos::formats::audio == format)
                {
                    // this example application doesn't actually stream, so just indicate a sensible value for packet time
                    const double packet_time = nmos::fields::channels(source->data).size() > 8 ? 0.125 : 1;
                    return nmos::make_audio_sdp_parameters(node->data, source->data, flow->data, sender.data, nmos::details::payload_type_audio_default, mids, {}, packet_time);
                }
                else if (nmos::formats::data == format)
                {
                    return nmos::make_data_sdp_parameters(node->data, source->data, flow->data, sender.data, nmos::details::payload_type_data_default, mids, {}, {});
                }
                else if (nmos::formats::mux == format)
                {
                    return nmos::make_mux_sdp_parameters(node->data, source->data, flow->data, sender.data, nmos::details::payload_type_mux_default, mids, {}, sdp::type_parameters::type_N);
                }
                else
                {
                    throw std::logic_error("unexpected flow format");
                }
            }();

            auto& transport_params = nmos::fields::transport_params(nmos::fields::endpoint_active(connection_sender.data));
            auto session_description = nmos::make_session_description(sdp_params, transport_params);
            auto sdp = utility::s2us(sdp::make_session_description(session_description));
            endpoint_transportfile = nmos::make_connection_rtp_sender_transportfile(sdp);
        }
    };
}

// Example Events WebSocket API client message handler
nmos::events_ws_message_handler make_node_implementation_events_ws_message_handler(const nmos::node_model& model, slog::base_gate& gate)
{
    const auto seed_id = nmos::experimental::fields::seed_id(model.settings);
    const auto how_many = impl::fields::how_many(model.settings);
    const auto receiver_ports = impl::parse_ports(impl::fields::receivers(model.settings));
    const auto ws_receiver_ports = boost::copy_range<std::vector<impl::port>>(receiver_ports | boost::adaptors::filtered(impl::is_ws_port));
    const auto ws_receiver_ids = impl::make_ids(seed_id, nmos::types::receiver, ws_receiver_ports, how_many);

    // the message handler will be used for all Events WebSocket connections, and each connection may potentially
    // have subscriptions to a number of sources, for multiple receivers, so this example uses a handler adaptor
    // that enables simple processing of "state" messages (events) per receiver
    return nmos::experimental::make_events_ws_message_handler(model, [ws_receiver_ids, &gate](const nmos::resource& receiver, const nmos::resource& connection_receiver, const web::json::value& message)
    {
        const auto found = boost::range::find(ws_receiver_ids, connection_receiver.id);
        if (ws_receiver_ids.end() != found)
        {
            const auto event_type = nmos::event_type(nmos::fields::state_event_type(message));
            const auto& payload = nmos::fields::state_payload(message);

            if (nmos::is_matching_event_type(nmos::event_types::wildcard(nmos::event_types::number), event_type))
            {
                const nmos::events_number value(nmos::fields::payload_number_value(payload).to_double(), nmos::fields::payload_number_scale(payload));
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Event received: " << value.scaled_value() << " (" << event_type.name << ")";
            }
            else if (nmos::is_matching_event_type(nmos::event_types::wildcard(nmos::event_types::string), event_type))
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Event received: " << nmos::fields::payload_string_value(payload) << " (" << event_type.name << ")";
            }
            else if (nmos::is_matching_event_type(nmos::event_types::wildcard(nmos::event_types::boolean), event_type))
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Event received: " << std::boolalpha << nmos::fields::payload_boolean_value(payload) << " (" << event_type.name << ")";
            }
        }
    }, gate);
}

// Example Connection API activation callback to perform application-specific operations to complete activation
nmos::connection_activation_handler make_node_implementation_connection_activation_handler(nmos::node_model& model, slog::base_gate& gate)
{
    auto handle_load_ca_certificates = nmos::make_load_ca_certificates_handler(model.settings, gate);
    // this example uses this callback to (un)subscribe a IS-07 Events WebSocket receiver when it is activated
    // and, in addition to the message handler, specifies the optional close handler in order that any subsequent
    // connection errors are reflected into the /active endpoint by setting master_enable to false
    auto handle_events_ws_message = make_node_implementation_events_ws_message_handler(model, gate);
    auto handle_close = nmos::experimental::make_events_ws_close_handler(model, gate);
    auto connection_events_activation_handler = nmos::make_connection_events_websocket_activation_handler(handle_load_ca_certificates, handle_events_ws_message, handle_close, model.settings, gate);

    return [connection_events_activation_handler, &gate](const nmos::resource& resource, const nmos::resource& connection_resource)
    {
        const std::pair<nmos::id, nmos::type> id_type{ resource.id, resource.type };
        slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Activating " << id_type;

        connection_events_activation_handler(resource, connection_resource);
    };
}

// Example Channel Mapping API callback to perform application-specific validation of the merged active map during a POST /map/activations request
nmos::details::channelmapping_output_map_validator make_node_implementation_map_validator()
{
    // this example uses an 'empty' std::function because it does not need to do any validation
    // beyond what is expressed by the schemas and /caps endpoints
    return{};
}

// Example Channel Mapping API activation callback to perform application-specific operations to complete activation
nmos::channelmapping_activation_handler make_node_implementation_channelmapping_activation_handler(slog::base_gate& gate)
{
    return [&gate](const nmos::resource& channelmapping_output)
    {
        const auto output_id = nmos::fields::channelmapping_id(channelmapping_output.data);
        slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Activating output: " << output_id;
    };
}

// Example Control Protocol WebSocket API property changed callback to perform application-specific operations to complete the property changed
nmos::control_protocol_property_changed_handler make_node_implementation_control_protocol_property_changed_handler(slog::base_gate& gate)
{
    return [&gate](const nmos::resource& resource, const utility::string_t& property_name, int index)
    {
        if (index >= 0)
        {
            // sequence property
            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Property: " << property_name << " index " << index << " has value changed to " << resource.data.at(property_name).at(index).serialize();
        }
        else if (index == -1)
        {
            // non-sequence property
            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Property: " << property_name << " has value changed to " << resource.data.at(property_name).serialize();
        }
        else if (index == -2)
        {
            // sequence property removed
            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Property: " << property_name << " has sequence item removed. Value changed to " << resource.data.at(property_name).serialize();
        }
    };
}

// Example Control Protocol WebSocket API Receiver Status Monitor callback to get network interface controller lost packet counters
nmos::get_packet_counters_handler make_node_implementation_get_lost_packet_counters_handler()
{
    return []()
    {
        return boost::copy_range<std::vector<nmos::nc::counter>>(impl::nic_packet_counters | boost::adaptors::transformed([](const impl::nic_packet_counter& counter)
        {
            return nmos::nc::counter{ counter.lost_packet_counter.name, counter.lost_packet_counter.value, counter.lost_packet_counter.description };
        }));
    };
}

// Example Control Protocol WebSocket API Receiver Status Monitor callback to get network interface controller late packet counters
nmos::get_packet_counters_handler make_node_implementation_get_late_packet_counters_handler()
{
    return []()
    {
        return boost::copy_range<std::vector<nmos::nc::counter>>(impl::nic_packet_counters | boost::adaptors::transformed([](const impl::nic_packet_counter& counter)
        {
            return nmos::nc::counter{ counter.late_packet_counter.name, counter.late_packet_counter.value, counter.late_packet_counter.description };
        }));
    };
}

// Example Control Protocol WebSocket API Receiver Status Monitor callback to reset network interface controller packet counters
nmos::reset_monitor_handler make_node_implementation_reset_monitor_handler()
{
    return []()
    {
        for (auto& counter : impl::nic_packet_counters)
        {
            counter.lost_packet_counter.value = 0;
            counter.late_packet_counter.value = 0;
        }
    };
}

// Example Device Configuration callbacks called when a rebuildable object is modified in Rebuild mode.

// IS-14 Device Configuration callback
// This function should generate a fingerprint that can be used for subsequent validation.
nmos::create_validation_fingerprint_handler make_create_validation_fingerprint_handler()
{
    return [](const nmos::resources& resources, const nmos::resource& resource)
    {
        return U("Sony nmos-cpp node");
    };
}

// IS-14 Device Configuration callback
// This function called by a validate or restore and can be used to validate a validation fingerprint. Returning false will fail the validate or restore operation.
nmos::validate_validation_fingerprint_handler make_validate_validation_fingerprint_handler()
{
    return [](const nmos::resources& resources, const nmos::resource& resource, const utility::string_t& validation_fingerprint)
    {
        return true;
    };
}

// IS-14 Device Configuration callback
// This function is called when the Device Configuration API is attempting to modify
// the read only properties of a rebuildable Device Model object. This callback returns an "allow list" of property ids
// for properties that can be updated - the "allowed" read only property will be updated according to backup dataset received.
nmos::get_read_only_modification_allow_list_handler make_get_read_only_modification_allow_list_handler(slog::base_gate& gate)
{
    return [&gate](const nmos::resources& resources, const nmos::resource& resource, const std::vector<utility::string_t>& target_role_path, const std::vector<nmos::nc_property_id>& property_ids)
    {
        // Use this function to create allow list of property ids for properties in the object should be modified by the configuration API
        slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::node_implementation) << "Do filter_property_holders";

        // Filter out any read only properties that should not be modified
        return property_ids;
    };
}

// IS-14 Device Configuration callback
// This function is called before an object is deleted from the device model.
// If this function returns true and validate is false then the object will be deleted.
// If this function returns true/false or validate is true then the object will not be deleted.
// If this function returns false an appropriate error will be passed to the calling client.
nmos::remove_device_model_object_handler make_remove_device_model_object_handler()
{
    return [](const nmos::resource& resource, const std::vector<utility::string_t>& role_path, bool validate)
    {
        // Perform application code functions here
        // resource - device model object about to be deleted
        // role_path - role path of device object about to be deleted
        // validate - true when only checks are performed, false when checks and deletion are performed
        return true;
    };
}

// IS-14 Device Configuration callback
// This function is called when an object is to be created.
// The returned object is then added to the Device Model
// This example shows the creation of a receiver monitor resource
// In the Device Model the receivers block that contains the monitors must be rebuildable
nmos::create_device_model_object_handler make_create_device_model_object_handler(slog::base_gate& gate)
{
    return[&gate](const nmos::nc_class_id& class_id, nmos::nc_oid oid, bool constant_oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, bool validate, const std::map<nmos::nc_property_id, web::json::value>& property_values)
    {
        if (touchpoints.size() != 1)
        {
            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Either zero or more than one touchpoint found (ambiguous) when attempting to create " << role;
            return nmos::control_protocol_resource(); // return empty resource on error
        }
        const auto& touchpoint_uuid = nmos::fields::nc::id(nmos::fields::nc::resource(*touchpoints.as_array().begin()));

        // In the case of validate = true, the object created will not be added to the device model, but it's values will be checked against the backup dataset
        return nmos::make_receiver_monitor(oid, true, owner, role, user_label, U(""), web::json::value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, touchpoint_uuid.as_string()})} }));
    };
}

namespace impl
{
    nmos::interlace_mode get_interlace_mode(const nmos::settings& settings)
    {
        if (settings.has_field(impl::fields::interlace_mode))
        {
            return nmos::interlace_mode{ impl::fields::interlace_mode(settings) };
        }
        // for the default, 1080i50 and 1080i59.94 are arbitrarily preferred to 1080p25 and 1080p29.97
        // for 1080i formats, ST 2110-20 says that "the fields of an interlaced image are transmitted in time order,
        // first field first [and] the sample rows of the temporally second field are displaced vertically 'below' the
        // like-numbered sample rows of the temporally first field."
        const auto frame_rate = nmos::parse_rational(impl::fields::frame_rate(settings));
        const auto frame_height = impl::fields::frame_height(settings);
        return (nmos::rates::rate25 == frame_rate || nmos::rates::rate29_97 == frame_rate) && 1080 == frame_height
            ? nmos::interlace_modes::interlaced_tff
            : nmos::interlace_modes::progressive;
    }

    bool is_rtp_port(const impl::port& port)
    {
        return impl::ports::rtp.end() != boost::range::find(impl::ports::rtp, port);
    }

    bool is_ws_port(const impl::port& port)
    {
        return impl::ports::ws.end() != boost::range::find(impl::ports::ws, port);
    }

    std::vector<port> parse_ports(const web::json::value& value)
    {
        if (value.is_null()) return impl::ports::all;
        return boost::copy_range<std::vector<port>>(value.as_array() | boost::adaptors::transformed([&](const web::json::value& value)
        {
            return port{ value.as_string() };
        }));
    }

    // find interface with the specified address
    std::vector<web::hosts::experimental::host_interface>::const_iterator find_interface(const std::vector<web::hosts::experimental::host_interface>& interfaces, const utility::string_t& address)
    {
        return boost::range::find_if(interfaces, [&](const web::hosts::experimental::host_interface& interface)
        {
            return interface.addresses.end() != boost::range::find(interface.addresses, address);
        });
    }

    // generate repeatable ids for the example node's resources
    nmos::id make_id(const nmos::id& seed_id, const nmos::type& type, const impl::port& port, int index)
    {
        return nmos::make_repeatable_id(seed_id, U("/x-nmos/node/") + type.name + U('/') + port.name + utility::conversions::details::to_string_t(index));
    }

    std::vector<nmos::id> make_ids(const nmos::id& seed_id, const nmos::type& type, const impl::port& port, int how_many)
    {
        return boost::copy_range<std::vector<nmos::id>>(boost::irange(0, how_many) | boost::adaptors::transformed([&](const int& index)
        {
            return impl::make_id(seed_id, type, port, index);
        }));
    }

    std::vector<nmos::id> make_ids(const nmos::id& seed_id, const nmos::type& type, const std::vector<port>& ports, int how_many)
    {
        // hm, boost::range::combine arrived in Boost 1.56.0
        std::vector<nmos::id> ids;
        for (const auto& port : ports)
        {
            boost::range::push_back(ids, make_ids(seed_id, type, port, how_many));
        }
        return ids;
    }

    std::vector<nmos::id> make_ids(const nmos::id& seed_id, const std::vector<nmos::type>& types, const std::vector<port>& ports, int how_many)
    {
        // hm, boost::range::combine arrived in Boost 1.56.0
        std::vector<nmos::id> ids;
        for (const auto& type : types)
        {
            boost::range::push_back(ids, make_ids(seed_id, type, ports, how_many));
        }
        return ids;
    }

    // generate a repeatable source-specific multicast address for each leg of a sender
    utility::string_t make_source_specific_multicast_address_v4(const nmos::id& id, int leg)
    {
        // hash the pseudo-random id and leg to generate the address
        const auto s = id + U('/') + utility::conversions::details::to_string_t(leg);
        const auto h = std::hash<utility::string_t>{}(s);
        auto a = boost::asio::ip::address_v4(uint32_t(h)).to_bytes();
        // ensure the address is in the source-specific multicast block reserved for local host allocation, 232.0.1.0-232.255.255.255
        // see https://www.iana.org/assignments/multicast-addresses/multicast-addresses.xhtml#multicast-addresses-10
        a[0] = 232;
        a[2] |= 1;
        return utility::s2us(boost::asio::ip::address_v4(a).to_string());
    }

    // add a selection of parents to a source or flow
    void insert_parents(nmos::resource& resource, const nmos::id& seed_id, const port& port, int index)
    {
        // algorithm to produce signal ancestry with a range of depths and breadths
        // see https://github.com/sony/nmos-cpp/issues/312#issuecomment-1335641637
        int b = 0;
        while (index & (1 << b)) ++b;
        if (!b) return;
        index &= ~(1 << (b - 1));
        do
        {
            index &= ~(1 << b);
            web::json::push_back(resource.data[nmos::fields::parents], impl::make_id(seed_id, resource.type, port, index));
            ++b;
        } while (index & (1 << b));
    }

    // add a helpful suffix to the label of a sub-resource for the example node
    void set_label_description(nmos::resource& resource, const impl::port& port, int index)
    {
        using web::json::value;

        auto label = nmos::fields::label(resource.data);
        if (!label.empty()) label += U('/');
        label += resource.type.name + U('/') + port.name + utility::conversions::details::to_string_t(index);
        resource.data[nmos::fields::label] = value::string(label);

        auto description = nmos::fields::description(resource.data);
        if (!description.empty()) description += U('/');
        description += resource.type.name + U('/') + port.name + utility::conversions::details::to_string_t(index);
        resource.data[nmos::fields::description] = value::string(description);
    }

    // add an example "natural grouping" hint to a sender or receiver
    void insert_group_hint(nmos::resource& resource, const impl::port& port, int index)
    {
        web::json::push_back(resource.data[nmos::fields::tags][nmos::fields::group_hint], nmos::make_group_hint({ U("example"), resource.type.name + U(' ') + port.name + utility::conversions::details::to_string_t(index) }));
    }
}

// This constructs all the callbacks used to integrate the example device-specific underlying implementation
// into the server instance for the NMOS Node.
nmos::experimental::node_implementation make_node_implementation(nmos::node_model& model, slog::base_gate& gate)
{
    return nmos::experimental::node_implementation()
        .on_load_server_certificates(nmos::make_load_server_certificates_handler(model.settings, gate))
        .on_load_dh_param(nmos::make_load_dh_param_handler(model.settings, gate))
        .on_load_ca_certificates(nmos::make_load_ca_certificates_handler(model.settings, gate))
        .on_system_changed(make_node_implementation_system_global_handler(model, gate)) // may be omitted if not required
        .on_registration_changed(make_node_implementation_registration_handler(gate)) // may be omitted if not required
        .on_parse_transport_file(make_node_implementation_transport_file_parser()) // may be omitted if the default is sufficient
        .on_validate_connection_resource_patch(make_node_implementation_patch_validator()) // may be omitted if not required
        .on_resolve_auto(make_node_implementation_auto_resolver(model.settings))
        .on_set_transportfile(make_node_implementation_transportfile_setter(model.node_resources, model.settings))
        .on_connection_activated(make_node_implementation_connection_activation_handler(model, gate))
        .on_validate_channelmapping_output_map(make_node_implementation_map_validator()) // may be omitted if not required
        .on_channelmapping_activated(make_node_implementation_channelmapping_activation_handler(gate))
        .on_control_protocol_property_changed(make_node_implementation_control_protocol_property_changed_handler(gate)) // may be omitted if IS-12 not required
        .on_create_validation_fingerprint(make_create_validation_fingerprint_handler())
        .on_validate_validation_fingerprint(make_validate_validation_fingerprint_handler())
        .on_get_read_only_modification_allow_list(make_get_read_only_modification_allow_list_handler(gate)) // may be omitted if either IS-14 not required, or IS-14 Rebuild functionality not required
        .on_remove_device_model_object(make_remove_device_model_object_handler()) // may be omitted if either IS-14 not required, or IS-14 Rebuild functionality not required
        .on_create_device_model_object(make_create_device_model_object_handler(gate)) // may be omitted if either IS-14 not required, or IS-14 Rebuild functionality not required
        .on_get_lost_packet_counters(make_node_implementation_get_lost_packet_counters_handler()) // may be omitted if IS-12/BCP-008-1 not required
        .on_get_late_packet_counters(make_node_implementation_get_late_packet_counters_handler()) // may be omitted if IS-12/BCP-008-1 not required
        .on_reset_monitor(make_node_implementation_reset_monitor_handler()); // may be omitted if IS-12/BCP-008-1 not required
}

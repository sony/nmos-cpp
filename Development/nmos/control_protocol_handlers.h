#ifndef NMOS_CONTROL_PROTOCOL_HANDLERS_H
#define NMOS_CONTROL_PROTOCOL_HANDLERS_H

#include <functional>
#include "bst/optional.h"
#include "nmos/connection_activation.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/resources.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    namespace experimental
    {
        struct control_protocol_state;
        struct control_class_descriptor;
        struct datatype_descriptor;
    }

    // callback to retrieve a specific control protocol class descriptor
    // this callback should not throw exceptions
    typedef std::function<experimental::control_class_descriptor(const nc_class_id& class_id)> get_control_protocol_class_descriptor_handler;

    // callback to add user control protocol class descriptor
    // this callback should not throw exceptions
    typedef std::function<bool(const nc_class_id& class_id, const experimental::control_class_descriptor& control_class_descriptor)> add_control_protocol_class_descriptor_handler;

    // callback to retrieve a control protocol datatype descriptor
    // this callback should not throw exceptions
    typedef std::function<experimental::datatype_descriptor(const nc_name& name)> get_control_protocol_datatype_descriptor_handler;

    // a control_protocol_property_changed_handler is a notification that the specified (IS-12) property has changed
    // index is set to -1 for non-sequence property
    // index is set to -2 when a sequence item had been removed
    // this callback should not throw exceptions, as the relevant property will already has been changed and those changes will not be rolled back
    typedef std::function<void(const nmos::resource& resource, const utility::string_t& property_name, int index)> control_protocol_property_changed_handler;

    // callback to set monitor pending
    typedef std::function<void(void)> monitor_status_pending_handler;

    // Receiver & Sender Monitor status callbacks
    // these callbacks should not throw exceptions
    namespace nc
    {
        struct counter
        {
            utility::string_t name;
            uint64_t value;
            utility::string_t description;

            counter() : name(), value(0), description() {}
            counter(const utility::string_t& name, uint64_t value, const utility::string_t& description)
                : name(name), value(value), description(description) {}
        };
    }
    typedef std::function<std::vector<nc::counter>(void)> get_packet_counters_handler;
    typedef std::function<void(void)> reset_monitor_handler;

    namespace experimental
    {
        // control method handler definition
        typedef std::function<web::json::value(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)> control_protocol_method_handler;

        // method definition (NcMethodDescriptor vs method handler)
        typedef std::pair<web::json::value, control_protocol_method_handler> method;

        inline method make_control_class_method(const web::json::value& nc_method_descriptor, control_protocol_method_handler method_handler)
        {
            return std::make_pair(nc_method_descriptor, method_handler);
        }
    }

    // callback to retrieve a specific method
    // this callback should not throw exceptions
    typedef std::function<experimental::method(const nc_class_id& class_id, const nc_method_id& method_id)> get_control_protocol_method_descriptor_handler;

    // construct callback to retrieve a specific control protocol class descriptor
    get_control_protocol_class_descriptor_handler make_get_control_protocol_class_descriptor_handler(experimental::control_protocol_state& control_protocol_state);

    // construct callback to retrieve a specific datatype descriptor
    get_control_protocol_datatype_descriptor_handler make_get_control_protocol_datatype_descriptor_handler(experimental::control_protocol_state& control_protocol_state);

    // construct callback to retrieve a specific method
    get_control_protocol_method_descriptor_handler make_get_control_protocol_method_descriptor_handler(experimental::control_protocol_state& control_protocol_state);

    // control_protocol_connection_activation_handler notifies that the active parameters for the specified (IS-05) sender/connection_sender or receiver/connection_receiver have changed
    control_protocol_connection_activation_handler make_monitor_connection_activation_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // construct callback to get values from device model
    typedef std::function<web::json::value(nmos::nc_oid oid, const nmos::nc_property_id& property_id)> get_control_protocol_property_handler;
    get_control_protocol_property_handler make_get_control_protocol_property_handler(const resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // construct callback to set values on device model
    typedef std::function<bool(nc_oid oid, const nc_property_id& property_id, const web::json::value& value)> set_control_protocol_property_handler;
    set_control_protocol_property_handler make_set_control_protocol_property_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // NcReceiverMonitor handlers

    // Set receiver monitor link status and link status message
    typedef std::function<bool(nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message)> set_receiver_monitor_link_status_handler;
    set_receiver_monitor_link_status_handler make_set_receiver_monitor_link_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // Set receiver monitor connection status and connection status message
    typedef std::function<bool(nc_oid oid, nmos::nc_connection_status::status connection_status, const utility::string_t& connection_status_message)> set_receiver_monitor_connection_status_handler;
    set_receiver_monitor_connection_status_handler make_set_receiver_monitor_connection_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // Set receiver monitor external synchronization status and external synchronization status message
    typedef std::function<bool(nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message)> set_receiver_monitor_external_synchronization_status_handler;
    set_receiver_monitor_external_synchronization_status_handler make_set_receiver_monitor_external_synchronization_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // Set receiver monitor stream status and stream status message
    typedef std::function<bool(nc_oid oid, nmos::nc_stream_status::status stream_status, const utility::string_t& stream_status_message)> set_receiver_monitor_stream_status_handler;
    set_receiver_monitor_stream_status_handler make_set_receiver_monitor_stream_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // Set receiver monitor synchronization source id
    typedef std::function<bool(nc_oid oid, const bst::optional<utility::string_t>& source_id)> set_receiver_monitor_synchronization_source_id_handler;
    set_receiver_monitor_synchronization_source_id_handler make_set_receiver_monitor_synchronization_source_id_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // NcSenderMonitor handlers

    // Set sender monitor link status and link status message
    typedef std::function<bool(nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message)> set_sender_monitor_link_status_handler;
    set_sender_monitor_link_status_handler make_set_sender_monitor_link_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // Set sender monitor transmission status and transmission status message
    typedef std::function<bool(nc_oid oid, nmos::nc_transmission_status::status transmission_status, const utility::string_t& transmission_status_message)> set_sender_monitor_transmission_status_handler;
    set_sender_monitor_transmission_status_handler make_set_sender_monitor_transmission_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // Set sender monitor external synchronization status and external synchronization status message
    typedef std::function<bool(nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message)> set_sender_monitor_external_synchronization_status_handler;
    set_sender_monitor_external_synchronization_status_handler make_set_sender_monitor_external_synchronization_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // Set sender monitor essence status and essence status message
    typedef std::function<bool(nc_oid oid, nmos::nc_essence_status::status essence_status, const utility::string_t& essence_status_message)> set_sender_monitor_essence_status_handler;
    set_sender_monitor_essence_status_handler make_set_sender_monitor_essence_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // Set sender monitor synchronization source id
    typedef std::function<bool(nc_oid oid, const bst::optional<utility::string_t>& source_id)> set_sender_monitor_synchronization_source_id_handler;
    set_sender_monitor_synchronization_source_id_handler make_set_sender_monitor_synchronization_source_id_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);
}

#endif

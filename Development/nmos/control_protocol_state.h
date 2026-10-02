#ifndef NMOS_CONTROL_PROTOCOL_STATE_H
#define NMOS_CONTROL_PROTOCOL_STATE_H

#include <map>
#include "bst/optional.h"
#include "cpprest/json_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/control_protocol_handlers.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/mutex.h"

namespace slog { class base_gate; }

namespace nmos
{
    namespace experimental
    {
        // Library-only metadata describing a status domain.
        // This is not included in the NcClassDescriptor exposed by the control protocol.
        struct monitor_domain
        {
            // Status values which contribute to overall status must use NcOverallStatus-compatible values.
            nc_property_id status_property_id;
            nc_property_id status_message_property_id;
            nc_property_id status_transition_counter_property_id;
            utility::string_t status_pending_field_name;
            utility::string_t status_message_pending_field_name;
            utility::string_t status_pending_received_time_field_name;
            // A matching inactive status forces the overall status to Inactive.
            bst::optional<int32_t> inactive_status;
            // A matching ignored status is neutral and is omitted from the overall status calculation.
            bst::optional<int32_t> ignored_status;
            bst::optional<int32_t> activation_status;
            utility::string_t activation_message;
            bst::optional<int32_t> deactivation_status;
            utility::string_t deactivation_message;

            monitor_domain(const nc_property_id& status_property_id, const nc_property_id& status_message_property_id, const nc_property_id& status_transition_counter_property_id,
                const utility::string_t& status_pending_field_name, const utility::string_t& status_message_pending_field_name, const utility::string_t& status_pending_received_time_field_name,
                const bst::optional<int32_t>& inactive_status = {}, const bst::optional<int32_t>& ignored_status = {},
                const bst::optional<int32_t>& activation_status = {}, const utility::string_t& activation_message = {},
                const bst::optional<int32_t>& deactivation_status = {}, const utility::string_t& deactivation_message = {})
                : status_property_id(status_property_id)
                , status_message_property_id(status_message_property_id)
                , status_transition_counter_property_id(status_transition_counter_property_id)
                , status_pending_field_name(status_pending_field_name)
                , status_message_pending_field_name(status_message_pending_field_name)
                , status_pending_received_time_field_name(status_pending_received_time_field_name)
                , inactive_status(inactive_status)
                , ignored_status(ignored_status)
                , activation_status(activation_status)
                , activation_message(activation_message)
                , deactivation_status(deactivation_status)
                , deactivation_message(deactivation_message)
            {}
        };

        struct control_class_descriptor // NcClassDescriptor
        {
            utility::string_t description;
            nmos::nc_class_id class_id;
            nmos::nc_name name;
            web::json::value fixed_role;

            web::json::value property_descriptors = web::json::value::array(); // NcPropertyDescriptor array
            std::vector<method> method_descriptors; // NcMethodDescriptor method handler array
            web::json::value event_descriptors = web::json::value::array();  // NcEventDescriptor array

            control_class_descriptor()
                : class_id({ 0 })
            {}

            control_class_descriptor(utility::string_t description, nmos::nc_class_id class_id, nmos::nc_name name, web::json::value fixed_role, web::json::value property_descriptors, std::vector<method> method_descriptors, web::json::value event_descriptors)
                : description(std::move(description))
                , class_id(std::move(class_id))
                , name(std::move(name))
                , fixed_role(std::move(fixed_role))
                , property_descriptors(std::move(property_descriptors))
                , method_descriptors(std::move(method_descriptors))
                , event_descriptors(std::move(event_descriptors))
            {}
        };

        struct datatype_descriptor // NcDatatypeDescriptorEnum/NcDatatypeDescriptorPrimitive/NcDatatypeDescriptorStruct/NcDatatypeDescriptorTypeDef
        {
            web::json::value descriptor;
        };

        typedef std::map<nmos::nc_class_id, control_class_descriptor> control_class_descriptors;
        typedef std::map<nmos::nc_name, datatype_descriptor> datatype_descriptors;
        typedef std::map<nmos::nc_class_id, std::vector<monitor_domain>> monitor_domain_profiles;

        struct control_protocol_state
        {
            // mutex to be used to protect the members from simultaneous access by multiple threads
            mutable nmos::mutex mutex;

            // true : at least one of the receiver/sender monitors statuses is pending
            // false: no more receiver/sender monitors statuses are pending
            bool monitor_status_pending;

            experimental::control_class_descriptors control_class_descriptors;
            experimental::datatype_descriptors datatype_descriptors;
            experimental::monitor_domain_profiles monitor_domain_profiles;

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }

            control_protocol_state(control_protocol_property_changed_handler property_changed = nullptr, create_validation_fingerprint_handler create_validation_fingerprint = nullptr, validate_validation_fingerprint_handler validate_validation_fingerprintget_read_only_modification_allow_list_handler = nullptr, get_read_only_modification_allow_list_handler get_read_only_modification_allow_list = nullptr, remove_device_model_object_handler remove_device_model_object = nullptr, create_device_model_object_handler create_device_model_object = nullptr, get_packet_counters_handler get_lost_packet_counters = nullptr, get_packet_counters_handler get_late_packet_counters = nullptr, reset_monitor_handler reset_monitor = nullptr);
            // insert control class descriptor, false if class descriptor already inserted
            bool insert(const experimental::control_class_descriptor& control_class_descriptor);
            // erase control class of the given class id, false if the required class not found
            bool erase(nc_class_id class_id);

            // insert datatype descriptor, false if datatype descriptor already inserted
            bool insert(const experimental::datatype_descriptor& datatype_descriptor);
            // erase datatype descriptor of the given datatype name, false if the required datatype descriptor not found
            bool erase(const utility::string_t& datatype_name);

            // insert monitor domains for the given class id, false if a profile already exists
            bool insert_monitor_domains(const nc_class_id& class_id, const std::vector<monitor_domain>& monitor_domains);
            // erase monitor domains for the given class id, false if a profile was not found
            bool erase_monitor_domains(const nc_class_id& class_id);
        };

        std::vector<monitor_domain> make_receiver_monitor_domains();
        std::vector<monitor_domain> make_sender_monitor_domains();

        // helper functions to create non-standard control class
        //

        // create control class descriptor with fixed role
        control_class_descriptor make_control_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const utility::string_t& fixed_role, const std::vector<web::json::value>& properties = {}, const std::vector<method>& methods = {}, const std::vector<web::json::value>& events = {});
        // create control class descriptor with no fixed role
        control_class_descriptor make_control_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const std::vector<web::json::value>& properties = {}, const std::vector<method>& methods = {}, const std::vector<web::json::value>& events = {});

        // create control class property descriptor
        web::json::value make_control_class_property_descriptor(const utility::string_t& description, const nc_property_id& id, const nc_name& name, const utility::string_t& type_name,
            bool is_read_only = false, bool is_nullable = false, bool is_sequence = false, bool is_deprecated = false, const web::json::value& constraints = web::json::value::null());

        // create control class method parameter descriptor
        web::json::value make_control_class_method_parameter_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name,
            bool is_nullable = false, bool is_sequence = false, const web::json::value& constraints = web::json::value::null());
        // create control class method descriptor
        method make_control_class_method_descriptor(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype,
            const std::vector<web::json::value>& parameters, bool is_deprecated, control_protocol_method_handler method_handler);

        // create control class event descriptor
        web::json::value make_control_class_event_descriptor(const utility::string_t& description, const nc_event_id& id, const nc_name& name, const utility::string_t& event_datatype,
            bool is_deprecated = false);
    }
}

#endif

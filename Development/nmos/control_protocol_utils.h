#ifndef NMOS_CONTROL_PROTOCOL_UTILS_H
#define NMOS_CONTROL_PROTOCOL_UTILS_H

#include "cpprest/basic_utils.h"
#include "nmos/control_protocol_handlers.h"

namespace nmos
{
    struct control_protocol_resource;

    struct control_protocol_exception : std::runtime_error
    {
        control_protocol_exception(const std::string& message) : std::runtime_error(message) {}
    };

    namespace nc
    {
        namespace details
        {
            // get the runtime property constraints of a given property_id
            web::json::value get_runtime_property_constraints(const nc_property_id& property_id, const web::json::value& runtime_property_constraints_list);

            // get the datatype descriptor of a specific type_name
            web::json::value get_datatype_descriptor(const web::json::value& type_name, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype);

            // get the datatype property constraints of a given type_name
            web::json::value get_datatype_constraints(const web::json::value& type_name, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype);

	        struct datatype_constraints_validation_parameters
	        {
	            web::json::value datatype_descriptor;
	            get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor;
	        };
	        // multiple levels of constraints validation, may throw nmos::control_protocol_exception
	        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
	        void constraints_validation(const web::json::value& value, const web::json::value& runtime_property_constraints, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params);

	        // method parameter constraints validation, may throw nmos::control_protocol_exception
	        void method_parameter_constraints_validation(const web::json::value& data, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params);

            // convert . delimited string into role path object
            web::json::value parse_role_path(const utility::string_t& role_path);

	        bool set_monitor_status(resources& resources, nc_oid oid, int status, const utility::string_t& status_message, const nc_property_id& status_property_id,
	            const nc_property_id& status_message_property_id,
	            const nc_property_id& status_transition_counter_property_id,
	            const utility::string_t& status_pending_received_time_field_name,
	            get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor,
	            slog::base_gate& gate);
	        bool set_monitor_status_with_delay(resources& resources, nc_oid oid, int status, const utility::string_t& status_message,
	             const nc_property_id& status_property_id,
	             const nc_property_id& status_message_property_id,
	             const nc_property_id& status_transition_counter_property_id,
	             const utility::string_t& status_pending_field_name,
	             const utility::string_t& status_message_pending_time_field_name,
	             const utility::string_t& status_pending_received_time_field_name,
	             long long current_time,
	             monitor_status_pending_handler monitor_status_pending,
	             get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor,
	             slog::base_gate& gate);
	        bool update_receiver_monitor_overall_status(resources& resources, nc_oid oid, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
	        bool update_sender_monitor_overall_status(resources& resources, nc_oid oid, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        }

        // is the given class_id a NcBlock
        bool is_block(const nc_class_id& class_id);

        // is the given class_id a NcWorker
        bool is_worker(const nc_class_id& class_id);

        // is the given class_id a NcManager
        bool is_manager(const nc_class_id& class_id);

        // is the given class_id a NcDeviceManager
        bool is_device_manager(const nc_class_id& class_id);

        // is the given class_id a NcClassManager
        bool is_class_manager(const nc_class_id& class_id);

	    // is the given class_id a NcStatusMonitor
	    bool is_status_monitor(const nc_class_id& class_id);

	    // is the given class_id a NcSenderMonitor
	    bool is_sender_monitor(const nc_class_id& class_id);

	    // construct NcClassId
	    nc_class_id make_class_id(const nc_class_id& prefix, int32_t authority_key, const std::vector<int32_t>& suffix);
	    nc_class_id make_class_id(const nc_class_id& prefix, const std::vector<int32_t>& suffix); // using default authority_key 0

        // find control class property descriptor (NcPropertyDescriptor)
        web::json::value find_property_descriptor(const nc_property_id& property_id, const nc_class_id& class_id, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor);

        // get block memeber descriptors
        void get_member_descriptors(const resources& resources, const resource& resource, bool recurse, web::json::array& descriptors);

        // find members with given role name or fragment
        void find_members_by_role(const resources& resources, const resource& resource, const utility::string_t& role, bool match_whole_string, bool case_sensitive, bool recurse, web::json::array& nc_block_member_descriptors);

        // find members with given class id
        void find_members_by_class_id(const resources& resources, const resource& resource, const nc_class_id& class_id, bool include_derived, bool recurse, web::json::array& descriptors);

        // push control protocol resource into other control protocol NcBlock resource
        void push_back(control_protocol_resource& nc_block_resource, const control_protocol_resource& resource);

        // insert root block and all sub control protocol resources
        void insert_root(resources& resources, control_protocol_resource& root);

        // insert a control protocol resource
        std::pair<resources::iterator, bool> insert_resource(resources& resources, resource&& resource);

        // modify a control protocol resource, and insert notification event to all subscriptions
        bool modify_resource(resources& resources, const id& id, std::function<void(resource&)> modifier, const web::json::value& notification_event=web::json::value::null());

        // erase a control protocol resource
        resources::size_type erase_resource(resources& resources, const id& id);

        // find the control protocol resource which is associated with the given IS-04/IS-05/IS-08 resource id
        resources::const_iterator find_resource(resources& resources, type type, const id& id);

        // find resource based on role path.
        resources::const_iterator find_resource_by_role_path(const resources& resources, const web::json::array& role_path);

        // find resource based on role path. Roles in role path string must be delimited with a '.'
        resources::const_iterator find_resource_by_role_path(const resources& resources, const utility::string_t& role_path);

        // method parameters constraints validation, may throw nmos::control_protocol_exception
        void method_parameters_contraints_validation(const web::json::value& arguments, const web::json::value& nc_method_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor);

        // Validate that the resource has been correctly constructed according to the class_descriptor
        void validate_resource(const resource& resource, const experimental::control_class_descriptor& class_descriptor);

        resources::const_iterator find_touchpoint_resource(const resources& resources, const resource& resource);

        // insert 'value changed', 'sequence item added', 'sequence item changed' or 'sequence item removed' notification events into all grains whose subscriptions match the specified version, type and "pre" or "post" values
        void insert_notification_events(resources& resources, const api_version& version, const api_version& downgrade_version, const type& type, const web::json::value& pre, const web::json::value& post, const web::json::value& event);

        // get property value given oid and property_id
        web::json::value get_property(const resources& resources, nc_oid oid, const nc_property_id& property_id, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // set property value given oid and property_id and notify
        bool set_property_and_notify(resources& resources, nc_oid oid, const nc_property_id& property_id, const web::json::value& value, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // set hidden property but don't notify, as property isn't part of class definition
        bool set_property(resources& resources, nc_oid oid, const utility::string_t& property_name, const web::json::value& value, slog::base_gate& gate);

        // Set link status and link status message
        bool set_receiver_monitor_link_status(resources& resources, nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set link status and status message and apply status reporting delay
        bool set_receiver_monitor_link_status_with_delay(resources& resources, nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Set connection status and connection status message
        bool set_receiver_monitor_connection_status(resources& resources, nc_oid oid, nmos::nc_connection_status::status connection_status, const utility::string_t& connection_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set connection status and status message and apply status reporting delay
        bool set_receiver_monitor_connection_status_with_delay(resources& resources, nc_oid oid, nmos::nc_connection_status::status connection_status, const utility::string_t& connection_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Set external synchronization status and external synchronization status message
        bool set_receiver_monitor_external_synchronization_status(resources& resources, nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set external synchronization status and status message and apply status reporting delay
        bool set_receiver_monitor_external_synchronization_status_with_delay(resources& resources, nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Set stream status and stream status message
        bool set_receiver_monitor_stream_status(resources& resources, nc_oid oid, nmos::nc_stream_status::status stream_status, const utility::string_t& stream_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set stream status and status message and apply status reporting delay
        bool set_receiver_monitor_stream_status_with_delay(resources& resources, nc_oid oid, nmos::nc_stream_status::status stream_status, const utility::string_t& stream_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Set synchronization source id
        bool set_monitor_synchronization_source_id(resources& resources, nc_oid oid, const bst::optional<utility::string_t>& source_id, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Call when monitor is activated
        bool activate_monitor(resources& resources, nc_oid oid, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_method_descriptor_handler get_control_protocol_method_descriptor, slog::base_gate& gate);
        // Call when monitor is deactivated
        bool deactivate_monitor(resources& resources, nc_oid oid, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Set link status and link status message
        bool set_sender_monitor_link_status(resources& resources, nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set link status and status message and apply status reporting delay
        bool set_sender_monitor_link_status_with_delay(resources& resources, nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Set transmission status and transmission status message
        bool set_sender_monitor_transmission_status(resources& resources, nc_oid oid, nmos::nc_transmission_status::status transmission_status, const utility::string_t& transmission_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set transmission status and status message and apply status reporting delay
        bool set_sender_monitor_transmission_status_with_delay(resources& resources, nc_oid oid, nmos::nc_transmission_status::status transmission_status, const utility::string_t& transmission_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Set external synchronization status and external synchronization status message
        bool set_sender_monitor_external_synchronization_status(resources& resources, nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set external synchronization status and status message and apply status reporting delay
        bool set_sender_monitor_external_synchronization_status_with_delay(resources& resources, nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Set essence status and stream status message
        bool set_sender_monitor_essence_status(resources& resources, nc_oid oid, nmos::nc_essence_status::status essence_status, const utility::string_t& essence_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set essence status and status message and apply status reporting delay
        bool set_sender_monitor_essence_status_with_delay(resources& resources, nc_oid oid, nmos::nc_essence_status::status essence_status, const utility::string_t& essence_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // Get property by name, rather than by property id. Used to get "hidden" resource properties
        web::json::value get_property(const resources& resources, nc_oid oid, const utility::string_t& property_name, slog::base_gate& gate);
    }

    namespace details
    {
        // Deprecated: use nc::details::get_runtime_property_constraints
        inline web::json::value get_runtime_property_constraints(const nc_property_id& property_id, const web::json::value& runtime_property_constraints_list) { return nc::details::get_runtime_property_constraints(property_id, runtime_property_constraints_list); }

        // Deprecated: use nc::details::get_datatype_descriptor
        inline web::json::value get_datatype_descriptor(const web::json::value& type_name, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype) { return nc::details::get_datatype_descriptor(type_name, get_control_protocol_datatype); }

        // Deprecated: use nc::details::get_datatype_constraints
        inline web::json::value get_datatype_constraints(const web::json::value& type_name, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype) { return nc::details::get_datatype_constraints(type_name, get_control_protocol_datatype); }

        struct datatype_constraints_validation_parameters
        {
            web::json::value datatype_descriptor;
            get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor;
        };
        // Deprecated: use nc::details::constraints_validation
        inline void constraints_validation(const web::json::value& value, const web::json::value& runtime_property_constraints, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params) { return nc::details::constraints_validation(value, runtime_property_constraints, property_constraints, nc::details::datatype_constraints_validation_parameters{params.datatype_descriptor, params.get_control_protocol_datatype_descriptor}); }

        // Deprecated: use nc::details::method_parameter_constraints_validation
        inline void method_parameter_constraints_validation(const web::json::value& data, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params) { return nc::details::method_parameter_constraints_validation(data, property_constraints, nc::details::datatype_constraints_validation_parameters{params.datatype_descriptor, params.get_control_protocol_datatype_descriptor}); }
    }

    // Deprecated: use nc::is_block
    inline bool is_nc_block(const nc_class_id& class_id) { return nc::is_block(class_id); }

    // Deprecated: use nc::is_worker
    inline bool is_nc_worker(const nc_class_id& class_id) { return nc::is_worker(class_id); }

    // Deprecated: use nc::is_manager
    inline bool is_nc_manager(const nc_class_id& class_id) { return nc::is_manager(class_id); }

    // Deprecated: use nc::is_device_manager
    inline bool is_nc_device_manager(const nc_class_id& class_id) { return nc::is_device_manager(class_id); }

    // Deprecated: use nc::is_class_manager
    inline bool is_nc_class_manager(const nc_class_id& class_id) { return nc::is_class_manager(class_id); }

    // Deprecated: use nc::make_class_id
    inline nc_class_id make_nc_class_id(const nc_class_id& prefix, int32_t authority_key, const std::vector<int32_t>& suffix) { return nc::make_class_id(prefix, authority_key, suffix); }
    inline nc_class_id make_nc_class_id(const nc_class_id& prefix, const std::vector<int32_t>& suffix) { return nc::make_class_id(prefix, suffix); }

    // Deprecated: use nc::find_property_descriptor
    inline web::json::value find_property_descriptor(const nc_property_id& property_id, const nc_class_id& class_id, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor) { return nc::find_property_descriptor(property_id, class_id, get_control_protocol_class_descriptor); }

    // Deprecated: use nc::get_member_descriptors
    inline void get_member_descriptors(const resources& resources, const resource& resource, bool recurse, web::json::array& descriptors) { return nc::get_member_descriptors(resources, resource, recurse, descriptors); }

    // Deprecated: use nc::find_members_by_role
    inline void find_members_by_role(const resources& resources, const resource& resource, const utility::string_t& role, bool match_whole_string, bool case_sensitive, bool recurse, web::json::array& nc_block_member_descriptors) { return nc::find_members_by_role(resources, resource, role, match_whole_string, case_sensitive, recurse, nc_block_member_descriptors); }

    // Deprecated: use nc::find_members_by_class_id
    inline void find_members_by_class_id(const resources& resources, const resource& resource, const nc_class_id& class_id, bool include_derived, bool recurse, web::json::array& descriptors) { return nc::find_members_by_class_id(resources, resource, class_id, include_derived, recurse, descriptors); }

    // Deprecated: use nc::push_back
    inline void push_back(control_protocol_resource& nc_block_resource, const control_protocol_resource& resource) { return nc::push_back(nc_block_resource, resource); }

    // Deprecated: use nc::insert_resource
    inline std::pair<resources::iterator, bool> insert_control_protocol_resource(resources& resources, resource&& resource) { return nc::insert_resource(resources, std::move(resource)); }

    // Deprecated: use nc::modify_resource
    inline bool modify_control_protocol_resource(resources& resources, const id& id, std::function<void(resource&)> modifier, const web::json::value& notification_event = web::json::value::null()) { return nc::modify_resource(resources, id, modifier, notification_event); }

    // Deprecated: use nc::erase_resource
    inline resources::size_type erase_control_protocol_resource(resources& resources, const id& id) { return nc::erase_resource(resources, id); }

    // Deprecated: use nc::find_resource
    inline resources::const_iterator find_control_protocol_resource(resources& resources, type type, const id& id) { return nc::find_resource(resources, type, id); }

    // Deprecated: use nc::method_parameters_contraints_validation
    inline void method_parameters_contraints_validation(const web::json::value& arguments, const web::json::value& nc_method_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor) { return nc::method_parameters_contraints_validation(arguments, nc_method_descriptor, get_control_protocol_datatype_descriptor); }

    // Deprecated: use nc::insert_notification_events
    inline void insert_notification_events(resources& resources, const api_version& version, const api_version& downgrade_version, const type& type, const web::json::value& pre, const web::json::value& post, const web::json::value& event) { return nc::insert_notification_events(resources, version, downgrade_version, type, pre, post, event); }

    // Deprecated: use nc::get_property
    inline web::json::value get_control_protocol_property(const resources& resources, nc_oid oid, const nc_property_id& property_id, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate) { return nc::get_property(resources, oid, property_id, get_control_protocol_class_descriptor, gate); }
}

#endif

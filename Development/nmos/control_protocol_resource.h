#ifndef NMOS_CONTROL_PROTOCOL_RESOURCE_H
#define NMOS_CONTROL_PROTOCOL_RESOURCE_H

#include "cpprest/json_utils.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/resource.h"

namespace web
{
    namespace json
    {
        class value;
    }
    class uri;
}

namespace nmos
{
    struct control_protocol_resource : resource
    {
        control_protocol_resource()
            : resource()
        {}

        control_protocol_resource(api_version version, nmos::type type, web::json::value&& data, nmos::id id, bool never_expire)
            : resource(version, type, std::move(data), id, never_expire)
        {}

        control_protocol_resource(api_version version, nmos::type type, web::json::value data, bool never_expire)
            : resource(version, type, data, never_expire)
        {}

        // temporary storage to hold the resources until they are moved to the model resources
        std::vector<control_protocol_resource> resources;
    };
}

namespace nmos
{
    namespace experimental
    {
        struct control_protocol_state;
    }

    namespace nc
    {
        namespace details
        {
            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethodresult
            web::json::value make_method_result(const nc_method_result& method_result);
            web::json::value make_method_result_error(const nc_method_result& method_result, const utility::string_t& error_message);
            web::json::value make_method_result(const nc_method_result& method_result, const web::json::value& value);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncelementid
            web::json::value make_element_id(const nc_element_id& element_id);
            nc_element_id parse_element_id(const web::json::value& element_id);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nceventid
            web::json::value make_event_id(const nc_event_id& event_id);
            nc_event_id parse_event_id(const web::json::value& event_id);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethodid
            web::json::value make_method_id(const nc_method_id& method_id);
            nc_method_id parse_method_id(const web::json::value& method_id);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyid
            web::json::value make_property_id(const nc_property_id& property_id);
            nc_property_id parse_property_id(const web::json::value& property_id);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassid
            web::json::value make_class_id(const nc_class_id& class_id);
            nc_class_id parse_class_id(const web::json::array& class_id);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanufacturer
            web::json::value make_manufacturer(const utility::string_t& name, nc_organization_id organization_id, const web::uri& website);
            web::json::value make_manufacturer(const utility::string_t& name, nc_organization_id organization_id);
            web::json::value make_manufacturer(const utility::string_t& name);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncproduct
            web::json::value make_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
                const utility::string_t& brand_name, const nc_uuid& uuid, const utility::string_t& description);
            web::json::value make_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
                const utility::string_t& brand_name, const nc_uuid& uuid);
            web::json::value make_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
                const utility::string_t& brand_name);
            web::json::value make_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdeviceoperationalstate
            // device_specific_details can be null
            web::json::value make_device_operational_state(nc_device_generic_state::state generic_state, const web::json::value& device_specific_details);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblockmemberdescriptor
            web::json::value make_block_member_descriptor(const utility::string_t& description, const utility::string_t& role, nc_oid oid, bool constant_oid, const nc_class_id& class_id, const utility::string_t& user_label, nc_oid owner);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassdescriptor
            web::json::value make_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const utility::string_t& fixed_role, const web::json::value& properties, const web::json::value& methods, const web::json::value& events);
            web::json::value make_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const web::json::value& properties, const web::json::value& methods, const web::json::value& events);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncenumitemdescriptor
            web::json::value make_enum_item_descriptor(const utility::string_t& description, const nc_name& name, uint16_t val);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nceventdescriptor
            web::json::value make_event_descriptor(const utility::string_t& description, const nc_event_id& id, const nc_name& name, const utility::string_t& event_datatype, bool is_deprecated);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncfielddescriptor
            // constraints can be null
            web::json::value make_field_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints);
            web::json::value make_field_descriptor(const utility::string_t& description, const nc_name& name, bool is_nullable, bool is_sequence, const web::json::value& constraints);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethoddescriptor
            // sequence<NcParameterDescriptor> parameters
            web::json::value make_method_descriptor(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype, const web::json::value& parameters, bool is_deprecated);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterdescriptor
            // constraints can be null
            web::json::value make_parameter_descriptor(const utility::string_t& description, const nc_name& name, bool is_nullable, bool is_sequence, const web::json::value& constraints);
            web::json::value make_parameter_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertydescriptor
            // constraints can be null
            web::json::value make_property_descriptor(const utility::string_t& description, const nc_property_id& id, const nc_name& name, const utility::string_t& type_name,
                bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorenum
            // constraints can be null
            // items: sequence<NcEnumItemDescriptor>
            web::json::value make_datatype_descriptor_enum(const utility::string_t& description, const nc_name& name, const web::json::value& items, const web::json::value& constraints);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorprimitive
            // constraints can be null
            web::json::value make_datatype_descriptor_primitive(const utility::string_t& description, const nc_name& name, const web::json::value& constraints);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorstruct
            // constraints can be null
            // fields: sequence<NcFieldDescriptor>
            web::json::value make_datatype_descriptor_struct(const utility::string_t& description, const nc_name& name, const web::json::value& fields, const utility::string_t& parent_type, const web::json::value& constraints);
            web::json::value make_datatype_descriptor_struct(const utility::string_t& description, const nc_name& name, const web::json::value& fields, const web::json::value& constraints);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptortypedef
            web::json::value make_datatype_typedef(const utility::string_t& description, const nc_name& name, bool is_sequence, const utility::string_t& parent_type, const web::json::value& constraints);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyconstraints
            web::json::value make_property_constraints(const nc_property_id& property_id, const web::json::value& default_value);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyconstraintsnumber
            web::json::value make_property_constraints_number(const nc_property_id& property_id, uint64_t default_value, uint64_t minimum, uint64_t maximum, uint64_t step);
            web::json::value make_property_constraints_number(const nc_property_id& property_id, uint64_t minimum, uint64_t maximum, uint64_t step);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyconstraintsstring
            web::json::value make_property_constraints_string(const nc_property_id& property_id, const utility::string_t& default_value, uint32_t max_characters, const nc_regex& pattern);
            web::json::value make_property_constraints_string(const nc_property_id& property_id, uint32_t max_characters, const nc_regex& pattern);
            web::json::value make_property_constraints_string(const nc_property_id& property_id, uint32_t max_characters);
            web::json::value make_property_constraints_string(const nc_property_id& property_id, const nc_regex& pattern);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraints
            web::json::value make_parameter_constraints(const web::json::value& default_value);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsnumber
            web::json::value make_parameter_constraints_number(uint64_t default_value, uint64_t minimum, uint64_t maximum, uint64_t step);
            web::json::value make_parameter_constraints_number(uint64_t minimum, uint64_t maximum, uint64_t step);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsstring
            web::json::value make_parameter_constraints_string(const utility::string_t& default_value, uint32_t max_characters, const nc_regex& pattern);
            web::json::value make_parameter_constraints_string(uint32_t max_characters, const nc_regex& pattern);
            web::json::value make_parameter_constraints_string(uint32_t max_characters);
            web::json::value make_parameter_constraints_string(const nc_regex& pattern);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpoint
            web::json::value make_touchpoint(const utility::string_t& context_namespace);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointnmos
            web::json::value make_touchpoint_nmos(const nc_touchpoint_resource_nmos& resource);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointnmoschannelmapping
            web::json::value make_touchpoint_nmos_channel_mapping(const nc_touchpoint_resource_nmos_channel_mapping& resource);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
            web::json::value make_object(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
            web::json::value make_block(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, const web::json::value& members);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncworker
            web::json::value make_worker(const nc_class_id& class_id, nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled);

            // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncstatusmonitor
            web::json::value make_status_monitor(const nc_class_id& class_id, nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, nc_overall_status::status overall_status, const utility::string_t& overall_status_message, uint64_t status_reporting_delay);

            // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
            web::json::value make_receiver_monitor(const nc_class_id& class_id, nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, nc_overall_status::status overall_status, const utility::string_t& overall_status_message, nc_link_status::status link_status, const utility::string_t& link_status_message, nc_connection_status::status connection_status, const utility::string_t& connection_status_message, nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, const web::json::value& synchronization_source_id, nc_stream_status::status stream_status, const utility::string_t& stream_status_message, uint32_t status_reporting_delay, bool auto_reset_monitor);

            // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncsendermonitor
            web::json::value make_sender_monitor(const nc_class_id& class_id, nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, nc_overall_status::status overall_status, const utility::string_t& overall_status_message, nc_link_status::status link_status, const utility::string_t& link_status_message, nc_transmission_status::status transmission_status, const utility::string_t& transmission_status_message, nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, const web::json::value& synchronization_source_id, nc_essence_status::status essence_status, const utility::string_t& essence_status_message, uint32_t status_reporting_delay, bool auto_reset_monitor);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanager
            web::json::value make_manager(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
            web::json::value make_device_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints,
                const web::json::value& manufacturer, const web::json::value& product, const utility::string_t& serial_number,
                const web::json::value& user_inventory_code, const web::json::value& device_name, const web::json::value& device_role, const web::json::value& operational_state, nc_reset_cause::cause reset_cause);

            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
            web::json::value make_class_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const nmos::experimental::control_protocol_state& control_protocol_state);

            // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncbulkpropertiesmanager
            web::json::value make_bulk_properties_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints);

            // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncbulkpropertiesholder
            web::json::value make_bulk_properties_holder(const utility::string_t& validation_fingerprint, const web::json::value& object_properties_holders);

            // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncpropertyholder
            web::json::value make_property_holder(const nc_property_id& property_id, const web::json::value& descriptor, const web::json::value& property_value);

            // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncobjectpropertiesholder
            web::json::value make_object_properties_holder(const web::json::array& role_path, const web::json::array& property_holders, const web::json::array& dependency_paths, const web::json::array& allowed_members_classes, bool is_rebuildable);

            // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncpropertyrestorenotice
            web::json::value make_property_restore_notice(const nc_property_id& property_id, const nc_name& name, nc_property_restore_notice_type::type notice_type, const utility::string_t& notice_message);

            // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncobjectpropertiessetvalidation
            web::json::value make_object_properties_set_validation(const web::json::array& role_path, nc_restore_validation_status::status status, const web::json::array& notices, const web::json::value& status_message);
        }

        // command message response
        // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#command-response-message-type
        web::json::value make_response(int32_t handle, const web::json::value& method_result);
        web::json::value make_command_response(const web::json::value& responses);

        // subscription response
        // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#subscription-response-message-type
        web::json::value make_subscription_response(const web::json::value& subscriptions);

        // notification
        // See https://specs.amwa.tv/ms-05-01/branches/v1.0.x/docs/Core_Mechanisms.html#notification-messages
        // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#notification-message-type
        web::json::value make_notification(nc_oid oid, const nc_event_id& event_id, const nc_property_changed_event_data& property_changed_event_data);
        web::json::value make_notification_message(const web::json::value& notifications);

        // property changed notification event
        // See https://specs.amwa.tv/ms-05-01/branches/v1.0.x/docs/Core_Mechanisms.html#the-propertychanged-event
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/NcObject.html#propertychanged-event
        web::json::value make_property_changed_event(nc_oid oid, const std::vector<nc_property_changed_event_data>& property_changed_event_data_list);

        // error message
        // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#error-messages
        web::json::value make_error_message(const nc_method_result& method_result, const utility::string_t& error_message);

        // Control class models
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/#control-class-models-for-branch-v10-dev
        //
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.html
        web::json::value make_object_class();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.1.html
        web::json::value make_block_class();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.2.html
        web::json::value make_worker_class();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.html
        web::json::value make_manager_class();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.1.html
        web::json::value make_device_manager_class();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.2.html
        web::json::value make_class_manager_class();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
        web::json::value make_ident_beacon_class();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
        web::json::value make_receiver_monitor_class();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncsendermonitor
        web::json::value make_sender_monitor_class();

        // control classes properties/methods/events
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
        web::json::value make_object_properties();
        web::json::value make_object_methods();
        web::json::value make_object_events();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
        web::json::value make_block_properties();
        web::json::value make_block_methods();
        web::json::value make_block_events();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncworker
        web::json::value make_worker_properties();
        web::json::value make_worker_methods();
        web::json::value make_worker_events();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanager
        web::json::value make_manager_properties();
        web::json::value make_manager_methods();
        web::json::value make_manager_events();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
        web::json::value make_device_manager_properties();
        web::json::value make_device_manager_methods();
        web::json::value make_device_manager_events();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
        web::json::value make_class_manager_properties();
        web::json::value make_class_manager_methods();
        web::json::value make_class_manager_events();
        // Monitoring feature set control classes
        // https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncstatusmonitor
        web::json::value make_status_monitor_properties();
        web::json::value make_status_monitor_methods();
        web::json::value make_status_monitor_events();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
        web::json::value make_receiver_monitor_properties();
        web::json::value make_receiver_monitor_methods();
        web::json::value make_receiver_monitor_events();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncsendermonitor
        web::json::value make_sender_monitor_properties();
        web::json::value make_sender_monitor_methods();
        web::json::value make_sender_monitor_events();

        // Identification feature set control classes
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
        web::json::value make_ident_beacon_properties();
        web::json::value make_ident_beacon_methods();
        web::json::value make_ident_beacon_events();

        // Device configuration classes
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncbulkpropertiesmanager
        web::json::value make_bulk_properties_manager_properties();
        web::json::value make_bulk_properties_manager_methods();
        web::json::value make_bulk_properties_manager_events();

        // Datatype models
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/#datatype-models-for-branch-v10-dev
        //
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_boolean_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_int16_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_int32_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_int64_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_uint16_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_uint32_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_uint64_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_float32_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_float64_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
        web::json::value make_string_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcBlockMemberDescriptor.html
        web::json::value make_block_member_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcClassDescriptor.html
        web::json::value make_class_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcClassId.html
        web::json::value make_class_id_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptor.html
        web::json::value make_datatype_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorEnum.html
        web::json::value make_datatype_descriptor_enum_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorPrimitive.html
        web::json::value make_datatype_descriptor_primitive_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorStruct.html
        web::json::value make_datatype_descriptor_struct_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorTypeDef.html
        web::json::value make_datatype_descriptor_type_def_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeType.html
        web::json::value make_datatype_type_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDescriptor.html
        web::json::value make_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDeviceGenericState.html
        web::json::value make_device_generic_state_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDeviceOperationalState.html
        web::json::value make_device_operational_state_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcElementId.html
        web::json::value make_element_id_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEnumItemDescriptor.html
        web::json::value make_enum_item_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEventDescriptor.html
        web::json::value make_event_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEventId.html
        web::json::value make_event_id_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcFieldDescriptor.html
        web::json::value make_field_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcId.html
        web::json::value make_id_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcManufacturer.html
        web::json::value make_manufacturer_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodDescriptor.html
        web::json::value make_method_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodId.html
        web::json::value make_method_id_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResult.html
        web::json::value make_method_result_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultBlockMemberDescriptors.html
        web::json::value make_method_result_block_member_descriptors_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultClassDescriptor.html
        web::json::value make_method_result_class_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultDatatypeDescriptor.html
        web::json::value make_method_result_datatype_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultError.html
        web::json::value make_method_result_error_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultId.html
        web::json::value make_method_result_id_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultLength.html
        web::json::value make_method_result_length_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultPropertyValue.html
        web::json::value make_method_result_property_value_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodStatus.html
        web::json::value make_method_status_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcName.html
        web::json::value make_name_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcOid.html
        web::json::value make_oid_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcOrganizationId.html
        web::json::value make_organization_id_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraints.html
        web::json::value make_parameter_constraints_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraintsNumber.html
        web::json::value make_parameter_constraints_number_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraintsString.html
        web::json::value make_parameter_constraints_string_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterDescriptor.html
        web::json::value make_parameter_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcProduct.html
        web::json::value make_product_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyChangeType.html
        web::json::value make_property_change_type_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyChangedEventData.html
        web::json::value make_property_changed_event_data_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraints.html
        web::json::value make_property_contraints_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraintsNumber.html
        web::json::value make_property_constraints_number_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraintsString.html
        web::json::value make_property_constraints_string_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyDescriptor.html
        web::json::value make_property_descriptor_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyId.html
        web::json::value make_property_id_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcRegex.html
        web::json::value make_regex_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcResetCause.html
        web::json::value make_reset_cause_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcRolePath.html
        web::json::value make_role_path_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTimeInterval.html
        web::json::value make_time_interval_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpoint.html
        web::json::value make_touchpoint_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointNmos.html
        web::json::value make_touchpoint_nmos_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointNmosChannelMapping.html
        web::json::value make_touchpoint_nmos_channel_mapping_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResource.html
        web::json::value make_touchpoint_resource_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResourceNmos.html
        web::json::value make_touchpoint_resource_nmos_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResourceNmosChannelMapping.html
        web::json::value make_touchpoint_resource_nmos_channel_mapping_datatype();
        // See // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcUri.html
        web::json::value make_uri_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcUuid.html
        web::json::value make_uuid_datatype();
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcVersionCode.html
        web::json::value make_version_code_datatype();

        // Monitoring feature set datatypes
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#datatypes
        //
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncconnectionstatus
        web::json::value make_connection_status_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncessencestatus
        web::json::value make_essence_status_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncoverallstatus
        web::json::value make_overall_status_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#nclinkstatus
        web::json::value make_link_status_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncsynchronizationstatus
        web::json::value make_synchronization_status_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncstreamstatus
        web::json::value make_stream_status_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#nccounter
        web::json::value make_counter_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#nctransmissionstatus
        web::json::value make_transmission_status_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncmethodresultcounters
        web::json::value make_method_result_counters_datatype();

        // Device configuration feature set datatypes
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncrestoremode
        web::json::value make_restore_mode_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncpropertyholder
        web::json::value make_property_holder_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncobjectpropertiesholder
        web::json::value make_object_properties_holder_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncbulkpropertiesholder
        web::json::value make_bulk_properties_holder_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncrestorevalidationstatus
        web::json::value make_restore_validation_status_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncpropertyrestorenoticetype
        web::json::value make_property_restore_notice_type_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncpropertyrestorenotice
        web::json::value make_property_restore_notice_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncobjectpropertiessetvalidation
        web::json::value make_object_properties_set_validation_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncmethodresultbulkpropertiesholder
        web::json::value make_method_result_bulk_properties_holder_datatype();
        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncmethodresultobjectpropertiessetvalidation
        web::json::value make_method_result_object_properties_set_validation_datatype();
    }

    namespace details
    {
        // Deprecated : use nc::details::make_method_result
        inline web::json::value make_nc_method_result(const nc_method_result& method_result) { return nc::details::make_method_result(method_result); };
        inline web::json::value make_nc_method_result_error(const nc_method_result& method_result, const utility::string_t& error_message) { return nc::details::make_method_result_error(method_result, error_message); };
        inline web::json::value make_nc_method_result(const nc_method_result& method_result, const web::json::value& value) { return nc::details::make_method_result(method_result, value); };

        // Deprecated: use nc::details::make use nc::details::make_element_id
        inline web::json::value make_nc_element_id(const nc_element_id& element_id) { return nc::details::make_element_id(element_id); };
        // Deprecated: use nc::details::make use nc::details::parse_element_id
        inline nc_element_id parse_nc_element_id(const web::json::value& element_id) { return nc::details::parse_element_id(element_id); };

        // Deprecated: use nc::details::make use nc::details::make_event_id
        inline web::json::value make_nc_event_id(const nc_event_id& event_id) { return nc::details::make_event_id(event_id);  };
        // Deprecated use nc::details::parse_event_id
        inline nc_event_id parse_nc_event_id(const web::json::value& event_id) { return nc::details::parse_event_id(event_id);  };

        // Deprecated: use nc::details::make use nc::details::make_method_id
        inline web::json::value make_nc_method_id(const nc_method_id& method_id) { return nc::details::make_method_id(method_id); };
        // Deprecated: use nc::details::parse_method_id
        inline nc_method_id parse_nc_method_id(const web::json::value& method_id) { return nc::details::parse_method_id(method_id); };

        // Deprecated: use nc::details::make_property_id
        inline web::json::value make_nc_property_id(const nc_property_id& property_id) { return nc::details::make_property_id(property_id); };
        // Deprecated: use nc::details::parse_property_id
        inline nc_property_id parse_nc_property_id(const web::json::value& property_id) { return nc::details::parse_property_id(property_id); };

        // Deprecated: use nc::details::make_class_id
        inline web::json::value make_nc_class_id(const nc_class_id& class_id) { return nc::details::make_class_id(class_id);  };
        // Deprecated: use nc::details::make
        inline nc_class_id parse_nc_class_id(const web::json::array& class_id) { return nc::details::parse_class_id(class_id); };

        // Deprecated: use nc::details::make_manufacturer
        inline web::json::value make_nc_manufacturer(const utility::string_t& name, nc_organization_id organization_id, const web::uri& website) { return nc::details::make_manufacturer(name, organization_id, website);  };
        inline web::json::value make_nc_manufacturer(const utility::string_t& name, nc_organization_id organization_id) { return nc::details::make_manufacturer(name, organization_id);  };
        inline web::json::value make_nc_manufacturer(const utility::string_t& name) { return nc::details::make_manufacturer(name);  };

        // Deprecated: use nc::details::make_product
        inline web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const utility::string_t& brand_name, const nc_uuid& uuid, const utility::string_t& description) { return nc::details::make_product(name, key, revision_level, brand_name, uuid, description); };
        inline web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const utility::string_t& brand_name, const nc_uuid& uuid) { return nc::details::make_product(name, key, revision_level, brand_name, uuid); };
        inline web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const utility::string_t& brand_name)  { return nc::details::make_product(name, key, revision_level, brand_name); };
        inline web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level) { return nc::details::make_product(name, key, revision_level); };

        // Deprecated: use nc::details::make_device_operational_state
        inline web::json::value make_nc_device_operational_state(nc_device_generic_state::state generic_state, const web::json::value& device_specific_details) { return nc::details::make_device_operational_state(generic_state, device_specific_details); };

        // Deprecated: use nc::details::make_block_member_descriptor
        inline web::json::value make_nc_block_member_descriptor(const utility::string_t& description, const utility::string_t& role, nc_oid oid, bool constant_oid, const nc_class_id& class_id, const utility::string_t& user_label, nc_oid owner) { return nc::details::make_block_member_descriptor(description, role, oid, constant_oid, class_id, user_label, owner); };

        // Deprecated: use nc::details::make_class_descriptor
        inline web::json::value make_nc_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const utility::string_t& fixed_role, const web::json::value& properties, const web::json::value& methods, const web::json::value& events) { return nc::details::make_class_descriptor(description, class_id, name, fixed_role, properties, methods, events); };
        inline web::json::value make_nc_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const web::json::value& properties, const web::json::value& methods, const web::json::value& events) { return nc::details::make_class_descriptor(description, class_id, name, properties, methods, events); };

        // Deprecated: use nc::details::make_enum_item_descriptor
        inline web::json::value make_nc_enum_item_descriptor(const utility::string_t& description, const nc_name& name, uint16_t val) { return nc::details::make_enum_item_descriptor(description, name, val); };

        // Deprecated: use nc::details::make_event_descriptor
        inline web::json::value make_nc_event_descriptor(const utility::string_t& description, const nc_event_id& id, const nc_name& name, const utility::string_t& event_datatype, bool is_deprecated) { return nc::details::make_event_descriptor(description, id, name, event_datatype, is_deprecated); };

        // Deprecated: use nc::details::make_field_descriptor
        inline web::json::value make_nc_field_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints) { return nc::details::make_field_descriptor(description, name, type_name, is_nullable, is_sequence, constraints); };
        inline web::json::value make_nc_field_descriptor(const utility::string_t& description, const nc_name& name, bool is_nullable, bool is_sequence, const web::json::value& constraints) { return nc::details::make_field_descriptor(description, name, is_nullable, is_sequence, constraints); };

        // Deprecated: use nc::details::make_method_descriptor
        inline web::json::value make_nc_method_descriptor(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype, const web::json::value& parameters, bool is_deprecated) { return nc::details::make_method_descriptor(description, id, name, result_datatype, parameters, is_deprecated);  };

        // Deprecated: use nc::details::make_parameter_descriptor
        inline web::json::value make_nc_parameter_descriptor(const utility::string_t& description, const nc_name& name, bool is_nullable, bool is_sequence, const web::json::value& constraints) { return nc::details::make_parameter_descriptor(description, name, is_nullable, is_sequence, constraints); };
        inline web::json::value make_nc_parameter_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints) { return nc::details::make_parameter_descriptor(description, name, type_name, is_nullable, is_sequence, constraints); };

        // Deprecated: use nc::details::make_property_descriptor
        inline web::json::value make_nc_property_descriptor(const utility::string_t& description, const nc_property_id& id, const nc_name& name, const utility::string_t& type_name,
            bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints) { return nc::details::make_property_descriptor(description, id, name, type_name, is_read_only, is_nullable, is_sequence, is_deprecated, constraints); };

        // Deprecated: use nc::details::make_datatype_descriptor_enum
        inline web::json::value make_nc_datatype_descriptor_enum(const utility::string_t& description, const nc_name& name, const web::json::value& items, const web::json::value& constraints) { return nc::details::make_datatype_descriptor_enum(description, name, items, constraints); };

        // Deprecated: use nc::details::make_datatype_descriptor_primitive
        inline web::json::value make_nc_datatype_descriptor_primitive(const utility::string_t& description, const nc_name& name, const web::json::value& constraints) { return nc::details::make_datatype_descriptor_primitive(description, name, constraints); };

        // Deprecated: use nc::details::make_datatype_descriptor_struct
        inline web::json::value make_nc_datatype_descriptor_struct(const utility::string_t& description, const nc_name& name, const web::json::value& fields, const utility::string_t& parent_type, const web::json::value& constraints) { return nc::details::make_datatype_descriptor_struct(description, name, fields, parent_type, constraints); };
        inline web::json::value make_nc_datatype_descriptor_struct(const utility::string_t& description, const nc_name& name, const web::json::value& fields, const web::json::value& constraints) { return nc::details::make_datatype_descriptor_struct(description, name, fields, constraints); };

        // Deprecated: use nc::details::make_datatype_typedef
        inline web::json::value make_nc_datatype_typedef(const utility::string_t& description, const nc_name& name, bool is_sequence, const utility::string_t& parent_type, const web::json::value& constraints) { return nc::details::make_datatype_typedef(description, name, is_sequence, parent_type, constraints); };

        // Deprecated: use nc::details::make_property_constraints
        inline web::json::value make_nc_property_constraints(const nc_property_id& property_id, const web::json::value& default_value) { return nc::details::make_property_constraints(property_id, default_value); };

        // Deprecated: use nc::details::make_property_constraints_number
        inline web::json::value make_nc_property_constraints_number(const nc_property_id& property_id, uint64_t default_value, uint64_t minimum, uint64_t maximum, uint64_t step) { return nc::details::make_property_constraints_number(property_id, default_value, minimum, maximum, step); };
        inline web::json::value make_nc_property_constraints_number(const nc_property_id& property_id, uint64_t minimum, uint64_t maximum, uint64_t step) { return nc::details::make_property_constraints_number(property_id, minimum, maximum, step); };

        // Deprecated: use nc::details::make_property_constraints_string
        inline web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, const utility::string_t& default_value, uint32_t max_characters, const nc_regex& pattern) { return nc::details::make_property_constraints_string(property_id, default_value, max_characters, pattern); };
        inline web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, uint32_t max_characters, const nc_regex& pattern) { return nc::details::make_property_constraints_string(property_id, max_characters, pattern); };
        inline web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, uint32_t max_characters) { return nc::details::make_property_constraints_string(property_id, max_characters); };
        inline web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, const nc_regex& pattern) { return nc::details::make_property_constraints_string(property_id, pattern); };

        // Deprecated: use nc::details::make_parameter_constraints
        inline web::json::value make_nc_parameter_constraints(const web::json::value& default_value) { return nc::details::make_parameter_constraints(default_value); };

        // Deprecated: use nc::details::make_parameter_constraints_number
        inline web::json::value make_nc_parameter_constraints_number(uint64_t default_value, uint64_t minimum, uint64_t maximum, uint64_t step) { return nc::details::make_parameter_constraints_number(default_value, minimum, maximum, step); };
        inline web::json::value make_nc_parameter_constraints_number(uint64_t minimum, uint64_t maximum, uint64_t step) { return nc::details::make_parameter_constraints_number(minimum, maximum, step); };

        // Deprecated: use nc::details::make_parameter_constraints_string
        inline web::json::value make_nc_parameter_constraints_string(const utility::string_t& default_value, uint32_t max_characters, const nc_regex& pattern) { return nc::details::make_parameter_constraints_string(default_value, max_characters, pattern); };
        inline web::json::value make_nc_parameter_constraints_string(uint32_t max_characters, const nc_regex& pattern) { return nc::details::make_parameter_constraints_string(max_characters, pattern); };
        inline web::json::value make_nc_parameter_constraints_string(uint32_t max_characters) { return nc::details::make_parameter_constraints_string(max_characters); };
        inline web::json::value make_nc_parameter_constraints_string(const nc_regex& pattern) { return nc::details::make_parameter_constraints_string(pattern); };

        // Deprecated: use nc::details::make_touchpoint
        inline web::json::value make_nc_touchpoint(const utility::string_t& context_namespace) { return nc::details::make_touchpoint(context_namespace); };

        // Deprecated: use nc::details::make_touchpoint_nmos
        inline web::json::value make_nc_touchpoint_nmos(const nc_touchpoint_resource_nmos& resource) { return nc::details::make_touchpoint_nmos(resource); };

        // Deprecated: use nc::details::make_touchpoint_nmos_channel_mapping
        inline web::json::value make_nc_touchpoint_nmos_channel_mapping(const nc_touchpoint_resource_nmos_channel_mapping& resource) { return nc::details::make_touchpoint_nmos_channel_mapping(resource); };

        // Deprecated: use nc::details::make_object
        inline web::json::value make_nc_object(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints) { return nc::details::make_object(class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints); };

        // Deprecated: use nc::details::make_block
        inline web::json::value make_nc_block(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, const web::json::value& members) { return nc::details::make_block(class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints, enabled, members); };

        // Deprecated: use nc::details::make_worker
        inline web::json::value make_nc_worker(const nc_class_id& class_id, nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled) { return nc::details::make_worker(class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints, enabled); };

        // Deprecated: use nc::details::make_manager
        inline web::json::value make_nc_manager(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints) { return nc::details::make_manager(class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints); };

        // Deprecated: use nc::details::make_device_manager
        inline web::json::value make_nc_device_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints,
            const web::json::value& manufacturer, const web::json::value& product, const utility::string_t& serial_number,
            const web::json::value& user_inventory_code, const web::json::value& device_name, const web::json::value& device_role, const web::json::value& operational_state, nc_reset_cause::cause reset_cause) { return nc::details::make_device_manager(oid, owner, user_label, description, touchpoints, runtime_property_constraints, manufacturer, product, serial_number, user_inventory_code, device_name, device_role, operational_state, reset_cause); };

        // Deprecated: use nc::details::make_class_manager
        inline web::json::value make_nc_class_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const nmos::experimental::control_protocol_state& control_protocol_state) { return nc::details::make_class_manager(oid, owner, user_label, description, touchpoints, runtime_property_constraints, control_protocol_state); };
    }

    // Deprecated: use nc::make_response
    inline web::json::value make_control_protocol_response(int32_t handle, const web::json::value& method_result) { return nc::make_response(handle, method_result); };
    // Deprecated: use nc::make_command_response
    inline web::json::value make_control_protocol_command_response(const web::json::value& responses) { return nc::make_command_response(responses); };

    // Deprecated: use nc::make_subscription_response
    inline web::json::value make_control_protocol_subscription_response(const web::json::value& subscriptions) { return nc::make_subscription_response(subscriptions); };

    // Deprecated: use nc::make_notification
    inline web::json::value make_control_protocol_notification(nc_oid oid, const nc_event_id& event_id, const nc_property_changed_event_data& property_changed_event_data) { return nc::make_notification(oid, event_id, property_changed_event_data); };
    // Deprecated: use nc::make_notification_message
    inline web::json::value make_control_protocol_notification_message(const web::json::value& notifications) { return nc::make_notification_message(notifications); };

    // Deprecated: use nc::make_property_changed_event
    inline web::json::value make_property_changed_event(nc_oid oid, const std::vector<nc_property_changed_event_data>& property_changed_event_data_list) { return nc::make_property_changed_event(oid, property_changed_event_data_list); };

    // Deprecated: use nc::make_error_message
    inline web::json::value make_control_protocol_error_message(const nc_method_result& method_result, const utility::string_t& error_message) { return nc::make_error_message(method_result, error_message); };

    // Deprecated: use nc::make_object_class
    inline web::json::value make_nc_object_class() { return nc::make_object_class(); };
    // Deprecated: use nc::make_block_class
    inline web::json::value make_nc_block_class() { return nc::make_block_class(); };
    // Deprecated: use nc::make_worker_class
    inline web::json::value make_nc_worker_class() { return nc::make_worker_class(); };
    // Deprecated: use nc::make_manager_class
    inline web::json::value make_nc_manager_class() { return nc::make_manager_class(); };
    // Deprecated: use nc::make_device_manager_class
    inline web::json::value make_nc_device_manager_class() { return nc::make_device_manager_class();  };
    // Deprecated: use nc::make_class_manager_class
    inline web::json::value make_nc_class_manager_class() { return nc::make_class_manager_class(); };
    // Deprecated: use nc::make_ident_beacon_class
    inline web::json::value make_nc_ident_beacon_class() { return nc::make_ident_beacon_class(); };
    // Deprecated: use nc::make_receiver_monitor_class
    inline web::json::value make_nc_receiver_monitor_class() { return nc::make_receiver_monitor_class(); };
    // Deprecated: use nc::make_sender_monitor_class
    inline web::json::value make_nc_sender_monitor_class() { return nc::make_sender_monitor_class(); };
}
#endif

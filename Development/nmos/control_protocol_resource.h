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

    namespace details
    {
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethodresult
        web::json::value make_nc_method_result(const nc_method_result& method_result);
        web::json::value make_nc_method_result_error(const nc_method_result& method_result, const utility::string_t& error_message);
        web::json::value make_nc_method_result(const nc_method_result& method_result, const web::json::value& value);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncelementid
        web::json::value make_nc_element_id(const nc_element_id& element_id);
        nc_element_id parse_nc_element_id(const web::json::value& element_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nceventid
        web::json::value make_nc_event_id(const nc_event_id& event_id);
        nc_event_id parse_nc_event_id(const web::json::value& event_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethodid
        web::json::value make_nc_method_id(const nc_method_id& method_id);
        nc_method_id parse_nc_method_id(const web::json::value& method_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyid
        web::json::value make_nc_property_id(const nc_property_id& property_id);
        nc_property_id parse_nc_property_id(const web::json::value& property_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassid
        web::json::value make_nc_class_id(const nc_class_id& class_id);
        nc_class_id parse_nc_class_id(const web::json::array& class_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanufacturer
        web::json::value make_nc_manufacturer(const utility::string_t& name, nc_organization_id organization_id, const web::uri& website);
        web::json::value make_nc_manufacturer(const utility::string_t& name, nc_organization_id organization_id);
        web::json::value make_nc_manufacturer(const utility::string_t& name);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncproduct
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const utility::string_t& brand_name, const nc_uuid& uuid, const utility::string_t& description);
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const utility::string_t& brand_name, const nc_uuid& uuid);
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const utility::string_t& brand_name);
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdeviceoperationalstate
        // device_specific_details can be null
        web::json::value make_nc_device_operational_state(nc_device_generic_state::state generic_state, const web::json::value& device_specific_details);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblockmemberdescriptor
        web::json::value make_nc_block_member_descriptor(const utility::string_t& description, const utility::string_t& role, nc_oid oid, bool constant_oid, const nc_class_id& class_id, const utility::string_t& user_label, nc_oid owner);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassdescriptor
        web::json::value make_nc_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const utility::string_t& fixed_role, const web::json::value& properties, const web::json::value& methods, const web::json::value& events);
        web::json::value make_nc_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const web::json::value& properties, const web::json::value& methods, const web::json::value& events);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncenumitemdescriptor
        web::json::value make_nc_enum_item_descriptor(const utility::string_t& description, const nc_name& name, uint16_t val);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nceventdescriptor
        web::json::value make_nc_event_descriptor(const utility::string_t& description, const nc_event_id& id, const nc_name& name, const utility::string_t& event_datatype, bool is_deprecated);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncfielddescriptor
        // constraints can be null
        web::json::value make_nc_field_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints);
        web::json::value make_nc_field_descriptor(const utility::string_t& description, const nc_name& name, bool is_nullable, bool is_sequence, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethoddescriptor
        // sequence<NcParameterDescriptor> parameters
        web::json::value make_nc_method_descriptor(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype, const web::json::value& parameters, bool is_deprecated);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterdescriptor
        // constraints can be null
        web::json::value make_nc_parameter_descriptor(const utility::string_t& description, const nc_name& name, bool is_nullable, bool is_sequence, const web::json::value& constraints);
        web::json::value make_nc_parameter_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertydescriptor
        // constraints can be null
        web::json::value make_nc_property_descriptor(const utility::string_t& description, const nc_property_id& id, const nc_name& name, const utility::string_t& type_name,
            bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorenum
        // constraints can be null
        // items: sequence<NcEnumItemDescriptor>
        web::json::value make_nc_datatype_descriptor_enum(const utility::string_t& description, const nc_name& name, const web::json::value& items, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorprimitive
        // constraints can be null
        web::json::value make_nc_datatype_descriptor_primitive(const utility::string_t& description, const nc_name& name, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorstruct
        // constraints can be null
        // fields: sequence<NcFieldDescriptor>
        web::json::value make_nc_datatype_descriptor_struct(const utility::string_t& description, const nc_name& name, const web::json::value& fields, const utility::string_t& parent_type, const web::json::value& constraints);
        web::json::value make_nc_datatype_descriptor_struct(const utility::string_t& description, const nc_name& name, const web::json::value& fields, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptortypedef
        web::json::value make_nc_datatype_typedef(const utility::string_t& description, const nc_name& name, bool is_sequence, const utility::string_t& parent_type, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyconstraints
        web::json::value make_nc_property_constraints(const nc_property_id& property_id, const web::json::value& default_value);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyconstraintsnumber
        web::json::value make_nc_property_constraints_number(const nc_property_id& property_id, uint64_t default_value, uint64_t minimum, uint64_t maximum, uint64_t step);
        web::json::value make_nc_property_constraints_number(const nc_property_id& property_id, uint64_t minimum, uint64_t maximum, uint64_t step);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyconstraintsstring
        web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, const utility::string_t& default_value, uint32_t max_characters, const nc_regex& pattern);
        web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, uint32_t max_characters, const nc_regex& pattern);
        web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, uint32_t max_characters);
        web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, const nc_regex& pattern);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraints
        web::json::value make_nc_parameter_constraints(const web::json::value& default_value);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsnumber
        web::json::value make_nc_parameter_constraints_number(uint64_t default_value, uint64_t minimum, uint64_t maximum, uint64_t step);
        web::json::value make_nc_parameter_constraints_number(uint64_t minimum, uint64_t maximum, uint64_t step);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsstring
        web::json::value make_nc_parameter_constraints_string(const utility::string_t& default_value, uint32_t max_characters, const nc_regex& pattern);
        web::json::value make_nc_parameter_constraints_string(uint32_t max_characters, const nc_regex& pattern);
        web::json::value make_nc_parameter_constraints_string(uint32_t max_characters);
        web::json::value make_nc_parameter_constraints_string(const nc_regex& pattern);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpoint
        web::json::value make_nc_touchpoint(const utility::string_t& context_namespace);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointnmos
        web::json::value make_nc_touchpoint_nmos(const nc_touchpoint_resource_nmos& resource);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointnmoschannelmapping
        web::json::value make_nc_touchpoint_nmos_channel_mapping(const nc_touchpoint_resource_nmos_channel_mapping& resource);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
        web::json::value make_nc_object(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
        web::json::value make_nc_block(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, const web::json::value& members);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncworker
        web::json::value make_nc_worker(const nc_class_id& class_id, nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled);

        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
        web::json::value make_receiver_monitor(const nc_class_id& class_id, nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled,
            nc_connection_status::status connection_status, const utility::string_t& connection_status_message, nc_payload_status::status payload_status, const utility::string_t& payload_status_message);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanager
        web::json::value make_nc_manager(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
        web::json::value make_nc_device_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints,
            const web::json::value& manufacturer, const web::json::value& product, const utility::string_t& serial_number,
            const web::json::value& user_inventory_code, const web::json::value& device_name, const web::json::value& device_role, const web::json::value& operational_state, nc_reset_cause::cause reset_cause);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
        web::json::value make_nc_class_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const nmos::experimental::control_protocol_state& control_protocol_state);

        // TODO: add link
        web::json::value make_nc_bulk_properties_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints);
    }

    // command message response
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#command-response-message-type
    web::json::value make_control_protocol_response(int32_t handle, const web::json::value& method_result);
    web::json::value make_control_protocol_command_response(const web::json::value& responses);

    // subscription response
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#subscription-response-message-type
    web::json::value make_control_protocol_subscription_response(const web::json::value& subscriptions);

    // notification
    // See https://specs.amwa.tv/ms-05-01/branches/v1.0.x/docs/Core_Mechanisms.html#notification-messages
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#notification-message-type
    web::json::value make_control_protocol_notification(nc_oid oid, const nc_event_id& event_id, const nc_property_changed_event_data& property_changed_event_data);
    web::json::value make_control_protocol_notification_message(const web::json::value& notifications);

    // property changed notification event
    // See https://specs.amwa.tv/ms-05-01/branches/v1.0.x/docs/Core_Mechanisms.html#the-propertychanged-event
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/NcObject.html#propertychanged-event
    web::json::value make_property_changed_event(nc_oid oid, const std::vector<nc_property_changed_event_data>& property_changed_event_data_list);

    // error message
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#error-messages
    web::json::value make_control_protocol_error_message(const nc_method_result& method_result, const utility::string_t& error_message);

    // Control class models
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/#control-class-models-for-branch-v10-dev
    //
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.html
    web::json::value make_nc_object_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.1.html
    web::json::value make_nc_block_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.2.html
    web::json::value make_nc_worker_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.html
    web::json::value make_nc_manager_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.1.html
    web::json::value make_nc_device_manager_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.2.html
    web::json::value make_nc_class_manager_class();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    web::json::value make_nc_ident_beacon_class();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    web::json::value make_nc_receiver_monitor_class();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    web::json::value make_nc_receiver_monitor_protected_class();

    // control classes proprties/methods/events
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
    web::json::value make_nc_object_properties();
    web::json::value make_nc_object_methods();
    web::json::value make_nc_object_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
    web::json::value make_nc_block_properties();
    web::json::value make_nc_block_methods();
    web::json::value make_nc_block_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncworker
    web::json::value make_nc_worker_properties();
    web::json::value make_nc_worker_methods();
    web::json::value make_nc_worker_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanager
    web::json::value make_nc_manager_properties();
    web::json::value make_nc_manager_methods();
    web::json::value make_nc_manager_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
    web::json::value make_nc_device_manager_properties();
    web::json::value make_nc_device_manager_methods();
    web::json::value make_nc_device_manager_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
    web::json::value make_nc_class_manager_properties();
    web::json::value make_nc_class_manager_methods();
    web::json::value make_nc_class_manager_events();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    web::json::value make_nc_receiver_monitor_properties();
    web::json::value make_nc_receiver_monitor_methods();
    web::json::value make_nc_receiver_monitor_events();

    // Monitoring feature set control classes
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    web::json::value make_nc_receiver_monitor_protected_properties();
    web::json::value make_nc_receiver_monitor_protected_methods();
    web::json::value make_nc_receiver_monitor_protected_events();

    // Identification feature set control classes
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    web::json::value make_nc_ident_beacon_properties();
    web::json::value make_nc_ident_beacon_methods();
    web::json::value make_nc_ident_beacon_events();

    // Device configuration classes
    // TODO: add link
    web::json::value make_nc_bulk_properties_manager_properties();
    web::json::value make_nc_bulk_properties_manager_methods();
    web::json::value make_nc_bulk_properties_manager_events();

    // Datatype models
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/#datatype-models-for-branch-v10-dev
    //
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_boolean_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_int16_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_int32_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_int64_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_uint16_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_uint32_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_uint64_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_float32_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_float64_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_string_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcBlockMemberDescriptor.html
    web::json::value make_nc_block_member_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcClassDescriptor.html
    web::json::value make_nc_class_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcClassId.html
    web::json::value make_nc_class_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptor.html
    web::json::value make_nc_datatype_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorEnum.html
    web::json::value make_nc_datatype_descriptor_enum_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorPrimitive.html
    web::json::value make_nc_datatype_descriptor_primitive_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorStruct.html
    web::json::value make_nc_datatype_descriptor_struct_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorTypeDef.html
    web::json::value make_nc_datatype_descriptor_type_def_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeType.html
    web::json::value make_nc_datatype_type_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDescriptor.html
    web::json::value make_nc_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDeviceGenericState.html
    web::json::value make_nc_device_generic_state_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDeviceOperationalState.html
    web::json::value make_nc_device_operational_state_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcElementId.html
    web::json::value make_nc_element_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEnumItemDescriptor.html
    web::json::value make_nc_enum_item_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEventDescriptor.html
    web::json::value make_nc_event_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEventId.html
    web::json::value make_nc_event_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcFieldDescriptor.html
    web::json::value make_nc_field_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcId.html
    web::json::value make_nc_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcManufacturer.html
    web::json::value make_nc_manufacturer_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodDescriptor.html
    web::json::value make_nc_method_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodId.html
    web::json::value make_nc_method_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResult.html
    web::json::value make_nc_method_result_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultBlockMemberDescriptors.html
    web::json::value make_nc_method_result_block_member_descriptors_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultClassDescriptor.html
    web::json::value make_nc_method_result_class_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultDatatypeDescriptor.html
    web::json::value make_nc_method_result_datatype_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultError.html
    web::json::value make_nc_method_result_error_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultId.html
    web::json::value make_nc_method_result_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultLength.html
    web::json::value make_nc_method_result_length_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultPropertyValue.html
    web::json::value make_nc_method_result_property_value_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodStatus.html
    web::json::value make_nc_method_status_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcName.html
    web::json::value make_nc_name_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcOid.html
    web::json::value make_nc_oid_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcOrganizationId.html
    web::json::value make_nc_organization_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraints.html
    web::json::value make_nc_parameter_constraints_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraintsNumber.html
    web::json::value make_nc_parameter_constraints_number_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraintsString.html
    web::json::value make_nc_parameter_constraints_string_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterDescriptor.html
    web::json::value make_nc_parameter_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcProduct.html
    web::json::value make_nc_product_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyChangeType.html
    web::json::value make_nc_property_change_type_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyChangedEventData.html
    web::json::value make_nc_property_changed_event_data_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraints.html
    web::json::value make_nc_property_contraints_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraintsNumber.html
    web::json::value make_nc_property_constraints_number_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraintsString.html
    web::json::value make_nc_property_constraints_string_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyDescriptor.html
    web::json::value make_nc_property_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyId.html
    web::json::value make_nc_property_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcRegex.html
    web::json::value make_nc_regex_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcResetCause.html
    web::json::value make_nc_reset_cause_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcRolePath.html
    web::json::value make_nc_role_path_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTimeInterval.html
    web::json::value make_nc_time_interval_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpoint.html
    web::json::value make_nc_touchpoint_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointNmos.html
    web::json::value make_nc_touchpoint_nmos_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointNmosChannelMapping.html
    web::json::value make_nc_touchpoint_nmos_channel_mapping_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResource.html
    web::json::value make_nc_touchpoint_resource_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResourceNmos.html
    web::json::value make_nc_touchpoint_resource_nmos_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResourceNmosChannelMapping.html
    web::json::value make_nc_touchpoint_resource_nmos_channel_mapping_datatype();
    // See // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcUri.html
    web::json::value make_nc_uri_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcUuid.html
    web::json::value make_nc_uuid_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcVersionCode.html
    web::json::value make_nc_version_code_datatype();

    // Monitoring feature set datatypes
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#datatypes
    //
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncconnectionstatus
    web::json::value make_nc_connection_status_datatype();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncpayloadstatus
    web::json::value make_nc_payload_status_datatype();

    // Device configuration feature set datatypes
    // TODO: add link
    //
    web::json::value make_nc_property_value_holder_datatype();
    //
    web::json::value make_nc_object_properties_holder_datatype();
    //
    web::json::value make_nc_bulk_values_holder_datatype();
    //
    web::json::value make_nc_object_properties_set_validation_datatype();
    //
    web::json::value make_nc_method_result_bulk_values_holder_datatype();
    //
    web::json::value make_nc_method_result_object_properties_set_validation_datatype();
}

#endif

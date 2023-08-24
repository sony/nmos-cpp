#ifndef NMOS_CONTROL_PROTOCOL_RESOURCE_H
#define NMOS_CONTROL_PROTOCOL_RESOURCE_H

#include "cpprest/json_utils.h"
#include "nmos/control_protocol_typedefs.h"

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace nmos
{
    namespace experimental
    {
        struct control_protocol_state;
    }

    namespace details
    {
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncelementid
        //web::json::value make_nc_element_id(uint16_t level, uint16_t index);
        web::json::value make_nc_element_id(const nc_element_id& element_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#nceventid
        //web::json::value make_nc_event_id(uint16_t level, uint16_t index);
        web::json::value make_nc_event_id(const nc_event_id& event_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmethodid
        //web::json::value make_nc_method_id(uint16_t level, uint16_t index);
        web::json::value make_nc_method_id(const nc_method_id& event_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncpropertyid
        //web::json::value make_nc_property_id(uint16_t level, uint16_t index);
        web::json::value make_nc_property_id(const nc_property_id& event_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassid
        web::json::value make_nc_class_id(const nc_class_id& class_id);
        nc_class_id parse_nc_class_id(const web::json::array& class_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmanufacturer
        web::json::value make_nc_manufacturer(const utility::string_t& name, const web::json::value& organization_id = web::json::value::null(), const web::json::value& website = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncproduct
        // brand_name can be null
        // uuid can be null
        // description can be null
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const web::json::value& brand_name = web::json::value::null(), const web::json::value& uuid = web::json::value::null(), const web::json::value& description = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdeviceoperationalstate
        // device_specific_details can be null
        web::json::value make_nc_device_operational_state(nc_device_generic_state::state generic_state, const web::json::value& device_specific_details);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdescriptor
        // description can be null
        web::json::value make_nc_descriptor(const web::json::value& description);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncblockmemberdescriptor
        // description can be null
        // user_label can be null
        web::json::value make_nc_block_member_descriptor(const web::json::value& description, const utility::string_t& role, nc_oid oid, bool constant_oid, const nc_class_id& class_id, const web::json::value& user_label, nc_oid owner);
        web::json::value make_nc_block_member_descriptor(const utility::string_t& description, const utility::string_t& role, nc_oid oid, bool constant_oid, const nc_class_id& class_id, const utility::string_t& user_label, nc_oid owner);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassdescriptor
        // description can be null
        // fixedRole can be null
        web::json::value make_nc_class_descriptor(const web::json::value& description, const nc_class_id& class_id, const utility::string_t& name, const web::json::value& fixed_role, const web::json::value& properties, const web::json::value& methods, const web::json::value& events);
        web::json::value make_nc_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const utility::string_t& name, const web::json::value& fixed_role, const web::json::value& properties, const web::json::value& methods, const web::json::value& events);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncenumitemdescriptor
        // description can be null
        web::json::value make_nc_enum_item_descriptor(const web::json::value& description, const utility::string_t& name, uint16_t val);
        web::json::value make_nc_enum_item_descriptor(const utility::string_t& description, const utility::string_t& name, uint16_t val);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#nceventdescriptor
        // description can be null
        // id = make_nc_event_id(level, index)
        web::json::value make_nc_event_descriptor(const web::json::value& description, const nc_event_id& id, const utility::string_t& name, const utility::string_t& event_datatype, bool is_deprecated);
        web::json::value make_nc_event_descriptor(const utility::string_t& description, const nc_event_id& id, const utility::string_t& name, const utility::string_t& event_datatype, bool is_deprecated);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncfielddescriptor
        // description can be null
        // type_name can be null
        // constraints can be null
        web::json::value make_nc_field_descriptor(const web::json::value& description, const utility::string_t& name, const web::json::value& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints = web::json::value::null());
        web::json::value make_nc_field_descriptor(const utility::string_t& description, const utility::string_t& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmethoddescriptor
        // description can be null
        // id = make_nc_method_id(level, index)
        // sequence<NcParameterDescriptor> parameters
        web::json::value make_nc_method_descriptor(const web::json::value& description, const nc_method_id& id, const utility::string_t& name, const utility::string_t& result_datatype, const web::json::value& parameters, bool is_deprecated);
        web::json::value make_nc_method_descriptor(const utility::string_t& description, const nc_method_id& id, const utility::string_t& name, const utility::string_t& result_datatype, const web::json::value& parameters, bool is_deprecated);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncparameterdescriptor
        // description can be null
        // type_name can be null
        web::json::value make_nc_parameter_descriptor(const web::json::value& description, const utility::string_t& name, const web::json::value& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints = web::json::value::null());
        web::json::value make_nc_parameter_descriptor(const utility::string_t& description, const utility::string_t& name, bool is_nullable, bool is_sequence, const web::json::value& constraints = web::json::value::null());
        web::json::value make_nc_parameter_descriptor(const utility::string_t& description, const utility::string_t& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncpropertydescriptor
        // description can be null
        // id = make_nc_property_id(level, index);
        // type_name can be null
        // constraints can be null
        web::json::value make_nc_property_descriptor(const web::json::value& description, const nc_property_id& id, const utility::string_t& name, const web::json::value& type_name,
            bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints = web::json::value::null());
        web::json::value make_nc_property_descriptor(const utility::string_t& description, const nc_property_id& id, const utility::string_t& name, const utility::string_t& type_name,
            bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptor
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_descriptor(const web::json::value& description, const utility::string_t& name, nc_datatype_type::type type, const web::json::value& constraints = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptorenum
        // description can be null
        // constraints can be null
        // items: sequence<NcEnumItemDescriptor>
        web::json::value make_nc_datatype_descriptor_enum(const web::json::value& description, const utility::string_t& name, const web::json::value& items, const web::json::value& constraints = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptorprimitive
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_descriptor_primitive(const web::json::value& description, const utility::string_t& name, const web::json::value& constraints = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptorstruct
        // description can be null
        // constraints can be null
        // fields: sequence<NcFieldDescriptor>
        // parent_type can be null
        web::json::value make_nc_datatype_descriptor_struct(const web::json::value& description, const utility::string_t& name, const web::json::value& fields, const web::json::value& parent_type, const web::json::value& constraints = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptortypedef
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_typedef(const web::json::value& description, const utility::string_t& name, bool is_sequence, const utility::string_t& parent_type, const web::json::value& constraints = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncobject
        web::json::value make_nc_object(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncblock
        web::json::value make_nc_block(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, const web::json::value& members);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncworker
        web::json::value make_nc_worker(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmanager
        web::json::value make_nc_manager(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdevicemanager
        web::json::value make_nc_device_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints,
            const web::json::value& manufacturer, const web::json::value& product, const utility::string_t& serial_number,
            const web::json::value& user_inventory_code, const web::json::value& device_name, const web::json::value& device_role, const web::json::value& operational_state, nc_reset_cause::cause reset_cause);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassmanager
        web::json::value make_nc_class_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const nmos::experimental::control_protocol_state& control_protocol_state);
    }

    // message response
    // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#command-message-type
    web::json::value make_control_protocol_message_response(nc_message_type::type type, const web::json::value& responses);

    // error message
    // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#error-messages
    web::json::value make_control_protocol_error_message(const nc_method_result& method_result, const utility::string_t& error_message);

    // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#command-response-message-type
    web::json::value make_control_protocol_error_response(int32_t handle, const nc_method_result& method_result, const utility::string_t& error_message);
    web::json::value make_control_protocol_response(int32_t handle, const nc_method_result& method_result);
    web::json::value make_control_protocol_response(int32_t handle, const nc_method_result& method_result, const web::json::value& value); // value can be sequence<NcBlockMemberDescriptor>, NcClassDescriptor, NcDatatypeDescriptor
    web::json::value make_control_protocol_response(int32_t handle, const nc_method_result& method_result, uint32_t value);

    // Control class models
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/classes/#control-class-models-for-branch-v10-dev
    //
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/classes/1.html
    web::json::value make_nc_object_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/classes/1.1.html
    web::json::value make_nc_block_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/classes/1.2.html
    web::json::value make_nc_worker_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/classes/1.3.html
    web::json::value make_nc_manager_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/classes/1.3.1.html
    web::json::value make_nc_device_manager_class();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/classes/1.3.2.html
    web::json::value make_nc_class_manager_class();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    web::json::value make_nc_ident_beacon_class();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    web::json::value make_nc_receiver_monitor_class();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    web::json::value make_nc_receiver_monitor_protected_class();

    // control classes proprties/methods/events
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncobject
    web::json::value make_nc_object_properties();
    web::json::value make_nc_object_methods();
    web::json::value make_nc_object_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncblock
    web::json::value make_nc_block_properties();
    web::json::value make_nc_block_methods();
    web::json::value make_nc_block_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncworker
    web::json::value make_nc_worker_properties();
    web::json::value make_nc_worker_methods();
    web::json::value make_nc_worker_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmanager
    web::json::value make_nc_manager_properties();
    web::json::value make_nc_manager_methods();
    web::json::value make_nc_manager_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdevicemanager
    web::json::value make_nc_device_manager_properties();
    web::json::value make_nc_device_manager_methods();
    web::json::value make_nc_device_manager_events();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassmanager
    web::json::value make_nc_class_manager_properties();
    web::json::value make_nc_class_manager_methods();
    web::json::value make_nc_class_manager_events();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    web::json::value make_nc_receiver_monitor_properties();
    web::json::value make_nc_receiver_monitor_methods();
    web::json::value make_nc_receiver_monitor_events();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    web::json::value make_nc_receiver_monitor_protected_properties();
    web::json::value make_nc_receiver_monitor_protected_methods();
    web::json::value make_nc_receiver_monitor_protected_events();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    web::json::value make_nc_ident_beacon_properties();
    web::json::value make_nc_ident_beacon_methods();
    web::json::value make_nc_ident_beacon_events();

    // Datatype models
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/#datatype-models-for-branch-v10-dev
    //
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcBlockMemberDescriptor.html
    web::json::value make_nc_block_member_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcClassDescriptor.html
    web::json::value make_nc_class_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcClassId.html
    web::json::value make_nc_class_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcDatatypeDescriptor.html
    web::json::value make_nc_datatype_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcDatatypeDescriptorEnum.html
    web::json::value make_nc_datatype_descriptor_enum_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcDatatypeDescriptorPrimitive.html
    web::json::value make_nc_datatype_descriptor_primitive_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcDatatypeDescriptorStruct.html
    web::json::value make_nc_datatype_descriptor_struct_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcDatatypeDescriptorTypeDef.html
    web::json::value make_nc_datatype_descriptor_type_def_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcDatatypeType.html
    web::json::value make_nc_datatype_type_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcDescriptor.html
    web::json::value make_nc_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcDeviceGenericState.html
    web::json::value make_nc_device_generic_state_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcDeviceOperationalState.html
    web::json::value make_nc_device_operational_state_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcElementId.html
    web::json::value make_nc_element_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcEnumItemDescriptor.html
    web::json::value make_nc_enum_item_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcEventDescriptor.html
    web::json::value make_nc_event_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcEventId.html
    web::json::value make_nc_event_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcFieldDescriptor.html
    web::json::value make_nc_field_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcId.html
    web::json::value make_nc_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcManufacturer.html
    web::json::value make_nc_manufacturer_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodDescriptor.html
    web::json::value make_nc_method_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodId.html
    web::json::value make_nc_method_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodResult.html
    web::json::value make_nc_method_result_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodResultBlockMemberDescriptors.html
    web::json::value make_nc_method_result_block_member_descriptors_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodResultClassDescriptor.html
    web::json::value make_nc_method_result_class_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodResultDatatypeDescriptor.html
    web::json::value make_nc_method_result_datatype_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodResultError.html
    web::json::value make_nc_method_result_error_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodResultId.html
    web::json::value make_nc_method_result_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodResultLength.html
    web::json::value make_nc_method_result_length_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodResultPropertyValue.html
    web::json::value make_nc_method_result_property_value_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcMethodStatus.html
    web::json::value make_nc_method_status_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcName.html
    web::json::value make_nc_name_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcOid.html
    web::json::value make_nc_oid_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcOrganizationId.html
    web::json::value make_nc_organization_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcParameterConstraints.html
    web::json::value make_nc_parameter_constraints_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcParameterConstraintsNumber.html
    web::json::value make_nc_parameter_constraints_number_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcParameterConstraintsString.html
    web::json::value make_nc_parameter_constraints_string_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcParameterDescriptor.html
    web::json::value make_nc_parameter_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcProduct.html
    web::json::value make_nc_product_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcPropertyChangeType.html
    web::json::value make_nc_property_change_type_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcPropertyChangedEventData.html
    web::json::value make_nc_property_changed_event_data_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcPropertyConstraints.html
    web::json::value make_nc_property_contraints_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcPropertyConstraintsNumber.html
    web::json::value make_nc_property_constraints_number_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcPropertyConstraintsString.html
    web::json::value make_nc_property_constraints_string_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcPropertyDescriptor.html
    web::json::value make_nc_property_descriptor_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcPropertyId.html
    web::json::value make_nc_property_id_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcRegex.html
    web::json::value make_nc_regex_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcResetCause.html
    web::json::value make_nc_reset_cause_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcRolePath.html
    web::json::value make_nc_role_path_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcTimeInterval.html
    web::json::value make_nc_time_interval_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcTouchpoint.html
    web::json::value make_nc_touchpoint_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcTouchpointNmos.html
    web::json::value make_nc_touchpoint_nmos_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcTouchpointNmosChannelMapping.html
    web::json::value make_nc_touchpoint_nmos_channel_mapping_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcTouchpointResource.html
    web::json::value make_nc_touchpoint_resource_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcTouchpointResourceNmos.html
    web::json::value make_nc_touchpoint_resource_nmos_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcTouchpointResourceNmosChannelMapping.html
    web::json::value make_nc_touchpoint_resource_nmos_channel_mapping_datatype();
    // See // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcUri.html
    web::json::value make_nc_uri_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcUuid.html
    web::json::value make_nc_uuid_datatype();
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/NcVersionCode.html
    web::json::value make_nc_version_code_datatype();

    // Monitoring datatypes
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#datatypes
    //
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncconnectionstatus
    web::json::value make_nc_connection_status_datatype();
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncpayloadstatus
    web::json::value make_nc_payload_status_datatype();
}

#endif

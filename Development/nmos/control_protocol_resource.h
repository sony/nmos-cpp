#ifndef NMOS_CONTROL_PROTOCOL_RESOURCE_H
#define NMOS_CONTROL_PROTOCOL_RESOURCE_H

#include <map>
#include "cpprest/json_utils.h"

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace nmos
{
    namespace details
    {
        namespace nc_message_type
        {
            enum type
            {
                command = 0,
                command_response = 1,
                notification = 2,
                subscription = 3,
                subscription_response = 4,
                error = 5
            };
        }

        // Method invokation status
        namespace nc_method_status
        {
            enum status
            {
                ok = 200,                       // Method call was successful
                property_deprecated = 298,      // Method call was successful but targeted property is deprecated
                method_deprecated = 299,        // Method call was successful but method is deprecated
                bad_command_format = 400,       // Badly-formed command
                unathorized = 401,              // Client is not authorized
                bad_oid = 404,                  // Command addresses a nonexistent object
                read_only = 405,                // Attempt to change read-only state
                invalid_request = 406,          // Method call is invalid in current operating context
                conflict = 409,                 // There is a conflict with the current state of the device
                buffer_overflow = 413,          // Something was too big
                parameter_error = 417,          // Method parameter does not meet expectations
                locked = 423,                   // Addressed object is locked
                device_error = 500,             // Internal device error
                method_not_implemented = 501,   // Addressed method is not implemented by the addressed object
                property_not_implemented = 502, // Addressed property is not implemented by the addressed object
                not_ready = 503,                // The device is not ready to handle any commands
                timeout = 504,                  // Method call did not finish within the allotted time
                property_version_error = 505    // Incompatible protocol version
            };
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmethodresult
        struct nc_method_result
        {
            nc_method_status::status status;
        };

        // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#command-response-message-type
        web::json::value make_control_protocol_error_response(int32_t handle, const nc_method_result& method_result, const utility::string_t& error_message);
        web::json::value make_control_protocol_response(int32_t handle, const nc_method_result& method_result);
        web::json::value make_control_protocol_response(int32_t handle, const nc_method_result& method_result, const web::json::value& value);

        // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#command-message-type
        web::json::value make_control_protocol_message_response(nc_message_type::type type, const web::json::value& responses);
        // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#error-messages
        web::json::value make_control_protocol_error_message(const nc_method_result& method_result, const utility::string_t& error_message);

        // Datatype type
        namespace nc_datatype_type
        {
            enum type
            {
                Primitive = 0,
                Typedef = 1,
                Struct = 2,
                Enum = 3
            };
        }

        // Device generic operational state
        namespace nc_device_generic_state
        {
            enum state
            {
                Unknown = 0,            // Unknown
                NormalOperation = 1,    // Normal operation
                Initializing = 2,       // Device is initializing
                Updating = 3,           // Device is performing a software or firmware update
                LicensingError = 4,     // Device is experiencing a licensing error
                InternalError = 5       // Device is experiencing an internal error
            };
        }

        // Reset cause enum
        namespace nc_reset_cause
        {
            enum cause
            {
                Unknown = 0,            // 0 Unknown
                Power_on = 1,           // 1 Power on
                InternalError = 2,      // 2 Internal error
                Upgrade = 3,            // 3 Upgrade
                Controller_request = 4, // 4 Controller request
                ManualReset = 5         // 5 Manual request from the front panel
            };
        }

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncid
        typedef uint32_t nc_id;

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncoid
        typedef uint32_t nc_oid;

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncuri
        typedef utility::string_t nc_uri;

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncuuid
        typedef utility::string_t nc_uuid;

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassid
        typedef std::vector<int32_t> nc_class_id;
        const nc_class_id nc_object_class_id({ 1 });
        const nc_class_id nc_block_class_id({ 1, 1 });
        const nc_class_id nc_worker_class_id({ 1, 2 });
        const nc_class_id nc_manager_class_id({ 1, 3 });
        const nc_class_id nc_device_manager_class_id({ 1, 3, 1 });
        const nc_class_id nc_class_manager_class_id({ 1, 3, 2 });

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#nctouchpoint
        typedef utility::string_t nc_touch_point;

        typedef std::map<web::json::value, utility::string_t> properties;

//        typedef std::function<web::json::value(const properties& properties, int32_t handle, int32_t oid, const web::json::value& arguments)> method;
//        typedef std::map<web::json::value, method> methods; // method_id vs method handler

        typedef std::function<web::json::value(const web::json::array& properties, int32_t handle, int32_t oid, const web::json::value& arguments)> method;
        typedef std::map<web::json::value, method> methods; // method_id vs method handler

        web::json::value make_control_protocol_result(const nc_method_result& method_result);
        web::json::value make_control_protocol_error_result(const nc_method_result& method_result, const utility::string_t& error_message);

        web::json::value make_control_protocol_result(const nc_method_result& method_result, const web::json::value& value);
        web::json::value make_control_protocol_error_response(int32_t handle, const nc_method_result& method_result, const utility::string_t& error_message);

        web::json::value make_control_protocol_response(int32_t handle, const nc_method_result& method_result);

        web::json::value make_control_protocol_response(int32_t handle, const nc_method_result& method_result, const web::json::value& value);

        // message response
        // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#command-message-type
        web::json::value make_control_protocol_message_response(nc_message_type::type type, const web::json::value& responses);

        // error message
        // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#error-messages
        web::json::value make_control_protocol_error_message(const nc_method_result& method_result, const utility::string_t& error_message);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassid
        web::json::value make_nc_class_id(const nc_class_id& class_id);
        nc_class_id parse_nc_class_id(const web::json::value& class_id);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncelementid
        web::json::value make_nc_element_id(uint16_t level, uint16_t index);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#nceventid
        web::json::value make_nc_event_id(uint16_t level, uint16_t index);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmethodid
        web::json::value make_nc_method_id(uint16_t level, uint16_t index);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncpropertyid
        web::json::value make_nc_property_id(uint16_t level, uint16_t index);

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
        web::json::value make_nc_block_member_descriptor(const web::json::value& description, const utility::string_t& role, nc_oid oid, bool constant_oid, const web::json::value& class_id, const web::json::value& user_label, nc_oid owner);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassdescriptor
        // description can be null
        // fixedRole can be null
        web::json::value make_nc_class_descriptor(const web::json::value& description, const web::json::value& class_id, const utility::string_t& name, const web::json::value& fixed_role, const web::json::value& properties, const web::json::value& methods, const web::json::value& events);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncenumitemdescriptor
        // description can be null
        web::json::value make_nc_enum_item_descriptor(const web::json::value& description, const utility::string_t& name, uint16_t val);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#nceventdescriptor
        // description can be null
        // id = make_nc_event_id(level, index)
        web::json::value make_nc_event_descriptor(const web::json::value& description, const web::json::value& id, const utility::string_t& name, const utility::string_t& event_datatype, bool is_deprecated);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncfielddescriptor
        // description can be null
        // type_name can be null
        // constraints can be null
        web::json::value make_nc_field_descriptor(const web::json::value& description, const utility::string_t& name, const web::json::value& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmethoddescriptor
        // description can be null
        // id = make_nc_method_id(level, index)
        // sequence<NcParameterDescriptor> parameters
        web::json::value make_nc_method_descriptor(const web::json::value& description, const web::json::value& id, const utility::string_t& name, const utility::string_t& result_datatype, const web::json::value& parameters, bool is_deprecated);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncparameterdescriptor
        // description can be null
        // type_name can be null
        web::json::value make_nc_parameter_descriptor(const web::json::value& description, const utility::string_t& name, const web::json::value& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncpropertydescriptor
        // description can be null
        // id = make_nc_property_id(level, index);
        // type_name can be null
        // constraints can be null
        web::json::value make_nc_property_descriptor(const web::json::value& description, const web::json::value& id, const utility::string_t& name, const web::json::value& type_name,
            bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptor
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_descriptor(const web::json::value& description, const utility::string_t& name, nc_datatype_type::type type, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptorenum
        // description can be null
        // constraints can be null
        // items: sequence<NcEnumItemDescriptor>
        web::json::value make_nc_datatype_descriptor_enum(const web::json::value& description, const utility::string_t& name, const web::json::value& constraints, const web::json::value& items);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptorprimitive
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_descriptor_primitive(const web::json::value& description, const utility::string_t& name, const web::json::value& constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptorstruct
        // description can be null
        // constraints can be null
        // fields: sequence<NcFieldDescriptor>
        // parent_type can be null
        web::json::value make_nc_datatype_descriptor_struct(const web::json::value& description, const utility::string_t& name, const web::json::value& constraints, const web::json::value& fields, const web::json::value& parent_type);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdatatypedescriptortypedef
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_typedef(const web::json::value& description, const utility::string_t& name, const web::json::value& constraints, const utility::string_t& parent_type, bool is_sequence);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncobject
        web::json::value make_nc_object(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncblock
        web::json::value make_nc_block(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, const web::json::value& members);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmanager
        web::json::value make_nc_manager(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints = web::json::value::null(), const web::json::value& runtime_property_constraints = web::json::value::null());

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdevicemanager
        web::json::value make_nc_device_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints,
            const web::json::value& manufacturer, const web::json::value& product, const utility::string_t& serial_number,
            const web::json::value& user_inventory_code, const web::json::value& device_name, const web::json::value& device_role, const web::json::value& operational_state, nc_reset_cause::cause reset_cause);

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassmanager
        web::json::value make_nc_class_manager(details::nc_oid oid, nc_oid owner, const web::json::value& user_label);

        // make the core classes proprties/methods/events
        web::json::value make_nc_object_properties();
        web::json::value make_nc_object_methods();
        web::json::value make_nc_object_events();
        web::json::value make_nc_block_properties();
        web::json::value make_nc_block_methods();
        web::json::value make_nc_block_events();
        web::json::value make_nc_worker_properties();
        web::json::value make_nc_worker_methods();
        web::json::value make_nc_worker_events();
        web::json::value make_nc_manager_properties();
        web::json::value make_nc_manager_methods();
        web::json::value make_nc_manager_events();
        web::json::value make_nc_device_manager_properties();
        web::json::value make_nc_device_manager_methods();
        web::json::value make_nc_device_manager_events();
        web::json::value make_nc_class_manager_properties();
        web::json::value make_nc_class_manager_methods();
        web::json::value make_nc_class_manager_events();
    }
}

#endif

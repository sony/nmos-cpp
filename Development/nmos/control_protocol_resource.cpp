#include "nmos/control_protocol_resource.h"

//#include "nmos/resource.h"
#include "nmos/control_protocol_state.h"  // for nmos::experimental::control_classes definitions
#include "nmos/json_fields.h"

namespace nmos
{
    namespace details
    {
        web::json::value make_nc_method_result(const nc_method_result& method_result)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::status, method_result.status }
            });
        }

        web::json::value make_nc_method_result_error(const nc_method_result& method_result, const utility::string_t& error_message)
        {
            auto result = make_nc_method_result(method_result);
            if (!error_message.empty()) { result[nmos::fields::nc::error_message] = web::json::value::string(error_message); }
            return result;
        }

        web::json::value make_nc_method_result(const nc_method_result& method_result, const web::json::value& value)
        {
            auto result = make_nc_method_result(method_result);
            result[nmos::fields::nc::value] = value;
            return result;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncelementid
        web::json::value make_nc_element_id(uint16_t level, uint16_t index)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::level, level },
                { nmos::fields::nc::index, index }
            });
        }
        web::json::value make_nc_element_id(const nc_element_id& id)
        {
            return make_nc_element_id(id.level, id.index);
        }
        nc_element_id parse_nc_element_id(const web::json::value& id)
        {
            return { uint16_t(nmos::fields::nc::level(id)), uint16_t(nmos::fields::nc::index(id)) };
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nceventid
        web::json::value make_nc_event_id(const nc_event_id& id)
        {
            return make_nc_element_id(id);
        }
        nc_event_id parse_nc_event_id(const web::json::value& id)
        {
            return parse_nc_element_id(id);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethodid
        web::json::value make_nc_method_id(const nc_method_id& id)
        {
            return make_nc_element_id(id);
        }
        nc_event_id parse_nc_method_id(const web::json::value& id)
        {
            return parse_nc_element_id(id);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyid
        web::json::value make_nc_property_id(const nc_property_id& id)
        {
            return make_nc_element_id(id);
        }
        nc_event_id parse_nc_property_id(const web::json::value& id)
        {
            return parse_nc_element_id(id);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassid
        web::json::value make_nc_class_id(const nc_class_id& class_id)
        {
            using web::json::value;

            auto nc_class_id = value::array();
            for (const auto class_id_item : class_id) { web::json::push_back(nc_class_id, class_id_item); }
            return nc_class_id;
        }
        nc_class_id parse_nc_class_id(const web::json::array& class_id_)
        {
            nc_class_id class_id;
            for (auto& element : class_id_)
            {
                class_id.push_back(element.as_integer());
            }
            return class_id;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanufacturer
        web::json::value make_nc_manufacturer(const utility::string_t& name, const web::json::value& organization_id, const web::json::value& website)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::name, name },
                { nmos::fields::nc::organization_id, organization_id },
                { nmos::fields::nc::website, website }
            });
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncproduct
        // brand_name can be null
        // uuid can be null
        // description can be null
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const web::json::value& brand_name, const web::json::value& uuid, const web::json::value& description)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::name, name },
                { nmos::fields::nc::key, key },
                { nmos::fields::nc::revision_level, revision_level },
                { nmos::fields::nc::brand_name, brand_name },
                { nmos::fields::nc::uuid, uuid },
                { nmos::fields::nc::description, description }
            });
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdeviceoperationalstate
        // device_specific_details can be null
        web::json::value make_nc_device_operational_state(nc_device_generic_state::state generic_state, const web::json::value& device_specific_details)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::generic_state, generic_state },
                { nmos::fields::nc::device_specific_details, device_specific_details }
            });
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdescriptor
        // description can be null
        web::json::value make_nc_descriptor(const web::json::value& description)
        {
            using web::json::value_of;

            return value_of({ { nmos::fields::nc::description, description } });
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblockmemberdescriptor
        // description can be null
        // user_label can be null
        web::json::value make_nc_block_member_descriptor(const web::json::value& description, const utility::string_t& role, nc_oid oid, bool constant_oid, const nc_class_id& class_id, const web::json::value& user_label, nc_oid owner)
        {
            using web::json::value;

            auto data = make_nc_descriptor(description);
            data[nmos::fields::nc::role] = value::string(role);
            data[nmos::fields::nc::oid] = oid;
            data[nmos::fields::nc::constant_oid] = value::boolean(constant_oid);
            data[nmos::fields::nc::class_id] = make_nc_class_id(class_id);
            data[nmos::fields::nc::user_label] = user_label;
            data[nmos::fields::nc::owner] = owner;

            return data;
        }
        web::json::value make_nc_block_member_descriptor(const utility::string_t& description, const utility::string_t& role, nc_oid oid, bool constant_oid, const nc_class_id& class_id, const utility::string_t& user_label, nc_oid owner)
        {
            using web::json::value;

            return make_nc_block_member_descriptor(value::string(description), role, oid, constant_oid, class_id, value::string(user_label), owner);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassdescriptor
        // description can be null
        // fixedRole can be null
        web::json::value make_nc_class_descriptor(const web::json::value& description, const nc_class_id& class_id, const nc_name& name, const web::json::value& fixed_role, const web::json::value& properties, const web::json::value& methods, const web::json::value& events)
        {
            using web::json::value;

            auto data = make_nc_descriptor(description);
            data[nmos::fields::nc::class_id] = make_nc_class_id(class_id);
            data[nmos::fields::nc::name] = value::string(name);
            data[nmos::fields::nc::fixed_role] = fixed_role;
            data[nmos::fields::nc::properties] = properties;
            data[nmos::fields::nc::methods] = methods;
            data[nmos::fields::nc::events] = events;

            return data;
        }
        web::json::value make_nc_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const web::json::value& fixed_role, const web::json::value& properties, const web::json::value& methods, const web::json::value& events)
        {
            using web::json::value;

            return make_nc_class_descriptor(value::string(description), class_id, name, fixed_role, properties, methods, events);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncenumitemdescriptor
        // description can be null
        web::json::value make_nc_enum_item_descriptor(const web::json::value& description, const nc_name& name, uint16_t val)
        {
            using web::json::value;

            auto data = make_nc_descriptor(description);
            data[nmos::fields::nc::name] = value::string(name);
            data[nmos::fields::nc::value] = val;

            return data;
        }
        web::json::value make_nc_enum_item_descriptor(const utility::string_t& description, const nc_name& name, uint16_t val)
        {
            using web::json::value;

            return make_nc_enum_item_descriptor(value::string(description), name, val);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nceventdescriptor
        // description can be null
        // id = make_nc_event_id(level, index)
        web::json::value make_nc_event_descriptor(const web::json::value& description, const nc_event_id& id, const nc_name& name, const utility::string_t& event_datatype, bool is_deprecated)
        {
            using web::json::value;

            auto data = make_nc_descriptor(description);
            data[nmos::fields::nc::id] = make_nc_event_id(id);
            data[nmos::fields::nc::name] = value::string(name);
            data[nmos::fields::nc::event_datatype] = value::string(event_datatype);
            data[nmos::fields::nc::is_deprecated] = value::boolean(is_deprecated);

            return data;
        }
        web::json::value make_nc_event_descriptor(const utility::string_t& description, const nc_event_id& id, const nc_name& name, const utility::string_t& event_datatype, bool is_deprecated)
        {
            using web::json::value;

            return make_nc_event_descriptor(value::string(description), id, name, event_datatype, is_deprecated);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncfielddescriptor
        // description can be null
        // type_name can be null
        // constraints can be null
        web::json::value make_nc_field_descriptor(const web::json::value& description, const nc_name& name, const web::json::value& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints)
        {
            using web::json::value;

            auto data = make_nc_descriptor(description);
            data[nmos::fields::nc::name] = value::string(name);
            data[nmos::fields::nc::type_name] = type_name;
            data[nmos::fields::nc::is_nullable] = value::boolean(is_nullable);
            data[nmos::fields::nc::is_sequence] = value::boolean(is_sequence);
            data[nmos::fields::nc::constraints] = constraints;

            return data;
        }
        web::json::value make_nc_field_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints)
        {
            using web::json::value;

            return make_nc_field_descriptor(value::string(description), name, value::string(type_name), is_nullable, is_sequence, constraints);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethoddescriptor
        // description can be null
        // id = make_nc_method_id(level, index)
        // sequence<NcParameterDescriptor> parameters
        web::json::value make_nc_method_descriptor(const web::json::value& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype, const web::json::value& parameters, bool is_deprecated)
        {
            using web::json::value;

            auto data = make_nc_descriptor(description);
            data[nmos::fields::nc::id] = make_nc_method_id(id);
            data[nmos::fields::nc::name] = value::string(name);
            data[nmos::fields::nc::result_datatype] = value::string(result_datatype);
            data[nmos::fields::nc::parameters] = parameters;
            data[nmos::fields::nc::is_deprecated] = value::boolean(is_deprecated);

            return data;
        }
        web::json::value make_nc_method_descriptor(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype, const web::json::value& parameters, bool is_deprecated)
        {
            using web::json::value;

            return make_nc_method_descriptor(value::string(description), id, name, result_datatype, parameters, is_deprecated);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterdescriptor
        // description can be null
        // type_name can be null
        web::json::value make_nc_parameter_descriptor(const web::json::value& description, const nc_name& name, const web::json::value& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints)
        {
            using web::json::value;

            auto data = make_nc_descriptor(description);
            data[nmos::fields::nc::name] = value::string(name);
            data[nmos::fields::nc::type_name] = type_name;
            data[nmos::fields::nc::is_nullable] = value::boolean(is_nullable);
            data[nmos::fields::nc::is_sequence] = value::boolean(is_sequence);
            data[nmos::fields::nc::constraints] = constraints;

            return data;
        }
        web::json::value make_nc_parameter_descriptor(const utility::string_t& description, const nc_name& name, bool is_nullable, bool is_sequence, const web::json::value& constraints)
        {
            using web::json::value;

            return make_nc_parameter_descriptor(value::string(description), name, value::null(), is_nullable, is_sequence, constraints);
        }
        web::json::value make_nc_parameter_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints)
        {
            using web::json::value;

            return make_nc_parameter_descriptor(value::string(description), name, value::string(type_name), is_nullable, is_sequence, constraints);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertydescriptor
        // description can be null
        // constraints can be null
        web::json::value make_nc_property_descriptor(const web::json::value& description, const nc_property_id& id, const nc_name& name, const web::json::value& type_name,
            bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints)
        {
            using web::json::value;

            auto data = make_nc_descriptor(description);
            data[nmos::fields::nc::id] = make_nc_property_id(id);
            data[nmos::fields::nc::name] = value::string(name);
            data[nmos::fields::nc::type_name] = type_name;
            data[nmos::fields::nc::is_read_only] = value::boolean(is_read_only);
            data[nmos::fields::nc::is_nullable] = value::boolean(is_nullable);
            data[nmos::fields::nc::is_sequence] = value::boolean(is_sequence);
            data[nmos::fields::nc::is_deprecated] = value::boolean(is_deprecated);
            data[nmos::fields::nc::constraints] = constraints;

            return data;
        }
        web::json::value make_nc_property_descriptor(const utility::string_t& description, const nc_property_id& id, const nc_name& name, const utility::string_t& type_name,
            bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints)
        {
            using web::json::value;

            return nmos::details::make_nc_property_descriptor(value::string(description), id, name, value::string(type_name), is_read_only, is_nullable, is_sequence, is_deprecated, constraints);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptor
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_descriptor(const web::json::value& description, const nc_name& name, nc_datatype_type::type type, const web::json::value& constraints)
        {
            using web::json::value;

            auto data = make_nc_descriptor(description);
            data[nmos::fields::nc::name] = value::string(name);
            data[nmos::fields::nc::type] = type;
            data[nmos::fields::nc::constraints] = constraints;

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorenum
        // description can be null
        // constraints can be null
        // items: sequence<NcEnumItemDescriptor>
        web::json::value make_nc_datatype_descriptor_enum(const web::json::value& description, const nc_name& name, const web::json::value& items, const web::json::value& constraints)
        {
            auto data = make_nc_datatype_descriptor(description, name, nc_datatype_type::Enum, constraints);
            data[nmos::fields::nc::items] = items;

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorprimitive
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_descriptor_primitive(const web::json::value& description, const nc_name& name, const web::json::value& constraints)
        {
            return make_nc_datatype_descriptor(description, name, nc_datatype_type::Primitive, constraints);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorstruct
        // description can be null
        // constraints can be null
        // fields: sequence<NcFieldDescriptor>
        // parent_type can be null
        web::json::value make_nc_datatype_descriptor_struct(const web::json::value& description, const nc_name& name, const web::json::value& fields, const web::json::value& parent_type, const web::json::value& constraints)
        {
            auto data = make_nc_datatype_descriptor(description, name, nc_datatype_type::Struct, constraints);
            data[nmos::fields::nc::fields] = fields;
            data[nmos::fields::nc::parent_type] = parent_type;

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptortypedef
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_typedef(const web::json::value& description, const nc_name& name, bool is_sequence, const utility::string_t& parent_type, const web::json::value& constraints)
        {
            using web::json::value;

            auto data = make_nc_datatype_descriptor(description, name, nc_datatype_type::Typedef, constraints);
            data[nmos::fields::nc::parent_type] = value::string(parent_type);
            data[nmos::fields::nc::is_sequence] = value::boolean(is_sequence);

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
        web::json::value make_nc_object(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints)
        {
            using web::json::value;

            const auto id = utility::conversions::details::to_string_t(oid);
//            auto data = make_resource_core(id, user_label.is_null() ? U("") : user_label.as_string(), {});
            value data;
            data[nmos::fields::id] = value::string(id); // required for nmos::resource
            data[nmos::fields::nc::class_id] = make_nc_class_id(class_id);
            data[nmos::fields::nc::oid] = oid;
            data[nmos::fields::nc::constant_oid] = value::boolean(constant_oid);
            data[nmos::fields::nc::owner] = owner;
            data[nmos::fields::nc::role] = value::string(role);
            data[nmos::fields::nc::user_label] = user_label;
            data[nmos::fields::nc::touchpoints] = touchpoints;
            data[nmos::fields::nc::runtime_property_constraints] = runtime_property_constraints;

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
        web::json::value make_nc_block(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, const web::json::value& members)
        {
            using web::json::value;

            auto data = details::make_nc_object(class_id, oid, constant_oid, owner, role, user_label, touchpoints, runtime_property_constraints);
            data[nmos::fields::nc::enabled] = value::boolean(enabled);
            data[nmos::fields::nc::members] = members;

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncworker
        web::json::value make_nc_worker(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled)
        {
            using web::json::value;

            auto data = details::make_nc_object(class_id, oid, constant_oid, owner, role, user_label, touchpoints, runtime_property_constraints);
            data[nmos::fields::nc::enabled] = value::boolean(enabled);

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanager
        web::json::value make_nc_manager(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints)
        {
            return make_nc_object(class_id, oid, constant_oid, owner, role, user_label, touchpoints, runtime_property_constraints);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
        web::json::value make_nc_device_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints,
            const web::json::value& manufacturer, const web::json::value& product, const utility::string_t& serial_number,
            const web::json::value& user_inventory_code, const web::json::value& device_name, const web::json::value& device_role, const web::json::value& operational_state, nc_reset_cause::cause reset_cause)
        {
            using web::json::value;

            auto data = details::make_nc_manager(nc_device_manager_class_id, oid, true, owner, U("DeviceManager"), user_label, touchpoints, runtime_property_constraints);
            data[nmos::fields::nc::nc_version] = value::string(U("v1.0.0"));
            data[nmos::fields::nc::manufacturer] = manufacturer;
            data[nmos::fields::nc::product] = product;
            data[nmos::fields::nc::serial_number] = value::string(serial_number);
            data[nmos::fields::nc::user_inventory_code] = user_inventory_code;
            data[nmos::fields::nc::device_name] = device_name;
            data[nmos::fields::nc::device_role] = device_role;
            data[nmos::fields::nc::operational_state] = operational_state;
            data[nmos::fields::nc::reset_cause] = reset_cause;
            data[nmos::fields::nc::message] = value::null();

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
        web::json::value make_nc_class_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const nmos::experimental::control_protocol_state& control_protocol_state)
        {
            using web::json::value;

            auto data = make_nc_manager(nc_class_manager_class_id, oid, true, owner, U("ClassManager"), user_label, touchpoints, runtime_property_constraints);

            // add control classes
            data[nmos::fields::nc::control_classes] = value::array();
            auto& control_classes = data[nmos::fields::nc::control_classes];
            for (const auto& control_class : control_protocol_state.control_classes)
            {
                auto& ctl_class = control_class.second;
                web::json::push_back(control_classes, make_nc_class_descriptor(ctl_class.description, ctl_class.class_id, ctl_class.name, ctl_class.fixed_role, ctl_class.properties, ctl_class.methods, ctl_class.events));
            }

            // add datatypes
            data[nmos::fields::nc::datatypes] = value::array();
            auto& datatypes = data[nmos::fields::nc::datatypes];
            for (const auto& datatype : control_protocol_state.datatypes)
            {
                web::json::push_back(datatypes, datatype.second.descriptor);
            }

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertychangedeventdata
        web::json::value make_nc_property_changed_event_data(const nc_property_changed_event_data& property_changed_event_data)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::property_id, details::make_nc_property_id(property_changed_event_data.property_id) },
                { nmos::fields::nc::change_type, property_changed_event_data.change_type },
                { nmos::fields::nc::value, property_changed_event_data.value },
                { nmos::fields::nc::sequence_item_index, property_changed_event_data.sequence_item_index }
            });
        }
    }

    // message response
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#command-response-message-type
    web::json::value make_control_protocol_error_response(int32_t handle, const nc_method_result& method_result, const utility::string_t& error_message)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::handle, handle },
            { nmos::fields::nc::result, details::make_nc_method_result_error(method_result, error_message) }
        });
    }
    web::json::value make_control_protocol_message_response(int32_t handle, const nc_method_result& method_result)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::handle, handle },
            { nmos::fields::nc::result, details::make_nc_method_result(method_result) }
        });
    }
    web::json::value make_control_protocol_message_response(int32_t handle, const nc_method_result& method_result, const web::json::value& value)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::handle, handle },
            { nmos::fields::nc::result, details::make_nc_method_result(method_result, value) }
        });
    }
    web::json::value make_control_protocol_message_response(int32_t handle, const nc_method_result& method_result, uint32_t value_)
    {
        using web::json::value;

        return make_control_protocol_message_response(handle, method_result, value(value_));
    }
    web::json::value make_control_protocol_message_response(const web::json::value& responses)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::message_type, nc_message_type::command_response },
            { nmos::fields::nc::responses, responses }
        });
    }

    // subscription response
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#subscription-response-message-type
    web::json::value make_control_protocol_subscription_response(const web::json::value& subscriptions)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::message_type, nc_message_type::subscription_response },
            { nmos::fields::nc::subscriptions, subscriptions }
        });
    }

    // notification
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#notification-message-type
    web::json::value make_control_protocol_notification(nc_oid oid, const nc_event_id& event_id, const nc_property_changed_event_data& property_changed_event_data)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::oid, oid },
            { nmos::fields::nc::event_id, details::make_nc_event_id(event_id)},
            { nmos::fields::nc::event_data, details::make_nc_property_changed_event_data(property_changed_event_data) }
        });
    }
    web::json::value make_control_protocol_notification(const web::json::value& notifications)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::message_type, nc_message_type::notification },
            { nmos::fields::nc::notifications, notifications }
        });
    }

    // error message
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#error-messages
    web::json::value make_control_protocol_error_message(const nc_method_result& method_result, const utility::string_t& error_message)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::message_type, nc_message_type::error },
            { nmos::fields::nc::status, method_result.status},
            { nmos::fields::nc::error_message, error_message }
        });
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
    web::json::value make_nc_object_properties()
    {
        using web::json::value;

        auto properties = value::array();
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Static value. All instances of the same class will have the same identity value"), nc_object_class_id_property_id, nmos::fields::nc::class_id, U("NcClassId"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Object identifier"), nc_object_oid_property_id, nmos::fields::nc::oid, U("NcOid"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("TRUE iff OID is hardwired into device"), nc_object_constant_oid_property_id, nmos::fields::nc::constant_oid, U("NcBoolean"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("OID of containing block. Can only ever be null for the root block"), nc_object_owner_property_id, nmos::fields::nc::owner, U("NcOid"), true, true, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Role of object in the containing block"), nc_object_role_property_id, nmos::fields::nc::role, U("NcString"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Scribble strip"), nc_object_user_label_property_id, nmos::fields::nc::user_label, U("NcString"), false, true, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Touchpoints to other contexts"), nc_object_touchpoints_property_id, nmos::fields::nc::touchpoints, U("NcTouchpoint"), true, true, true, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Runtime property constraints"), nc_object_runtime_property_constraints_property_id, nmos::fields::nc::runtime_property_constraints, U("NcPropertyConstraints"), true, true, true, false));

        return properties;
    }
    web::json::value make_nc_object_methods()
    {
        using web::json::value;

        auto methods = value::array();
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Get property value"), nc_object_get_method_id, U("Get"), U("NcMethodResultPropertyValue"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property value"), nmos::fields::nc::value, true, false));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Set property value"), nc_object_set_method_id, U("Set"), U("NcMethodResult"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Index of item in the sequence"), nmos::fields::nc::index, U("NcId"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Get sequence item"), nc_object_get_sequence_item_method_id, U("GetSequenceItem"), U("NcMethodResultPropertyValue"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Index of item in the sequence"), nmos::fields::nc::index, U("NcId"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Value"), nmos::fields::nc::value, true, false));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Set sequence item value"), nc_object_set_sequence_item_method_id, U("SetSequenceItem"), U("NcMethodResult"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id,U("NcPropertyId"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Value"), nmos::fields::nc::value, true, false));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Add item to sequence"), nc_object_add_sequence_item_method_id, U("AddSequenceItem"), U("NcMethodResultId"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Index of item in the sequence"), nmos::fields::nc::index, U("NcId"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Delete sequence item"), nc_object_remove_sequence_item_method_id, U("RemoveSequenceItem"), U("NcMethodResult"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Get sequence length"), nc_object_get_sequence_length_method_id, U("GetSequenceLength"), U("NcMethodResultLength"), parameters, false));
        }

        return methods;
    }
    web::json::value make_nc_object_events()
    {
        using web::json::value;

        auto events = value::array();
        web::json::push_back(events, details::make_nc_event_descriptor(U("Property changed event"), nc_object_property_changed_event_id, U("PropertyChanged"), U("NcPropertyChangedEventData"), false));

        return events;
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
    web::json::value make_nc_block_properties()
    {
        using web::json::value;

        auto properties = value::array();
        web::json::push_back(properties, details::make_nc_property_descriptor(U("TRUE if block is functional"), nc_block_enabled_property_id, nmos::fields::nc::enabled, U("NcBoolean"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Descriptors of this block's members"), nc_block_members_property_id, nmos::fields::nc::members, U("NcBlockMemberDescriptor"), true, false, true, false));

        return properties;
    }
    web::json::value make_nc_block_methods()
    {
        using web::json::value;

        auto methods = value::array();
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If recurse is set to true, nested members can be retrieved"), nmos::fields::nc::recurse, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Gets descriptors of members of the block"), nc_block_get_member_descriptors_method_id, U("GetMemberDescriptors"), U("NcMethodResultBlockMemberDescriptors"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Relative path to search for (MUST not include the role of the block targeted by oid)"), nmos::fields::nc::path, U("NcRolePath"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Finds member(s) by path"), nc_block_find_members_by_path_method_id, U("FindMembersByPath"), U("NcMethodResultBlockMemberDescriptors"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Role text to search for"), nmos::fields::nc::role, U("NcString"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Signals if the comparison should be case sensitive"), nmos::fields::nc::case_sensitive, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("TRUE to only return exact matches"), nmos::fields::nc::match_whole_string, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("TRUE to search nested blocks"), nmos::fields::nc::recurse, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Finds members with given role name or fragment"), nc_block_find_members_by_role_method_id, U("FindMembersByRole"), U("NcMethodResultBlockMemberDescriptors"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Class id to search for"), nmos::fields::nc::class_id, U("NcClassId"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If TRUE it will also include derived class descriptors"), nmos::fields::nc::include_derived, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("TRUE to search nested blocks"), nmos::fields::nc::recurse,U("NcBoolean"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Finds members with given class id"), nc_block_find_members_by_class_id_method_id, U("FindMembersByClassId"), U("NcMethodResultBlockMemberDescriptors"), parameters, false));
        }

        return methods;
    }
    web::json::value make_nc_block_events()
    {
        using web::json::value;

        return value::array();
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncworker
    web::json::value make_nc_worker_properties()
    {
        using web::json::value;

        auto properties = value::array();
        web::json::push_back(properties, details::make_nc_property_descriptor(U("TRUE iff worker is enabled"), nc_worker_enabled_property_id, nmos::fields::nc::enabled, U("NcBoolean"), false, false, false, false));

        return properties;
    }
    web::json::value make_nc_worker_methods()
    {
        using web::json::value;

        return value::array();
    }
    web::json::value make_nc_worker_events()
    {
        using web::json::value;

        return value::array();
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanager
    web::json::value make_nc_manager_properties()
    {
        using web::json::value;

        return value::array();
    }
    web::json::value make_nc_manager_methods()
    {
        using web::json::value;

        return value::array();
    }
    web::json::value make_nc_manager_events()
    {
        using web::json::value;

        return value::array();
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
    web::json::value make_nc_device_manager_properties()
    {
        using web::json::value;

        auto properties = value::array();
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Version of MS-05-02 that this device uses"), nc_device_manager_nc_version_property_id, nmos::fields::nc::nc_version, U("NcVersionCode"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Manufacturer descriptor"), nc_device_manager_manufacturer_property_id, nmos::fields::nc::manufacturer, U("NcManufacturer"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Product descriptor"), nc_device_manager_product_property_id, nmos::fields::nc::product, U("NcProduct"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Serial number"), nc_device_manager_serial_number_property_id, nmos::fields::nc::serial_number, U("NcString"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Asset tracking identifier (user specified)"), nc_device_manager_user_inventory_code_property_id, nmos::fields::nc::user_inventory_code, U("NcString"), false, true, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Name of this device in the application. Instance name, not product name"), nc_device_manager_device_name_property_id, nmos::fields::nc::device_name, U("NcString"), false, true, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Role of this device in the application"), nc_device_manager_device_role_property_id, nmos::fields::nc::device_role, U("NcString"), false, true, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Device operational state"), nc_device_manager_operational_state_property_id, nmos::fields::nc::operational_state, U("NcDeviceOperationalState"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Reason for most recent reset"), nc_device_manager_reset_cause_property_id, nmos::fields::nc::reset_cause, U("NcResetCause"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Arbitrary message from dev to controller"), nc_device_manager_message_property_id, nmos::fields::nc::message, U("NcString"), true, true, false, false));

        return properties;
    }
    web::json::value make_nc_device_manager_methods()
    {
        using web::json::value;

        return value::array();
    }
    web::json::value make_nc_device_manager_events()
    {
        using web::json::value;

        return value::array();
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
    web::json::value make_nc_class_manager_properties()
    {
        using web::json::value;

        auto properties = value::array();
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Descriptions of all control classes in the device (descriptors do not contain inherited elements)"), nc_class_manager_control_classes_property_id, nmos::fields::nc::control_classes, U("NcClassDescriptor"), true, false, true, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Descriptions of all data types in the device (descriptors do not contain inherited elements)"), nc_class_manager_datatypes_property_id, nmos::fields::nc::datatypes, U("NcDatatypeDescriptor"), true, false, true, false));

        return properties;
    }
    web::json::value make_nc_class_manager_methods()
    {
        using web::json::value;

        auto methods = value::array();
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("class ID"), nmos::fields::nc::class_id, U("NcClassId"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If set the descriptor would contain all inherited elements"), nmos::fields::nc::include_inherited, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Get a single class descriptor"), nc_class_manager_get_control_class_method_id, U("GetControlClass"), U("NcMethodResultClassDescriptor"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("name of datatype"), nmos::fields::nc::name, U("NcName"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If set the descriptor would contain all inherited elements"), nmos::fields::nc::include_inherited, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Get a single datatype descriptor"), nc_class_manager_get_datatype_method_id, U("GetDatatype"), U("NcMethodResultDatatypeDescriptor"), parameters, false));
        }

        return methods;
    }
    web::json::value make_nc_class_manager_events()
    {
        using web::json::value;

        return value::array();
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    web::json::value make_nc_receiver_monitor_properties()
    {
        using web::json::value;

        auto properties = value::array();
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Connection status property"), nc_receiver_monitor_connection_status_property_id, nmos::fields::nc::connection_status, U("NcConnectionStatus"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Connection status message property"), nc_receiver_monitor_connection_status_message_property_id, nmos::fields::nc::connection_status_message, U("NcString"), true, true, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Payload status property"), nc_receiver_monitor_payload_status_property_id, nmos::fields::nc::payload_status, U("NcPayloadStatus"), true, false, false, false));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Payload status message property"), nc_receiver_monitor_payload_status_message_property_id, nmos::fields::nc::payload_status_message, U("NcString"), true, true, false, false));

        return properties;
    }
    web::json::value make_nc_receiver_monitor_methods()
    {
        using web::json::value;

        return value::array();
    }
    web::json::value make_nc_receiver_monitor_events()
    {
        using web::json::value;

        return value::array();
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    web::json::value make_nc_receiver_monitor_protected_properties()
    {
        using web::json::value;

        auto properties = value::array();
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Indicates if signal protection is active"), nc_receiver_monitor_protected_signal_protection_status_property_id, nmos::fields::nc::signal_protection_status, U("NcBoolean"), true, false, false, false));

        return properties;
    }
    web::json::value make_nc_receiver_monitor_protected_methods()
    {
        using web::json::value;

        return value::array();
    }
    web::json::value make_nc_receiver_monitor_protected_events()
    {
        using web::json::value;

        return value::array();
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    web::json::value make_nc_ident_beacon_properties()
    {
        using web::json::value;

        auto properties = value::array();
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Indicator active state"), nc_ident_beacon_active_property_id, nmos::fields::nc::active, U("NcBoolean"), false, false, false, false));

        return properties;
    }
    web::json::value make_nc_ident_beacon_methods()
    {
        using web::json::value;

        return value::array();
    }
    web::json::value make_nc_ident_beacon_events()
    {
        using web::json::value;

        return value::array();
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.html
    web::json::value make_nc_object_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(value::string(U("NcObject class descriptor")), nc_object_class_id, U("NcObject"), value::null(), make_nc_object_properties(), make_nc_object_methods(), make_nc_object_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.1.html
    web::json::value make_nc_block_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(value::string(U("NcBlock class descriptor")), nc_block_class_id, U("NcBlock"), value::null(), make_nc_block_properties(), make_nc_block_methods(), make_nc_block_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.2.html
    web::json::value make_nc_worker_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(value::string(U("NcWorker class descriptor")), nc_worker_class_id, U("NcWorker"), value::null(), make_nc_worker_properties(), make_nc_worker_methods(), make_nc_worker_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.html
    web::json::value make_nc_manager_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(value::string(U("NcManager class descriptor")), nc_manager_class_id, U("NcManager"), value::null(), make_nc_manager_properties(), make_nc_manager_methods(), make_nc_manager_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.1.html
    web::json::value make_nc_device_manager_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(value::string(U("NcDeviceManager class descriptor")), nc_device_manager_class_id, U("NcDeviceManager"), value::string(U("DeviceManager")), make_nc_device_manager_properties(), make_nc_device_manager_methods(), make_nc_device_manager_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.2.html
    web::json::value make_nc_class_manager_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(value::string(U("NcClassManager class descriptor")), nc_class_manager_class_id, U("NcClassManager"), value::string(U("ClassManager")), make_nc_class_manager_properties(), make_nc_class_manager_methods(), make_nc_class_manager_events());
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    web::json::value make_nc_ident_beacon_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(value::string(U("NcIdentBeacon class descriptor")), nc_ident_beacon_class_id, U("NcIdentBeacon"), value::null(), make_nc_ident_beacon_properties(), make_nc_ident_beacon_methods(), make_nc_ident_beacon_events());
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    web::json::value make_nc_receiver_monitor_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(value::string(U("NcReceiverMonitor class descriptor")), nc_receiver_monitor_class_id, U("NcReceiverMonitor"), value::null(), make_nc_receiver_monitor_properties(), make_nc_receiver_monitor_methods(), make_nc_receiver_monitor_events());
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    web::json::value make_nc_receiver_monitor_protected_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(value::string(U("NcReceiverMonitorProtected class descriptor")), nc_receiver_monitor_protected_class_id, U("NcReceiverMonitorProtected"), value::null(), make_nc_receiver_monitor_protected_properties(), make_nc_receiver_monitor_protected_methods(), make_nc_receiver_monitor_protected_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcBlockMemberDescriptor.html
    web::json::value make_nc_block_member_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Role of member in its containing block")), nmos::fields::nc::role, value::string(U("NcString")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("OID of member")), nmos::fields::nc::oid, value::string(U("NcOid")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff member's OID is hardwired into device")), nmos::fields::nc::constant_oid, value::string(U("NcBoolean")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Class ID")), nmos::fields::nc::class_id, value::string(U("NcClassId")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("User label")), nmos::fields::nc::user_label, value::string(U("NcString")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Containing block's OID")), nmos::fields::nc::owner, value::string(U("NcOid")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Descriptor which is specific to a block member")), U("NcBlockMemberDescriptor"), fields, value::string(U("NcDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcClassDescriptor.html
    web::json::value make_nc_class_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Identity of the class")), nmos::fields::nc::class_id, value::string(U("NcClassId")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of the class")), nmos::fields::nc::name, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Role if the class has fixed role (manager classes)")), nmos::fields::nc::fixed_role, value::string(U("NcString")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Property descriptors")), nmos::fields::nc::properties, value::string(U("NcPropertyDescriptor")), false, true));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Method descriptors")), nmos::fields::nc::methods, value::string(U("NcMethodDescriptor")), false, true));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Event descriptors")), nmos::fields::nc::events, value::string(U("NcEventDescriptor")), false, true));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Descriptor of a class")), U("NcClassDescriptor"), fields, value::string(U("NcDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcClassId.html
    web::json::value make_nc_class_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Sequence of class ID fields")), U("NcClassId"), true, U("NcInt32"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptor.html
    web::json::value make_nc_datatype_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Datatype name")), nmos::fields::nc::name, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Type: Primitive, Typedef, Struct, Enum")), nmos::fields::nc::type, value::string(U("NcDatatypeType")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional constraints on top of the underlying data type")), nmos::fields::nc::constraints, value::string(U("NcParameterConstraints")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Base datatype descriptor")), U("NcDatatypeDescriptor"), fields, value::string(U("NcDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorEnum.html
    web::json::value make_nc_datatype_descriptor_enum_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("One item descriptor per enum option")), nmos::fields::nc::items, value::string(U("NcEnumItemDescriptor")), false, true));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Enum datatype descriptor")), U("NcDatatypeDescriptorEnum"), fields, value::string(U("NcDatatypeDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorPrimitive.html
    web::json::value make_nc_datatype_descriptor_primitive_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        return details::make_nc_datatype_descriptor_struct(value::string(U("Primitive datatype descriptor")), U("NcDatatypeDescriptorPrimitive"), fields, value::string(U("NcDatatypeDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorStruct.html
    web::json::value make_nc_datatype_descriptor_struct_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("One item descriptor per field of the struct")), nmos::fields::nc::fields, value::string(U("NcFieldDescriptor")), false, true));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of the parent type if any or null if it has no parent")), nmos::fields::nc::parent_type, value::string(U("NcName")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Struct datatype descriptor")), U("NcDatatypeDescriptorStruct"), fields, value::string(U("NcDatatypeDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorTypeDef.html
    web::json::value make_nc_datatype_descriptor_type_def_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Original typedef datatype name")), nmos::fields::nc::parent_type, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff type is a typedef sequence of another type")), nmos::fields::nc::is_sequence, value::string(U("NcBoolean")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Type def datatype descriptor")), U("NcDatatypeDescriptorTypeDef"), fields, value::string(U("NcDatatypeDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeType.html
    web::json::value make_nc_datatype_type_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Primitive datatype")), U("Primitive"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Simple alias of another datatype")), U("Typedef"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Data structure")), U("Struct"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Enum datatype")), U("Enum"), 3));
        return details::make_nc_datatype_descriptor_enum(value::string(U("Datatype type")), U("NcDatatypeType"), items);
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDescriptor.html
    web::json::value make_nc_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional user facing description")), nmos::fields::nc::description, value::string(U("NcString")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Base descriptor")), U("NcDescriptor"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDeviceGenericState.html
    web::json::value make_nc_device_generic_state_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Unknown")), U("Unknown"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Normal operation")), U("NormalOperation"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Device is initializing")), U("Initializing"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Device is performing a software or firmware update")), U("Updating"), 3));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Device is experiencing a licensing error")), U("LicensingError"), 4));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Device is experiencing an internal error")), U("InternalError"), 5));
        return details::make_nc_datatype_descriptor_enum(value::string(U("Device generic operational state")), U("NcDeviceGenericState"), items);
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDeviceOperationalState.html
    web::json::value make_nc_device_operational_state_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Generic operational state")), nmos::fields::nc::generic_state, value::string(U("NcDeviceGenericState")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Specific device details")), nmos::fields::nc::device_specific_details, value::string(U("NcString")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Device operational state")), U("NcDeviceOperationalState"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcElementId.html
    web::json::value make_nc_element_id_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Level of the element")), nmos::fields::nc::level, value::string(U("NcUint16")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Index of the element")), nmos::fields::nc::index, value::string(U("NcUint16")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Class element id which contains the level and index")), U("NcElementId"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEnumItemDescriptor.html
    web::json::value make_nc_enum_item_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of option")), nmos::fields::nc::name, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Enum item numerical value")), nmos::fields::nc::value, value::string(U("NcUint16")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Descriptor of an enum item")), U("NcEnumItemDescriptor"), fields, value::string(U("NcDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEventDescriptor.html
    web::json::value make_nc_event_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Event id with level and index")), nmos::fields::nc::id, value::string(U("NcEventId")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of event")), nmos::fields::nc::name, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of event data's datatype")), nmos::fields::nc::event_datatype, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff property is marked as deprecated")), nmos::fields::nc::is_deprecated, value::string(U("NcBoolean")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Descriptor of a class event")), U("NcEventDescriptor"), fields, value::string(U("NcDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEventId.html
    web::json::value make_nc_event_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_struct(value::string(U("Event id which contains the level and index")), U("NcEventId"), value::array(), value::string(U("NcElementId")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcFieldDescriptor.html
    web::json::value make_nc_field_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of field")), nmos::fields::nc::name, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of field's datatype. Can only ever be null if the type is any")), nmos::fields::nc::type_name, value::string(U("NcName")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff field is nullable")), nmos::fields::nc::is_nullable, value::string(U("NcBoolean")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff field is a sequence")), nmos::fields::nc::is_sequence, value::string(U("NcBoolean")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional constraints on top of the underlying data type")), nmos::fields::nc::constraints, value::string(U("NcParameterConstraints")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Descriptor of a field of a struct")), U("NcFieldDescriptor"), fields, value::string(U("NcDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcId.html
    web::json::value make_nc_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Identity handler")), U("NcId"), false, U("NcUint32"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcManufacturer.html
    web::json::value make_nc_manufacturer_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Manufacturer's name")), nmos::fields::nc::name, value::string(U("NcString")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("IEEE OUI or CID of manufacturer")), nmos::fields::nc::organization_id, value::string(U("NcOrganizationId")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("URL of the manufacturer's website")), nmos::fields::nc::website, value::string(U("NcUri")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Manufacturer descriptor")), U("NcManufacturer"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodDescriptor.html
    web::json::value make_nc_method_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Method id with level and index")), nmos::fields::nc::id, value::string(U("NcMethodId")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of method")), nmos::fields::nc::name, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of method result's datatype")), nmos::fields::nc::result_datatype, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Parameter descriptors if any")), nmos::fields::nc::parameters, value::string(U("NcParameterDescriptor")), false, true));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff property is marked as deprecated")), nmos::fields::nc::is_deprecated, value::string(U("NcBoolean")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Descriptor of a class method")), U("NcMethodDescriptor"), fields, value::string(U("NcDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodId.html
    web::json::value make_nc_method_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_struct(value::string(U("Method id which contains the level and index")), U("NcMethodId"), value::array(), value::string(U("NcElementId")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResult.html
    web::json::value make_nc_method_result_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Status for the invoked method")), nmos::fields::nc::status, value::string(U("NcMethodStatus")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Base result of the invoked method")), U("NcMethodResult"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultBlockMemberDescriptors.html
    web::json::value make_nc_method_result_block_member_descriptors_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Block member descriptors method result value")), nmos::fields::nc::value, value::string(U("NcBlockMemberDescriptor")), false, true));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Method result containing block member descriptors as the value")), U("NcMethodResultBlockMemberDescriptors"), fields, value::string(U("NcMethodResult")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultClassDescriptor.html
    web::json::value make_nc_method_result_class_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Class descriptor method result value")), nmos::fields::nc::value, value::string(U("NcClassDescriptor")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Method result containing a class descriptor as the value")), U("NcMethodResultClassDescriptor"), fields, value::string(U("NcMethodResult")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultDatatypeDescriptor.html
    web::json::value make_nc_method_result_datatype_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Datatype descriptor method result value")), nmos::fields::nc::value, value::string(U("NcDatatypeDescriptor")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Method result containing a datatype descriptor as the value")), U("NcMethodResultDatatypeDescriptor"), fields, value::string(U("NcMethodResult")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultError.html
    web::json::value make_nc_method_result_error_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Error message")), nmos::fields::nc::error_message, value::string(U("NcString")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Error result - to be used when the method call encounters an error")), U("NcMethodResultError"), fields, value::string(U("NcMethodResult")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultId.html
    web::json::value make_nc_method_result_id_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Id result value")), nmos::fields::nc::value, value::string(U("NcId")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Id method result")), U("NcMethodResultId"), fields, value::string(U("NcMethodResult")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultLength.html
    web::json::value make_nc_method_result_length_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Length result value")), nmos::fields::nc::value, value::string(U("NcUint32")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Length method result")), U("NcMethodResultLength"), fields, value::string(U("NcMethodResult")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultPropertyValue.html
    web::json::value make_nc_method_result_property_value_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Getter method value for the associated property")), nmos::fields::nc::value, value::null(), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Result when invoking the getter method associated with a property")), U("NcMethodResultPropertyValue"), fields, value::string(U("NcMethodResult")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodStatus.html
    web::json::value make_nc_method_status_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Method call was successful")), U("Ok"), 200));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Method call was successful but targeted property is deprecated")), U("PropertyDeprecated"), 298));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Method call was successful but method is deprecated")), U("MethodDeprecated"), 299));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Badly-formed command (e.g. the incoming command has invalid message encoding and cannot be parsed by the underlying protocol)")), U("BadCommandFormat"), 400));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Client is not authorized")), U("Unauthorized"), 401));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Command addresses a nonexistent object")), U("BadOid"), 404));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Attempt to change read-only state")), U("Readonly"), 405));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Method call is invalid in current operating context (e.g. attempting to invoke a method when the object is disabled)")), U("InvalidRequest"), 406));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("There is a conflict with the current state of the device")), U("Conflict"), 409));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Something was too big")), U("BufferOverflow"), 413));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Index is outside the available range")), U("IndexOutOfBounds"), 414));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Method parameter does not meet expectations (e.g. attempting to invoke a method with an invalid type for one of its parameters)")), U("ParameterError"), 417));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Addressed object is locked")), U("Locked"), 423));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Internal device error")), U("DeviceError"), 500));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Addressed method is not implemented by the addressed object")), U("MethodNotImplemented"), 501));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Addressed property is not implemented by the addressed object")), U("PropertyNotImplemented"), 502));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("The device is not ready to handle any commands")), U("NotReady"), 503));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Method call did not finish within the allotted time")), U("Timeout"), 504));
        return details::make_nc_datatype_descriptor_enum(value::string(U("Method invokation status")), U("NcMethodStatus"), items);
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcName.html
    web::json::value make_nc_name_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Programmatically significant name, alphanumerics + underscore, no spaces")), U("NcName"), false, U("NcString"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcOid.html
    web::json::value make_nc_oid_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Object id")), U("NcOid"), false, U("NcUint32"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcOrganizationId.html
    web::json::value make_nc_organization_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Unique 24-bit organization id")), U("NcOrganizationId"), false, U("NcInt32"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraints.html
    web::json::value make_nc_parameter_constraints_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Default value")), nmos::fields::nc::default_value, value::null(), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Abstract parameter constraints class")), U("NcParameterConstraints"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraintsNumber.html
    web::json::value make_nc_parameter_constraints_number_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional maximum")), nmos::fields::nc::maximum, value::null(), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional minimum")), nmos::fields::nc::minimum, value::null(), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional step")), nmos::fields::nc::step, value::null(), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Number parameter constraints class")), U("NcParameterConstraintsNumber"), fields, value::string(U("NcParameterConstraints")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraintsString.html
    web::json::value make_nc_parameter_constraints_string_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Maximum characters allowed")), nmos::fields::nc::max_characters, value::string(U("NcUint32")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Regex pattern")), nmos::fields::nc::pattern, value::string(U("NcRegex")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("String parameter constraints class")), U("NcParameterConstraintsString"), fields, value::string(U("NcParameterConstraints")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterDescriptor.html
    web::json::value make_nc_parameter_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of parameter")), nmos::fields::nc::name, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of parameter's datatype. Can only ever be null if the type is any")), nmos::fields::nc::type_name, value::string(U("NcName")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff property is nullable")), nmos::fields::nc::is_nullable, value::string(U("NcBoolean")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff property is a sequence")), nmos::fields::nc::is_sequence, value::string(U("NcBoolean")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional constraints on top of the underlying data type")), nmos::fields::nc::constraints, value::string(U("NcParameterConstraints")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Descriptor of a method parameter")), U("NcParameterDescriptor"), fields, value::string(U("NcDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcProduct.html
    web::json::value make_nc_product_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Product name")), nmos::fields::nc::name, value::string(U("NcString")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Manufacturer's unique key to product - model number, SKU, etc")), nmos::fields::nc::key, value::string(U("NcString")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Manufacturer's product revision level code")), nmos::fields::nc::revision_level, value::string(U("NcString")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Brand name under which product is sold")), nmos::fields::nc::brand_name, value::string(U("NcString")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Unique UUID of product (not product instance)")), nmos::fields::nc::uuid, value::string(U("NcUuid")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Text description of product")), nmos::fields::nc::description, value::string(U("NcString")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Product descriptor")), U("NcProduct"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyChangeType.html
    web::json::value make_nc_property_change_type_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Current value changed")), U("ValueChanged"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Sequence item added")), U("SequenceItemAdded"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Sequence item changed")), U("SequenceItemChanged"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Sequence item removed")), U("SequenceItemRemoved"), 3));
        return details::make_nc_datatype_descriptor_enum(value::string(U("Type of property change")), U("NcPropertyChangeType"), items);
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyChangedEventData.html
    web::json::value make_nc_property_changed_event_data_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("The id of the property that changed")), nmos::fields::nc::property_id, value::string(U("NcPropertyId")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Information regarding the change type")), nmos::fields::nc::change_type, value::string(U("NcPropertyChangeType")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Property-type specific value")), nmos::fields::nc::value, value::null(), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Index of sequence item if the property is a sequence")), nmos::fields::nc::sequence_item_index, value::string(U("NcId")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Payload of property-changed event")), U("NcPropertyChangedEventData"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraints.html
    web::json::value make_nc_property_contraints_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("The id of the property being constrained")), nmos::fields::nc::property_id, value::string(U("NcPropertyId")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional default value")), nmos::fields::nc::default_value, value::null(), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Property constraints class")), U("NcPropertyConstraints"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraintsNumber.html
    web::json::value make_nc_property_constraints_number_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional maximum")), nmos::fields::nc::maximum, value::null(), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional minimum")), nmos::fields::nc::minimum, value::null(), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional step")), nmos::fields::nc::step, value::null(), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Number property constraints class")), U("NcPropertyConstraintsNumber"), fields, value::string(U("NcPropertyConstraints")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraintsString.html
    web::json::value make_nc_property_constraints_string_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Maximum characters allowed")), nmos::fields::nc::max_characters, value::string(U("NcUint32")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Regex pattern")), nmos::fields::nc::pattern, value::string(U("NcRegex")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("String property constraints class")), U("NcPropertyConstraintsString"), fields, value::string(U("NcPropertyConstraints")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyDescriptor.html
    web::json::value make_nc_property_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Property id with level and index")), nmos::fields::nc::id, value::string(U("NcPropertyId")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of property")), nmos::fields::nc::name, value::string(U("NcName")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Name of property's datatype. Can only ever be null if the type is any")), nmos::fields::nc::type_name, value::string(U("NcName")), true, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff property is read-only")), nmos::fields::nc::is_read_only, value::string(U("NcBoolean")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff property is nullable")), nmos::fields::nc::is_nullable, value::string(U("NcBoolean")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff property is a sequence")), nmos::fields::nc::is_sequence, value::string(U("NcBoolean")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("TRUE iff property is marked as deprecated")), nmos::fields::nc::is_deprecated, value::string(U("NcBoolean")), false, false));
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Optional constraints on top of the underlying data type")), nmos::fields::nc::constraints, value::string(U("NcParameterConstraints")), true, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Descriptor of a class property")), U("NcPropertyDescriptor"), fields, value::string(U("NcDescriptor")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyId.html
    web::json::value make_nc_property_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_struct(value::string(U("Property id which contains the level and index")), U("NcPropertyId"), value::array(), value::string(U("NcElementId")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcRegex.html
    web::json::value make_nc_regex_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Regex pattern")), U("NcRegex"), false, U("NcString"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcResetCause.html
    web::json::value make_nc_reset_cause_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Unknown")), U("Unknown"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Power on")), U("PowerOn"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Internal error")), U("InternalError"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Upgrade")), U("Upgrade"), 3));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Controller request")), U("ControllerRequest"), 4));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Manual request from the front panel")), U("ManualReset"), 5));
        return details::make_nc_datatype_descriptor_enum(value::string(U("Reset cause enum")), U("NcResetCause"), items);
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcRolePath.html
    web::json::value make_nc_role_path_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Role path")), U("NcRolePath"), true, U("NcString"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTimeInterval.html
    web::json::value make_nc_time_interval_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Time interval described in nanoseconds")), U("NcTimeInterval"), false, U("NcInt64"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpoint.html
    web::json::value make_nc_touchpoint_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Context namespace")), nmos::fields::nc::context_namespace, value::string(U("NcString")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Base touchpoint class")), U("NcTouchpoint"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointNmos.html
    web::json::value make_nc_touchpoint_nmos_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Context NMOS resource")), nmos::fields::nc::resource, value::string(U("NcTouchpointResourceNmos")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Touchpoint class for NMOS resources")), U("NcTouchpointNmos"), fields, value::string(U("NcTouchpoint")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointNmosChannelMapping.html
    web::json::value make_nc_touchpoint_nmos_channel_mapping_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("Context Channel Mapping resource")), nmos::fields::nc::resource, value::string(U("NcTouchpointResourceNmosChannelMapping")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Touchpoint class for NMOS IS-08 resources")), U("NcTouchpointNmosChannelMapping"), fields, value::string(U("NcTouchpoint")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResource.html
    web::json::value make_nc_touchpoint_resource_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("The type of the resource")), nmos::fields::nc::resource_type, value::string(U("NcString")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Touchpoint resource class")), U("NcTouchpointResource"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResourceNmos.html
    web::json::value make_nc_touchpoint_resource_nmos_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("NMOS resource UUID")), nmos::fields::nc::id, value::string(U("NcUuid")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Touchpoint resource class for NMOS resources")), U("NcTouchpointResourceNmos"), fields, value::string(U("NcTouchpointResource")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResourceNmosChannelMapping.html
    web::json::value make_nc_touchpoint_resource_nmos_channel_mapping_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(value::string(U("IS-08 Audio Channel Mapping input or output id")), nmos::fields::nc::io_id, value::string(U("NcString")), false, false));
        return details::make_nc_datatype_descriptor_struct(value::string(U("Touchpoint resource class for NMOS resources")), U("NcTouchpointResourceNmosChannelMapping"), fields, value::string(U("NcTouchpointResourceNmos")));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcUri.html
    web::json::value make_nc_uri_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Uniform resource identifier")), U("NcUri"), false, U("NcString"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcUuid.html
    web::json::value make_nc_uuid_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("UUID")), U("NcUuid"), false, U("NcString"));
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcVersionCode.html
    web::json::value make_nc_version_code_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(value::string(U("Version code in semantic versioning format")), U("NcVersionCode"), false, U("NcString"));
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncconnectionstatus
    web::json::value make_nc_connection_status_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("This is the value when there is no receiver")), U("Undefined"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Connected to a stream")), U("Connected"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Not connected to a stream")), U("Disconnected"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("A connection error was encountered")), U("ConnectionError"), 3));
        return details::make_nc_datatype_descriptor_enum(value::string(U("Connection status enum data typee")), U("NcConnectionStatus"), items);
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncpayloadstatus
    web::json::value make_nc_payload_status_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("This is the value when there's no connection")), U("Undefined"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Payload is being received without errors and is the correct type")), U("PayloadOK"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("Payload is being received but is of an unsupported type")), U("PayloadFormatUnsupported"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(value::string(U("A payload error was encountered")), U("PayloadError"), 3));
        return details::make_nc_datatype_descriptor_enum(value::string(U("Connection status enum data typee")), U("NcPayloadStatus"), items);
    }
}

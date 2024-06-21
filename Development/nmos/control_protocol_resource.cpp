#include "nmos/control_protocol_resource.h"

#include "cpprest/base_uri.h"
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
        nc_method_id parse_nc_method_id(const web::json::value& id)
        {
            return parse_nc_element_id(id);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyid
        web::json::value make_nc_property_id(const nc_property_id& id)
        {
            return make_nc_element_id(id);
        }
        nc_property_id parse_nc_property_id(const web::json::value& id)
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
        web::json::value make_nc_manufacturer(const utility::string_t& name, nc_organization_id organization_id, const web::uri& website)
        {
            using web::json::value;

            return make_nc_manufacturer(name, organization_id, value::string(website.to_string()));
        }
        web::json::value make_nc_manufacturer(const utility::string_t& name, nc_organization_id organization_id)
        {
            using web::json::value;

            return make_nc_manufacturer(name, organization_id, value::null());
        }
        web::json::value make_nc_manufacturer(const utility::string_t& name)
        {
            using web::json::value;

            return make_nc_manufacturer(name, value::null(), value::null());
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
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const utility::string_t& brand_name, const nc_uuid& uuid, const utility::string_t& description)
        {
            using web::json::value;

            return make_nc_product(name, key, revision_level, value::string(brand_name), value::string(uuid), value::string(description));
        }
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const utility::string_t& brand_name, const nc_uuid& uuid)
        {
            using web::json::value;

            return make_nc_product(name, key, revision_level, value::string(brand_name), value::string(uuid), value::null());
        }
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level,
            const utility::string_t& brand_name)
        {
            using web::json::value;

            return make_nc_product(name, key, revision_level, value::string(brand_name), value::null(), value::null());
        }
        web::json::value make_nc_product(const utility::string_t& name, const utility::string_t& key, const utility::string_t& revision_level)
        {
            using web::json::value;

            return make_nc_product(name, key, revision_level, value::null(), value::null(), value::null());
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
        web::json::value make_nc_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const utility::string_t& fixed_role, const web::json::value& properties, const web::json::value& methods, const web::json::value& events)
        {
            using web::json::value;

            return make_nc_class_descriptor(value::string(description), class_id, name, value::string(fixed_role), properties, methods, events);
        }
        web::json::value make_nc_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const web::json::value& properties, const web::json::value& methods, const web::json::value& events)
        {
            using web::json::value;

            return make_nc_class_descriptor(value::string(description), class_id, name, value::null(), properties, methods, events);
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
        web::json::value make_nc_field_descriptor(const utility::string_t& description, const nc_name& name, bool is_nullable, bool is_sequence, const web::json::value& constraints)
        {
            using web::json::value;

            return make_nc_field_descriptor(value::string(description), name, value::null(), is_nullable, is_sequence, constraints);
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
        web::json::value make_nc_datatype_descriptor_enum(const utility::string_t& description, const nc_name& name, const web::json::value& items, const web::json::value& constraints)
        {
            using web::json::value;

            return make_nc_datatype_descriptor_enum(value::string(description), name, items, constraints);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypedescriptorprimitive
        // description can be null
        // constraints can be null
        web::json::value make_nc_datatype_descriptor_primitive(const web::json::value& description, const nc_name& name, const web::json::value& constraints)
        {
            return make_nc_datatype_descriptor(description, name, nc_datatype_type::Primitive, constraints);
        }
        web::json::value make_nc_datatype_descriptor_primitive(const utility::string_t& description, const nc_name& name, const web::json::value& constraints)
        {
            using web::json::value;

            return make_nc_datatype_descriptor_primitive(value::string(description), name, constraints);
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
        web::json::value make_nc_datatype_descriptor_struct(const utility::string_t& description, const nc_name& name, const web::json::value& fields, const utility::string_t& parent_type, const web::json::value& constraints)
        {
            using web::json::value;

            return make_nc_datatype_descriptor_struct(value::string(description), name, fields, value::string(parent_type), constraints);
        }
        web::json::value make_nc_datatype_descriptor_struct(const utility::string_t& description, const nc_name& name, const web::json::value& fields, const web::json::value& constraints)
        {
            using web::json::value;

            return make_nc_datatype_descriptor_struct(value::string(description), name, fields, value::null(), constraints);
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
        web::json::value make_nc_datatype_typedef(const utility::string_t& description, const nc_name& name, bool is_sequence, const utility::string_t& parent_type, const web::json::value& constraints)
        {
            using web::json::value;

            return make_nc_datatype_typedef(value::string(description), name, is_sequence, parent_type, constraints);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyconstraints
        web::json::value make_nc_property_constraints(const nc_property_id& property_id, const web::json::value& default_value)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::property_id, make_nc_property_id(property_id) },
                { nmos::fields::nc::default_value, default_value }
            });
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyconstraintsnumber
        web::json::value make_nc_property_constraints_number(const nc_property_id& property_id, const web::json::value& default_value, const web::json::value& minimum, const web::json::value& maximum, const web::json::value& step)
        {
            using web::json::value;

            auto data = make_nc_property_constraints(property_id, default_value);
            data[nmos::fields::nc::minimum] = minimum;
            data[nmos::fields::nc::maximum] = maximum;
            data[nmos::fields::nc::step] = step;

            return data;
        }
        web::json::value make_nc_property_constraints_number(const nc_property_id& property_id, uint64_t default_value, uint64_t minimum, uint64_t maximum, uint64_t step)
        {
            using web::json::value;

            return make_nc_property_constraints_number(property_id, value(default_value), value(minimum), value(maximum), value(step));
        }
        web::json::value make_nc_property_constraints_number(const nc_property_id& property_id, uint64_t minimum, uint64_t maximum, uint64_t step)
        {
            using web::json::value;

            return make_nc_property_constraints_number(property_id, value::null(), minimum, maximum, step);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyconstraintsstring
        web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, const web::json::value& default_value, const web::json::value& max_characters, const web::json::value& pattern)
        {
            using web::json::value;

            auto data = make_nc_property_constraints(property_id, default_value);
            data[nmos::fields::nc::max_characters] = max_characters;
            data[nmos::fields::nc::pattern] = pattern;

            return data;
        }
        web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, const utility::string_t& default_value, uint32_t max_characters, const nc_regex& pattern)
        {
            using web::json::value;

            return make_nc_property_constraints_string(property_id, value::string(default_value), max_characters, value::string(pattern));
        }
        web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, uint32_t max_characters, const nc_regex& pattern)
        {
            using web::json::value;

            return make_nc_property_constraints_string(property_id, value::null(), max_characters, value::string(pattern));
        }
        web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, uint32_t max_characters)
        {
            using web::json::value;

            return make_nc_property_constraints_string(property_id, value::null(), max_characters, value::null());
        }
        web::json::value make_nc_property_constraints_string(const nc_property_id& property_id, const nc_regex& pattern)
        {
            using web::json::value;

            return make_nc_property_constraints_string(property_id, value::null(), value::null(), value::string(pattern));
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraints
        web::json::value make_nc_parameter_constraints(const web::json::value& default_value)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::default_value, default_value }
            });
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsnumber
        web::json::value make_nc_parameter_constraints_number(const web::json::value& default_value, const web::json::value& minimum, const web::json::value& maximum, const web::json::value& step)
        {
            using web::json::value;

            auto data = make_nc_parameter_constraints(default_value);
            data[nmos::fields::nc::minimum] = minimum;
            data[nmos::fields::nc::maximum] = maximum;
            data[nmos::fields::nc::step] = step;

            return data;
        }
        web::json::value make_nc_parameter_constraints_number(uint64_t default_value, uint64_t minimum, uint64_t maximum, uint64_t step)
        {
            using web::json::value;

            return make_nc_parameter_constraints_number(value(default_value), value(minimum), value(maximum), value(step));
        }
        web::json::value make_nc_parameter_constraints_number(uint64_t minimum, uint64_t maximum, uint64_t step)
        {
            using web::json::value;

            return make_nc_parameter_constraints_number(value::null(), minimum, maximum, step);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsstring
        web::json::value make_nc_parameter_constraints_string(const web::json::value& default_value, const web::json::value& max_characters, const web::json::value& pattern)
        {
            auto data = make_nc_parameter_constraints(default_value);
            data[nmos::fields::nc::max_characters] = max_characters;
            data[nmos::fields::nc::pattern] = pattern;

            return data;
        }
        web::json::value make_nc_parameter_constraints_string(const utility::string_t& default_value, uint32_t max_characters, const nc_regex& pattern)
        {
            using web::json::value;

            return make_nc_parameter_constraints_string(value::string(default_value), max_characters, value::string(pattern));
        }
        web::json::value make_nc_parameter_constraints_string(uint32_t max_characters, const nc_regex& pattern)
        {
            using web::json::value;

            return make_nc_parameter_constraints_string(value::null(), max_characters, value::string(pattern));
        }
        web::json::value make_nc_parameter_constraints_string(uint32_t max_characters)
        {
            using web::json::value;

            return make_nc_parameter_constraints_string(value::null(), max_characters, value::null());
        }
        web::json::value make_nc_parameter_constraints_string(const nc_regex& pattern)
        {
            using web::json::value;

            return make_nc_parameter_constraints_string(value::null(), value::null(), value::string(pattern));
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointresource
        web::json::value make_nc_touchpoint_resource(const nc_touchpoint_resource& resource)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::resource_type, resource.resource_type }
            });
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointresourcenmos
        web::json::value make_nc_touchpoint_resource_nmos(const nc_touchpoint_resource_nmos& resource)
        {
            using web::json::value;

            auto data = make_nc_touchpoint_resource(resource);
            data[nmos::fields::nc::id] = value::string(resource.id);

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointresourcenmoschannelmapping
        web::json::value make_nc_touchpoint_resource_nmos_channel_mapping(const nc_touchpoint_resource_nmos_channel_mapping& resource)
        {
            using web::json::value;

            auto data = make_nc_touchpoint_resource_nmos(resource);
            data[nmos::fields::nc::io_id] = value::string(resource.io_id);

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpoint
        web::json::value make_nc_touchpoint(const utility::string_t& context_namespace)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::context_namespace, context_namespace }
            });
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointnmos
        web::json::value make_nc_touchpoint_nmos(const nc_touchpoint_resource_nmos& resource)
        {
            auto data = make_nc_touchpoint(U("x-nmos"));
            data[nmos::fields::nc::resource] = make_nc_touchpoint_resource_nmos(resource);

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointnmoschannelmapping
        web::json::value make_nc_touchpoint_nmos_channel_mapping(const nc_touchpoint_resource_nmos_channel_mapping& resource)
        {
            auto data = make_nc_touchpoint(U("x-nmos/channelmapping"));
            data[nmos::fields::nc::resource] = make_nc_touchpoint_resource_nmos_channel_mapping(resource);

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
        web::json::value make_nc_object(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints)
        {
            using web::json::value;

            const auto id = utility::conversions::details::to_string_t(oid);
            auto data = make_resource_core(id, user_label.is_null() ? U("") : user_label.as_string(), description);  // required for nmos::resource
            data[nmos::fields::nc::class_id] = make_nc_class_id(class_id);
            data[nmos::fields::nc::oid] = oid;
            data[nmos::fields::nc::constant_oid] = value::boolean(constant_oid);
            data[nmos::fields::nc::owner] = owner;
            data[nmos::fields::nc::role] = value::string(role);
            data[nmos::fields::nc::user_label] = user_label;
            data[nmos::fields::nc::touchpoints] = touchpoints;
            data[nmos::fields::nc::runtime_property_constraints] = runtime_property_constraints; // level 2 runtime constraints. See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
        web::json::value make_nc_block(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled, const web::json::value& members)
        {
            using web::json::value;

            auto data = details::make_nc_object(class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints);
            data[nmos::fields::nc::enabled] = value::boolean(enabled);
            data[nmos::fields::nc::members] = members;

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncworker
        web::json::value make_nc_worker(const nc_class_id& class_id, nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled)
        {
            using web::json::value;

            auto data = details::make_nc_object(class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints);
            data[nmos::fields::nc::enabled] = value::boolean(enabled);

            return data;
        }

        // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
        web::json::value make_receiver_monitor(const nc_class_id& class_id, nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled,
            nc_connection_status::status connection_status, const utility::string_t& connection_status_message, nc_payload_status::status payload_status, const utility::string_t& payload_status_message)
        {
            using web::json::value;

            auto data = make_nc_worker(class_id, oid, constant_oid, owner, role, value::string(user_label), description, touchpoints, runtime_property_constraints, enabled);
            data[nmos::fields::nc::connection_status] = value::number(connection_status);
            data[nmos::fields::nc::connection_status_message] = value::string(connection_status_message);
            data[nmos::fields::nc::payload_status] = value::number(payload_status);
            data[nmos::fields::nc::payload_status_message] = value::string(payload_status_message);

            return data;
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanager
        web::json::value make_nc_manager(const nc_class_id& class_id, nc_oid oid, bool constant_oid, const web::json::value& owner, const utility::string_t& role, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints)
        {
            return make_nc_object(class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints);
        }

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
        web::json::value make_nc_device_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints,
            const web::json::value& manufacturer, const web::json::value& product, const utility::string_t& serial_number,
            const web::json::value& user_inventory_code, const web::json::value& device_name, const web::json::value& device_role, const web::json::value& operational_state, nc_reset_cause::cause reset_cause)
        {
            using web::json::value;

            auto data = details::make_nc_manager(nc_device_manager_class_id, oid, true, owner, U("DeviceManager"), user_label, description, touchpoints, runtime_property_constraints);
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
        web::json::value make_nc_class_manager(nc_oid oid, nc_oid owner, const web::json::value& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const nmos::experimental::control_protocol_state& control_protocol_state)
        {
            using web::json::value;

            auto data = make_nc_manager(nc_class_manager_class_id, oid, true, owner, U("ClassManager"), user_label, description, touchpoints, runtime_property_constraints);

            auto lock = control_protocol_state.read_lock();

            // add control classes
            data[nmos::fields::nc::control_classes] = value::array();
            auto& control_classes = data[nmos::fields::nc::control_classes];
            for (const auto& control_class : control_protocol_state.control_class_descriptors)
            {
                auto& ctl_class = control_class.second;

                auto method_descriptors = value::array();
                for (const auto& method_descriptor : ctl_class.method_descriptors) { web::json::push_back(method_descriptors, std::get<0>(method_descriptor)); }

                const auto class_description = ctl_class.fixed_role.is_null()
                    ? make_nc_class_descriptor(ctl_class.description, ctl_class.class_id, ctl_class.name, ctl_class.property_descriptors, method_descriptors, ctl_class.event_descriptors)
                    : make_nc_class_descriptor(ctl_class.description, ctl_class.class_id, ctl_class.name, ctl_class.fixed_role.as_string(), ctl_class.property_descriptors, method_descriptors, ctl_class.event_descriptors);
                web::json::push_back(control_classes, class_description);
            }

            // add datatypes
            data[nmos::fields::nc::datatypes] = value::array();
            auto& datatypes = data[nmos::fields::nc::datatypes];
            for (const auto& datatype_descriptor : control_protocol_state.datatype_descriptors)
            {
                web::json::push_back(datatypes, datatype_descriptor.second.descriptor);
            }

            return data;
        }

        // TODO: add link
        web::json::value make_nc_bulk_properties_manager(nc_oid oid, nc_oid owner, const web::json::value &user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints)
        {
            using web::json::value;

            auto data = make_nc_manager(nc_bulk_properties_manager_class_id, oid, true, owner, U("BulkPropertiesManager"), user_label, description, touchpoints, runtime_property_constraints);

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

    // command message response
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#command-response-message-type
    web::json::value make_control_protocol_response(int32_t handle, const web::json::value& method_result)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::handle, handle },
            { nmos::fields::nc::result, method_result }
        });
    }
    web::json::value make_control_protocol_command_response(const web::json::value& responses)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::message_type, ncp_message_type::command_response },
            { nmos::fields::nc::responses, responses }
        });
    }

    // subscription response
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#subscription-response-message-type
    web::json::value make_control_protocol_subscription_response(const web::json::value& subscriptions)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::message_type, ncp_message_type::subscription_response },
            { nmos::fields::nc::subscriptions, subscriptions }
        });
    }

    // notification
    // See https://specs.amwa.tv/ms-05-01/branches/v1.0.x/docs/Core_Mechanisms.html#notification-messages
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
    web::json::value make_control_protocol_notification_message(const web::json::value& notifications)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::message_type, ncp_message_type::notification },
            { nmos::fields::nc::notifications, notifications }
        });
    }

    // property changed notification event
    // See https://specs.amwa.tv/ms-05-01/branches/v1.0.x/docs/Core_Mechanisms.html#the-propertychanged-event
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/NcObject.html#propertychanged-event
    web::json::value make_property_changed_event(nc_oid oid, const std::vector<nc_property_changed_event_data>& property_changed_event_data_list)
    {
        using web::json::value;

        auto notifications = value::array();
        for (auto& property_changed_event_data : property_changed_event_data_list)
        {
            web::json::push_back(notifications, make_control_protocol_notification(oid, nc_object_property_changed_event_id, property_changed_event_data));
        }
        return make_control_protocol_notification_message(notifications);
    }

    // error message
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#error-messages
    web::json::value make_control_protocol_error_message(const nc_method_result& method_result, const utility::string_t& error_message)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::nc::message_type, ncp_message_type::error },
            { nmos::fields::nc::status, method_result.status},
            { nmos::fields::nc::error_message, error_message }
        });
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
    web::json::value make_nc_object_properties()
    {
        using web::json::value;

        auto properties = value::array();
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Static value. All instances of the same class will have the same identity value"), nc_object_class_id_property_id, nmos::fields::nc::class_id, U("NcClassId"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Object identifier"), nc_object_oid_property_id, nmos::fields::nc::oid, U("NcOid"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("TRUE iff OID is hardwired into device"), nc_object_constant_oid_property_id, nmos::fields::nc::constant_oid, U("NcBoolean"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("OID of containing block. Can only ever be null for the root block"), nc_object_owner_property_id, nmos::fields::nc::owner, U("NcOid"), true, true, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Role of object in the containing block"), nc_object_role_property_id, nmos::fields::nc::role, U("NcString"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Scribble strip"), nc_object_user_label_property_id, nmos::fields::nc::user_label, U("NcString"), false, true, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Touchpoints to other contexts"), nc_object_touchpoints_property_id, nmos::fields::nc::touchpoints, U("NcTouchpoint"), true, true, true, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Runtime property constraints"), nc_object_runtime_property_constraints_property_id, nmos::fields::nc::runtime_property_constraints, U("NcPropertyConstraints"), true, true, true, false, value::null()));

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
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property value"), nmos::fields::nc::value, true, false, value::null()));
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
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Value"), nmos::fields::nc::value, true, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Set sequence item value"), nc_object_set_sequence_item_method_id, U("SetSequenceItem"), U("NcMethodResult"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id,U("NcPropertyId"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("Value"), nmos::fields::nc::value, true, false, value::null()));
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
        web::json::push_back(properties, details::make_nc_property_descriptor(U("TRUE if block is functional"), nc_block_enabled_property_id, nmos::fields::nc::enabled, U("NcBoolean"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Descriptors of this block's members"), nc_block_members_property_id, nmos::fields::nc::members, U("NcBlockMemberDescriptor"), true, false, true, false, value::null()));

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
        web::json::push_back(properties, details::make_nc_property_descriptor(U("TRUE iff worker is enabled"), nc_worker_enabled_property_id, nmos::fields::nc::enabled, U("NcBoolean"), false, false, false, false, value::null()));

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
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Version of MS-05-02 that this device uses"), nc_device_manager_nc_version_property_id, nmos::fields::nc::nc_version, U("NcVersionCode"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Manufacturer descriptor"), nc_device_manager_manufacturer_property_id, nmos::fields::nc::manufacturer, U("NcManufacturer"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Product descriptor"), nc_device_manager_product_property_id, nmos::fields::nc::product, U("NcProduct"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Serial number"), nc_device_manager_serial_number_property_id, nmos::fields::nc::serial_number, U("NcString"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Asset tracking identifier (user specified)"), nc_device_manager_user_inventory_code_property_id, nmos::fields::nc::user_inventory_code, U("NcString"), false, true, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Name of this device in the application. Instance name, not product name"), nc_device_manager_device_name_property_id, nmos::fields::nc::device_name, U("NcString"), false, true, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Role of this device in the application"), nc_device_manager_device_role_property_id, nmos::fields::nc::device_role, U("NcString"), false, true, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Device operational state"), nc_device_manager_operational_state_property_id, nmos::fields::nc::operational_state, U("NcDeviceOperationalState"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Reason for most recent reset"), nc_device_manager_reset_cause_property_id, nmos::fields::nc::reset_cause, U("NcResetCause"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Arbitrary message from dev to controller"), nc_device_manager_message_property_id, nmos::fields::nc::message, U("NcString"), true, true, false, false, value::null()));

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
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Descriptions of all control classes in the device (descriptors do not contain inherited elements)"), nc_class_manager_control_classes_property_id, nmos::fields::nc::control_classes, U("NcClassDescriptor"), true, false, true, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Descriptions of all data types in the device (descriptors do not contain inherited elements)"), nc_class_manager_datatypes_property_id, nmos::fields::nc::datatypes, U("NcDatatypeDescriptor"), true, false, true, false, value::null()));

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
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Connection status property"), nc_receiver_monitor_connection_status_property_id, nmos::fields::nc::connection_status, U("NcConnectionStatus"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Connection status message property"), nc_receiver_monitor_connection_status_message_property_id, nmos::fields::nc::connection_status_message, U("NcString"), true, true, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Payload status property"), nc_receiver_monitor_payload_status_property_id, nmos::fields::nc::payload_status, U("NcPayloadStatus"), true, false, false, false, value::null()));
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Payload status message property"), nc_receiver_monitor_payload_status_message_property_id, nmos::fields::nc::payload_status_message, U("NcString"), true, true, false, false, value::null()));

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
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Indicates if signal protection is active"), nc_receiver_monitor_protected_signal_protection_status_property_id, nmos::fields::nc::signal_protection_status, U("NcBoolean"), true, false, false, false, value::null()));

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
        web::json::push_back(properties, details::make_nc_property_descriptor(U("Indicator active state"), nc_ident_beacon_active_property_id, nmos::fields::nc::active, U("NcBoolean"), false, false, false, false, value::null()));

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

    // Device configuration classes
    // NcBulkPropertiesManager
    // TODO: add link
    web::json::value make_nc_bulk_properties_manager_properties()
    {
        using web::json::value;

        return value::array();
    }
    web::json::value make_nc_bulk_properties_manager_methods()
    {
        using web::json::value;

        auto methods = value::array();
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("The target role path"), nmos::fields::nc::path, U("NcRolePath"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If true will return properties on specified path and all the nested paths"), nmos::fields::nc::recurse, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Get bulk object properties by given path"), nc_bulk_properties_manager_get_properties_by_path_method_id, U("GetPropertiesByPath"), U("NcMethodResultBulkValuesHolder"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("The values offered (this may include read-only values and also paths which are not the target role path)"), nmos::fields::nc::data_set, U("NcBulkValuesHolder"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If set the descriptor would contain all inherited elements"), nmos::fields::nc::path, U("NcRolePath"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If true will validate properties on target path and all the nested paths"), nmos::fields::nc::recurse, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Validate bulk properties for setting by given paths"), nc_bulk_properties_manager_validate_set_properties_by_path_method_id, U("ValidateSetPropertiesByPath"), U("NcMethodResultObjectPropertiesSetValidation"), parameters, false));
        }
        {
            auto parameters = value::array();
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("The values offered (this may include read-only values and also paths which are not the target role path)"), nmos::fields::nc::data_set, U("NcBulkValuesHolder"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If set the descriptor would contain all inherited elements"), nmos::fields::nc::path, U("NcRolePath"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If true will validate properties on target path and all the nested paths"), nmos::fields::nc::recurse, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(parameters, details::make_nc_parameter_descriptor(U("If true will allow the device to restore only the role paths which pass validation(perform an incomplete restore)"), nmos::fields::nc::allow_incomplete, U("NcBoolean"), false, false, value::null()));
            web::json::push_back(methods, details::make_nc_method_descriptor(U("Set bulk properties for setting by given paths"), nc_bulk_properties_manager_set_properties_by_path_method_id, U("SetPropertiesByPath"), U("NcMethodResultObjectPropertiesSetValidation"), parameters, false));
        }

        return methods;
    }
    web::json::value make_nc_bulk_properties_manager_events()
    {
        using web::json::value;

        return value::array();
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.html
    web::json::value make_nc_object_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcObject class descriptor"), nc_object_class_id, U("NcObject"), make_nc_object_properties(), make_nc_object_methods(), make_nc_object_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.1.html
    web::json::value make_nc_block_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcBlock class descriptor"), nc_block_class_id, U("NcBlock"), make_nc_block_properties(), make_nc_block_methods(), make_nc_block_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.2.html
    web::json::value make_nc_worker_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcWorker class descriptor"), nc_worker_class_id, U("NcWorker"), make_nc_worker_properties(), make_nc_worker_methods(), make_nc_worker_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.html
    web::json::value make_nc_manager_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcManager class descriptor"), nc_manager_class_id, U("NcManager"), make_nc_manager_properties(), make_nc_manager_methods(), make_nc_manager_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.1.html
    web::json::value make_nc_device_manager_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcDeviceManager class descriptor"), nc_device_manager_class_id, U("NcDeviceManager"), U("DeviceManager"), make_nc_device_manager_properties(), make_nc_device_manager_methods(), make_nc_device_manager_events());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.3.2.html
    web::json::value make_nc_class_manager_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcClassManager class descriptor"), nc_class_manager_class_id, U("NcClassManager"), U("ClassManager"), make_nc_class_manager_properties(), make_nc_class_manager_methods(), make_nc_class_manager_events());
    }

    // Identification feature set control classes
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    web::json::value make_nc_ident_beacon_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcIdentBeacon class descriptor"), nc_ident_beacon_class_id, U("NcIdentBeacon"), make_nc_ident_beacon_properties(), make_nc_ident_beacon_methods(), make_nc_ident_beacon_events());
    }

    // Monitoring feature set control classes
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    web::json::value make_nc_receiver_monitor_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcReceiverMonitor class descriptor"), nc_receiver_monitor_class_id, U("NcReceiverMonitor"), make_nc_receiver_monitor_properties(), make_nc_receiver_monitor_methods(), make_nc_receiver_monitor_events());
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    web::json::value make_nc_receiver_monitor_protected_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcReceiverMonitorProtected class descriptor"), nc_receiver_monitor_protected_class_id, U("NcReceiverMonitorProtected"), make_nc_receiver_monitor_protected_properties(), make_nc_receiver_monitor_protected_methods(), make_nc_receiver_monitor_protected_events());
    }

    // Device configuration feature set control classes
    // TODO: add link
    web::json::value make_nc_bulk_properties_manager_class()
    {
        using web::json::value;

        return details::make_nc_class_descriptor(U("NcBulkPropertiesManager class descriptor"), nc_bulk_properties_manager_class_id, U("NcBulkPropertiesManager"), U("BulkPropertiesManager"), make_nc_bulk_properties_manager_properties(), make_nc_bulk_properties_manager_methods(), make_nc_bulk_properties_manager_events());
    }

    // Primitive datatypes
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_boolean_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("Boolean primitive type"), U("NcBoolean"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_int16_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("short"), U("NcInt16"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_int32_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("long"), U("NcInt32"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_int64_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("longlong"), U("NcInt64"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_uint16_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("unsignedshort"), U("NcUint16"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_uint32_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("unsignedlong"), U("NcUint32"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_uint64_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("unsignedlonglong"), U("NcUint64"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_float32_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("unrestrictedfloat"), U("NcFloat32"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_float64_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("unrestricteddouble"), U("NcFloat64"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#primitives
    web::json::value make_nc_string_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_primitive(U("UTF-8 string"), U("NcString"), value::null());
    }


    // Standard datatypes
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcBlockMemberDescriptor.html
    web::json::value make_nc_block_member_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Role of member in its containing block"), nmos::fields::nc::role, U("NcString"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("OID of member"), nmos::fields::nc::oid, U("NcOid"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff member's OID is hardwired into device"), nmos::fields::nc::constant_oid, U("NcBoolean"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Class ID"), nmos::fields::nc::class_id, U("NcClassId"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("User label"), nmos::fields::nc::user_label, U("NcString"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Containing block's OID"), nmos::fields::nc::owner, U("NcOid"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Descriptor which is specific to a block member"), U("NcBlockMemberDescriptor"), fields, U("NcDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcClassDescriptor.html
    web::json::value make_nc_class_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Identity of the class"), nmos::fields::nc::class_id, U("NcClassId"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of the class"), nmos::fields::nc::name, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Role if the class has fixed role (manager classes)"), nmos::fields::nc::fixed_role, U("NcString"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Property descriptors"), nmos::fields::nc::properties, U("NcPropertyDescriptor"), false, true, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Method descriptors"), nmos::fields::nc::methods, U("NcMethodDescriptor"), false, true, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Event descriptors"), nmos::fields::nc::events, U("NcEventDescriptor"), false, true, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Descriptor of a class"), U("NcClassDescriptor"), fields, U("NcDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcClassId.html
    web::json::value make_nc_class_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Sequence of class ID fields"), U("NcClassId"), true, U("NcInt32"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptor.html
    web::json::value make_nc_datatype_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Datatype name"), nmos::fields::nc::name, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Type: Primitive, Typedef, Struct, Enum"), nmos::fields::nc::type, U("NcDatatypeType"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional constraints on top of the underlying data type"), nmos::fields::nc::constraints, U("NcParameterConstraints"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Base datatype descriptor"), U("NcDatatypeDescriptor"), fields, U("NcDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorEnum.html
    web::json::value make_nc_datatype_descriptor_enum_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("One item descriptor per enum option"), nmos::fields::nc::items, U("NcEnumItemDescriptor"), false, true, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Enum datatype descriptor"), U("NcDatatypeDescriptorEnum"), fields, U("NcDatatypeDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorPrimitive.html
    web::json::value make_nc_datatype_descriptor_primitive_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        return details::make_nc_datatype_descriptor_struct(U("Primitive datatype descriptor"), U("NcDatatypeDescriptorPrimitive"), fields, U("NcDatatypeDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorStruct.html
    web::json::value make_nc_datatype_descriptor_struct_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("One item descriptor per field of the struct"), nmos::fields::nc::fields, U("NcFieldDescriptor"), false, true, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of the parent type if any or null if it has no parent"), nmos::fields::nc::parent_type, U("NcName"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Struct datatype descriptor"), U("NcDatatypeDescriptorStruct"), fields, U("NcDatatypeDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeDescriptorTypeDef.html
    web::json::value make_nc_datatype_descriptor_type_def_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Original typedef datatype name"), nmos::fields::nc::parent_type, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff type is a typedef sequence of another type"), nmos::fields::nc::is_sequence, U("NcBoolean"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Type def datatype descriptor"), U("NcDatatypeDescriptorTypeDef"), fields, U("NcDatatypeDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDatatypeType.html
    web::json::value make_nc_datatype_type_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Primitive datatype"), U("Primitive"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Simple alias of another datatype"), U("Typedef"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Data structure"), U("Struct"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Enum datatype"), U("Enum"), 3));
        return details::make_nc_datatype_descriptor_enum(U("Datatype type"), U("NcDatatypeType"), items, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDescriptor.html
    web::json::value make_nc_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional user facing description"), nmos::fields::nc::description, U("NcString"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Base descriptor"), U("NcDescriptor"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDeviceGenericState.html
    web::json::value make_nc_device_generic_state_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Unknown"), U("Unknown"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Normal operation"), U("NormalOperation"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Device is initializing"), U("Initializing"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Device is performing a software or firmware update"), U("Updating"), 3));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Device is experiencing a licensing error"), U("LicensingError"), 4));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Device is experiencing an internal error"), U("InternalError"), 5));
        return details::make_nc_datatype_descriptor_enum(U("Device generic operational state"), U("NcDeviceGenericState"), items, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDeviceOperationalState.html
    web::json::value make_nc_device_operational_state_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Generic operational state"), nmos::fields::nc::generic_state, U("NcDeviceGenericState"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Specific device details"), nmos::fields::nc::device_specific_details, U("NcString"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Device operational state"), U("NcDeviceOperationalState"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcElementId.html
    web::json::value make_nc_element_id_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Level of the element"), nmos::fields::nc::level, U("NcUint16"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Index of the element"), nmos::fields::nc::index, U("NcUint16"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Class element id which contains the level and index"), U("NcElementId"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEnumItemDescriptor.html
    web::json::value make_nc_enum_item_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of option"), nmos::fields::nc::name, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Enum item numerical value"), nmos::fields::nc::value, U("NcUint16"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Descriptor of an enum item"), U("NcEnumItemDescriptor"), fields, U("NcDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEventDescriptor.html
    web::json::value make_nc_event_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Event id with level and index"), nmos::fields::nc::id, U("NcEventId"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of event"), nmos::fields::nc::name, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of event data's datatype"), nmos::fields::nc::event_datatype, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff property is marked as deprecated"), nmos::fields::nc::is_deprecated, U("NcBoolean"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Descriptor of a class event"), U("NcEventDescriptor"), fields, U("NcDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcEventId.html
    web::json::value make_nc_event_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_struct(U("Event id which contains the level and index"), U("NcEventId"), value::array(), U("NcElementId"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcFieldDescriptor.html
    web::json::value make_nc_field_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of field"), nmos::fields::nc::name, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of field's datatype. Can only ever be null if the type is any"), nmos::fields::nc::type_name, U("NcName"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff field is nullable"), nmos::fields::nc::is_nullable, U("NcBoolean"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff field is a sequence"), nmos::fields::nc::is_sequence, U("NcBoolean"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional constraints on top of the underlying data type"), nmos::fields::nc::constraints, U("NcParameterConstraints"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Descriptor of a field of a struct"), U("NcFieldDescriptor"), fields, U("NcDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcId.html
    web::json::value make_nc_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Identity handler"), U("NcId"), false, U("NcUint32"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcManufacturer.html
    web::json::value make_nc_manufacturer_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Manufacturer's name"), nmos::fields::nc::name, U("NcString"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("IEEE OUI or CID of manufacturer"), nmos::fields::nc::organization_id, U("NcOrganizationId"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("URL of the manufacturer's website"), nmos::fields::nc::website, U("NcUri"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Manufacturer descriptor"), U("NcManufacturer"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodDescriptor.html
    web::json::value make_nc_method_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Method id with level and index"), nmos::fields::nc::id, U("NcMethodId"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of method"), nmos::fields::nc::name, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of method result's datatype"), nmos::fields::nc::result_datatype, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Parameter descriptors if any"), nmos::fields::nc::parameters, U("NcParameterDescriptor"), false, true, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff property is marked as deprecated"), nmos::fields::nc::is_deprecated, U("NcBoolean"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Descriptor of a class method"), U("NcMethodDescriptor"), fields, U("NcDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodId.html
    web::json::value make_nc_method_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_struct(U("Method id which contains the level and index"), U("NcMethodId"), value::array(), U("NcElementId"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResult.html
    web::json::value make_nc_method_result_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Status for the invoked method"), nmos::fields::nc::status, U("NcMethodStatus"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Base result of the invoked method"), U("NcMethodResult"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultBlockMemberDescriptors.html
    web::json::value make_nc_method_result_block_member_descriptors_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Block member descriptors method result value"), nmos::fields::nc::value, U("NcBlockMemberDescriptor"), false, true, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Method result containing block member descriptors as the value"), U("NcMethodResultBlockMemberDescriptors"), fields, U("NcMethodResult"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultClassDescriptor.html
    web::json::value make_nc_method_result_class_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Class descriptor method result value"), nmos::fields::nc::value, U("NcClassDescriptor"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Method result containing a class descriptor as the value"), U("NcMethodResultClassDescriptor"), fields, U("NcMethodResult"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultDatatypeDescriptor.html
    web::json::value make_nc_method_result_datatype_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Datatype descriptor method result value"), nmos::fields::nc::value, U("NcDatatypeDescriptor"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Method result containing a datatype descriptor as the value"), U("NcMethodResultDatatypeDescriptor"), fields, U("NcMethodResult"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultError.html
    web::json::value make_nc_method_result_error_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Error message"), nmos::fields::nc::error_message, U("NcString"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Error result - to be used when the method call encounters an error"), U("NcMethodResultError"), fields, U("NcMethodResult"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultId.html
    web::json::value make_nc_method_result_id_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Id result value"), nmos::fields::nc::value, U("NcId"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Id method result"), U("NcMethodResultId"), fields, U("NcMethodResult"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultLength.html
    web::json::value make_nc_method_result_length_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Length result value"), nmos::fields::nc::value, U("NcUint32"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Length method result"), U("NcMethodResultLength"), fields, U("NcMethodResult"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodResultPropertyValue.html
    web::json::value make_nc_method_result_property_value_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Getter method value for the associated property"), nmos::fields::nc::value, true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Result when invoking the getter method associated with a property"), U("NcMethodResultPropertyValue"), fields, U("NcMethodResult"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcMethodStatus.html
    web::json::value make_nc_method_status_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Method call was successful"), U("Ok"), 200));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Method call was successful but targeted property is deprecated"), U("PropertyDeprecated"), 298));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Method call was successful but method is deprecated"), U("MethodDeprecated"), 299));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Badly-formed command (e.g. the incoming command has invalid message encoding and cannot be parsed by the underlying protocol)"), U("BadCommandFormat"), 400));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Client is not authorized"), U("Unauthorized"), 401));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Command addresses a nonexistent object"), U("BadOid"), 404));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Attempt to change read-only state"), U("Readonly"), 405));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Method call is invalid in current operating context (e.g. attempting to invoke a method when the object is disabled)"), U("InvalidRequest"), 406));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("There is a conflict with the current state of the device"), U("Conflict"), 409));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Something was too big"), U("BufferOverflow"), 413));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Index is outside the available range"), U("IndexOutOfBounds"), 414));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Method parameter does not meet expectations (e.g. attempting to invoke a method with an invalid type for one of its parameters)"), U("ParameterError"), 417));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Addressed object is locked"), U("Locked"), 423));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Internal device error"), U("DeviceError"), 500));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Addressed method is not implemented by the addressed object"), U("MethodNotImplemented"), 501));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Addressed property is not implemented by the addressed object"), U("PropertyNotImplemented"), 502));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("The device is not ready to handle any commands"), U("NotReady"), 503));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Method call did not finish within the allotted time"), U("Timeout"), 504));
        return details::make_nc_datatype_descriptor_enum(U("Method invokation status"), U("NcMethodStatus"), items, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcName.html
    web::json::value make_nc_name_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Programmatically significant name, alphanumerics + underscore, no spaces"), U("NcName"), false, U("NcString"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcOid.html
    web::json::value make_nc_oid_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Object id"), U("NcOid"), false, U("NcUint32"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcOrganizationId.html
    web::json::value make_nc_organization_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Unique 24-bit organization id"), U("NcOrganizationId"), false, U("NcInt32"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraints.html
    web::json::value make_nc_parameter_constraints_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Default value"), nmos::fields::nc::default_value, true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Abstract parameter constraints class"), U("NcParameterConstraints"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraintsNumber.html
    web::json::value make_nc_parameter_constraints_number_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional minimum"), nmos::fields::nc::minimum, true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional maximum"), nmos::fields::nc::maximum, true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional step"), nmos::fields::nc::step, true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Number parameter constraints class"), U("NcParameterConstraintsNumber"), fields, U("NcParameterConstraints"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterConstraintsString.html
    web::json::value make_nc_parameter_constraints_string_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Maximum characters allowed"), nmos::fields::nc::max_characters, U("NcUint32"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Regex pattern"), nmos::fields::nc::pattern, U("NcRegex"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("String parameter constraints class"), U("NcParameterConstraintsString"), fields, U("NcParameterConstraints"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcParameterDescriptor.html
    web::json::value make_nc_parameter_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of parameter"), nmos::fields::nc::name, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of parameter's datatype. Can only ever be null if the type is any"), nmos::fields::nc::type_name, U("NcName"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff property is nullable"), nmos::fields::nc::is_nullable, U("NcBoolean"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff property is a sequence"), nmos::fields::nc::is_sequence, U("NcBoolean"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional constraints on top of the underlying data type"), nmos::fields::nc::constraints, U("NcParameterConstraints"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Descriptor of a method parameter"), U("NcParameterDescriptor"), fields, U("NcDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcProduct.html
    web::json::value make_nc_product_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Product name"), nmos::fields::nc::name, U("NcString"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Manufacturer's unique key to product - model number, SKU, etc"), nmos::fields::nc::key, U("NcString"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Manufacturer's product revision level code"), nmos::fields::nc::revision_level, U("NcString"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Brand name under which product is sold"), nmos::fields::nc::brand_name, U("NcString"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Unique UUID of product (not product instance)"), nmos::fields::nc::uuid, U("NcUuid"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Text description of product"), nmos::fields::nc::description, U("NcString"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Product descriptor"), U("NcProduct"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyChangeType.html
    web::json::value make_nc_property_change_type_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Current value changed"), U("ValueChanged"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Sequence item added"), U("SequenceItemAdded"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Sequence item changed"), U("SequenceItemChanged"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Sequence item removed"), U("SequenceItemRemoved"), 3));
        return details::make_nc_datatype_descriptor_enum(U("Type of property change"), U("NcPropertyChangeType"), items, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyChangedEventData.html
    web::json::value make_nc_property_changed_event_data_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("The id of the property that changed"), nmos::fields::nc::property_id, U("NcPropertyId"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Information regarding the change type"), nmos::fields::nc::change_type, U("NcPropertyChangeType"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Property-type specific value"), nmos::fields::nc::value, true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Index of sequence item if the property is a sequence"), nmos::fields::nc::sequence_item_index,U("NcId"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Payload of property-changed event"), U("NcPropertyChangedEventData"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraints.html
    web::json::value make_nc_property_contraints_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("The id of the property being constrained"), nmos::fields::nc::property_id, U("NcPropertyId"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional default value"), nmos::fields::nc::default_value, true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Property constraints class"), U("NcPropertyConstraints"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraintsNumber.html
    web::json::value make_nc_property_constraints_number_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional minimum"), nmos::fields::nc::minimum, true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional maximum"), nmos::fields::nc::maximum, true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional step"), nmos::fields::nc::step, true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Number property constraints class"), U("NcPropertyConstraintsNumber"), fields, U("NcPropertyConstraints"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyConstraintsString.html
    web::json::value make_nc_property_constraints_string_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Maximum characters allowed"), nmos::fields::nc::max_characters, U("NcUint32"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Regex pattern"), nmos::fields::nc::pattern, U("NcRegex"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("String property constraints class"), U("NcPropertyConstraintsString"), fields, U("NcPropertyConstraints"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyDescriptor.html
    web::json::value make_nc_property_descriptor_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Property id with level and index"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of property"), nmos::fields::nc::name, U("NcName"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Name of property's datatype. Can only ever be null if the type is any"), nmos::fields::nc::type_name, U("NcName"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff property is read-only"), nmos::fields::nc::is_read_only, U("NcBoolean"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff property is nullable"), nmos::fields::nc::is_nullable, U("NcBoolean"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff property is a sequence"), nmos::fields::nc::is_sequence, U("NcBoolean"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("TRUE iff property is marked as deprecated"), nmos::fields::nc::is_deprecated, U("NcBoolean"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional constraints on top of the underlying data type"), nmos::fields::nc::constraints, U("NcParameterConstraints"), true, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Descriptor of a class property"), U("NcPropertyDescriptor"), fields, U("NcDescriptor"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcPropertyId.html
    web::json::value make_nc_property_id_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_descriptor_struct(U("Property id which contains the level and index"), U("NcPropertyId"), value::array(), U("NcElementId"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcRegex.html
    web::json::value make_nc_regex_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Regex pattern"), U("NcRegex"), false, U("NcString"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcResetCause.html
    web::json::value make_nc_reset_cause_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Unknown"), U("Unknown"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Power on"), U("PowerOn"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Internal error"), U("InternalError"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Upgrade"), U("Upgrade"), 3));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Controller request"), U("ControllerRequest"), 4));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Manual request from the front panel"), U("ManualReset"), 5));
        return details::make_nc_datatype_descriptor_enum(U("Reset cause enum"), U("NcResetCause"), items, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcRolePath.html
    web::json::value make_nc_role_path_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Role path"), U("NcRolePath"), true, U("NcString"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTimeInterval.html
    web::json::value make_nc_time_interval_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Time interval described in nanoseconds"), U("NcTimeInterval"), false, U("NcInt64"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpoint.html
    web::json::value make_nc_touchpoint_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Context namespace"), nmos::fields::nc::context_namespace, U("NcString"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Base touchpoint class"), U("NcTouchpoint"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointNmos.html
    web::json::value make_nc_touchpoint_nmos_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Context NMOS resource"), nmos::fields::nc::resource, U("NcTouchpointResourceNmos"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Touchpoint class for NMOS resources"), U("NcTouchpointNmos"), fields, U("NcTouchpoint"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointNmosChannelMapping.html
    web::json::value make_nc_touchpoint_nmos_channel_mapping_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Context Channel Mapping resource"), nmos::fields::nc::resource,U("NcTouchpointResourceNmosChannelMapping"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Touchpoint class for NMOS IS-08 resources"), U("NcTouchpointNmosChannelMapping"), fields, U("NcTouchpoint"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResource.html
    web::json::value make_nc_touchpoint_resource_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("The type of the resource"), nmos::fields::nc::resource_type, U("NcString"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Touchpoint resource class"), U("NcTouchpointResource"), fields, value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResourceNmos.html
    web::json::value make_nc_touchpoint_resource_nmos_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("NMOS resource UUID"), nmos::fields::nc::id, U("NcUuid"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Touchpoint resource class for NMOS resources"), U("NcTouchpointResourceNmos"), fields, U("NcTouchpointResource"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcTouchpointResourceNmosChannelMapping.html
    web::json::value make_nc_touchpoint_resource_nmos_channel_mapping_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("IS-08 Audio Channel Mapping input or output id"), nmos::fields::nc::io_id, U("NcString"), false, false, value::null()));
        return details::make_nc_datatype_descriptor_struct(U("Touchpoint resource class for NMOS resources"), U("NcTouchpointResourceNmosChannelMapping"), fields, U("NcTouchpointResourceNmos"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcUri.html
    web::json::value make_nc_uri_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Uniform resource identifier"), U("NcUri"), false, U("NcString"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcUuid.html
    web::json::value make_nc_uuid_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("UUID"), U("NcUuid"), false, U("NcString"), value::null());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcVersionCode.html
    web::json::value make_nc_version_code_datatype()
    {
        using web::json::value;

        return details::make_nc_datatype_typedef(U("Version code in semantic versioning format"), U("NcVersionCode"), false, U("NcString"), value::null());
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncconnectionstatus
    web::json::value make_nc_connection_status_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("This is the value when there is no receiver"), U("Undefined"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Connected to a stream"), U("Connected"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Not connected to a stream"), U("Disconnected"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("A connection error was encountered"), U("ConnectionError"), 3));
        return details::make_nc_datatype_descriptor_enum(U("Connection status enum data typee"), U("NcConnectionStatus"), items, value::null());
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncpayloadstatus
    web::json::value make_nc_payload_status_datatype()
    {
        using web::json::value;

        auto items = value::array();
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("This is the value when there's no connection"), U("Undefined"), 0));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Payload is being received without errors and is the correct type"), U("PayloadOK"), 1));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("Payload is being received but is of an unsupported type"), U("PayloadFormatUnsupported"), 2));
        web::json::push_back(items, details::make_nc_enum_item_descriptor(U("A payload error was encountered"), U("PayloadError"), 3));
        return details::make_nc_datatype_descriptor_enum(U("Connection status enum data typee"), U("NcPayloadStatus"), items, value::null());
    }

    // Device Configuration datatypes
    // TODO: add link
    web::json::value make_nc_property_value_holder_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Property name"), nmos::fields::nc::name, U("NcString"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Property type name. If null it means the type is any"), nmos::fields::nc::type_name, U("NcName"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Is the property ReadOnly?"), nmos::fields::nc::is_read_only, U("NcBoolean"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Property value"), nmos::fields::nc::value, true, false, value::null()));

        return details::make_nc_datatype_descriptor_struct(U("Property value holder descriptor"), U("NcPropertyValueHolder"), fields, value::null());
    }
    // TODO: add link
    web::json::value make_nc_object_properties_holder_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Object role path"), nmos::fields::nc::path, U("NcRolePath"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Object properties values"), nmos::fields::nc::values, U("NcPropertyValueHolder"), false, true, value::null()));

        return details::make_nc_datatype_descriptor_struct(U("Object properties holder descriptor"), U("NcObjectPropertiesHolder"), fields, value::null());
    }
    // TODO: add link
    web::json::value make_nc_bulk_values_holder_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Optional vendor specific fingerprinting mechanism used for validation purposes"), nmos::fields::nc::validation_fingerprint, U("NcString"), true, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Values by rolePath"), nmos::fields::nc::values, U("NcObjectPropertiesHolder"), false, true, value::null()));

        return details::make_nc_datatype_descriptor_struct(U("Bulk values holder descriptor"), U("NcBulkValuesHolder"), fields, value::null());
    }
    // TODO: add link
    web::json::value make_nc_object_properties_set_validation_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Object role path"), nmos::fields::nc::path, U("NcRolePath"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Validation status"), nmos::fields::nc::status, U("NcMethodStatus"), false, false, value::null()));
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Validation status message"), nmos::fields::nc::status_message, U("NcString"), true, false, value::null()));

        return details::make_nc_datatype_descriptor_struct(U("Object properties set validation descriptor"), U("NcObjectPropertiesSetValidation"), fields, value::null());
    }
    // TODO: add link
    web::json::value make_nc_method_result_bulk_values_holder_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Bulk values holder value"), nmos::fields::nc::value, U("NcBulkValuesHolder"), false, false, value::null()));

        return details::make_nc_datatype_descriptor_struct(U("Method result containing bulk values holder descriptor"), U("NcMethodResultBulkValuesHolder"), fields, U("NcMethodResult"), value::null());
    }
    // TODO: add link
    web::json::value make_nc_method_result_object_properties_set_validation_datatype()
    {
        using web::json::value;

        auto fields = value::array();
        web::json::push_back(fields, details::make_nc_field_descriptor(U("Object properties set path validation"), nmos::fields::nc::value, U("NcObjectPropertiesSetValidation"), false, true, value::null()));

        return details::make_nc_datatype_descriptor_struct(U("Method result containing object properties set validation descriptor"), U("NcMethodResultObjectPropertiesSetValidation"), fields, U("NcMethodResult"), value::null());
    }
}

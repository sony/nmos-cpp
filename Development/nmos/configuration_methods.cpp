#include "nmos/configuration_methods.h"

#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/configuration_utils.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"

namespace nmos
{
    namespace details
    {
        web::json::array make_property_holders(const nmos::resource& resource, bool include_descriptors, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
        {
            using web::json::value;

            value property_holders = value::array();

            nmos::nc_class_id class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(resource.data));

            // make NcPropertyHolder objects
            while (!class_id.empty())
            {
                const auto& control_class_descriptor = get_control_protocol_class_descriptor(class_id);

                for (const auto& property_descriptor : control_class_descriptor.property_descriptors.as_array())
                {
                    const auto descriptor = include_descriptors ? property_descriptor : value::null();
                    value property_holder = nmos::nc::details::make_property_holder(nmos::nc::details::parse_property_id(nmos::fields::nc::id(property_descriptor)), descriptor, resource.data.at(nmos::fields::nc::name(property_descriptor)));

                    web::json::push_back(property_holders, property_holder);
                }
                class_id.pop_back();
            }
            return property_holders.as_array();
        }

        void populate_object_property_holder(const nmos::resources& resources, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const nmos::resource& resource, bool recurse, bool include_descriptors, web::json::value& object_properties_holders)
        {
            using web::json::value;

            // Get property_holders for this resource
            const auto& property_holders = make_property_holders(resource, include_descriptors, get_control_protocol_class_descriptor);

            const auto role_path = get_role_path(resources, resource);

            // when include_descriptors = false, don't include the Class Manager
            if (!include_descriptors && role_path.size() > 1 && role_path.at(0).as_string() == U("root") && role_path.at(1).as_string() == U("ClassManager"))
            {
                return;
            }
            const auto& dependency_paths = nmos::fields::nc::dependency_paths(resource.data);
            const auto& allowed_member_classes = nmos::fields::nc::allowed_members_classes(resource.data);

            auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path, property_holders, dependency_paths, allowed_member_classes, nmos::fields::nc::is_rebuildable(resource.data));

            web::json::push_back(object_properties_holders, object_properties_holder);

            // Recurse into members...if we want to...and the object has them
            if (recurse && nmos::nc::is_block(nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(resource.data))))
            {
                if (resource.data.has_field(nmos::fields::nc::members))
                {
                    const auto& members = nmos::fields::nc::members(resource.data);

                    for (const auto& member : members)
                    {
                        const auto found = find_resource(resources, utility::s2us(std::to_string(nmos::fields::nc::oid(member))));

                        if (resources.end() != found)
                        {
                            populate_object_property_holder(resources, get_control_protocol_class_descriptor, *found, recurse, include_descriptors, object_properties_holders);
                        }
                    }
                }
            }
        }
    }

    web::json::value get_properties_by_path(const nmos::resources& resources, const nmos::resource& resource, bool recurse, bool include_descriptors, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, nmos::create_validation_fingerprint_handler create_validation_fingerprint)
    {
        using web::json::value;

        value object_properties_holders = value::array();

        details::populate_object_property_holder(resources, get_control_protocol_class_descriptor, resource, recurse, include_descriptors, object_properties_holders);

        utility::string_t validation_fingerprint = U("");

        if (create_validation_fingerprint)
        {
            validation_fingerprint = create_validation_fingerprint(resources, resource);
        }

        auto bulk_properties_holder = nmos::nc::details::make_bulk_properties_holder(validation_fingerprint, object_properties_holders);

        return nmos::nc::details::make_method_result({ nmos::nc_method_status::ok }, bulk_properties_holder);
    }

    web::json::value validate_set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, nmos::nc_restore_mode::restore_mode restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, nmos::validate_validation_fingerprint_handler validate_validation_fingerprint, nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list, nmos::remove_device_model_object_handler remove_device_model_object, nmos::create_device_model_object_handler create_device_model_object)
    {
        if (validate_validation_fingerprint)
        {
            const auto& validation_fingerprint = nmos::fields::nc::validation_fingerprint(backup_data_set);

            if (!validate_validation_fingerprint(resources, resource, validation_fingerprint.c_str()))
            {
                return nmos::nc::details::make_method_result_error({ nmos::nc_method_status::invalid_request }, U("Invalid validation fingerprint"));
            }
        }
        const auto& object_properties_holders = nmos::fields::nc::values(backup_data_set);

        const auto object_properties_set_validation = apply_backup_data_set(resources, resource, object_properties_holders, recurse, restore_mode, true, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, nullptr);

        return nmos::nc::details::make_method_result({ nmos::nc_method_status::ok }, object_properties_set_validation);
    }

    web::json::value set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, nmos::nc_restore_mode::restore_mode restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, nmos::validate_validation_fingerprint_handler validate_validation_fingerprint, nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list, nmos::remove_device_model_object_handler remove_device_model_object, nmos::create_device_model_object_handler create_device_model_object, nmos::control_protocol_property_changed_handler property_changed)
    {
        if (validate_validation_fingerprint)
        {
            const auto& validation_fingerprint = nmos::fields::nc::validation_fingerprint(backup_data_set);

            if (!validate_validation_fingerprint(resources, resource, validation_fingerprint.c_str()))
            {
                return nmos::nc::details::make_method_result_error({ nmos::nc_method_status::invalid_request }, U("Invalid validation fingerprint"));
            }
        }
        const auto& object_properties_holders = nmos::fields::nc::values(backup_data_set);

        const auto object_properties_set_validation = apply_backup_data_set(resources, resource, object_properties_holders, recurse, restore_mode, false, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        return nmos::nc::details::make_method_result({ nmos::nc_method_status::ok }, object_properties_set_validation);
    }
}
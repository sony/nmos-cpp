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
        web::json::array make_property_holders(const nmos::resource& resource, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
        {
            using web::json::value;

            value property_holders = value::array();

            nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data));

            // make NcPropertyHolder objects
            while (!class_id.empty())
            {
                const auto& control_class_descriptor = get_control_protocol_class_descriptor(class_id);

                for (const auto& property_descriptor : control_class_descriptor.property_descriptors.as_array())
                {
                    value property_holder = nmos::details::make_nc_property_holder(nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_descriptor)), property_descriptor, resource.data.at(nmos::fields::nc::name(property_descriptor)));

                    web::json::push_back(property_holders, property_holder);
                }
                class_id.pop_back();
            }
            return property_holders.as_array();
        }

        void populate_object_property_holder(const nmos::resources& resources, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const nmos::resource& resource, bool recurse, web::json::value& object_properties_holders)
        {
            using web::json::value;

            // Get property_holders for this resource
            const auto& property_holders = make_property_holders(resource, get_control_protocol_class_descriptor);

            const auto role_path = get_role_path(resources, resource);

            const auto& dependency_paths = nmos::fields::nc::dependency_paths(resource.data);
            const auto& allowed_member_classes = nmos::fields::nc::allowed_members_classes(resource.data);

            auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_holders, dependency_paths, allowed_member_classes, nmos::fields::nc::is_rebuildable(resource.data));

            web::json::push_back(object_properties_holders, object_properties_holder);

            // Recurse into members...if we want to...and the object has them
            if (recurse && nmos::nc::is_block(nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data))))
            {
                if (resource.data.has_field(nmos::fields::nc::members))
                {
                    const auto& members = nmos::fields::nc::members(resource.data);

                    for (const auto& member : members)
                    {
                        const auto& found = find_resource(resources, utility::s2us(std::to_string(nmos::fields::nc::oid(member))));

                        if (resources.end() != found)
                        {
                            populate_object_property_holder(resources, get_control_protocol_class_descriptor, *found, recurse, object_properties_holders);
                        }
                    }
                }
            }
        }

        std::size_t generate_validation_fingerprint(const nmos::resources& resources, const nmos::resource& resource)
        {
            // Generate a hash based on structure of the Device Model
            size_t hash(0);

            nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data));

            boost::hash_combine(hash, class_id);
            boost::hash_combine(hash, nmos::fields::nc::role(resource.data));

            // Recurse into members...if we want to...and the object has them
            if (nmos::nc::is_block(nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data))))
            {
                if (resource.data.has_field(nmos::fields::nc::members))
                {
                    const auto& members = nmos::fields::nc::members(resource.data);

                    // Generate hash for block members
                    for (const auto& member : members)
                    {
                        const auto& oid = nmos::fields::nc::oid(member);
                        const auto& found = find_resource(resources, utility::s2us(std::to_string(oid)));
                        if (resources.end() != found)
                        {
                            size_t sub_hash = generate_validation_fingerprint(resources, *found);
                            boost::hash_combine(hash, sub_hash);
                        }
                    }
                }
            }

            return hash;
        }
    }

    web::json::value get_properties_by_path(const nmos::resources& resources, const nmos::resource& resource, bool recurse, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor)
    {
        using web::json::value;
        using web::json::value_of;

        value object_properties_holders = value::array();

        details::populate_object_property_holder(resources, get_control_protocol_class_descriptor, resource, recurse, object_properties_holders);

        size_t validation_fingerprint = details::generate_validation_fingerprint(resources, resource);

        utility::ostringstream_t ss;
        ss << validation_fingerprint;

        auto bulk_properties_holder = nmos::details::make_nc_bulk_properties_holder(ss.str(), object_properties_holders);

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok }, bulk_properties_holder);
    }

    web::json::value validate_set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_holders_handler filter_property_holders, nmos::remove_device_model_object_handler remove_device_model_object, nmos::add_device_model_object_handler add_device_model_object)
    {
        // Do something with validation fingerprint?
        const auto& object_properties_holders = nmos::fields::nc::values(backup_data_set);

        const auto& object_properties_set_validation = apply_backup_data_set(resources, resource, object_properties_holders, recurse, restore_mode, true, get_control_protocol_class_descriptor, filter_property_holders, remove_device_model_object, add_device_model_object);

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok }, object_properties_set_validation);
    }

    web::json::value set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_holders_handler filter_property_holders, nmos::remove_device_model_object_handler remove_device_model_object, nmos::add_device_model_object_handler add_device_model_object)
    {
        // Do something with validation fingerprint?
        const auto& object_properties_holders = nmos::fields::nc::values(backup_data_set);

        const auto& object_properties_set_validation = apply_backup_data_set(resources, resource, object_properties_holders, recurse, restore_mode, false, get_control_protocol_class_descriptor, filter_property_holders, remove_device_model_object, add_device_model_object);

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok }, object_properties_set_validation);
    }
}
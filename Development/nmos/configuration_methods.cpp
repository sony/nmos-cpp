#include "nmos/configuration_methods.h"

#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/configuration_utils.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace details
    {
        web::json::value make_property_value_holders(const nmos::resource& resource, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
        {
            using web::json::value;

            value property_value_holders = value::array();

            nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data));

            // make NcPropertyValueHolder objects
            while (!class_id.empty())
            {
                const auto& control_class_descriptor = get_control_protocol_class_descriptor(class_id);

                for (const auto& property_descriptor : control_class_descriptor.property_descriptors.as_array())
                {
                    value property_value_holder = nmos::details::make_nc_property_value_holder(nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_descriptor)), nmos::fields::nc::name(property_descriptor), nmos::fields::nc::type_name(property_descriptor), nmos::fields::nc::is_read_only(property_descriptor), resource.data.at(nmos::fields::nc::name(property_descriptor)));

                    web::json::push_back(property_value_holders, property_value_holder);
                }
                class_id.pop_back();
            }
            return property_value_holders;
        }

        void populate_object_property_holder(const nmos::resources& resources, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const nmos::resource& resource, bool recurse, web::json::value& object_properties_holders)
        {
            using web::json::value;

            // Get property_value_holders for this resource
            const value property_value_holders = make_property_value_holders(resource, get_control_protocol_class_descriptor);

            const auto role_path = get_role_path(resources, resource);

            auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, nmos::fields::nc::is_rebuildable(resource.data));

            web::json::push_back(object_properties_holders, object_properties_holder);

            // Recurse into members...if we want to...and the object has them
            if (recurse && nmos::is_nc_block(nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data))))
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

            return;
        }

        std::size_t generate_validation_fingerprint(const nmos::resources& resources, const nmos::resource& resource)
        {
            // Generate a hash based on structure of the Device Model
            size_t hash(0);

            nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data));

            boost::hash_combine(hash, class_id);
            boost::hash_combine(hash, nmos::fields::nc::role(resource.data));

            // Recurse into members...if we want to...and the object has them
            if (nmos::is_nc_block(nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data))))
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

        auto bulk_values_holder = nmos::details::make_nc_bulk_values_holder(ss.str(), object_properties_holders);

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok }, bulk_values_holder);
    }

    web::json::value validate_set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_value_holders_handler filter_property_value_holders, nmos::modify_rebuildable_block_handler modify_rebuildable_block)
    {
        // Do something with validation fingerprint?
        const auto& object_properties_holders = nmos::fields::nc::values(backup_data_set);

        const auto& object_properties_set_validation = apply_backup_data_set(resources, resource, object_properties_holders, recurse, restore_mode, true, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok }, object_properties_set_validation);
    }

    web::json::value set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_value_holders_handler filter_property_value_holders, nmos::modify_rebuildable_block_handler modify_rebuildable_block)
    {
        // Do something with validation fingerprint?
        const auto& object_properties_holders = nmos::fields::nc::values(backup_data_set);

        const auto& object_properties_set_validation = apply_backup_data_set(resources, resource, object_properties_holders, recurse, restore_mode, false, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok }, object_properties_set_validation);
    }

}
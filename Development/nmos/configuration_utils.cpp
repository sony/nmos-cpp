#include "nmos/configuration_methods.h"

#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/configuration_resources.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace details
    {
        bool is_property_value_valid(web::json::value& property_restore_notices, const web::json::value& property_value, const web::json::value& property_descriptor, const web::json::value& restore_mode, bool is_rebuildable)
        {
            const nmos::nc_property_id& property_id = nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_descriptor));
            bool is_valid = true;
            // Check the name of the property is correct
            if (nmos::fields::nc::name(property_descriptor) != nmos::fields::nc::name(property_value))
            {
                utility::ostringstream_t os;
                os << U("unexpected property name: expected ") << nmos::fields::nc::name(property_descriptor) << U(", actual ") << nmos::fields::nc::name(property_value);
                const auto& property_restore_notice = nmos::details::make_nc_property_restore_notice(property_id, nmos::fields::nc::name(property_value), nmos::nc_property_restore_notice_type::error, os.str());
                web::json::push_back(property_restore_notices, property_restore_notice);
                is_valid = false;
            }
            // Check the type of the property value is correct
            if (nmos::fields::nc::type_name(property_descriptor) != nmos::fields::nc::type_name(property_value))
            {
                utility::ostringstream_t os;
                os << U("unexpected property type: expected ") << nmos::fields::nc::type_name(property_descriptor) << U(", actual ") << nmos::fields::nc::type_name(property_value);
                const auto& property_restore_notice = nmos::details::make_nc_property_restore_notice(property_id, nmos::fields::nc::name(property_value), nmos::nc_property_restore_notice_type::error, os.str());
                web::json::push_back(property_restore_notices, property_restore_notice);
                is_valid = false;
            }
            // Only allow modification of read only properties when in Rebuild mode
            if (bool(nmos::fields::nc::is_read_only(property_descriptor))
                && restore_mode != nmos::nc_restore_mode::restore_mode::rebuild)
            {
                const auto& property_restore_notice = nmos::details::make_nc_property_restore_notice(property_id, nmos::fields::nc::name(property_value), nmos::nc_property_restore_notice_type::error, U("read only properties can not be modified in Modify restore mode."));
                web::json::push_back(property_restore_notices, property_restore_notice);
                is_valid = false;
            }
            // Only allow modification of read only properties when object is rebuildable
            if (bool(nmos::fields::nc::is_read_only(property_descriptor))
                && !is_rebuildable)
            {
                const auto& property_restore_notice = nmos::details::make_nc_property_restore_notice(property_id, nmos::fields::nc::name(property_value), nmos::nc_property_restore_notice_type::error, U("object must be rebuildable to allow modification of read only properties."));
                web::json::push_back(property_restore_notices, property_restore_notice);
                is_valid = false;
            }

            return is_valid;
        }

        bool is_contains_read_only_property(const web::json::array& property_values, const nmos::nc_class_id& class_id, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
        {
            for (const auto& property_value : property_values)
            {
                const auto& property_id = nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_value));
                const auto& property_descriptor = nmos::nc::find_property_descriptor(property_id, class_id, get_control_protocol_class_descriptor);

                if (bool(nmos::fields::nc::is_read_only(property_descriptor)))
                {
                    return true;
                }
            }
            return false;
        }
    }

    // Check to see if root_role_path is root of role_path
    bool is_role_path_root(const web::json::array& role_path_root, const web::json::array& role_path)
    {
        if (role_path_root.size() > role_path.size())
        {
            // root can't be longed that the path
            return false;
        }
        for (size_t i = 0; i < role_path_root.size(); ++i)
        {
            if (role_path_root.at(i) != role_path.at(i))
            {
                return false;
            }
        }
        return true;
    }

    bool is_block_modified(const nmos::resource& resource, const web::json::value& object_properties_holder)
    {
        // Are they blocks?
        nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data));
        if (!nmos::nc::is_block(class_id))
        {
            return false;
        }
        const auto& block_members_properties_holders = boost::copy_range<std::set<web::json::value>>(nmos::fields::nc::values(object_properties_holder)
            | boost::adaptors::filtered([](const web::json::value& property_value_holder)
                {
                    return nmos::nc_property_id(2, 2) == nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_value_holder));
                })
        );
        // There should only be a single property holder for the members
        if (block_members_properties_holders.size() == 1)
        {
            const auto& members_property_holder = *block_members_properties_holders.begin();
            const auto& restore_members = nmos::fields::nc::value(members_property_holder);
            const auto& reference_members = nmos::fields::nc::members(resource.data);

            if (reference_members.size() != restore_members.as_array().size())
            {
                return true;
            }
            for (const auto& reference_member : reference_members)
            {
                const auto& filtered_members = boost::copy_range<std::set<web::json::value>>(restore_members.as_array()
                    | boost::adaptors::filtered([&reference_member](const web::json::value& member)
                        {
                            return nmos::fields::nc::oid(reference_member) == nmos::fields::nc::oid(member);
                        })
                );
                if (filtered_members.size() != 1)
                {
                    // can't find this oid, so member has been removed
                    return true;
                }
                const auto& restore_member = *filtered_members.begin();
                // We ignore the description and user label as these are non-normative
                if (nmos::fields::nc::role(reference_member) != nmos::fields::nc::role(restore_member)
                    || nmos::fields::nc::constant_oid(reference_member) != nmos::fields::nc::constant_oid(restore_member)
                    || nmos::fields::nc::class_id(reference_member) != nmos::fields::nc::class_id(restore_member)
                    || nmos::fields::nc::owner(reference_member) != nmos::fields::nc::owner(restore_member))
                {
                    return true;
                }
            }
        }

        return false;
    }

    web::json::array get_object_properties_holder(const web::json::array& object_properties_holders, const web::json::array& target_role_path)
    {
        const auto& target_object_properties_holders = boost::copy_range<std::set<web::json::value>>(object_properties_holders
            | boost::adaptors::filtered([&target_role_path](const web::json::value& object_properties_holder)
                {
                    return target_role_path == nmos::fields::nc::path(object_properties_holder);
                })
        );
        return web::json::value_from_elements(target_object_properties_holders).as_array();
    }

    web::json::array get_child_object_properties_holders(const web::json::array& object_properties_holders, const web::json::array& target_role_path)
    {
        const auto& child_object_properties_holders = boost::copy_range<std::set<web::json::value>>(object_properties_holders
            | boost::adaptors::filtered([&target_role_path](const web::json::value& object_properties_holder)
                {
                    return is_role_path_root(target_role_path, nmos::fields::nc::path(object_properties_holder));
                })
        );
        return web::json::value_from_elements(child_object_properties_holders).as_array();
    }

    web::json::value modify_device_model(nmos::resources& resources, const nmos::resource& resource, const web::json::array& target_role_path, const web::json::array& object_properties_holders, bool recurse, const web::json::value& restore_mode, bool validate, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_value_holders_handler filter_property_value_holders, nmos::modify_rebuildable_block_handler modify_rebuildable_block)
    {
        auto object_properties_set_validation_values = web::json::value::array();

        // Filter for the target_role_path and child objects
        //
        // hmmmmm, I don't like this two step filter process - creating a boost array and then converting to a json array.
        // Could this be done in a single step?
        const auto& child_object_properties_holders = get_child_object_properties_holders(object_properties_holders, target_role_path);

        // get object_properties_holder for the target role path, if there is one
        const auto& target_object_properties_holders = get_object_properties_holder(object_properties_holders, target_role_path);

        // there should be 0 or 1 object_properties_holder for any role path.
        if (target_object_properties_holders.size() > 1)
        {
            // Error in the backup dataset
            const auto& object_properties_set_validation = nmos::make_object_properties_set_validation(nmos::fields::nc::path(*target_object_properties_holders.begin()), nmos::nc_restore_validation_status::failed, U("more than one object_properties_holder for role path"));
            web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
            return object_properties_set_validation_values;
        }

        const auto& class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data));

        if (nmos::nc::is_block(class_id))
        {
            // if rebuildable and the block has changed then callback
            if (nmos::fields::nc::is_rebuildable(resource.data) && target_object_properties_holders.size() && is_block_modified(resource, *target_object_properties_holders.begin()))
            {
                if (modify_rebuildable_block)
                {
                    // call back to application code which will return an object_properties_set_validation_values object
                    return modify_rebuildable_block(resource, target_role_path, child_object_properties_holders, recurse, validate, get_control_protocol_class_descriptor);
                }
                else
                {
                    // Rebuilding blocks not supported
                    const auto& object_properties_set_validation = nmos::make_object_properties_set_validation(target_role_path, nmos::nc_restore_validation_status::failed, U("Rebuilding of Device Model blocks not supported"));
                    web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
                    return object_properties_set_validation_values;
                }
            }
            // iterate through child objects
            if (resource.data.has_field(nmos::fields::nc::members))
            {
                const auto& members = nmos::fields::nc::members(resource.data);

                for (const auto& member : members)
                {
                    const auto& child = find_resource(resources, utility::s2us(std::to_string(nmos::fields::nc::oid(member))));

                    if (resources.end() != child)
                    {
                        // Append the role of the child to the target role path to create the child role path
                        auto child_role_path = web::json::value_from_elements(target_role_path);
                        web::json::push_back(child_role_path, nmos::fields::nc::role(child->data));

                        web::json::value child_object_properties_set_validation_values = modify_device_model(resources, *child, child_role_path.as_array(), child_object_properties_holders, recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);
                        // Hmmm, there must be a better way of merging two json array objects
                        for (const auto& validation_values : child_object_properties_set_validation_values.as_array())
                        {
                            web::json::push_back(object_properties_set_validation_values, validation_values);
                        }
                    }
                }
            }
        }
        for (const auto& target_object_properties_holder : target_object_properties_holders)
        {
            auto property_restore_notices = web::json::value::array();
            // Validate property_values - filter out the incorrect, ignored or unallowed values
            const auto& filtered_property_values = boost::copy_range<std::set<web::json::value>>(nmos::fields::nc::values(target_object_properties_holder)
                | boost::adaptors::filtered([&property_restore_notices, &resource, class_id, get_control_protocol_class_descriptor, restore_mode](const web::json::value& property_value)
                    {
                        const auto& property_id = nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_value));
                        const auto& property_descriptor = nmos::nc::find_property_descriptor(property_id, class_id, get_control_protocol_class_descriptor);

                        return resource.data.at(nmos::fields::nc::name(property_descriptor)) != nmos::fields::nc::value(property_value)
                            && details::is_property_value_valid(property_restore_notices, property_value, property_descriptor, restore_mode, bool(nmos::fields::nc::is_rebuildable(resource.data)));
                    })
            );
            auto property_modify_list = web::json::value_from_elements(filtered_property_values).as_array();

            if (details::is_contains_read_only_property(property_modify_list, class_id, get_control_protocol_class_descriptor))
            {
                if (filter_property_value_holders)
                {
                    // If the property_modify_list contains read only properties then we call back to the application code to
                    // check that it's OK to change those value.  Bear in mind that they could be the class Id, or the oid or some other
                    // property that we don't want changed
                    property_modify_list = filter_property_value_holders(resource, target_role_path, property_modify_list, recurse, validate, property_restore_notices.as_array(), get_control_protocol_class_descriptor);
                }
                else
                {
                    // Modify of read only properties not supported
                    const auto& object_properties_set_validation = nmos::make_object_properties_set_validation(target_role_path, nmos::nc_restore_validation_status::failed, property_restore_notices.as_array(), U("Modification of read only properties not supported"));
                    web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
                    continue;
                }
            }
            for (const auto& property_value : property_modify_list)
            {
                const auto& property_id = nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_value));

                // hmmm, ideally we would pass the value into modify_resource with the validate
                // flag, so that it's subject to property contraints and also the application code can decide if it's a legal value
                if (!validate)
                {
                    // modify control protocol resources
                    const auto& value = nmos::fields::nc::value(property_value);

                    nc::modify_resource(resources, resource.id, [&](nmos::resource& resource_)
                        {
                            resource_.data[nmos::fields::nc::name(property_value)] = value;

                        }, nmos::make_property_changed_event(nmos::fields::nc::oid(resource.data), { {property_id, nmos::nc_property_change_type::type::value_changed, value} }));
                }
            }
            const auto& object_properties_set_validation = nmos::make_object_properties_set_validation(target_role_path, nmos::nc_restore_validation_status::ok, property_restore_notices.as_array(), U("OK"));
            web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
        }

        return object_properties_set_validation_values;
    }

    web::json::array get_role_path(const nmos::resources& resources, const nmos::resource& resource)
    {
        // Find role path for object
        // Hmmm do we not have a library function to do this?
        using web::json::value;

        auto role_path = value::array();
        web::json::push_back(role_path, nmos::fields::nc::role(resource.data));

        auto oid = nmos::fields::nc::id(resource.data);
        nmos::resource found_resource = resource;

        while (utility::s2us(std::to_string(nmos::root_block_oid)) != oid.as_string())
        {
            const auto& found = nmos::find_resource(resources, utility::s2us(std::to_string(nmos::fields::nc::owner(found_resource.data))));
            if (resources.end() == found)
            {
                break;
            }

            found_resource = (*found);
            web::json::push_back(role_path, nmos::fields::nc::role(found_resource.data));
            oid = nmos::fields::nc::id(found_resource.data);
        }

        std::reverse(role_path.as_array().begin(), role_path.as_array().end());

        return role_path.as_array();
    }

    web::json::value apply_backup_data_set(nmos::resources& resources, const nmos::resource& resource, const web::json::array& object_properties_holders, bool recurse, const web::json::value& restore_mode, bool validate, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_value_holders_handler filter_property_value_holders, nmos::modify_rebuildable_block_handler modify_rebuildable_block)
    {
        auto object_properties_set_validation_values = web::json::value::array();

        const auto target_role_path = get_role_path(resources, resource);

        // Detect and warn if there are any object_properties_holders outside of the target role path's scope
        // Hmmm, can this be done as a one step process rather than filtering and then iterating over filtered list?
        const auto& orphan_object_properties_holders = boost::copy_range<std::set<web::json::value>>(object_properties_holders
            | boost::adaptors::filtered([&target_role_path](const web::json::value& object_properties_holder)
                {
                    return !nmos::is_role_path_root(target_role_path, nmos::fields::nc::path(object_properties_holder));
                })
        );
        for (const auto& orphan_object_properties_holder : orphan_object_properties_holders)
        {
            const auto& object_properties_set_validation = nmos::make_object_properties_set_validation(nmos::fields::nc::path(orphan_object_properties_holder), nmos::nc_restore_validation_status::not_found, U("object role path not found under target role path"));
            web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
        }

        web::json::value child_object_properties_set_validation_values = modify_device_model(resources, resource, target_role_path, object_properties_holders, recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);
        // Hmmm - there must be a better way to append an array
        for (const auto& validation_values : child_object_properties_set_validation_values.as_array())
        {
            web::json::push_back(object_properties_set_validation_values, validation_values);
        }

        return object_properties_set_validation_values;
    }

    web::json::value get_property_value_holder(const web::json::value& object_properties_holder, const nmos::nc_property_id& property_id)
    {
        const auto& filtered_property_value_holders = boost::copy_range<std::set<web::json::value>>(nmos::fields::nc::values(object_properties_holder)
            | boost::adaptors::filtered([&property_id](const web::json::value& property_value_holder)
                {
                    return property_id == nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_value_holder));
                })
        );
        // There should only be a single property holder for the members
        if (filtered_property_value_holders.size() != 1)
        {
            // Error
            return web::json::value::null();
        }

        return *filtered_property_value_holders.begin();
    }
}

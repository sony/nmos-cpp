#include "nmos/configuration_methods.h"

#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_utils.h"
#include "nmos/configuration_handlers.h"
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

    web::json::value get_role_path(const nmos::resources& resources, const nmos::resource& resource)
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

        return role_path;
    }

    // Check to see if root_role_path is root of role_path_
    bool is_role_path_root(const web::json::value& role_path_root, const web::json::value& role_path_)
    {
        if (role_path_root.as_array().size() > role_path_.as_array().size())
        {
            // root can't be longed that the path
            return false;
        }
        for (size_t i = 0; i < role_path_root.as_array().size(); ++i)
        {
            if (role_path_root.as_array().at(i) != role_path_.as_array().at(i))
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
        if (!nmos::is_nc_block(class_id))
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
                const auto restore_member = *filtered_members.begin();
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

    web::json::value get_properties_by_path(const nmos::resources& resources, nmos::experimental::control_protocol_state& control_protocol_state, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, const nmos::resource& resource, bool recurse)
    {
        using web::json::value;
        using web::json::value_of;

        auto lock = control_protocol_state.read_lock();

        value object_properties_holders = value::array();

        details::populate_object_property_holder(resources, get_control_protocol_class_descriptor, resource, recurse, object_properties_holders);

        size_t validation_fingerprint = details::generate_validation_fingerprint(resources, resource);

        utility::ostringstream_t ss;
        ss << validation_fingerprint;

        auto bulk_values_holder = nmos::details::make_nc_bulk_values_holder(ss.str(), object_properties_holders);

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok }, bulk_values_holder);
    }

    web::json::value check_property_value(const web::json::value& property_value, const web::json::value& property_descriptor, const web::json::value& restore_mode)
    {
        const nmos::nc_property_id& property_id = nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_descriptor));

        auto property_restore_notices = web::json::value::array();

        // Check the name of the property is correct
        if (nmos::fields::nc::name(property_descriptor) != nmos::fields::nc::name(property_value))
        {
            utility::ostringstream_t os;
            os << U("unexpected property name: expected ") << nmos::fields::nc::name(property_descriptor) << U(", actual ") << nmos::fields::nc::name(property_value);
            const auto& property_restore_notice = nmos::details::make_nc_property_restore_notice(property_id, nmos::fields::nc::name(property_value), nmos::nc_property_restore_notice_type::error, os.str());
            web::json::push_back(property_restore_notices, property_restore_notice);
        }
        // Check the type of the property value is correct
        if (nmos::fields::nc::type_name(property_descriptor) != nmos::fields::nc::type_name(property_value))
        {
            utility::ostringstream_t os;
            os << U("unexpected property type: expected ") << nmos::fields::nc::type_name(property_descriptor) << U(", actual ") << nmos::fields::nc::type_name(property_value);
            const auto& property_restore_notice = nmos::details::make_nc_property_restore_notice(property_id, nmos::fields::nc::name(property_value), nmos::nc_property_restore_notice_type::error, os.str());
            web::json::push_back(property_restore_notices, property_restore_notice);
        }
        // Only allow modification of read only properties when in Rebuild mode
        if (bool(nmos::fields::nc::is_read_only(property_descriptor)) && restore_mode != nmos::nc_restore_mode::restore_mode::rebuild)
        {
            const auto& property_restore_notice = nmos::details::make_nc_property_restore_notice(property_id, nmos::fields::nc::name(property_value), nmos::nc_property_restore_notice_type::error, U("read only properties can not be modified in Modify restore mode."));
            web::json::push_back(property_restore_notices, property_restore_notice);
        }
        return property_restore_notices;
    }

    web::json::value modify_device_model(nmos::resources& resources, nmos::experimental::control_protocol_state& control_protocol_state, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const web::json::value& target_role_path, const web::json::array& object_properties_holders, bool recurse, const web::json::value& restore_mode, bool validate, nmos::modify_read_only_config_properties_handler modify_read_only_config_properties, nmos::modify_rebuildable_block_handler modify_rebuildable_block)
    {
        auto object_properties_set_validation_values = web::json::value::array();

        // filter for the target_role_path and child objects
        const auto& filtered_object_properties_holders = boost::copy_range<std::set<web::json::value>>(object_properties_holders
            | boost::adaptors::filtered([&target_role_path](const web::json::value& object_properties_holder)
                {
                    return is_role_path_root(target_role_path, nmos::fields::nc::path(object_properties_holder));
                })
        );
        web::json::value child_object_properties_holders = web::json::value::array();
        for (const auto& filtered_holder : filtered_object_properties_holders)
        {
            web::json::push_back(child_object_properties_holders, filtered_holder);
        }

        // get object_properties_holder for the target role path, if there is one
        const auto& target_object_properties_holders = boost::copy_range<std::set<web::json::value>>(object_properties_holders
            | boost::adaptors::filtered([&target_role_path](const web::json::value& object_properties_holder)
                {
                    return target_role_path == nmos::fields::nc::path(object_properties_holder);
                })
        );
        // there should be 0 or 1 object_properties_holder for any role path.
        if (target_object_properties_holders.size() > 1)
        {
            const auto& object_properties_set_validation = nmos::details::make_nc_object_properties_set_validation(nmos::fields::nc::path(*target_object_properties_holders.begin()), nmos::nc_restore_validation_status::failed, web::json::value::array(), U("more than one object_properties_holder for role path"));
            web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
            return object_properties_set_validation_values;
        }

        const auto& found = nmos::find_control_protocol_resource_by_role_path(resources, target_role_path);

        if (resources.end() != found)
        {
            nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(found->data));

            if (nmos::is_nc_block(class_id))
            {
                // if rebuildable and the block has changed then callback
                if (nmos::fields::nc::is_rebuildable(found->data) && target_object_properties_holders.size() && is_block_modified(*found, *target_object_properties_holders.begin()))
                {
                    // call back to application code
                    return modify_rebuildable_block(control_protocol_state, get_control_protocol_class_descriptor, target_role_path, child_object_properties_holders, recurse, restore_mode, validate);
                }
                // iterate through child objects
                if (found->data.has_field(nmos::fields::nc::members))
                {
                    const auto& members = nmos::fields::nc::members(found->data);

                    for (const auto& member : members)
                    {
                        const auto& child = find_resource(resources, utility::s2us(std::to_string(nmos::fields::nc::oid(member))));

                        if (resources.end() != child)
                        {
                            auto child_role_path = web::json::value::array();
                            for (const auto& path_element : target_role_path.as_array())
                            {
                                web::json::push_back(child_role_path, path_element);
                            }
                            web::json::push_back(child_role_path, nmos::fields::nc::role(child->data));

                            web::json::value child_object_properties_set_validation_values = modify_device_model(resources, control_protocol_state, get_control_protocol_class_descriptor, child_role_path, child_object_properties_holders.as_array(), recurse, restore_mode, validate, modify_read_only_config_properties, modify_rebuildable_block);
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
                auto property_modify_list = web::json::value::array();
                unsigned int rebuildable_property_count = 0;
                // Validate property_values - filter out the incorrect, ignored or unallowed
                for (const auto& property_value : nmos::fields::nc::values(target_object_properties_holder))
                {
                    const auto& property_id = nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_value));
                    const auto& property_descriptor = nmos::find_property_descriptor(property_id, class_id, get_control_protocol_class_descriptor);
                    const auto& property_restore_notices_ = check_property_value(property_value, property_descriptor, restore_mode);
                    if (property_restore_notices_.size() > 0)
                    {
                        for (const auto& notice : property_restore_notices_.as_array())
                        {
                            web::json::push_back(property_restore_notices, notice);
                        }
                        continue;
                    }
                    // Ignore if no change is being requested
                    if (found->data.at(nmos::fields::nc::name(property_descriptor)) == nmos::fields::nc::value(property_value))
                    {
                        continue;
                    }
                    // Only allow modification of read only properties when in Rebuild mode
                    if (bool(nmos::fields::nc::is_read_only(property_descriptor)) && restore_mode == nmos::nc_restore_mode::restore_mode::rebuild)
                    {
                        rebuildable_property_count++;
                    }
                    web::json::push_back(property_modify_list, property_value);
                }
                if (rebuildable_property_count > 0 && property_modify_list.as_array().size() > 0)
                {
                    // If this is a read only property then we should call back to the application code to 
                    // check that it's OK to change this value.  Bear in mind that this could be a class Id, or an oid or some other
                    // property that we don't want changed
                    const auto& object_properties_set_validation = modify_read_only_config_properties(control_protocol_state, get_control_protocol_class_descriptor, target_role_path, property_modify_list, recurse, restore_mode, validate);
                    // add in already generated property_restore_notices 
                    auto modified_object_properties_set_validation = object_properties_set_validation;
                    auto& notices = nmos::fields::nc::notices(modified_object_properties_set_validation);
                    for (const auto& notice : property_restore_notices.as_array())
                    {
                        web::json::push_back(notices, notice);
                    }

                    web::json::push_back(object_properties_set_validation_values, modified_object_properties_set_validation);
                }
                else
                {
                    for (const auto& property_value : property_modify_list.as_array())
                    {
                        const auto& property_id = nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_value));

                        if (!validate)
                        {
                            // modify control protocol resources
                            const auto& value = nmos::fields::nc::value(property_value);

                            modify_control_protocol_resource(resources, found->id, [&](nmos::resource& resource_)
                                {
                                    resource_.data[nmos::fields::nc::name(property_value)] = value;

                                }, nmos::make_property_changed_event(nmos::fields::nc::oid(found->data), {{property_id, nmos::nc_property_change_type::type::value_changed, value}}));
                        }
                    }
                    const auto& object_properties_set_validation = nmos::details::make_nc_object_properties_set_validation(target_role_path, nmos::nc_restore_validation_status::ok, property_restore_notices, U("OK"));
                    web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
                }
            }
        }
        return object_properties_set_validation_values;
    }

    web::json::value apply_backup_data_set(nmos::resources& resources, nmos::experimental::control_protocol_state& control_protocol_state, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const nmos::resource& resource, const web::json::array& object_properties_holders, bool recurse, const web::json::value& restore_mode, bool validate, nmos::modify_read_only_config_properties_handler modify_read_only_config_properties, nmos::modify_rebuildable_block_handler modify_rebuildable_block)
    {
        auto object_properties_set_validation_values = web::json::value::array();

        const auto target_role_path = get_role_path(resources, resource);

        // Detect and warn if there are any object_properties_holders outside of the target role path's scope
        const auto& orphan_object_properties_holders = boost::copy_range<std::set<web::json::value>>(object_properties_holders
            | boost::adaptors::filtered([&target_role_path](const web::json::value& object_properties_holder)
                {
                    return !nmos::is_role_path_root(target_role_path, nmos::fields::nc::path(object_properties_holder));
                })
        );
        for (const auto& orphan_object_properties_holder : orphan_object_properties_holders)
        {
            const auto& object_properties_set_validation = nmos::details::make_nc_object_properties_set_validation(nmos::fields::nc::path(orphan_object_properties_holder), nmos::nc_restore_validation_status::not_found, web::json::value::array(), U("object role path not found under target role path"));
            web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
        }

        web::json::value child_object_properties_set_validation_values = modify_device_model(resources, control_protocol_state, get_control_protocol_class_descriptor, target_role_path, object_properties_holders, recurse, restore_mode, validate, modify_read_only_config_properties, modify_rebuildable_block);
        for (const auto& validation_values : child_object_properties_set_validation_values.as_array())
        {
            web::json::push_back(object_properties_set_validation_values, validation_values);
        }

        return object_properties_set_validation_values;
    }

    web::json::value validate_set_properties_by_path(nmos::resources& resources, nmos::experimental::control_protocol_state& control_protocol_state, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::modify_read_only_config_properties_handler modify_read_only_config_properties, nmos::modify_rebuildable_block_handler modify_rebuildable_block)
    {
        // Do something with validation fingerprint?
        const auto& object_properties_holders = nmos::fields::nc::values(backup_data_set);

        const auto& object_properties_set_validation = apply_backup_data_set(resources, control_protocol_state, get_control_protocol_class_descriptor, resource, object_properties_holders, recurse, restore_mode, true, modify_read_only_config_properties, modify_rebuildable_block);

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok }, object_properties_set_validation);
    }

    web::json::value set_properties_by_path(nmos::resources& resources, nmos::experimental::control_protocol_state& control_protocol_state, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::modify_read_only_config_properties_handler modify_read_only_config_properties, nmos::modify_rebuildable_block_handler modify_rebuildable_block)
    {
        // Do something with validation fingerprint?
        const auto& object_properties_holders = nmos::fields::nc::values(backup_data_set);

        const auto& object_properties_set_validation = apply_backup_data_set(resources, control_protocol_state, get_control_protocol_class_descriptor, resource, object_properties_holders, recurse, restore_mode, false, modify_read_only_config_properties, modify_rebuildable_block);

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok }, object_properties_set_validation);
    }

}
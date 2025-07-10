#include "nmos/configuration_methods.h"

#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/configuration_resources.h"
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

        web::json::value modify_device_model_object(nmos::resources& resources, const nmos::resource& resource, const web::json::array& target_role_path, const web::json::value& target_object_properties_holder, const web::json::value& restore_mode, bool validate, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_holders_handler filter_property_holders)
        {
            const auto& class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data));

            auto object_properties_set_validation_values = web::json::value::array();

            auto property_restore_notices = web::json::value::array();
            // Validate property_values - filter out the incorrect, ignored or unallowed values
            // notices are created for any properties that can't be modified according to restore mode
            // and whether this object is rebuildable
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
                if (filter_property_holders)
                {
                    // If the property_modify_list contains read only properties then we call back to the application code to
                    // check that it's OK to change those value.  Bear in mind that these could be the class Id, or the oid or some other
                    // property that we don't want changed ordinarily
                    property_modify_list = filter_property_holders(resource, target_role_path, property_modify_list, true, validate, property_restore_notices.as_array(), get_control_protocol_class_descriptor);
                }
                else
                {
                    // Modify of read only properties not supported
                    return nmos::make_object_properties_set_validation(target_role_path, nmos::nc_restore_validation_status::failed, property_restore_notices.as_array(), U("Modification of read only properties not supported"));
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
            return nmos::make_object_properties_set_validation(target_role_path, nmos::nc_restore_validation_status::ok, property_restore_notices.as_array(), U("OK"));
        }

        web::json::value modify_rebuildable_block(nmos::resources& resources, object_properties_map& object_properties_holder_map, const nmos::resource& resource, const web::json::array& target_role_path, const web::json::value& block_object_properties_holder, bool validate, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::remove_device_model_object_handler remove_device_model_object, nmos::add_device_model_object_handler add_device_model_object)
        {
            auto object_properties_set_validations = web::json::value::array();

            // Find object_properties_holder for resource
            // the calling function guarantees that this target role path is in the object_properties_holders
            // and property_holder for block members exists
            const auto& block_members_properties_holder = nmos::get_property_holder(block_object_properties_holder, nmos::nc_block_members_property_id);

            const auto& restore_members = nmos::fields::nc::value(block_members_properties_holder);
            const auto& reference_members = nmos::fields::nc::members(resource.data);

            const auto& block_oid = nmos::fields::nc::oid(resource.data);
            const auto& allowed_member_classes = nmos::fields::nc::allowed_members_classes(resource.data);

            std::vector<int> members_to_remove;
            std::vector<web::json::value> members_to_add;

            // If there are any data problems they should be reported as warning/error notices
            auto block_notices = web::json::value::array();

            // Iterate through the members of the block and compare to the members in the backup dataset
            for (const auto& reference_member : reference_members)
            {
                auto child_role_path = web::json::value_from_elements(target_role_path);
                web::json::push_back(child_role_path, nmos::fields::nc::role(reference_member));

                const auto& filtered_members = boost::copy_range<std::set<web::json::value>>(restore_members.as_array()
                    | boost::adaptors::filtered([&reference_member](const web::json::value& member)
                        {
                            return nmos::fields::nc::role(reference_member) == nmos::fields::nc::role(member);
                        })
                );
                if (filtered_members.size() != 1)
                {
                    // can't find this role in restore dataset, so member has been removed
                    bool success = remove_device_model_object(nmos::fields::nc::oid(reference_member), validate);
                    if (success)
                    {
                        if (!validate)
                        {
                            members_to_remove.push_back(nmos::fields::nc::oid(reference_member));
                        }
                    }
                    else
                    {
                        // unable to delete resource so report the error and don't update block
                        auto notices = web::json::value::array();
                        const auto notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("members"), nmos::nc_property_restore_notice_type::error, U("Unable to delete resource from Device Model."));
                        web::json::push_back(block_notices, notice);
                    }
                }
            }

            for (const auto& restore_member : restore_members.as_array())
            {
                auto child_role_path = web::json::value_from_elements(target_role_path);
                web::json::push_back(child_role_path, nmos::fields::nc::role(restore_member));

                const auto& filtered_members = boost::copy_range<std::set<web::json::value>>(reference_members
                    | boost::adaptors::filtered([&restore_member](const web::json::value& member)
                        {
                            return nmos::fields::nc::role(restore_member) == nmos::fields::nc::role(member);
                        })
                );
                if (filtered_members.size() != 1)
                {
                    // can't find this role in existing members, so member need to be added
                    // Find the object_properties_holder that describes the receiver monitor object
                    if (object_properties_holder_map.find(child_role_path.as_array()) == object_properties_holder_map.end())
                    {
                        auto status_message = U("Cannot find NcObjectPropertiesHolder for new resource");
                        auto object_properties_set_validation = nmos::make_object_properties_set_validation(child_role_path.as_array(), nmos::nc_restore_validation_status::failed, status_message);
                        web::json::push_back(object_properties_set_validations, object_properties_set_validation);

                        continue;
                    }

                    const auto& child_object_properties_holder = object_properties_holder_map.find(child_role_path.as_array());

                    // Get member descriptor properties
                    auto role = nmos::fields::nc::role(restore_member);
                    auto owner = nmos::fields::nc::owner(restore_member);
                    auto constant_oid = nmos::fields::nc::constant_oid(restore_member);
                    auto oid = nmos::fields::nc::oid(restore_member);
                    const auto& block_member_description = nmos::fields::nc::description(restore_member);
                    const auto& block_member_user_label = nmos::fields::nc::user_label(restore_member);

                    auto added_object_notices = web::json::value::array();

                    const auto& oid_property_holder = nmos::get_property_holder(child_object_properties_holder->second, nmos::nc_object_oid_property_id);
                    oid = oid_property_holder == web::json::value::null() ? oid : nmos::fields::nc::value(oid_property_holder).as_integer();

                    if (oid_property_holder != web::json::value::null() && oid != nmos::fields::nc::value(oid_property_holder).as_integer())
                    {
                        const auto notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("members"), nmos::nc_property_restore_notice_type::warning, U("OID value in block member inconsistent with value in property holder. Property holder value takes precidence."));
                        web::json::push_back(block_notices, notice);
                    }

                    if (constant_oid)
                    {
                        // If constant oid then check oid from property holder and verify with oid from block member
                        // Is this oid already in use?
                        const auto& child = find_resource(resources, utility::s2us(std::to_string(oid)));

                        if (resources.end() != child)
                        {
                            // oid already in use!
                            // create device error for new object
                            auto status_message = U("Constant OID error. OID already in use");
                            auto object_properties_set_validation = nmos::make_object_properties_set_validation(child_role_path.as_array(), nmos::nc_restore_validation_status::device_error, status_message);
                            web::json::push_back(object_properties_set_validations, object_properties_set_validation);
                            // also create error notice for the block
                            const auto block_notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("members"), nmos::nc_property_restore_notice_type::error, status_message);
                            web::json::push_back(block_notices, block_notice);
                            // erase object from object_properties_holder_map so it isn't processed subsequently
                            object_properties_holder_map.erase(child_role_path.as_array());
                            continue;
                        }
                    }
                    else
                    {
                        // Ignore specified OID and generate new one
                        int max_oid = -1;
                        for (const auto& r: resources)
                        {
                            max_oid = std::max(max_oid, nmos::fields::nc::oid(r.data));
                        }
                        oid = ++max_oid;
                        const auto block_notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("members"), nmos::nc_property_restore_notice_type::warning, U("Dynamically generating new OID for new block member."));
                        web::json::push_back(block_notices, block_notice);

                        const auto added_object_notice = nmos::details::make_nc_property_restore_notice(nmos::nc_object_oid_property_id, U("oid"), nmos::nc_property_restore_notice_type::warning, U("Dynamically generating new OID for new object."));
                        web::json::push_back(added_object_notices, added_object_notice);
                    }

                    // The values in the block member object properties holder will take precidence over the block member descriptor values
                    // If the block member values are inconsistant then warn - description and user label values can be independant
                    const auto& role_property_holder = nmos::get_property_holder(child_object_properties_holder->second, nmos::nc_object_role_property_id);
                    role = role_property_holder == web::json::value::null() ? role : nmos::fields::nc::value(role_property_holder).as_string();

                    if (role_property_holder != web::json::value::null() && role != nmos::fields::nc::value(role_property_holder).as_string())
                    {
                        utility::stringstream_t ss;
                        ss << U("Role value in block member inconsistent with value in property holder. Property holder value takes precidence for role=") << role;
                        const auto notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("members"), nmos::nc_property_restore_notice_type::warning, ss.str());
                        web::json::push_back(block_notices, notice);
                    }
                    if (owner != block_oid)
                    {
                        utility::stringstream_t ss;
                        ss << U("Owner value in block member inconsistent with oid of Block. Block oid takes precidence for role=") << role;
                        const auto notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("members"), nmos::nc_property_restore_notice_type::warning, ss.str());
                        web::json::push_back(block_notices, notice);
                    }
                    const auto& owner_property_holder = nmos::get_property_holder(child_object_properties_holder->second, nmos::nc_object_owner_property_id);
                    if (owner_property_holder != web::json::value::null() && block_oid != nmos::fields::nc::value(owner_property_holder).as_integer())
                    {
                        utility::stringstream_t ss;
                        ss << U("Owner value in block property holder inconsistent with oid of Block. Block oid takes precidence for role=") << role;
                        const auto notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("owner"), nmos::nc_property_restore_notice_type::warning, ss.str());
                        web::json::push_back(added_object_notices, notice);
                    }
                    const auto& constant_oid_property_holder = nmos::get_property_holder(child_object_properties_holder->second, nmos::nc_object_constant_oid_property_id);
                    constant_oid = constant_oid_property_holder == web::json::value::null() ? constant_oid : nmos::fields::nc::value(constant_oid_property_holder).as_bool();

                    if (constant_oid_property_holder != web::json::value::null() && constant_oid != nmos::fields::nc::value(constant_oid_property_holder).as_bool())
                    {
                        utility::stringstream_t ss;
                        ss << U("Constant OID value in block property holder inconsistent with oid of Block. Block oid takes precidence for role=") << role;
                        const auto notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("members"), nmos::nc_property_restore_notice_type::warning, ss.str());
                        web::json::push_back(block_notices, notice);
                    }

                    // Validate that the class of the object to add is allowed
                    if (allowed_member_classes.size() > 0)
                    {
                        const auto& class_id_property_holder = nmos::get_property_holder(child_object_properties_holder->second, nmos::nc_object_class_id_property_id);

                        if (class_id_property_holder != web::json::value::null())
                        {
                            const auto& class_id = nmos::fields::nc::value(class_id_property_holder);

                            const auto& filtered_classes = boost::copy_range<std::set<web::json::value>>(allowed_member_classes
                                | boost::adaptors::filtered([&](const web::json::value& member)
                                    {
                                        return member.as_array() == class_id.as_array();
                                    })
                            );

                            // If receiver monitor class not allowed then return with error
                            if (filtered_classes.size() == 0)
                            {
                                utility::stringstream_t ss;
                                ss << U("Device model error: attempting to add unexpected class for role=") << role;
                                const auto notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("class_id"), nmos::nc_property_restore_notice_type::error, ss.str());
                                web::json::push_back(added_object_notices, notice);
                                auto object_properties_set_validation = nmos::make_object_properties_set_validation(child_role_path.as_array(), nmos::nc_restore_validation_status::failed, added_object_notices.as_array(), ss.str());
                                web::json::push_back(object_properties_set_validations, object_properties_set_validation);
                                // also create error notice for the block
                                const auto block_notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("members"), nmos::nc_property_restore_notice_type::error, ss.str());
                                web::json::push_back(block_notices, block_notice);

                                // erase object from object_properties_holder_map so it isn't processed subsequently
                                object_properties_holder_map.erase(child_role_path.as_array());

                                continue;
                            }
                        }
                        else
                        {
                            utility::stringstream_t ss;
                            ss << U("Class ID property value holder missing for role=") << role;
                            const auto notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("class_id"), nmos::nc_property_restore_notice_type::error, ss.str());
                            web::json::push_back(added_object_notices, notice);
                            auto object_properties_set_validation = nmos::make_object_properties_set_validation(child_role_path.as_array(), nmos::nc_restore_validation_status::failed, added_object_notices.as_array(), ss.str());
                            web::json::push_back(object_properties_set_validations, object_properties_set_validation);
                            // also create error notice for the block
                            const auto block_notice = nmos::details::make_nc_property_restore_notice(nmos::nc_block_members_property_id, U("members"), nmos::nc_property_restore_notice_type::error, ss.str());
                            web::json::push_back(block_notices, block_notice);

                            // erase object from object_properties_holder_map so it isn't processed subsequently
                            object_properties_holder_map.erase(child_role_path.as_array());

                            continue;
                        }
                    }

                    const auto& user_label_property_holder = nmos::get_property_holder(child_object_properties_holder->second, nmos::nc_object_user_label_property_id);
                    const auto& user_label = (user_label_property_holder == web::json::value::null()) ? block_member_user_label : nmos::fields::nc::value(user_label_property_holder).as_string();

                    auto object_properties_set_validation = add_device_model_object(child_object_properties_holder->second, oid, owner, role, user_label, validate);
                    // Add warnings about known inconsistancies between backup dataset and new device model object
                    for (const auto& added_object_notice: added_object_notices.as_array())
                    {
                        web::json::push_back(nmos::fields::nc::notices(object_properties_set_validation), added_object_notice);
                    }

                    // If the status is anything other than OK assume the object wasn't created
                    if (nmos::fields::nc::status(object_properties_set_validation) == nmos::nc_restore_validation_status::ok && !validate)
                    {
                        auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(block_member_description, role, oid, constant_oid, nmos::nc_receiver_monitor_class_id, block_member_user_label, owner);
                        members_to_add.push_back(block_member_descriptor);
                    }

                    web::json::push_back(object_properties_set_validations, object_properties_set_validation);

                    // erase object from object_properties_holder_map so it isn't processed subsequently
                    object_properties_holder_map.erase(child_role_path.as_array());
                }
            }
            // If there are any error notices, then give an overall error status for object properties set validation
            const auto& error_notices = boost::copy_range<std::set<web::json::value>>(block_notices.as_array()
                | boost::adaptors::filtered([&](const web::json::value& notice)
                    {
                        return nmos::fields::nc::notice_type(notice) == nmos::nc_property_restore_notice_type::error;
                    })
            );
            const auto block_status = error_notices.size() ? nmos::nc_restore_validation_status::failed : nmos::nc_restore_validation_status::ok;
            auto block_set_validation = nmos::make_object_properties_set_validation(target_role_path, block_status, block_notices.as_array());
            web::json::push_back(object_properties_set_validations, block_set_validation);

            // Update the members of the receivers block
            if (members_to_remove.size() > 0 || members_to_add.size() > 0)
            {
                auto modified_members = web::json::value::array();

                for (const auto& member : reference_members)
                {
                    // Is this member in the members_to_remove array?
                    const auto& remove_member = boost::copy_range<std::set<int>>(members_to_remove | boost::adaptors::filtered([&member](int oid)
                        {
                            return oid == nmos::fields::nc::oid(member);
                        })
                    );

                    // If not add it to the members array
                    if (remove_member.size() == 0)
                    {
                        web::json::push_back(modified_members, member);
                    }
                }
                // Then add the members_to_add
                for (const auto& member : members_to_add)
                {
                    web::json::push_back(modified_members, member);
                }

                // Update the block in the device model
                nmos::nc::modify_resource(resources, resource.id, [&](nmos::resource& resource)
                    {
                        resource.data[nmos::fields::nc::members] = modified_members;

                    }, nmos::make_property_changed_event(nmos::fields::nc::oid(resource.data), { { nmos::nc_block_members_property_id, nmos::nc_property_change_type::type::value_changed, modified_members } }));
            }

            return object_properties_set_validations;
        }
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

    web::json::value get_property_holder(const web::json::value& object_properties_holder, const nmos::nc_property_id& property_id)
    {
        const auto& filtered_property_holders = boost::copy_range<std::set<web::json::value>>(nmos::fields::nc::values(object_properties_holder)
            | boost::adaptors::filtered([&property_id](const web::json::value& property_holder)
                {
                    return property_id == nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_holder));
                })
        );
        // There should only be a single property holder for the members
        if (filtered_property_holders.size() != 1)
        {
            // Error
            return web::json::value::null();
        }

        return *filtered_property_holders.begin();
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
            | boost::adaptors::filtered([](const web::json::value& property_holder)
                {
                    return nmos::nc_property_id(2, 2) == nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_holder));
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

    web::json::value apply_backup_data_set(nmos::resources& resources, const nmos::resource& resource, const web::json::array& object_properties_holders, bool recurse, const web::json::value& restore_mode, bool validate, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_holders_handler filter_property_holders, nmos::remove_device_model_object_handler remove_device_model_object, nmos::add_device_model_object_handler add_device_model_object)
    {
        auto object_properties_set_validation_values = web::json::value::array();

        const auto target_role_path = get_role_path(resources, resource);

        // Process blocks and objects separately.
        object_properties_map object_properties_holder_map;
        std::vector< web::json::array > role_paths;

        utility::stringstream_t seed;

        for (const auto& object_properties_holder: object_properties_holders)
        {
            const auto& role_path = nmos::fields::nc::path(object_properties_holder);
            // Only process role paths within the restore scope, or target_role_path only
            if ((recurse && is_role_path_root(target_role_path, role_path)) || (!recurse && target_role_path == role_path))
            {
                const auto& find_role_paths = get_object_properties_holder(object_properties_holders, role_path);

                // Make errors for duplicate role paths
                if (find_role_paths.size() > 1)
                {
                    seed << "."; // to avoid making identical objects
                    utility::stringstream_t ss;
                    ss << U("Duplicate object_properties_holder for role path: ");
                    for (const auto& element: role_path)
                    {
                        ss << element << ".";
                    }
                    ss << seed.str();
                    const auto& object_properties_set_validation = nmos::make_object_properties_set_validation(nmos::fields::nc::path(object_properties_holder), nmos::nc_restore_validation_status::failed, ss.str());
                    web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
                }
                else
                {
                    object_properties_holder_map.insert({ role_path, object_properties_holder });
                    role_paths.push_back(role_path);
                }
            }
        }

        // Order role paths by length so we implicitly process blocks before the child objects of that block
        std::sort(role_paths.begin(), role_paths.end(), [](auto a, auto b) { return a.size() < b.size(); });

        for (const auto& role_path: role_paths)
        {
            const auto& r = nc::find_resource_by_role_path(resources, role_path);

            if (r == resources.end())
            {
                // This could be a resource that's yet to be created in Rebuild mode (OK), or referencing a resource that doesn't exist (not OK)
                // Ignore for now and we will check again once all role paths have been processed
                continue;
            }
            if (object_properties_holder_map.find(role_path) == object_properties_holder_map.end())
            {
                // If this role_path is no longer in the map it may have been erased by modify_rebuildable_block
                continue;
            }

            const auto& object_properties_holder = object_properties_holder_map.at(role_path);

            const auto& class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(r->data));

            if (nmos::nc::is_block(class_id) && nmos::fields::nc::is_rebuildable(r->data) && restore_mode == nmos::nc_restore_mode::rebuild && is_block_modified(*r, object_properties_holder))
            {
                // Modify rebuildable block
                if (remove_device_model_object && add_device_model_object)
                {
                    // Process this block to add / remove device model objects as members of this block
                    // the object properties holder for any added objects will be erased from the object_properties_holder_map to avoid double processing
                    const auto& child_object_properties_set_validations = details::modify_rebuildable_block(resources, object_properties_holder_map, *r, role_path, object_properties_holder, validate, get_control_protocol_class_descriptor, remove_device_model_object, add_device_model_object);
                    for (const auto& validation_values : child_object_properties_set_validations.as_array())
                    {
                        web::json::push_back(object_properties_set_validation_values, validation_values);
                    }

                }
                else
                {
                    // Rebuilding blocks not supported
                    const auto& object_properties_set_validation = nmos::make_object_properties_set_validation(role_path, nmos::nc_restore_validation_status::failed, U("Rebuilding of Device Model blocks not supported"));
                    web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
                }
            }
            else
            {
                const auto object_properties_set_validation = details::modify_device_model_object(resources, *r, role_path, object_properties_holder, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_holders);
                web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
            }

            object_properties_holder_map.erase(role_path);
        }

        // What ever remains is referencing an object not in the device model
        for(const auto& object_properties_holder: object_properties_holder_map)
        {
            const auto& object_properties_set_validation = nmos::make_object_properties_set_validation(object_properties_holder.first, nmos::nc_restore_validation_status::not_found, U("Unknown role path"));
            web::json::push_back(object_properties_set_validation_values, object_properties_set_validation);
        }

        return object_properties_set_validation_values;
    }
}

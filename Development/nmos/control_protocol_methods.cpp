#include "nmos/control_protocol_methods.h"

#include "cpprest/json_utils.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/json_fields.h"
#include "nmos/slog.h"

namespace nmos
{
    // NcObject methods implementation
    // Get property value
    web::json::value get(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        const auto& property_id = nmos::fields::nc::id(arguments);

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Get property: " << property_id.serialize();

        // find the relevant nc_property_descriptor
        const auto& property = find_property_descriptor(details::parse_nc_property_id(property_id), details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data)), get_control_protocol_class_descriptor);
        if (!property.is_null())
        {
            return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::fields::nc::is_deprecated(property) ? nc_method_status::property_deprecated : nc_method_status::ok }, resource.data.at(nmos::fields::nc::name(property)));
        }

        // unknown property
        utility::stringstream_t ss;
        ss << U("unknown property: ") << property_id.serialize() << U(" to do Get");
        return details::make_nc_method_result_error({ nc_method_status::property_not_implemented }, ss.str());
    }

    // Set property value
    web::json::value set(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        const auto& property_id = nmos::fields::nc::id(arguments);
        const auto& val = nmos::fields::nc::value(arguments);

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Set property: " << property_id.serialize() << " value: " << val.serialize();

        // find the relevant nc_property_descriptor
        const auto property_id_ = details::parse_nc_property_id(property_id);
        const auto& property = find_property_descriptor(property_id_, details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data)), get_control_protocol_class_descriptor);
        if (!property.is_null())
        {
            if (nmos::fields::nc::is_read_only(property))
            {
                return details::make_nc_method_result({ nc_method_status::read_only });
            }

            if ((val.is_null() && !nmos::fields::nc::is_nullable(property))
                || (!val.is_array() && nmos::fields::nc::is_sequence(property))
                || (val.is_array() && !nmos::fields::nc::is_sequence(property)))
            {
                return details::make_nc_method_result({ nc_method_status::parameter_error });
            }

            try
            {
                // do property constraints validation
                nmos::details::constraints_validation(val, details::get_runtime_property_constraints(property_id_, resource.data.at(nmos::fields::nc::runtime_property_constraints)), nmos::fields::nc::constraints(property), { details::get_datatype_descriptor(property.at(nmos::fields::nc::type_name), get_control_protocol_datatype_descriptor), get_control_protocol_datatype_descriptor });

                // update property
                modify_control_protocol_resource(resources, resource.id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::nc::name(property)] = val;

                    // do notification that the specified property has changed
                    if (property_changed)
                    {
                        property_changed(resource, nmos::fields::nc::name(property), -1);
                    }

                }, make_property_changed_event(nmos::fields::nc::oid(resource.data), { { property_id_, nc_property_change_type::type::value_changed, val } }));

                return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::fields::nc::is_deprecated(property) ? nc_method_status::property_deprecated : nc_method_status::ok });
            }
            catch (const nmos::control_protocol_exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set property: " << property_id.serialize() << " value: " << val.serialize() << " error: " << e.what();

                return details::make_nc_method_result({ nc_method_status::parameter_error });
            }
        }

        // unknown property
        utility::stringstream_t ss;
        ss << U("unknown property: ") << property_id.serialize() << " to do Set";
        return details::make_nc_method_result_error({ nc_method_status::property_not_implemented }, ss.str());
    }

    // Get sequence item
    web::json::value get_sequence_item(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        const auto& property_id = nmos::fields::nc::id(arguments);
        const auto& index = nmos::fields::nc::index(arguments);

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Get sequence item: " << property_id.serialize() << " index: " << index;

        // find the relevant nc_property_descriptor
        const auto& property = find_property_descriptor(details::parse_nc_property_id(property_id), details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data)), get_control_protocol_class_descriptor);
        if (!property.is_null())
        {
            const auto& data = resource.data.at(nmos::fields::nc::name(property));

            if (!nmos::fields::nc::is_sequence(property) || data.is_null() || !data.is_array())
            {
                // property is not a sequence
                utility::stringstream_t ss;
                ss << U("property: ") << property_id.serialize() << U(" is not a sequence to do GetSequenceItem");
                return details::make_nc_method_result_error({ nc_method_status::invalid_request }, ss.str());
            }

            if (data.as_array().size() > (size_t)index)
            {
                return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::fields::nc::is_deprecated(property) ? nc_method_status::property_deprecated : nc_method_status::ok }, data.at(index));
            }

            // out of bound
            utility::stringstream_t ss;
            ss << U("property: ") << property_id.serialize() << U(" is outside the available range to do GetSequenceItem");
            return details::make_nc_method_result_error({ nc_method_status::index_out_of_bounds }, ss.str());
        }

        // unknown property
        utility::stringstream_t ss;
        ss << U("unknown property: ") << property_id.serialize() << U(" to do GetSequenceItem");
        return details::make_nc_method_result_error({ nc_method_status::property_not_implemented }, ss.str());
    }

    // Set sequence item
    web::json::value set_sequence_item(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        const auto& property_id = nmos::fields::nc::id(arguments);
        const auto& index = nmos::fields::nc::index(arguments);
        const auto& val = nmos::fields::nc::value(arguments);

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Set sequence item: " << property_id.serialize() << " index: " << index << " value: " << val.serialize();

        // find the relevant nc_property_descriptor
        const auto property_id_ = details::parse_nc_property_id(property_id);
        const auto& property = find_property_descriptor(property_id_, details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data)), get_control_protocol_class_descriptor);
        if (!property.is_null())
        {
            if (nmos::fields::nc::is_read_only(property))
            {
                return  details::make_nc_method_result({ nc_method_status::read_only });
            }

            auto& data = resource.data.at(nmos::fields::nc::name(property));

            if (!nmos::fields::nc::is_sequence(property) || data.is_null() || !data.is_array())
            {
                // property is not a sequence
                utility::stringstream_t ss;
                ss << U("property: ") << property_id.serialize() << U(" is not a sequence to do SetSequenceItem");
                return details::make_nc_method_result_error({ nc_method_status::invalid_request }, ss.str());
            }

            if (data.as_array().size() > (size_t)index)
            {
                try
                {
                    // do property constraints validation
                    nmos::details::constraints_validation(val, details::get_runtime_property_constraints(property_id_, resource.data.at(nmos::fields::nc::runtime_property_constraints)), nmos::fields::nc::constraints(property), { details::get_datatype_descriptor(property.at(nmos::fields::nc::type_name), get_control_protocol_datatype_descriptor), get_control_protocol_datatype_descriptor });

                    // update property
                    modify_control_protocol_resource(resources, resource.id, [&](nmos::resource& resource)
                    {
                        resource.data[nmos::fields::nc::name(property)][index] = val;

                        // do notification that the specified property has changed
                        if (property_changed)
                        {
                            property_changed(resource, nmos::fields::nc::name(property), index);
                        }

                    }, make_property_changed_event(nmos::fields::nc::oid(resource.data), { { property_id_, nc_property_change_type::type::sequence_item_changed, val, nc_id(index) } }));

                    return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::fields::nc::is_deprecated(property) ? nc_method_status::property_deprecated : nc_method_status::ok });
                }
                catch (const nmos::control_protocol_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set sequence item: " << property_id.serialize() << " index: " << index << " value: " << val.serialize() << " error: " << e.what();

                    return  details::make_nc_method_result({ nc_method_status::parameter_error });
                }
            }

            // out of bound
            utility::stringstream_t ss;
            ss << U("property: ") << property_id.serialize() << U(" is outside the available range to do SetSequenceItem");
            return details::make_nc_method_result_error({ nc_method_status::index_out_of_bounds }, ss.str());
        }

        // unknown property
        utility::stringstream_t ss;
        ss << U("unknown property: ") << property_id.serialize() << U(" to do SetSequenceItem");
        return details::make_nc_method_result_error({ nc_method_status::property_not_implemented }, ss.str());
    }

    // Add item to sequence
    web::json::value add_sequence_item(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        using web::json::value;

        const auto& property_id = nmos::fields::nc::id(arguments);
        const auto& val = nmos::fields::nc::value(arguments);

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Add sequence item: " << property_id.serialize() << " value: " << val.serialize();

        // find the relevant nc_property_descriptor
        const auto property_id_ = details::parse_nc_property_id(property_id);
        const auto& property = find_property_descriptor(property_id_, details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data)), get_control_protocol_class_descriptor);
        if (!property.is_null())
        {
            if (nmos::fields::nc::is_read_only(property))
            {
                return details::make_nc_method_result({ nc_method_status::read_only });
            }

            if (!nmos::fields::nc::is_sequence(property))
            {
                // property is not a sequence
                utility::stringstream_t ss;
                ss << U("property: ") << property_id.serialize() << U(" is not a sequence to do AddSequenceItem");
                return details::make_nc_method_result_error({ nc_method_status::invalid_request }, ss.str());
            }

            auto& data = resource.data.at(nmos::fields::nc::name(property));

            const nc_id sequence_item_index = data.is_null() ? 0 : nc_id(data.as_array().size());

            try
            {
                // do property constraints validation
                nmos::details::constraints_validation(val, details::get_runtime_property_constraints(property_id_, resource.data.at(nmos::fields::nc::runtime_property_constraints)), nmos::fields::nc::constraints(property), { details::get_datatype_descriptor(property.at(nmos::fields::nc::type_name), get_control_protocol_datatype_descriptor), get_control_protocol_datatype_descriptor });

                // update property
                modify_control_protocol_resource(resources, resource.id, [&](nmos::resource& resource)
                {
                    auto& sequence = resource.data[nmos::fields::nc::name(property)];
                    if (data.is_null()) { sequence = value::array(); }
                    web::json::push_back(sequence, val);

                    // do notification that the specified property has changed
                    if (property_changed)
                    {
                        property_changed(resource, nmos::fields::nc::name(property), (int)sequence.as_array().size()-1);
                    }

                }, make_property_changed_event(nmos::fields::nc::oid(resource.data), { { property_id_, nc_property_change_type::type::sequence_item_added, val, sequence_item_index } }));

                return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::fields::nc::is_deprecated(property) ? nc_method_status::property_deprecated : nc_method_status::ok }, sequence_item_index);
            }
            catch (const nmos::control_protocol_exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Add sequence item: " << property_id.serialize() << " value: " << val.serialize() << " error: " << e.what();

                return details::make_nc_method_result({ nc_method_status::parameter_error });
            }
        }

        // unknown property
        utility::stringstream_t ss;
        ss << U("unknown property: ") << property_id.serialize() << U(" to do AddSequenceItem");
        return details::make_nc_method_result_error({ nc_method_status::property_not_implemented }, ss.str());
    }

    // Delete sequence item
    web::json::value remove_sequence_item(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        const auto& property_id = nmos::fields::nc::id(arguments);
        const auto& index = nmos::fields::nc::index(arguments);

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Remove sequence item: " << property_id.serialize() << " index: " << index;

        // find the relevant nc_property_descriptor
        const auto& property = find_property_descriptor(details::parse_nc_property_id(property_id), details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data)), get_control_protocol_class_descriptor);
        if (!property.is_null())
        {
            const auto& data = resource.data.at(nmos::fields::nc::name(property));

            if (!nmos::fields::nc::is_sequence(property) || data.is_null() || !data.is_array())
            {
                // property is not a sequence
                utility::stringstream_t ss;
                ss << U("property: ") << property_id.serialize() << U(" is not a sequence to do RemoveSequenceItem");
                return details::make_nc_method_result_error({ nc_method_status::invalid_request }, ss.str());
            }

            if (data.as_array().size() > (size_t)index)
            {
                modify_control_protocol_resource(resources, resource.id, [&](nmos::resource& resource)
                {
                    auto& sequence = resource.data[nmos::fields::nc::name(property)].as_array();
                    sequence.erase(index);

                }, make_property_changed_event(nmos::fields::nc::oid(resource.data), { { details::parse_nc_property_id(property_id), nc_property_change_type::type::sequence_item_removed, nc_id(index) } }));

                return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::fields::nc::is_deprecated(property) ? nc_method_status::property_deprecated : nc_method_status::ok });
            }

            // out of bound
            utility::stringstream_t ss;
            ss << U("property: ") << property_id.serialize() << U(" is outside the available range to do RemoveSequenceItem");
            return details::make_nc_method_result_error({ nc_method_status::index_out_of_bounds }, ss.str());
        }

        // unknown property
        utility::stringstream_t ss;
        ss << U("unknown property: ") << property_id.serialize() << U(" to do RemoveSequenceItem");
        return details::make_nc_method_result_error({ nc_method_status::property_not_implemented }, ss.str());
    }

    // Get sequence length
    web::json::value get_sequence_length(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        using web::json::value;

        const auto& property_id = nmos::fields::nc::id(arguments);

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Get sequence length: " << property_id.serialize();

        // find the relevant nc_property_descriptor
        const auto& property = find_property_descriptor(details::parse_nc_property_id(property_id), details::parse_nc_class_id(nmos::fields::nc::class_id(resource.data)), get_control_protocol_class_descriptor);
        if (!property.is_null())
        {
            if (!nmos::fields::nc::is_sequence(property))
            {
                // property is not a sequence
                utility::stringstream_t ss;
                ss << U("property: ") << property_id.serialize() << U(" is not a sequence to do GetSequenceLength");
                return details::make_nc_method_result_error({ nc_method_status::invalid_request }, ss.str());
            }

            const auto& data = resource.data.at(nmos::fields::nc::name(property));

            if (nmos::fields::nc::is_nullable(property))
            {
                // can be null
                if (data.is_null())
                {
                    // null
                    return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::fields::nc::is_deprecated(property) ? nc_method_status::property_deprecated : nc_method_status::ok }, value::null());
                }
            }
            else
            {
                // cannot be null
                if (data.is_null())
                {
                    // null
                    utility::stringstream_t ss;
                    ss << U("property: ") << property_id.serialize() << " is a null sequence to do GetSequenceLength";
                    return details::make_nc_method_result_error({ nc_method_status::invalid_request }, ss.str());
                }
            }
            return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::fields::nc::is_deprecated(property) ? nc_method_status::property_deprecated : nc_method_status::ok }, value(uint32_t(data.as_array().size())));
        }

        // unknown property
        utility::stringstream_t ss;
        ss << U("unknown property: ") << property_id.serialize() << " to do GetSequenceLength";
        return details::make_nc_method_result_error({ nc_method_status::property_not_implemented }, ss.str());
    }

    // NcBlock methods implementation
    // Gets descriptors of members of the block
    web::json::value get_member_descriptors(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        using web::json::value;

        const auto& recurse = nmos::fields::nc::recurse(arguments); // If recurse is set to true, nested members is to be retrieved

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Get descriptors of members of the block: " << "recurse: " << recurse;

        auto descriptors = value::array();
        nmos::get_member_descriptors(resources, resource, recurse, descriptors.as_array());

        return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nc_method_status::ok }, descriptors);
    }

    // Finds member(s) by path
    web::json::value find_members_by_path(nmos::resources& resources, const nmos::resource& resource_, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        using web::json::value;

        // Relative path to search for (MUST not include the role of the block targeted by oid)
        const auto& path = arguments.at(nmos::fields::nc::path);

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Find member(s) by path: " << "path: " << path.serialize();

        if (0 == path.size())
        {
            // empty path
            return details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("empty path to do FindMembersByPath"));
        }

        auto descriptors = value::array();
        value descriptor;

        nmos::resource resource = resource_;
        for (const auto& role : path.as_array())
        {
            // look for the role in members

            if (resource.data.has_field(nmos::fields::nc::members))
            {
                auto& members = nmos::fields::nc::members(resource.data);
                auto member_found = std::find_if(members.begin(), members.end(), [&](const web::json::value& nc_block_member_descriptor)
                {
                    return role.as_string() == nmos::fields::nc::role(nc_block_member_descriptor);
                });

                if (members.end() != member_found)
                {
                    descriptor = *member_found;

                    // use oid to look for the next resource
                    resource = *nmos::find_resource(resources, utility::s2us(std::to_string(nmos::fields::nc::oid(*member_found))));
                }
                else
                {
                    // no role
                    utility::stringstream_t ss;
                    ss << U("role: ") << role.as_string() << U(" not found to do FindMembersByPath");
                    return details::make_nc_method_result_error({ nc_method_status::parameter_error }, ss.str());
                }
            }
            else
            {
                // no members
                return details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("no members to do FindMembersByPath"));
            }
        }

        web::json::push_back(descriptors, descriptor);
        return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nc_method_status::ok }, descriptors);
    }

    // Finds members with given role name or fragment
    web::json::value find_members_by_role(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        using web::json::value;

        const auto& role = nmos::fields::nc::role(arguments); // Role text to search for
        const auto& case_sensitive = nmos::fields::nc::case_sensitive(arguments); // Signals if the comparison should be case sensitive
        const auto& match_whole_string = nmos::fields::nc::match_whole_string(arguments); // TRUE to only return exact matches
        const auto& recurse = nmos::fields::nc::recurse(arguments); // TRUE to search nested blocks

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Find members with given role name or fragment: " << "role: " << role;

        if (role.empty())
        {
            // empty role
            return details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("empty role to do FindMembersByRole"));
        }

        auto descriptors = value::array();
        nmos::find_members_by_role(resources, resource, role, match_whole_string, case_sensitive, recurse, descriptors.as_array());

        return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nc_method_status::ok }, descriptors);
    }

    // Finds members with given class id
    web::json::value find_members_by_class_id(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        using web::json::value;

        const auto& class_id = details::parse_nc_class_id(nmos::fields::nc::class_id(arguments)); // Class id to search for
        const auto& include_derived = nmos::fields::nc::include_derived(arguments); // If TRUE it will also include derived class descriptors
        const auto& recurse = nmos::fields::nc::recurse(arguments); // TRUE to search nested blocks

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Find members with given class id: " << "class_id: " << nmos::details::make_nc_class_id(class_id).serialize();

        if (class_id.empty())
        {
            // empty class_id
            return details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("empty classId to do FindMembersByClassId"));
        }

        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        auto descriptors = value::array();
        nmos::find_members_by_class_id(resources, resource, class_id, include_derived, recurse, descriptors.as_array());

        return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nc_method_status::ok }, descriptors);
    }

    // NcClassManager methods implementation
    // Get a single class descriptor
    web::json::value get_control_class(nmos::resources&, const nmos::resource&, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
    {
        using web::json::value;

        const auto& class_id = details::parse_nc_class_id(nmos::fields::nc::class_id(arguments)); // Class id to search for
        const auto& include_inherited = nmos::fields::nc::include_inherited(arguments); // If set the descriptor would contain all inherited elements

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Get a single class descriptor: " << "class_id: " << nmos::details::make_nc_class_id(class_id).serialize();

        if (class_id.empty())
        {
            // empty class_id
            return details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("empty classId to do GetControlClass"));
        }

        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        const auto& control_class = get_control_protocol_class_descriptor(class_id);
        if (!control_class.class_id.empty())
        {
            auto& description = control_class.description;
            auto& name = control_class.name;
            auto& fixed_role = control_class.fixed_role;
            auto property_descriptors = control_class.property_descriptors;
            auto method_descriptors = value::array();
            for (const auto& method_descriptor : control_class.method_descriptors) { web::json::push_back(method_descriptors, std::get<0>(method_descriptor)); }
            auto event_descriptors = control_class.event_descriptors;

            if (include_inherited)
            {
                auto inherited_class_id = class_id;
                inherited_class_id.pop_back();

                while (!inherited_class_id.empty())
                {
                    const auto& inherited_control_class = get_control_protocol_class_descriptor(inherited_class_id);
                    {
                        for (const auto& property_descriptor : inherited_control_class.property_descriptors.as_array()) { web::json::push_back(property_descriptors, property_descriptor); }
                        for (const auto& method_descriptor : inherited_control_class.method_descriptors) { web::json::push_back(method_descriptors, std::get<0>(method_descriptor)); }
                        for (const auto& event_descriptor : inherited_control_class.event_descriptors.as_array()) { web::json::push_back(event_descriptors, event_descriptor); }
                    }
                    inherited_class_id.pop_back();
                }
            }
            const auto descriptor = fixed_role.is_null()
                ? details::make_nc_class_descriptor(description, class_id, name, property_descriptors, method_descriptors, event_descriptors)
                : details::make_nc_class_descriptor(description, class_id, name, fixed_role.as_string(), property_descriptors, method_descriptors, event_descriptors);

            return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nc_method_status::ok }, descriptor);
        }

        return details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("classId not found"));
    }

    // Get a single datatype descriptor
    web::json::value get_datatype(nmos::resources&, const nmos::resource&, const web::json::value& arguments, bool is_deprecated, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, slog::base_gate& gate)
    {
        // note, model mutex is already locked by the outer function, so access to control_protocol_resources is OK...

        const auto& name = nmos::fields::nc::name(arguments); // name of datatype
        const auto& include_inherited = nmos::fields::nc::include_inherited(arguments); // If set the descriptor would contain all inherited elements

        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Get a single datatype descriptor: " << "name: " << name;

        if (name.empty())
        {
            // empty name
            return details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("empty name to do GetDatatype"));
        }

        const auto& datatype = get_control_protocol_datatype_descriptor(name);
        if (datatype.descriptor.size())
        {
            auto descriptor = datatype.descriptor;

            if (include_inherited)
            {
                const auto& type = nmos::fields::nc::type(descriptor);
                if (nc_datatype_type::Struct == type)
                {
                    auto descriptor_ = descriptor;

                    for (;;)
                    {
                        const auto& parent_type = descriptor_.at(nmos::fields::nc::parent_type);
                        if (!parent_type.is_null())
                        {
                            const auto& parent_datatype = get_control_protocol_datatype_descriptor(parent_type.as_string());
                            if (parent_datatype.descriptor.size())
                            {
                                descriptor_ = parent_datatype.descriptor;

                                const auto& fields = nmos::fields::nc::fields(descriptor_);
                                for (const auto& field : fields)
                                {
                                    web::json::push_back(descriptor.at(nmos::fields::nc::fields), field);
                                }
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }

            return details::make_nc_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nc_method_status::ok }, descriptor);
        }

        return details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("name not found"));
    }
}

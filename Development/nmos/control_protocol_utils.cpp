#include "nmos/control_protocol_utils.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include "cpprest/json_utils.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/json_fields.h"
#include "nmos/resources.h"

namespace nmos
{
    namespace details
    {
        bool is_control_class(const nc_class_id& control_class_id, const nc_class_id& class_id_)
        {
            nc_class_id class_id{ class_id_ };
            if (control_class_id.size() < class_id.size())
            {
                // truncate test class_id to relevant class_id
                class_id.resize(control_class_id.size());
            }
            return control_class_id == class_id;
        }
    }

    // is the given class_id a NcBlock
    bool is_nc_block(const nc_class_id& class_id)
    {
        return details::is_control_class(nc_object_class_id, class_id);
    }

    // is the given class_id a NcManager
    bool is_nc_manager(const nc_class_id& class_id)
    {
        return details::is_control_class(nc_manager_class_id, class_id);
    }

    // is the given class_id a NcDeviceManager
    bool is_nc_device_manager(const nc_class_id& class_id)
    {
        return details::is_control_class(nc_device_manager_class_id, class_id);
    }

    // is the given class_id a NcClassManager
    bool is_nc_class_manager(const nc_class_id& class_id)
    {
        return details::is_control_class(nc_class_manager_class_id, class_id);
    }

    // find control class property (NcPropertyDescriptor)
    web::json::value find_property(const nc_property_id& property_id, const nc_class_id& class_id_, get_control_protocol_class_handler get_control_protocol_class)
    {
        using web::json::value;

        auto class_id = class_id_;

        while (!class_id.empty())
        {
            const auto& control_class = get_control_protocol_class(class_id);
            auto& properties = control_class.properties.as_array();
            if (properties.size())
            {
                for (const auto& property : properties)
                {
                    if (property_id == nmos::details::parse_nc_property_id(nmos::fields::nc::id(property)))
                    {
                        return property;
                    }
                }
            }
            class_id.pop_back();
        }

        return value::null();
    }

    // construct NcClassId
    nc_class_id make_nc_class_id(const nc_class_id& prefix, int32_t authority_key, const std::vector<int32_t>& suffix)
    {
        nc_class_id class_id = prefix;
        class_id.push_back(authority_key);
        class_id.insert(class_id.end(), suffix.begin(), suffix.end());
        return class_id;
    }

    // get descriptors of members of the block
    void get_member_descriptors(const resources& resources, resources::iterator resource, bool recurse, web::json::array& descriptors)
    {
        if (resource->data.has_field(nmos::fields::nc::members))
        {
            const auto& members = nmos::fields::nc::members(resource->data);

            // hmm, maybe an easier way to apeend array to array
            for (const auto& member : members)
            {
                web::json::push_back(descriptors, member);
            }

            if (recurse)
            {
                // get members on all NcBlock(s)
                for (const auto& member : members)
                {
                    if (is_nc_block(nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(member))))
                    {
                        // get resource based on the oid
                        const auto& oid = nmos::fields::nc::oid(member);

                        get_member_descriptors(resources, find_resource(resources, utility::s2us(std::to_string(oid))), recurse, descriptors);
                    }
                }
            }
        }
    }

    // find members with given role name or fragment
    void find_members_by_role(const resources& resources, resources::iterator resource, const utility::string_t& role, bool match_whole_string, bool case_sensitive, bool recurse, web::json::array& descriptors)
    {
        auto find_members_by_matching_role = [&](const web::json::array& members)
        {
            using web::json::value;

            auto match = [&](const web::json::value& descriptor)
            {
                if (match_whole_string)
                {
                    if (case_sensitive) { return role == nmos::fields::nc::role(descriptor); }
                    else { return boost::algorithm::to_upper_copy(role) == boost::algorithm::to_upper_copy(nmos::fields::nc::role(descriptor)); }
                }
                else
                {
                    if (case_sensitive) { return !boost::find_first(nmos::fields::nc::role(descriptor), role).empty(); }
                    else { return !boost::ifind_first(nmos::fields::nc::role(descriptor), role).empty(); }
                }
            };

            return boost::make_iterator_range(boost::make_filter_iterator(match, members.begin(), members.end()), boost::make_filter_iterator(match, members.end(), members.end()));
        };

        if (resource->data.has_field(nmos::fields::nc::members))
        {
            const auto& members = nmos::fields::nc::members(resource->data);

            auto members_found = find_members_by_matching_role(members);
            for (const auto& member : members_found)
            {
                web::json::push_back(descriptors, member);
            }

            if (recurse)
            {
                // do role match on all NcBlock(s)
                for (const auto& member : members)
                {
                    if (is_nc_block(nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(member))))
                    {
                        // get resource based on the oid
                        const auto& oid = nmos::fields::nc::oid(member);

                        find_members_by_role(resources, find_resource(resources, utility::s2us(std::to_string(oid))), role, match_whole_string, case_sensitive, recurse, descriptors);
                    }
                }
            }
        }
    }

    // find members with given class id
    void find_members_by_class_id(const resources& resources, resources::iterator resource, const nc_class_id& class_id_, bool include_derived, bool recurse, web::json::array& descriptors)
    {
        auto find_members_by_matching_class_id = [&](const web::json::array& members)
        {
            using web::json::value;

            auto match = [&](const web::json::value& descriptor)
            {
                const auto& class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(descriptor));

                if (include_derived) { return !boost::find_first(class_id, class_id_).empty(); }
                else { return class_id == class_id_; }
            };

            return boost::make_iterator_range(boost::make_filter_iterator(match, members.begin(), members.end()), boost::make_filter_iterator(match, members.end(), members.end()));
        };

        if (resource->data.has_field(nmos::fields::nc::members))
        {
            auto& members = nmos::fields::nc::members(resource->data);

            auto members_found = find_members_by_matching_class_id(members);
            for (const auto& member : members_found)
            {
                web::json::push_back(descriptors, member);
            }

            if (recurse)
            {
                // do class_id match on all NcBlock(s)
                for (const auto& member : members)
                {
                    if (is_nc_block(nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(member))))
                    {
                        // get resource based on the oid
                        const auto& oid = nmos::fields::nc::oid(member);

                        find_members_by_class_id(resources, find_resource(resources, utility::s2us(std::to_string(oid))), class_id_, include_derived, recurse, descriptors);
                    }
                }
            }
        }
    }
}

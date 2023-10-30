#include "nmos/control_protocol_utils.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include "bst/regex.h"
#include "cpprest/json_utils.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/json_fields.h"
#include "nmos/query_utils.h"
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

        // get the runtime property constraints of a given property_id
        web::json::value get_runtime_property_constraints(const nc_property_id& property_id, const web::json::value& runtime_property_constraints)
        {
            using web::json::value;

            if (!runtime_property_constraints.is_null())
            {
                auto& runtime_prop_constraints = runtime_property_constraints.as_array();
                auto found_constraints = std::find_if(runtime_prop_constraints.begin(), runtime_prop_constraints.end(), [&property_id](const web::json::value& constraints)
                    {
                        //return nmos::fields::nc::id(property) == nmos::fields::nc::property_id(constraints);
                        return property_id == parse_nc_property_id(nmos::fields::nc::property_id(constraints));
                    });

                if (runtime_prop_constraints.end() != found_constraints)
                {
                    return *found_constraints;
                }
            }
            return value::null();
        }

        // get the datatype property constraints of a given type_name
        web::json::value get_datatype_constraints(const web::json::value& type_name, get_control_protocol_datatype_handler get_control_protocol_datatype)
        {
            using web::json::value;

            if (!type_name.is_null())
            {
                const auto& datatype = get_control_protocol_datatype(type_name.as_string());
                if (!datatype.descriptor.is_null()) // NcDatatypeDescriptor
                {
                    return nmos::fields::nc::constraints(datatype.descriptor);
                }
            }
            return value::null();
        }
    }

    // is the given class_id a NcBlock
    bool is_nc_block(const nc_class_id& class_id)
    {
        return details::is_control_class(nc_block_class_id, class_id);
    }

    // is the given class_id a NcWorker
    bool is_nc_worker(const nc_class_id& class_id)
    {
        return details::is_control_class(nc_worker_class_id, class_id);
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

    // construct NcClassId
    nc_class_id make_nc_class_id(const nc_class_id& prefix, int32_t authority_key, const std::vector<int32_t>& suffix)
    {
        nc_class_id class_id = prefix;
        class_id.push_back(authority_key);
        class_id.insert(class_id.end(), suffix.begin(), suffix.end());
        return class_id;
    }
    nc_class_id make_nc_class_id(const nc_class_id& prefix, const std::vector<int32_t>& suffix)
    {
        return make_nc_class_id(prefix, 0, suffix);
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
            auto found = std::find_if(properties.begin(), properties.end(), [&property_id](const web::json::value& property)
            {
                return (property_id == nmos::details::parse_nc_property_id(nmos::fields::nc::id(property)));
            });
            if (properties.end() != found) { return *found; }

            class_id.pop_back();
        }

        return value::null();
    }

    // get block member descriptors
    void get_member_descriptors(const resources& resources, resources::iterator resource, bool recurse, web::json::array& descriptors)
    {
        if (resource->data.has_field(nmos::fields::nc::members))
        {
            const auto& members = nmos::fields::nc::members(resource->data);

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

    // push a control protocol resource into other control protocol NcBlock resource
    void push_back(control_protocol_resource& nc_block_resource, const control_protocol_resource& resource)
    {
        using web::json::value;

        auto& parent = nc_block_resource.data;
        const auto& child = resource.data;

        if (!is_nc_block(details::parse_nc_class_id(nmos::fields::nc::class_id(parent)))) throw std::logic_error("non-NcBlock cannot be nested");

        web::json::push_back(parent[nmos::fields::nc::members],
            details::make_nc_block_member_descriptor(nmos::fields::description(child), nmos::fields::nc::role(child), nmos::fields::nc::oid(child), nmos::fields::nc::constant_oid(child), details::parse_nc_class_id(nmos::fields::nc::class_id(child)), nmos::fields::nc::user_label(child), nmos::fields::nc::oid(parent)));

        nc_block_resource.resources.push_back(resource);
    }

    // modify a control protocol resource, and insert notification event to all subscriptions
    bool modify_control_protocol_resource(resources& resources, const id& id, std::function<void(resource&)> modifier, const web::json::value& notification_event)
    {
        auto found = resources.find(id);
        if (resources.end() == found || !found->has_data()) return false;

        auto pre = found->data;

        // "If an exception is thrown by some user-provided operation, then the element pointed to by position is erased."
        // This seems too surprising, despite the fact that it means that a modification may have been partially completed,
        // so capture and rethrow.
        // See https://www.boost.org/doc/libs/1_68_0/libs/multi_index/doc/reference/ord_indices.html#modify
        std::exception_ptr modifier_exception;

        auto resource_updated = nmos::strictly_increasing_update(resources);
        auto result = resources.modify(found, [&resource_updated, &modifier, &modifier_exception](resource& resource)
        {
            try
            {
                modifier(resource);
            }
            catch (...)
            {
                modifier_exception = std::current_exception();
            }

            // set the update timestamp
            resource.updated = resource_updated;
        });

        if (result)
        {
            auto& modified = *found;

            insert_notification_events(resources, modified.version, modified.downgrade_version, modified.type, pre, modified.data, notification_event);
        }

        if (modifier_exception)
        {
            std::rethrow_exception(modifier_exception);
        }

        return result;
    }

    // find the control protocol resource which is assoicated with the given IS-04/IS-05/IS-08 resource id
    resources::const_iterator find_control_protocol_resource(resources& resources, type type, const id& resource_id)
    {
        return find_resource_if(resources, type, [resource_id](const nmos::resource& resource)
        {
            auto& touchpoints = resource.data.at(nmos::fields::nc::touchpoints);
            if (!touchpoints.is_null() && touchpoints.is_array())
            {
                auto& tps = touchpoints.as_array();
                auto found_tp = std::find_if(tps.begin(), tps.end(), [resource_id](const web::json::value& touchpoint)
                {
                    auto& resource = nmos::fields::nc::resource(touchpoint);
                    return (resource_id == nmos::fields::nc::id(resource).as_string()
                        && nmos::ncp_nmos_resource_types::receiver.name == nmos::fields::nc::resource_type(resource));
                });
                return (tps.end() != found_tp);
            }
            return false;
        });
    }

    // constraints validation
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsnumber
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsstring
    bool constraints_validation(const web::json::value& value, const web::json::value& constraints)
    {
        // is numeric constraints
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsnumber
        if (constraints.has_field(nmos::fields::nc::step) && !nmos::fields::nc::step(constraints).is_null())
        {
            if (!value.is_integer()) { return false; }

            const auto step = nmos::fields::nc::step(constraints).as_double();
            if (step <= 0) { return false; }

            const auto value_double = value.as_double();
            if (constraints.has_field(nmos::fields::nc::minimum) && !nmos::fields::nc::minimum(constraints).is_null())
            {
                auto min = nmos::fields::nc::minimum(constraints).as_double();
                if (0 != std::fmod(value_double - min, step)) { return false; }
            }
            else if (constraints.has_field(nmos::fields::nc::maximum) && !nmos::fields::nc::maximum(constraints).is_null())
            {
                auto max = nmos::fields::nc::maximum(constraints).as_double();
                if (0 != std::fmod(max - value_double, step)) { return false; }
            }
            else
            {
                if (0 != std::fmod(value_double, step)) { return false; }
            }
        }
        if (constraints.has_field(nmos::fields::nc::minimum) && !nmos::fields::nc::minimum(constraints).is_null())
        {
            if (!value.is_integer() || value.as_double() < nmos::fields::nc::minimum(constraints).as_double()) { return false; }
        }
        if (constraints.has_field(nmos::fields::nc::maximum) && !nmos::fields::nc::maximum(constraints).is_null())
        {
            if (!value.is_integer() || value.as_double() > nmos::fields::nc::maximum(constraints).as_double()) { return false; }
        }

        // is string constraints
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsstring
        if (constraints.has_field(nmos::fields::nc::max_characters) && !constraints.at(nmos::fields::nc::max_characters).is_null())
        {
            const auto max_characters = nmos::fields::nc::max_characters(constraints);
            if (!value.is_string() || value.as_string().length() > max_characters) { return false; }
        }
        if (constraints.has_field(nmos::fields::nc::pattern) && !constraints.at(nmos::fields::nc::pattern).is_null())
        {
            if (!value.is_string()) { return false; }
            const auto value_string = utility::us2s(value.as_string());
            bst::regex pattern(utility::us2s(nmos::fields::nc::pattern(constraints)));
            if (!bst::regex_match(value_string, pattern)) { return false; }
        }

        return true;
    }

    // multiple levels of constraints validation
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
    bool constraints_validation(const web::json::value& value, const web::json::value& runtime_property_constraints, const web::json::value& property_constraints, const web::json::value& datatype_constraints)
    {
        // do level 2 runtime property constraints validation
        if (!runtime_property_constraints.is_null()) { return constraints_validation(value, runtime_property_constraints); }

        // do level 1 property constraints validation
        if (!property_constraints.is_null()) { return constraints_validation(value, property_constraints); }

        // do level 0 datatype constraints validation
        if (!datatype_constraints.is_null()) { return constraints_validation(value, datatype_constraints); }

        // reaching here, no validation is required
        return true;
    }
}
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

        // get the runtime property constraints of a specific property_id
        web::json::value get_runtime_property_constraints(const nc_property_id& property_id, const web::json::value& runtime_property_constraints)
        {
            using web::json::value;

            if (!runtime_property_constraints.is_null())
            {
                auto& runtime_prop_constraints = runtime_property_constraints.as_array();
                auto found_constraints = std::find_if(runtime_prop_constraints.begin(), runtime_prop_constraints.end(), [&property_id](const web::json::value& constraints)
                {
                    return property_id == parse_nc_property_id(nmos::fields::nc::property_id(constraints));
                });

                if (runtime_prop_constraints.end() != found_constraints)
                {
                    return *found_constraints;
                }
            }
            return value::null();
        }

        // get the datatype descriptor of a specific type_name
        web::json::value get_datatype_descriptor(const web::json::value& type_name, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor)
        {
            using web::json::value;

            if (!type_name.is_null())
            {
                return get_control_protocol_datatype_descriptor(type_name.as_string()).descriptor;
            }
            return value::null();
        }

        // get the datatype property constraints of a specific type_name
        web::json::value get_datatype_constraints(const web::json::value& type_name, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor)
        {
            using web::json::value;

            // NcDatatypeDescriptor
            const auto& datatype_descriptor = get_datatype_descriptor(type_name, get_control_protocol_datatype_descriptor);
            if (!datatype_descriptor.is_null())
            {
                return nmos::fields::nc::constraints(datatype_descriptor);
            }
            return value::null();
        }

        // constraints validation, may throw nmos::control_protocol_exception
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsnumber
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsstring
        void constraints_validation(const web::json::value& data, const web::json::value& constraints)
        {
            auto parameter_constraints_validation = [&constraints](const web::json::value& value)
            {
                // is numeric constraints
                // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsnumber
                if (constraints.has_field(nmos::fields::nc::step) && !nmos::fields::nc::step(constraints).is_null())
                {
                    if (value.is_null()) { throw control_protocol_exception("value is null"); }

                    if (!value.is_integer()) { throw control_protocol_exception("value is not an integer"); }

                    const auto step = nmos::fields::nc::step(constraints).as_double();
                    if (step <= 0) { throw control_protocol_exception("step is not a positive integer"); }

                    const auto value_double = value.as_double();
                    if (constraints.has_field(nmos::fields::nc::minimum) && !nmos::fields::nc::minimum(constraints).is_null())
                    {
                        auto min = nmos::fields::nc::minimum(constraints).as_double();
                        if (0 != std::fmod(value_double - min, step)) { throw control_protocol_exception("value is not divisible by step"); }
                    }
                    else if (constraints.has_field(nmos::fields::nc::maximum) && !nmos::fields::nc::maximum(constraints).is_null())
                    {
                        auto max = nmos::fields::nc::maximum(constraints).as_double();
                        if (0 != std::fmod(max - value_double, step)) { throw control_protocol_exception("value is not divisible by step"); }
                    }
                    else
                    {
                        if (0 != std::fmod(value_double, step)) { throw control_protocol_exception("value is not divisible by step"); }
                    }
                }
                if (constraints.has_field(nmos::fields::nc::minimum) && !nmos::fields::nc::minimum(constraints).is_null())
                {
                    if (value.is_null()) { throw control_protocol_exception("value is null"); }

                    if (!value.is_integer() || value.as_double() < nmos::fields::nc::minimum(constraints).as_double()) { throw control_protocol_exception("value is less than minimum"); }
                }
                if (constraints.has_field(nmos::fields::nc::maximum) && !nmos::fields::nc::maximum(constraints).is_null())
                {
                    if (value.is_null()) { throw control_protocol_exception("value is null"); }

                    if (!value.is_integer() || value.as_double() > nmos::fields::nc::maximum(constraints).as_double()) { throw control_protocol_exception("value is greater than maximum"); }
                }

                // is string constraints
                // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncparameterconstraintsstring
                if (constraints.has_field(nmos::fields::nc::max_characters) && !constraints.at(nmos::fields::nc::max_characters).is_null())
                {
                    if (value.is_null()) { throw control_protocol_exception("value is null"); }

                    const size_t max_characters = nmos::fields::nc::max_characters(constraints);
                    if (!value.is_string() || value.as_string().length() > max_characters) { throw control_protocol_exception("value is longer than maximum characters"); }
                }
                if (constraints.has_field(nmos::fields::nc::pattern) && !constraints.at(nmos::fields::nc::pattern).is_null())
                {
                    if (value.is_null()) { throw control_protocol_exception("value is null"); }

                    if (!value.is_string()) { throw control_protocol_exception("value is not a string"); }
                    const auto value_string = utility::us2s(value.as_string());
                    bst::regex pattern(utility::us2s(nmos::fields::nc::pattern(constraints)));
                    if (!bst::regex_match(value_string, pattern)) { throw control_protocol_exception("value dose not match the pattern"); }
                }

                // reaching here, parameter validation successfully
            };

            if (data.is_array())
            {
                for (const auto& value : data.as_array())
                {
                    parameter_constraints_validation(value);
                }
            }
            else
            {
                parameter_constraints_validation(data);
            }
        }

        // level 0 datatype constraints validation, may throw nmos::control_protocol_exception
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
        void datatype_constraints_validation(const web::json::value& data, const datatype_constraints_validation_parameters& params)
        {
            auto parameter_constraints_validation = [&params](const web::json::value& value_)
            {
                // no constraints validation required
                if (params.datatype_descriptor.is_null()) { return; }

                const auto& datatype_type = nmos::fields::nc::type(params.datatype_descriptor);

                // do NcDatatypeDescriptorPrimitive constraints validation
                if (nc_datatype_type::Primitive == datatype_type)
                {
                    // hmm, for the primitive type, it should not have datatype constraints specified via the datatype_descriptor but just in case
                    const auto& datatype_constraints = nmos::fields::nc::constraints(params.datatype_descriptor);
                    if (datatype_constraints.is_null())
                    {
                        auto primitive_validation = [](const nc_name& name, const web::json::value& value)
                            {
                                auto is_int16 = [](int32_t value)
                                    {
                                        return value >= (std::numeric_limits<int16_t>::min)()
                                            && value <= (std::numeric_limits<int16_t>::max)();
                                    };
                                auto is_uint16 = [](uint32_t value)
                                    {
                                        return value >= (std::numeric_limits<uint16_t>::min)()
                                            && value <= (std::numeric_limits<uint16_t>::max)();
                                    };
                                auto is_float32 = [](double value)
                                    {
                                        return value >= (std::numeric_limits<float_t>::lowest)()
                                            && value <= (std::numeric_limits<float_t>::max)();
                                    };

                                if (U("NcBoolean") == name) { return value.is_boolean(); }
                                if (U("NcInt16") == name && value.is_number()) { return is_int16(value.as_number().to_int32()); }
                                if (U("NcInt32") == name && value.is_number()) { return value.as_number().is_int32(); }
                                if (U("NcInt64") == name && value.is_number()) { return value.as_number().is_int64(); }
                                if (U("NcUint16") == name && value.is_number()) { return is_uint16(value.as_number().to_uint32()); }
                                if (U("NcUint32") == name && value.is_number()) { return value.as_number().is_uint32(); }
                                if (U("NcUint64") == name && value.is_number()) { return value.as_number().is_uint64(); }
                                if (U("NcFloat32") == name && value.is_number()) { return is_float32(value.as_number().to_double()); }
                                if (U("NcFloat64") == name && value.is_number()) { return !value.as_number().is_integral(); }
                                if (U("NcString") == name) { return value.is_string(); }

                                // invalid primitive type
                                return false;
                            };

                        // do primitive type constraints validation
                        const auto& name = nmos::fields::nc::name(params.datatype_descriptor);
                        if (!primitive_validation(name, value_))
                        {
                            throw control_protocol_exception("value is not a " + utility::us2s(name) + " type");;
                        }
                    }
                    else
                    {
                        constraints_validation(value_, datatype_constraints);
                    }

                    return;
                }

                // do NcDatatypeDescriptorTypeDef constraints validation
                if (nc_datatype_type::Typedef == datatype_type)
                {
                    // do the datatype constraints specified via the datatype_descriptor if presented
                    const auto& datatype_constraints = nmos::fields::nc::constraints(params.datatype_descriptor);
                    if (datatype_constraints.is_null())
                    {
                        // do parent typename constraints validation
                        const auto& type_name = params.datatype_descriptor.at(nmos::fields::nc::parent_type); // parent type_name
                        datatype_constraints_validation(value_, { details::get_datatype_descriptor(type_name, params.get_control_protocol_datatype_descriptor), params.get_control_protocol_datatype_descriptor });
                    }
                    else
                    {
                        constraints_validation(value_, datatype_constraints);
                    }

                    return;
                }

                // do NcDatatypeDescriptorEnum constraints validation
                if (nc_datatype_type::Enum == datatype_type)
                {
                    const auto& items = nmos::fields::nc::items(params.datatype_descriptor);
                    if (items.end() == std::find_if(items.begin(), items.end(), [&](const web::json::value& nc_enum_item_descriptor) { return nmos::fields::nc::value(nc_enum_item_descriptor) == value_; }))
                    {
                        const auto& name = nmos::fields::nc::name(params.datatype_descriptor);
                        throw control_protocol_exception("value is not an enum " + utility::us2s(name) + " type");
                    }

                    return;
                }

                // do NcDatatypeDescriptorStruct constraints validation
                if (nc_datatype_type::Struct == datatype_type)
                {
                    const auto& datatype_name = nmos::fields::nc::name(params.datatype_descriptor);
                    const auto& fields = nmos::fields::nc::fields(params.datatype_descriptor);
                    // NcFieldDescriptor
                    for (const web::json::value& nc_field_descriptor : fields)
                    {
                        const auto& field_name = nmos::fields::nc::name(nc_field_descriptor);
                        // is field in strurcture
                        if (!value_.has_field(field_name)) { throw control_protocol_exception("missing " + utility::us2s(field_name) + " in " + utility::us2s(datatype_name)); }

                        // is field nullable
                        if (nmos::fields::nc::is_nullable(nc_field_descriptor) != value_.is_null()) { throw control_protocol_exception(utility::us2s(field_name) + " is not nullable"); }

                        // is field sequenceable
                        if (nmos::fields::nc::is_sequence(nc_field_descriptor) != value_.is_array()) { throw control_protocol_exception(utility::us2s(field_name) + " is not sequenceable"); }

                        // check against field constraints if presented
                        const auto& constraints = nmos::fields::nc::constraints(nc_field_descriptor);
                        if (constraints.is_null())
                        {
                            // no field constraints, move to check the constraints of its typeName
                            const auto& field_type_name = nc_field_descriptor.at(nmos::fields::nc::type_name);

                            if (!field_type_name.is_null())
                            {
                                auto value = value_.at(field_name);

                                if (value.is_array())
                                {
                                    for (const auto& val : value.as_array())
                                    {
                                        // do typename constraints validation
                                        datatype_constraints_validation(val, { details::get_datatype_descriptor(field_type_name, params.get_control_protocol_datatype_descriptor), params.get_control_protocol_datatype_descriptor });
                                    }
                                }
                                else
                                {
                                    // do typename constraints validation
                                    datatype_constraints_validation(value, { details::get_datatype_descriptor(field_type_name, params.get_control_protocol_datatype_descriptor), params.get_control_protocol_datatype_descriptor });
                                }
                            }
                        }
                        else
                        {
                            // do field constraints validation
                            const auto& value = value_.at(field_name);
                            constraints_validation(value, constraints);
                        }
                    }
                    // unsupported datatype_type, no validation is required
                    return;
                }
            };

            if (data.is_array())
            {
                for (const auto& value : data.as_array())
                {
                    parameter_constraints_validation(value);
                }
            }
            else
            {
                parameter_constraints_validation(data);
            }
        }

        // multiple levels of constraints validation, may throw nmos::control_protocol_exception
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
        void constraints_validation(const web::json::value& data, const web::json::value& runtime_property_constraints, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params)
        {
            // do level 2 runtime property constraints validation
            if (!runtime_property_constraints.is_null()) { constraints_validation(data, runtime_property_constraints); return; }

            // do level 1 property constraints validation
            if (!property_constraints.is_null()) { constraints_validation(data, property_constraints); return; }

            // do level 0 datatype constraints validation
            datatype_constraints_validation(data, params);
        }

        // method parameter constraints validation, may throw nmos::control_protocol_exception
        void method_parameter_constraints_validation(const web::json::value& data, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params)
        {
            using web::json::value;

            // do level 1 property constraints & level 0 datatype constraints validation
            constraints_validation(data, value::null(), property_constraints, params);
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

    // find control class property descriptor (NcPropertyDescriptor)
    web::json::value find_property_descriptor(const nc_property_id& property_id, const nc_class_id& class_id_, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
    {
        using web::json::value;

        auto class_id = class_id_;

        while (!class_id.empty())
        {
            const auto& control_class = get_control_protocol_class_descriptor(class_id);
            auto& property_descriptors = control_class.property_descriptors.as_array();
            auto found = std::find_if(property_descriptors.begin(), property_descriptors.end(), [&property_id](const web::json::value& property_descriptor)
            {
                return (property_id == nmos::details::parse_nc_property_id(nmos::fields::nc::id(property_descriptor)));
            });
            if (property_descriptors.end() != found) { return *found; }

            class_id.pop_back();
        }

        return value::null();
    }

    // get block member descriptors
    void get_member_descriptors(const resources& resources, const resource& resource, bool recurse, web::json::array& descriptors)
    {
        if (resource.data.has_field(nmos::fields::nc::members))
        {
            const auto& members = nmos::fields::nc::members(resource.data);

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
                        const auto& found = find_resource(resources, utility::s2us(std::to_string(oid)));
                        if (resources.end() != found)
                        {
                            get_member_descriptors(resources, *found, recurse, descriptors);
                        }
                    }
                }
            }
        }
    }

    // find members with given role name or fragment
    void find_members_by_role(const resources& resources, const resource& resource, const utility::string_t& role, bool match_whole_string, bool case_sensitive, bool recurse, web::json::array& descriptors)
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

        if (resource.data.has_field(nmos::fields::nc::members))
        {
            const auto& members = nmos::fields::nc::members(resource.data);

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
                        const auto& found = find_resource(resources, utility::s2us(std::to_string(oid)));
                        if (resources.end() != found)
                        {
                            find_members_by_role(resources, *found, role, match_whole_string, case_sensitive, recurse, descriptors);
                        }
                    }
                }
            }
        }
    }

    // find members with given class id
    void find_members_by_class_id(const resources& resources, const nmos::resource& resource, const nc_class_id& class_id_, bool include_derived, bool recurse, web::json::array& descriptors)
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

        if (resource.data.has_field(nmos::fields::nc::members))
        {
            auto& members = nmos::fields::nc::members(resource.data);

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
                        const auto& found = find_resource(resources, utility::s2us(std::to_string(oid)));
                        if (resources.end() != found)
                        {
                            find_members_by_class_id(resources, *found, class_id_, include_derived, recurse, descriptors);
                        }
                    }
                }
            }
        }
    }

    // push a control protocol resource into other control protocol NcBlock resource
    void push_back(control_protocol_resource& nc_block_resource, const control_protocol_resource& resource)
    {
        // note, model write lock should aleady be applied by the outer function, so access to control_protocol_resources is OK...

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
        // note, model write lock should aleady be applied by the outer function, so access to control_protocol_resources is OK...

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

    // method parameters constraints validation
    void method_parameters_contraints_validation(const web::json::value& arguments, const web::json::value& nc_method_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor)
    {
        for (const auto& param : nmos::fields::nc::parameters(nc_method_descriptor))
        {
            const auto& name = nmos::fields::nc::name(param);
            const auto& constraints = nmos::fields::nc::constraints(param);
            const auto& type_name = param.at(nmos::fields::nc::type_name);
            if (arguments.is_null() || !arguments.has_field(name))
            {
                // missing argument parameter
                throw control_protocol_exception("missing argument parameter " + utility::us2s(name));
            }
            details::method_parameter_constraints_validation(arguments.at(name), constraints, { nmos::details::get_datatype_descriptor(type_name, get_control_protocol_datatype_descriptor), get_control_protocol_datatype_descriptor });
        }
    }
}

#include "nmos/control_protocol_ws_api.h"

#include <boost/range/join.hpp>
#include "cpprest/json_validator.h"
#include "cpprest/regex_utils.h"
#include "nmos/api_utils.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/is12_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/slog.h"
#include "nmos/resources.h"

namespace nmos
{
    namespace details
    {
        static const web::json::experimental::json_validator& controlprotocol_validator()
        {
            // hmm, could be based on supported API versions from settings, like other APIs' validators?
            static const web::json::experimental::json_validator validator
            {
                nmos::experimental::load_json_schema,
                boost::copy_range<std::vector<web::uri>>(boost::join(boost::join(
                    is12_versions::all | boost::adaptors::transformed(experimental::make_controlprotocolapi_base_message_schema_uri),
                    is12_versions::all | boost::adaptors::transformed(experimental::make_controlprotocolapi_command_message_schema_uri)),
                    is12_versions::all | boost::adaptors::transformed(experimental::make_controlprotocolapi_subscription_message_schema_uri)
                ))
            };
            return validator;
        }

        // Validate against specification schema
        // throws web::json::json_exception on failure, which results in a 400 Badly-formed command
        void validate_controlprotocolapi_base_message_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            controlprotocol_validator().validate(request_data, experimental::make_controlprotocolapi_base_message_schema_uri(version));
        }
        void validate_controlprotocolapi_command_message_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            controlprotocol_validator().validate(request_data, experimental::make_controlprotocolapi_command_message_schema_uri(version));
        }
        void validate_controlprotocolapi_subscription_message_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            controlprotocol_validator().validate(request_data, experimental::make_controlprotocolapi_subscription_message_schema_uri(version));
        }

        std::pair<web::json::value, methods> create_properties_methods(nmos::node_model& model, const nc_class_id& class_id_, const nmos::experimental::control_classes& control_classes)
        {
            using web::json::value;
            using web::json::value_of;

            // hmm, methods should also be passing in via the control_class::methods

            // NcObject methods implementation
            // Get property value
            const auto get = [&model](const web::json::array& properties, int32_t handle, int32_t oid, const value& arguments)
            {
                // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

                auto& resources = model.control_protocol_resources;
                auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != resource)
                {
                    // where arguments is the property id = (level, index)
                    const auto& property_id = nmos::fields::nc::id(arguments);

                    // find the relevant nc_property_descriptor
                    auto property_found = std::find_if(properties.begin(), properties.end(), [property_id](const web::json::value& property)
                    {
                        return property_id == nmos::fields::nc::id(property);
                    });

                    if (property_found != properties.end())
                    {
                        return details::make_control_protocol_response(handle, { details::nc_method_status::ok }, resource->data.at(nmos::fields::nc::name(*property_found)));
                    }

                    // unknown property
                    utility::stringstream_t ss;
                    ss << U("unknown property: ") << property_id.serialize() << " to do Get";
                    return details::make_control_protocol_error_response(handle, { details::nc_method_status::property_not_implemented }, ss.str());
                }

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to do Get";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };
            // Set property value
            const auto set = [&model](const web::json::array& properties, int32_t handle, int32_t oid, const value& arguments)
            {
                // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

                auto& resources = model.control_protocol_resources;
                auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != resource)
                {
                    const auto& property_id = nmos::fields::nc::id(arguments);
                    const auto& val = nmos::fields::nc::value(arguments);

                    // find the relevant nc_property_descriptor
                    auto property_found = std::find_if(properties.begin(), properties.end(), [property_id](const web::json::value& property)
                    {
                        return property_id == nmos::fields::nc::id(property);
                    });
                    if (property_found != properties.end())
                    {
                        if (nmos::fields::nc::is_read_only(*property_found))
                        {
                            return details::make_control_protocol_response(handle, { details::nc_method_status::read_only });
                        }

                        if ((val.is_null() && !nmos::fields::nc::is_nullable(*property_found))
                            || (val.is_array() && !nmos::fields::nc::is_sequence(*property_found)))
                        {
                            return details::make_control_protocol_response(handle, { details::nc_method_status::parameter_error });
                        }

                        resources.modify(resource, [&](nmos::resource& resource)
                        {
                            resource.data[nmos::fields::nc::name(*property_found)] = val;

                            resource.updated = strictly_increasing_update(resources);
                        });
                        return details::make_control_protocol_response(handle, { details::nc_method_status::ok });
                    }

                    // unknown property
                    utility::stringstream_t ss;
                    ss << U("unknown property: ") << property_id.serialize() << " to do Set";
                    return details::make_control_protocol_error_response(handle, { details::nc_method_status::property_not_implemented }, ss.str());
                }

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to do Set";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };
            // Get sequence item
            const auto get_sequence_item = [&model](const web::json::array& properties, int32_t handle, int32_t oid, const value& arguments)
            {
                // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

                auto& resources = model.control_protocol_resources;
                auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != resource)
                {
                    const auto& property_id = nmos::fields::nc::id(arguments);
                    const auto& index = nmos::fields::nc::index(arguments);

                    // find the relevant nc_property_descriptor
                    auto property_found = std::find_if(properties.begin(), properties.end(), [property_id](const web::json::value& property)
                    {
                        return property_id == nmos::fields::nc::id(property);
                    });

                    if (property_found != properties.end())
                    {
                        if (!nmos::fields::nc::is_sequence(*property_found))
                        {
                            // property is not a sequence
                            utility::stringstream_t ss;
                            ss << U("property: ") << property_id.serialize() << " is not a sequence to do GetSequenceItem";
                            return details::make_control_protocol_error_response(handle, { details::nc_method_status::invalid_request }, ss.str());
                        }

                        auto& data = resource->data.at(nmos::fields::nc::name(*property_found));

                        if (!data.is_null() && data.as_array().size() > index)
                        {
                            return details::make_control_protocol_response(handle, { details::nc_method_status::ok }, data.at(index));
                        }

                        // out of bound
                        utility::stringstream_t ss;
                        ss << U("property: ") << property_id.serialize() << " is outside the available range to do GetSequenceItem";
                        return details::make_control_protocol_error_response(handle, { details::nc_method_status::index_out_of_bounds }, ss.str());
                    }

                    // unknown property
                    utility::stringstream_t ss;
                    ss << U("unknown property: ") << property_id.serialize(
                    ) << " to do GetSequenceItem";
                    return details::make_control_protocol_error_response(handle, { details::nc_method_status::property_not_implemented }, ss.str());
                }

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to do GetSequenceItem";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };
            // Set sequence item
            const auto set_sequence_item = [&model](const web::json::array& properties, int32_t handle, int32_t oid, const value& arguments)
            {
                // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

                auto& resources = model.control_protocol_resources;
                auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != resource)
                {
                    const auto& property_id = nmos::fields::nc::id(arguments);
                    const auto& index = nmos::fields::nc::index(arguments);
                    const auto& val = nmos::fields::nc::value(arguments);

                    // find the relevant nc_property_descriptor
                    auto property_found = std::find_if(properties.begin(), properties.end(), [property_id](const web::json::value& property)
                    {
                        return property_id == nmos::fields::nc::id(property);
                    });

                    if (!nmos::fields::nc::is_sequence(*property_found))
                    {
                        // property is not a sequence
                        utility::stringstream_t ss;
                        ss << U("property: ") << property_id.serialize() << " is not a sequence to do SetSequenceItem";
                        return details::make_control_protocol_error_response(handle, { details::nc_method_status::invalid_request }, ss.str());
                    }

                    auto& data = resource->data.at(nmos::fields::nc::name(*property_found));

                    if (!data.is_null() && data.as_array().size() > index)
                    {
                        resources.modify(resource, [&](nmos::resource& resource)
                        {
                            resource.data[nmos::fields::nc::name(*property_found)][index] = val;

                            resource.updated = strictly_increasing_update(resources);
                        });
                        return details::make_control_protocol_response(handle, { details::nc_method_status::ok });
                    }

                    // out of bound
                    utility::stringstream_t ss;
                    ss << U("property: ") << property_id.serialize() << " is outside the available range to do SetSequenceItem";
                    return details::make_control_protocol_error_response(handle, { details::nc_method_status::index_out_of_bounds }, ss.str());
                }

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to do SetSequenceItem";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };
            // Add item to sequence
            const auto add_sequence_item = [&model](const web::json::array& properties, int32_t handle, int32_t oid, const value& arguments)
            {
                // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

                auto& resources = model.control_protocol_resources;
                auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != resource)
                {
                    const auto& property_id = nmos::fields::nc::id(arguments);
                    const auto& val = nmos::fields::nc::value(arguments);

                    // find the relevant nc_property_descriptor
                    auto property_found = std::find_if(properties.begin(), properties.end(), [property_id](const web::json::value& property)
                    {
                        return property_id == nmos::fields::nc::id(property);
                    });

                    if (!nmos::fields::nc::is_sequence(*property_found))
                    {
                        // property is not a sequence
                        utility::stringstream_t ss;
                        ss << U("property: ") << property_id.serialize() << " is not a sequence to do AddSequenceItem";
                        return details::make_control_protocol_error_response(handle, { details::nc_method_status::invalid_request }, ss.str());
                    }

                    auto& data = resource->data.at(nmos::fields::nc::name(*property_found));

                    resources.modify(resource, [&](nmos::resource& resource)
                    {
                        auto& sequence = resource.data[nmos::fields::nc::name(*property_found)];
                        if (data.is_null()) { sequence = value::array(); }
                        web::json::push_back(sequence, val);

                        resource.updated = strictly_increasing_update(resources);
                    });
                    return details::make_control_protocol_response(handle, { details::nc_method_status::ok }, data.as_array().size() - 1);
                }

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to do AddSequenceItem";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };
            // Delete sequence item
            const auto remove_sequence_item = [&model](const web::json::array& properties, int32_t handle, int32_t oid, const value& arguments)
            {
                // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

                auto& resources = model.control_protocol_resources;
                auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != resource)
                {
                    const auto& property_id = nmos::fields::nc::id(arguments);
                    const auto& index = nmos::fields::nc::index(arguments);

                    // find the relevant nc_property_descriptor
                    auto property_found = std::find_if(properties.begin(), properties.end(), [property_id](const web::json::value& property)
                    {
                        return property_id == nmos::fields::nc::id(property);
                    });

                    if (!nmos::fields::nc::is_sequence(*property_found))
                    {
                        // property is not a sequence
                        utility::stringstream_t ss;
                        ss << U("property: ") << property_id.serialize() << " is not a sequence to do RemoveSequenceItem";
                        return details::make_control_protocol_error_response(handle, { details::nc_method_status::invalid_request }, ss.str());
                    }

                    auto& data = resource->data.at(nmos::fields::nc::name(*property_found));

                    if (!data.is_null() && data.as_array().size() > index)
                    {
                        resources.modify(resource, [&](nmos::resource& resource)
                        {
                            auto& sequence = resource.data[nmos::fields::nc::name(*property_found)].as_array();
                            sequence.erase(index);

                            resource.updated = strictly_increasing_update(resources);
                        });
                        return details::make_control_protocol_response(handle, { details::nc_method_status::ok });
                    }

                    // out of bound
                    utility::stringstream_t ss;
                    ss << U("property: ") << property_id.serialize() << " is outside the available range to do RemoveSequenceItem";
                    return details::make_control_protocol_error_response(handle, { details::nc_method_status::index_out_of_bounds }, ss.str());
                }

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to do RemoveSequenceItem";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };
            // Get sequence length
            const auto get_sequence_length = [&model](const web::json::array& properties, int32_t handle, int32_t oid, const value& arguments)
            {
                // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

                auto& resources = model.control_protocol_resources;
                auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != resource)
                {
                    const auto& property_id = nmos::fields::nc::id(arguments);

                    // find the relevant nc_property_descriptor
                    auto property_found = std::find_if(properties.begin(), properties.end(), [property_id](const web::json::value& property)
                    {
                        return property_id == nmos::fields::nc::id(property);
                    });

                    if (property_found != properties.end())
                    {
                        if (nmos::fields::nc::is_sequence(*property_found))
                        {
                            auto& data = resource->data.at(nmos::fields::nc::name(*property_found));

                            if (nmos::fields::nc::is_nullable(*property_found))
                            {
                                // can be null
                                if (data.is_null())
                                {
                                    // null
                                    return details::make_control_protocol_response(handle, { details::nc_method_status::ok }, value::null());
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
                                    return details::make_control_protocol_error_response(handle, { details::nc_method_status::invalid_request }, ss.str());
                                }
                            }
                            return details::make_control_protocol_response(handle, { details::nc_method_status::ok }, data.as_array().size());
                        }
                        else
                        {
                            // property is not a sequence
                            utility::stringstream_t ss;
                            ss << U("property: ") << property_id.serialize() << " is not a sequence to do GetSequenceLength";
                            return details::make_control_protocol_error_response(handle, { details::nc_method_status::invalid_request }, ss.str());
                        }
                    }

                    // unknown property
                    utility::stringstream_t ss;
                    ss << U("unknown property: ") << property_id.serialize() << " to do GetSequenceLength";
                    return details::make_control_protocol_error_response(handle, { details::nc_method_status::property_not_implemented }, ss.str());
                }

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to do GetSequenceLength";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };

            // NcBlock methods implementation
            // Gets descriptors of members of the block
            const auto get_member_descriptors = [&model](const web::json::array&, int32_t handle, int32_t oid, const value& arguments)
            {
                // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

                auto& resources = model.control_protocol_resources;
                auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != resource)
                {
                    // where arguments is the boolean recurse value
                    // hmm, If recurse is set to true, nested members is to be retrieved
                    const auto& recurse = nmos::fields::nc::recurse(arguments);

                    return details::make_control_protocol_response(handle, { details::nc_method_status::ok }, resource->data.at(nmos::fields::nc::members));
                }

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to do GetMemberDescriptors";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };
            // Finds member(s) by path
            const auto find_members_by_path = [&model](const web::json::array&, int32_t handle, int32_t oid, const value& arguments)
            {
                // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

                auto& resources = model.control_protocol_resources;

                auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != resource)
                {
                    // Relative path to search for (MUST not include the role of the block targeted by oid)
                    const auto& path = nmos::fields::nc::path(arguments);

                    if (0 == path.size())
                    {
                        // empty path
                        return details::make_control_protocol_error_response(handle, { details::nc_method_status::parameter_error }, U("empty path to do FindMembersByPath"));
                    }

                    value nc_block_member_descriptor;

                    for (const auto& role : path)
                    {
                        // look for the role in members
                        if (resource->data.has_field(nmos::fields::nc::members))
                        {
                            auto& members = nmos::fields::nc::members(resource->data);
                            auto member_found = std::find_if(members.begin(), members.end(), [&](const web::json::value& nc_block_member_descriptor)
                            {
                                return role.as_string() == nmos::fields::nc::role(nc_block_member_descriptor);
                            });

                            if (members.end() != member_found)
                            {
                                nc_block_member_descriptor = *member_found;

                                // use oid to look for next resource
                                resource = nmos::find_resource(resources, utility::s2us(std::to_string(nmos::fields::nc::oid(*member_found))));
                            }
                            else
                            {
                                // should
                                // no role
                                utility::stringstream_t ss;
                                ss << U("role: ") << role.as_string() << U(" not found to do FindMembersByPath");
                                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
                            }
                        }
                        else
                        {
                            // should
                            // no members
                            return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, U("no members to do FindMembersByPath"));
                        }
                    }

                    return details::make_control_protocol_response(handle, { details::nc_method_status::ok }, nc_block_member_descriptor);
                }

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to do FindMembersByPath";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };

            // NcClassManager methods implementation
            // Get a single class descriptor
            const auto get_control_class = [&model](const web::json::array& properties, int32_t handle, int32_t oid, const value& arguments)
            {
                // hmm, todo

                // resource not found for the given oid
                utility::stringstream_t ss;
                ss << U("unknown oid: ") << oid << " to get control class";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
            };

            // method handlers for the different classes
            details::methods nc_object_method_handlers; // method_id vs NcObject method_handler
            details::methods nc_block_method_handlers; // method_id vs NcBlock method_handler
            details::methods nc_worker_method_handlers; // method_id vs NcWorker method_handler
            details::methods nc_manager_method_handlers; // method_id vs NcManager method_handler
            details::methods nc_device_manager_method_handlers; // method_id vs NcDeviceManager method_handler
            details::methods nc_class_manager_method_handlers; // method_id vs NcClassManager method_handler

            // NcObject methods
            // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncobject
            nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 1 } })] = get;
            nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 2 } })] = set;
            nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 3 } })] = get_sequence_item;
            nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 4 } })] = set_sequence_item;
            nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 5 } })] = add_sequence_item;
            nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 6 } })] = remove_sequence_item;
            nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 7 } })] = get_sequence_length;

            // NcBlock methods
            // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncblock
            nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 1 } })] = get_member_descriptors;
            nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 2 } })] = find_members_by_path;
            //nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 3 } })] = find_members_by_role;
            //nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 4 } })] = find_members_by_class_id;

            // NcWorker has no extended method
            // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncworker

            // NcManager has no extended method
            // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmanager

            // NcDeviceManger has no extended method
            // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdevicemanager

            // NcClassManager methods
            // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassmanager
            //nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 1 } })] = get_control_class;
            //nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 2 } })] = get_datatype;

            value properties = value::array(); // combined base classes nc_property_descriptor(s) for the required class_id
            details::methods methods; // list of combined base classes method handlers

            auto found_class = control_classes.find(make_nc_class_id(class_id_));
            if (control_classes.end() != found_class)
            {
                // hmm, update the array of properties, will be updated the list of method handlers
                auto insert_properties = [&properties, &control_classes](const nc_class_id& class_id_)
                {
                    auto class_id = make_nc_class_id(class_id_);
                    auto found = control_classes.find(class_id);
                    if (control_classes.end() != found)
                    {
                        auto& nc_class_properties = found->second.properties.as_array();
                        for (auto& nc_class_property : nc_class_properties)
                        {
                            web::json::push_back(properties, nc_class_property);
                        }
                    }
                };

                auto class_id = class_id_;
                while (class_id.size())
                {
                    insert_properties(class_id);

                    // hmm, to be deleted, once the methods are passed in
                    if (details::nc_object_class_id == class_id)
                    {
                        methods.insert(nc_object_method_handlers.begin(), nc_object_method_handlers.end());
                    }
                    else if (details::nc_block_class_id == class_id)
                    {
                        methods.insert(nc_block_method_handlers.begin(), nc_block_method_handlers.end());
                    }
                    else if (details::nc_device_manager_class_id == class_id)
                    {
                        methods.insert(nc_manager_method_handlers.begin(), nc_manager_method_handlers.end());
                    }
                    else if (details::nc_device_manager_class_id == class_id)
                    {
                        methods.insert(nc_device_manager_method_handlers.begin(), nc_device_manager_method_handlers.end());
                    }
                    else if (details::nc_class_manager_class_id == class_id)
                    {
                        methods.insert(nc_class_manager_method_handlers.begin(), nc_class_manager_method_handlers.end());
                    }
                    class_id.pop_back();
                }
            }
            else
            {
                throw std::runtime_error("unknown control class");
            }

            return { properties, methods };
        }
    }

    // IS-12 Control Protocol WebSocket API

    web::websockets::experimental::listener::validate_handler make_control_protocol_ws_validate_handler(nmos::node_model& model, slog::base_gate& gate_)
    {
        return [&model, &gate_](web::http::http_request req)
        {
            nmos::ws_api_gate gate(gate_, req.request_uri());

            // RFC 6750 defines two methods of sending bearer access tokens which are applicable to WebSocket
            // Clients SHOULD use the "Authorization Request Header Field" method.
            // Clients MAY use a "URI Query Parameter".
            // See https://tools.ietf.org/html/rfc6750#section-2

            // For now just return true
            const auto& ws_ncp_path = req.request_uri().path();
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating websocket connection to: " << ws_ncp_path;

            return true;
        };
    }

    web::websockets::experimental::listener::open_handler make_control_protocol_ws_open_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        using web::json::value;
        using web::json::value_of;

        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.control_protocol_resources;

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Opening websocket connection to: " << ws_ncp_path;

            // create a subscription (1-1 relationship with the connection)
            resources::const_iterator subscription;

            {
                const bool secure = nmos::experimental::fields::client_secure(model.settings);

                const auto ws_href = web::uri_builder()
                    .set_scheme(web::ws_scheme(secure))
                    .set_host(nmos::get_host(model.settings))
                    .set_port(nmos::fields::events_ws_port(model.settings))
                    .set_path(ws_ncp_path)
                    .to_uri();

                const bool non_persistent = false;
                value data = value_of({
                    { nmos::fields::id, nmos::make_id() },
                    { nmos::fields::max_update_rate_ms, 0 },
                    { nmos::fields::resource_path, U('/') + nmos::resourceType_from_type(nmos::types::source) },
                    { nmos::fields::params, value_of({ { U("query.rql"), U("in(id,())") } }) },
                    { nmos::fields::persist, non_persistent },
                    { nmos::fields::secure, secure },
                    { nmos::fields::ws_href, ws_href.to_string() }
                }, true);

                // hm, could version be determined from ws_resource_path?
                nmos::resource subscription_{ is12_versions::v1_0, nmos::types::subscription, std::move(data), non_persistent };

                subscription = insert_resource(resources, std::move(subscription_)).first;
            }

            {
                // create a websocket connection resource

                value data;
                nmos::id id = nmos::make_id();
                data[nmos::fields::id] = value::string(id);
                data[nmos::fields::subscription_id] = value::string(subscription->id);

                // create an initial websocket message with no data

                const auto resource_path = nmos::fields::resource_path(subscription->data);
                const auto topic = resource_path + U('/');
                // source_id and flow_id are set per-message depending on the source, unlike Query WebSocket API
                data[nmos::fields::message] = details::make_grain({}, {}, topic);

                resource grain{ is12_versions::v1_0, nmos::types::grain, std::move(data), false };
                insert_resource(resources, std::move(grain));

                websockets.insert({ id, connection_id });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Creating websocket connection: " << id;

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying control protocol websockets thread"; // and anyone else who cares...
                model.notify();
            }
        };
    }

    web::websockets::experimental::listener::close_handler make_control_protocol_ws_close_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, web::websockets::websocket_close_status close_status, const utility::string_t& close_reason)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.control_protocol_resources;

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing websocket connection to: " << ws_ncp_path << " [" << (int)close_status << ": " << close_reason << "]";

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(resources, { websocket->second, nmos::types::grain });

                if (resources.end() != grain)
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Deleting websocket connection";

                    // subscriptions have a 1-1 relationship with the websocket connection and both should now be erased immediately
                    auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });

                    if (resources.end() != subscription)
                    {
                        // this should erase grain too, as a subscription's subresource
                        erase_resource(resources, subscription->id);
                    }
                    else
                    {
                        // a grain without a subscription shouldn't be possible, but let's be tidy
                        erase_resource(resources, grain->id);
                    }
                    //erase_resource(resources, grain->id);
                }

                websockets.right.erase(websocket);

                model.notify();
            }
        };
    }

    web::websockets::experimental::listener::message_handler make_control_protocol_ws_message_handler(nmos::node_model& model, nmos::websockets& websockets, nmos::get_control_protocol_classes_handler get_control_protocol_classes, slog::base_gate& gate_)
    {
        using web::json::value;

        return [&model, &websockets, get_control_protocol_classes, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, const web::websockets::websocket_incoming_message& msg_)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);

            auto lock = model.write_lock();
            auto& resources = model.control_protocol_resources;

            // theoretically blocking, but in fact not
            auto msg = msg_.extract_string().get();

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received websocket message: " << msg << " on connection: " << ws_ncp_path;

            // hmm todo: extract the version from the ws_ncp_path
            const nmos::api_version version = is12_versions::v1_0;
            //const nmos::api_version version = nmos::parse_api_version(ws_ncp_path(nmos::patterns::version.name));

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(resources, { websocket->second, nmos::types::grain });

                if (resources.end() != grain)
                {
                    auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });

                    if (resources.end() != subscription)
                    {
                        try
                        {
                            const auto message = value::parse(utility::conversions::to_string_t(msg));

                            // validate the base-message
                            details::validate_controlprotocolapi_base_message_schema(version, message);

                            const auto msg_type = nmos::fields::nc::message_type(message);
                            switch (msg_type)
                            {
                            case details::nc_message_type::command:
                            {
                                // validate command-message
                                details::validate_controlprotocolapi_command_message_schema(version, message);

                                auto responses = value::array();
                                auto& commands = nmos::fields::nc::commands(message);
                                for (const auto& cmd : commands)
                                {
                                    const auto handle = nmos::fields::nc::handle(cmd);
                                    const auto oid = nmos::fields::nc::oid(cmd);

                                    // get methodId
                                    const auto& method_id = nmos::fields::nc::method_id(cmd);

                                    // get arguments
                                    const auto& arguments = nmos::fields::nc::arguments(cmd);

                                    auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                                    if (resources.end() != resource)
                                    {
                                        // create the combined properties and method handlers based on class_id
                                        auto class_id = details::parse_nc_class_id(resource->data.at(nmos::fields::nc::class_id));
                                        auto properties_methods = details::create_properties_methods(model, class_id, get_control_protocol_classes());
                                        auto& properties = properties_methods.first;
                                        auto& methods = properties_methods.second;

                                        // find the relevent method handler to execute
                                        auto method = methods.find(method_id);
                                        if (method != methods.end())
                                        {
                                            // execute the relevant method handler, then accumulating up their response to reponses
                                            web::json::push_back(responses, method->second(properties.as_array(), handle, oid, arguments));
                                        }
                                        else
                                        {
                                            utility::stringstream_t ss;
                                            ss << U("unsupported method id: ") << method_id.serialize();
                                            web::json::push_back(responses,
                                                details::make_control_protocol_error_response(handle, { details::nc_method_status::method_not_implemented }, ss.str()));
                                        }
                                    }
                                    else
                                    {
                                        // resource not found for the given oid
                                        utility::stringstream_t ss;
                                        ss << U("unknown oid: ") << oid;
                                        web::json::push_back(responses,
                                            details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str()));
                                    }
                                }

                                // add command_response for the control protocol response thread to return to the client
                                resources.modify(grain, [&](nmos::resource& grain)
                                {
                                    web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                        details::make_control_protocol_message_response(details::nc_message_type::command_response, responses));

                                    grain.updated = strictly_increasing_update(resources);
                                });
                            }
                            break;
                            case details::nc_message_type::subscription:
                            {
                                // hmm, todo...
                            }
                                break;
                            default:
                                // unexpected message type
                                break;
                            }

                        }
                        catch (const web::json::json_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();

                            resources.modify(grain, [&](nmos::resource& grain)
                            {
                                web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                    details::make_control_protocol_error_message({ details::nc_method_status::bad_command_format }, utility::s2us(e.what())));

                                grain.updated = strictly_increasing_update(resources);
                            });
                        }
                        catch (const std::exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception while handing control protocol command: " << e.what();

                            resources.modify(grain, [&](nmos::resource& grain)
                            {
                                web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                    details::make_control_protocol_error_message({ details::nc_method_status::bad_command_format },
                                        utility::s2us(std::string("Unexpected exception while handing control protocol command : ") + e.what())));

                                grain.updated = strictly_increasing_update(resources);
                            });
                        }
                        catch (...)
                        {
                            slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception for handing control protocol command";

                            resources.modify(grain, [&](nmos::resource& grain)
                            {
                                web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                    details::make_control_protocol_error_message({ details::nc_method_status::bad_command_format },
                                        U("Unexpected unknown exception while handing control protocol command")));

                                grain.updated = strictly_increasing_update(resources);
                            });
                        }
                        model.notify();
                    }
                }
            }
        };
    }

    // observe_websocket_exception is the same as the one defined in events_ws_api
    namespace details
    {
        struct observe_websocket_exception
        {
            observe_websocket_exception(slog::base_gate& gate) : gate(gate) {}

            void operator()(pplx::task<void> finally)
            {
                try
                {
                    finally.get();
                }
                catch (const web::websockets::websocket_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "WebSocket error: " << e.what() << " [" << e.error_code() << "]";
                }
            }

            slog::base_gate& gate;
        };
    }

    void send_control_protocol_ws_messages_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::send_control_protocol_ws_messages));

        using web::json::value;
        using web::json::value_of;

        // could start out as a shared/read lock, only upgraded to an exclusive/write lock when a grain in the resources is actually modified
        auto lock = model.write_lock();
        auto& condition = model.condition;
        auto& shutdown = model.shutdown;
        auto& resources = model.control_protocol_resources;

        tai most_recent_message{};
        auto earliest_necessary_update = (tai_clock::time_point::max)();

        for (;;)
        {
            // wait for the thread to be interrupted either because there are resource changes, or because the server is being shut down
            // or because message sending was throttled earlier
            details::wait_until(condition, lock, earliest_necessary_update, [&] { return shutdown || most_recent_message < most_recent_update(resources); });
            if (shutdown) break;
            most_recent_message = most_recent_update(resources);

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Got notification on control protocol websockets thread";

            earliest_necessary_update = (tai_clock::time_point::max)();

            std::vector<std::pair<web::websockets::experimental::listener::connection_id, web::websockets::websocket_outgoing_message>> outgoing_messages;

            for (auto wit = websockets.left.begin(); websockets.left.end() != wit;)
            {
                const auto& websocket = *wit;

                // for each websocket connection that has valid grain and subscription resources
                const auto grain = find_resource(resources, { websocket.first, nmos::types::grain });
                if (resources.end() == grain)
                {
                    auto close = listener.close(websocket.second, web::websockets::websocket_close_status::server_terminate, U("Expired"))
                        .then(details::observe_websocket_exception(gate));
                    // theoretically blocking, but in fact not
                    close.wait();

                    wit = websockets.left.erase(wit);
                    continue;
                }
                const auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });
                if (resources.end() == subscription)
                {
                    // a grain without a subscription shouldn't be possible, but let's be tidy
                    erase_resource(resources, grain->id);

                    auto close = listener.close(websocket.second, web::websockets::websocket_close_status::server_terminate, U("Expired"))
                        .then(details::observe_websocket_exception(gate));
                    // theoretically blocking, but in fact not
                    close.wait();

                    wit = websockets.left.erase(wit);
                    continue;
                }
                // and has events to send
                if (0 == nmos::fields::message_grain_data(grain->data).size())
                {
                    ++wit;
                    continue;
                }

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing to send " << nmos::fields::message_grain_data(grain->data).size() << " events on websocket connection: " << grain->id;

                for (const auto& event : nmos::fields::message_grain_data(grain->data).as_array())
                {
                    web::websockets::websocket_outgoing_message message;

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "outgoing_message: " << event.serialize();
                    message.set_utf8_message(utility::us2s(event.serialize()));
                    outgoing_messages.push_back({ websocket.second, message });
                }

                // reset the grain for next time
                resources.modify(grain, [&resources](nmos::resource& grain)
                {
                    // all messages have now been prepared
                    nmos::fields::message_grain_data(grain.data) = value::array();
                    grain.updated = strictly_increasing_update(resources);
                });

                ++wit;
            }

            // send the messages without the lock on resources
            details::reverse_lock_guard<nmos::write_lock> unlock{ lock };

            if (!outgoing_messages.empty()) slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sending " << outgoing_messages.size() << " websocket messages";

            for (auto& outgoing_message : outgoing_messages)
            {
                // hmmm, no way to cancel this currently...

                auto send = listener.send(outgoing_message.first, outgoing_message.second)
                    .then(details::observe_websocket_exception(gate));
                // current websocket_listener implementation is synchronous in any case, but just to make clear...
                // for now, wait for the message to be sent
                send.wait();
            }
        }
    }
}

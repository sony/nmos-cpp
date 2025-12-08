#include "nmos/control_protocol_utils.h"

#include <list>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/split.hpp>
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
    namespace nc
    {
        namespace details
        {
            bool is_control_class(const nc_class_id& control_class_id, const nc_class_id& class_id_)
            {
                nc_class_id class_id{class_id_};
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
                        return property_id == parse_property_id(nmos::fields::nc::property_id(constraints));
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
                            datatype_constraints_validation(value_, {details::get_datatype_descriptor(type_name, params.get_control_protocol_datatype_descriptor), params.get_control_protocol_datatype_descriptor});
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
                            if (!nmos::fields::nc::is_nullable(nc_field_descriptor) && value_.at(field_name).is_null()) { throw control_protocol_exception(utility::us2s(field_name) + " is not nullable"); }

                            // if field value is null continue to next field
                            if (value_.at(field_name).is_null()) continue;

                            // is field sequenceable
                            if (nmos::fields::nc::is_sequence(nc_field_descriptor) != value_.at(field_name).is_array()) { throw control_protocol_exception(utility::us2s(field_name) + " is not sequenceable"); }

                            // check constraints of its typeName
                            const auto& field_type_name = nc_field_descriptor.at(nmos::fields::nc::type_name);

                            if (!field_type_name.is_null())
                            {
                                auto value = value_.at(field_name);

                                // do typename constraints validation
                                datatype_constraints_validation(value, {details::get_datatype_descriptor(field_type_name, params.get_control_protocol_datatype_descriptor), params.get_control_protocol_datatype_descriptor});
                            }

                            // check against field constraints if present
                            const auto& constraints = nmos::fields::nc::constraints(nc_field_descriptor);
                            if (!constraints.is_null())
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

            web::json::value get_nc_block_member_descriptor(const resources& resources, const nmos::resource& parent_nc_block_resource, web::json::array& role_path_segments)
            {
                if (parent_nc_block_resource.data.has_field(nmos::fields::nc::members))
                {
                    const auto& members = nmos::fields::nc::members(parent_nc_block_resource.data);

                    const auto role_path_segement = web::json::front(role_path_segments);
                    role_path_segments.erase(0);
                    // find the role_path_segment member
                    auto member_found = std::find_if(members.begin(), members.end(), [&](const web::json::value& member)
                    {
                        return role_path_segement.as_string() == nmos::fields::nc::role(member);
                    });

                    if (members.end() != member_found)
                    {
                        if (role_path_segments.size() == 0)
                        {
                            // NcBlockMemberDescriptor
                            return *member_found;
                        }

                        // get the role_path_segement member resource
                        if (is_block(parse_class_id(nmos::fields::nc::class_id(*member_found))))
                        {
                            // get resource based on the oid
                            const auto& oid = nmos::fields::nc::oid(*member_found);
                            const auto found = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                            if (resources.end() != found)
                            {
                                return get_nc_block_member_descriptor(resources, *found, role_path_segments);
                            }
                        }
                    }
                }
                return web::json::value{};
            }

            web::json::value parse_role_path(const utility::string_t& role_path_)
            {
                // tokenize the role_path with the '.' delimiter
                std::list<utility::string_t> role_path_segments;
                boost::algorithm::split(role_path_segments, role_path_, [](utility::char_t c) { return '.' == c; });

                return web::json::value_from_elements(role_path_segments);
            }

            // Set status and status message
            bool set_monitor_status(resources& resources, nc_oid oid, int status, const utility::string_t& status_message,
                const nc_property_id& status_property_id,
                const nc_property_id& status_message_property_id,
                const nc_property_id& status_transition_counter_property_id,
                const utility::string_t& status_pending_received_time_field_name,
                get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor,
                slog::base_gate& gate)
            {
                auto current_connection_status = get_property(resources, oid, status_property_id, get_control_protocol_class_descriptor, gate);

                web::json::value json_status_message = status_message.size() ? web::json::value::string(status_message) : web::json::value::null();

                set_property_and_notify(resources, oid, status_property_id, status, get_control_protocol_class_descriptor, gate);
                set_property_and_notify(resources, oid, status_message_property_id, json_status_message, get_control_protocol_class_descriptor, gate);
                // Cancel any pending status updates
                set_property(resources, oid, status_pending_received_time_field_name, web::json::value::number(0), gate);

                // if status is "partially unhealthy" (2) or "unhealthy" (3) and less healthy than current state
                if (status > 1 && status > current_connection_status.as_integer())
                {
                    // increment transition_counter
                    auto transition_counter = get_property(resources, oid, status_transition_counter_property_id, get_control_protocol_class_descriptor, gate).as_integer();
                    set_property_and_notify(resources, oid, status_transition_counter_property_id, ++transition_counter, get_control_protocol_class_descriptor, gate);
                }
                const auto found = find_resource(resources, utility::s2us(std::to_string(oid)));
                if (resources.end() != found)
                {
                    if (nmos::nc::is_sender_monitor(parse_class_id(nmos::fields::nc::class_id(found->data))))
                    {
                        return details::update_sender_monitor_overall_status(resources, oid, get_control_protocol_class_descriptor, gate);
                    }
                    return details::update_receiver_monitor_overall_status(resources, oid, get_control_protocol_class_descriptor, gate);
                }
                return false;
            }

            // Set monitor status and status message
            bool set_monitor_status_with_delay(resources& resources, nc_oid oid, int status, const utility::string_t& status_message,
                const nc_property_id& status_property_id,
                const nc_property_id& status_message_property_id,
                const nc_property_id& status_transition_counter_property_id,
                const utility::string_t& status_pending_field_name,
                const utility::string_t& status_message_pending_time_field_name,
                const utility::string_t& status_pending_received_time_field_name,
                long long now_time,
                monitor_status_pending_handler monitor_status_pending,
                get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor,
                slog::base_gate& gate)
            {
                const auto& current_status = get_property(resources, oid, status_property_id, get_control_protocol_class_descriptor, gate);
                if (current_status.is_null())
                {
                    // should never happen, missing receiver/sender monitor status property
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << U("receiver/sender monitor status property: {level=") << status_property_id.level << U(", index=") << status_property_id.index << U("} not found");
                    return false;
                }

                // check whether the the status message changed when the status is unchanged
                if (status == current_status.as_integer())
                {
                    // has status message changed?
                    const auto& current_status_message = get_property(resources, oid, status_message_property_id, get_control_protocol_class_descriptor, gate);
                    if ((current_status_message.is_null() && status_message.size()) || (!current_status_message.is_null() && current_status_message.as_string() != status_message))
                    {
                        // If the status message has changed then update only that
                        web::json::value json_status_message = status_message.size() ? web::json::value::string(status_message) : web::json::value::null();
                        return set_property_and_notify(resources, oid, status_message_property_id, json_status_message, get_control_protocol_class_descriptor, gate);
                    }
                    // no update required, no changes on status or status message
                    return true;
                }

                // status has changed

                utility::string_t updated_status_message = status_message;

                // if status is "healthy" then preserve the previous status message
                if (1 == status && status_message.size() == 0)
                {
                    // Get existing status message and prepend with "Previously: "
                    const auto& current_status_message = get_property(resources, oid, status_message_property_id, get_control_protocol_class_descriptor, gate);

                    if (!current_status_message.is_null())
                    {
                        utility::ostringstream_t ss;
                        ss << U("Previously: ") << current_status_message.as_string();
                        updated_status_message = ss.str();
                    }
                }
                // if status or current state is "inactive" (0), update status and status message immediately
                if (0 == status || 0 == current_status.as_integer())
                {
                    return set_monitor_status(resources, oid, status, updated_status_message, status_property_id, status_message_property_id, status_transition_counter_property_id, status_pending_received_time_field_name, get_control_protocol_class_descriptor, gate);
                }
                else
                {
                    const auto& activation_time = get_property(resources, oid, nmos::fields::nc::monitor_activation_time, gate);
                    const auto& status_reporting_delay = get_property(resources, oid, nc_status_monitor_status_reporting_delay, get_control_protocol_class_descriptor, gate);

                    if (status > current_status.as_integer() && now_time > (static_cast<long long>(activation_time.as_integer()) + status_reporting_delay.as_integer()))
                    {
                        // becoming less health and not in the initial activation state
                        // immediately set the status
                        return set_monitor_status(resources, oid, status, updated_status_message, status_property_id, status_message_property_id, status_transition_counter_property_id, status_pending_received_time_field_name, get_control_protocol_class_descriptor, gate);
                    }
                    else
                    {
                        web::json::value json_status_message = updated_status_message.size() ? web::json::value::string(updated_status_message) : web::json::value::null();
                        // becoming more health or in the initial activation state
                        // set the status with delay
                        const auto& pending_received_time = get_property(resources, oid, status_pending_received_time_field_name, gate);

                        // only update pending received time if not already set
                        if (pending_received_time.as_integer() == 0)
                        {
                            if (!set_property(resources, oid, status_pending_received_time_field_name, static_cast<int64_t>(now_time), gate))
                            {
                                return false;
                            }
                        }

                        if (set_property(resources, oid, status_pending_field_name, status, gate)
                            && set_property(resources, oid, status_message_pending_time_field_name, json_status_message, gate))
                        {
                            monitor_status_pending();
                            return true;
                        }
                        return false;
                    }
                }
            }

            web::json::value get_overall_status_message(const resources& resources, nc_oid oid, const std::vector<std::pair<nmos::nc_property_id, nmos::nc_property_id>>& status_property_ids, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
            {
                // overall status message displays the most unhealthy status message. Priority lowest to highest -> inactive, healthy, partially unhealthy, unhealthy
                // overall status message displays root cause of problem. Priority of messages lowest to highest -> sync, payload, transport, link
                auto overall_status_message = web::json::value::null();

                for (int status_health = 0; status_health < 5; ++status_health) // Iterate through health of statuses
                {
                    for (const auto& property_id_pair : status_property_ids)
                    {
                        auto status = get_property(resources, oid, property_id_pair.first, get_control_protocol_class_descriptor, gate);

                        if (status.as_integer() != status_health) continue;

                        auto status_message = get_property(resources, oid, property_id_pair.second, get_control_protocol_class_descriptor, gate);

                        if (!status_message.is_null())
                        {
                            overall_status_message = status_message;
                        }
                    }
                }
                return overall_status_message;
            }

            bool update_receiver_monitor_overall_status(resources& resources, nc_oid oid, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
            {
                std::vector<std::pair<nmos::nc_property_id, nmos::nc_property_id>> status_message_property_ids = {
                    std::pair<nmos::nc_property_id, nmos::nc_property_id>(nc_receiver_monitor_external_synchronization_status_property_id, nc_receiver_monitor_external_synchronization_status_message_property_id),
                    std::pair<nmos::nc_property_id, nmos::nc_property_id>(nc_receiver_monitor_stream_status_property_id, nc_receiver_monitor_stream_status_message_property_id),
                    std::pair<nmos::nc_property_id, nmos::nc_property_id>(nc_receiver_monitor_connection_status_property_id, nc_receiver_monitor_connection_status_message_property_id),
                    std::pair<nmos::nc_property_id, nmos::nc_property_id>(nc_receiver_monitor_link_status_property_id, nc_receiver_monitor_link_status_message_property_id)};

                // Update Overall Status
                auto connection_status = get_property(resources, oid, nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
                auto stream_status = get_property(resources, oid, nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
                const auto& overall_status_message = get_overall_status_message(resources, oid, status_message_property_ids, get_control_protocol_class_descriptor, gate);

                // if connection or stream status is Inactive
                if (nc_connection_status::status::inactive == connection_status.as_integer() || nc_stream_status::status::inactive == stream_status.as_integer())
                {
                    // Overall status is set to Inactive
                    bool success = set_property_and_notify(resources, oid, nc_status_monitor_overall_status_property_id, nc_overall_status::status::inactive, get_control_protocol_class_descriptor, gate);
                    return success && set_property_and_notify(resources, oid, nc_status_monitor_overall_status_message_property_id, overall_status_message, get_control_protocol_class_descriptor, gate);
                }

                auto link_status = get_property(resources, oid, nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
                auto external_synchronization_status = get_property(resources, oid, nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);

                // otherwise take the least healthy status as the overall status
                std::vector<int32_t> statuses = {link_status.as_integer(), connection_status.as_integer(), stream_status.as_integer()};

                // Ignore external synchronization status if it is not used
                if (nc_synchronization_status::status::not_used != external_synchronization_status.as_integer())
                {
                    statuses.push_back(external_synchronization_status.as_integer());
                }
                // Find most unhealthy status
                auto overall_status = *std::max_element(statuses.begin(), statuses.end());
                bool success = set_property_and_notify(resources, oid, nc_status_monitor_overall_status_property_id, web::json::value::number(overall_status), get_control_protocol_class_descriptor, gate);
                return success && set_property_and_notify(resources, oid, nc_status_monitor_overall_status_message_property_id, overall_status_message, get_control_protocol_class_descriptor, gate);
            }

            bool update_sender_monitor_overall_status(resources& resources, nc_oid oid, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
            {
                std::vector<std::pair<nmos::nc_property_id, nmos::nc_property_id>> status_message_property_ids = {
                    std::pair<nmos::nc_property_id, nmos::nc_property_id>(nc_sender_monitor_external_synchronization_status_property_id, nc_sender_monitor_external_synchronization_status_message_property_id),
                    std::pair<nmos::nc_property_id, nmos::nc_property_id>(nc_sender_monitor_essence_status_property_id, nc_sender_monitor_essence_status_message_property_id),
                    std::pair<nmos::nc_property_id, nmos::nc_property_id>(nc_sender_monitor_transmission_status_property_id, nc_sender_monitor_transmission_status_message_property_id),
                    std::pair<nmos::nc_property_id, nmos::nc_property_id>(nc_sender_monitor_link_status_property_id, nc_sender_monitor_link_status_message_property_id)};

                // Update Overall Status
                auto transmission_status = get_property(resources, oid, nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
                auto essence_status = get_property(resources, oid, nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);

                const auto& overall_status_message = get_overall_status_message(resources, oid, status_message_property_ids, get_control_protocol_class_descriptor, gate);

                // if transmission or stream status is Inactive
                if (nc_transmission_status::status::inactive == transmission_status.as_integer() || nc_essence_status::status::inactive == essence_status.as_integer())
                {
                    // Overall status is set to Inactive
                    bool success = set_property_and_notify(resources, oid, nc_status_monitor_overall_status_property_id, nc_overall_status::status::inactive, get_control_protocol_class_descriptor, gate);
                    return success && set_property_and_notify(resources, oid, nc_status_monitor_overall_status_message_property_id, overall_status_message, get_control_protocol_class_descriptor, gate);
                }

                auto link_status = get_property(resources, oid, nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
                auto external_synchronization_status = get_property(resources, oid, nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);

                // otherwise take the least healthy status as the overall status
                std::vector<int32_t> statuses = {link_status.as_integer(), transmission_status.as_integer(), essence_status.as_integer()};

                // Ignore external synchronization status if it is not used
                if (nc_synchronization_status::status::not_used != external_synchronization_status.as_integer())
                {
                    statuses.push_back(external_synchronization_status.as_integer());
                }
                // Find most unhealthy status
                auto overall_status = *std::max_element(statuses.begin(), statuses.end());
                bool success = set_property_and_notify(resources, oid, nc_status_monitor_overall_status_property_id, web::json::value::number(overall_status), get_control_protocol_class_descriptor, gate);
                return success && set_property_and_notify(resources, oid, nc_status_monitor_overall_status_message_property_id, overall_status_message, get_control_protocol_class_descriptor, gate);
            }
        }

        // is the given class_id a NcBlock
        bool is_block(const nc_class_id& class_id)
        {
            return details::is_control_class(nc_block_class_id, class_id);
        }

        // is the given class_id a NcWorker
        bool is_worker(const nc_class_id& class_id)
        {
            return details::is_control_class(nc_worker_class_id, class_id);
        }

        // is the given class_id a NcManager
        bool is_manager(const nc_class_id& class_id)
        {
            return details::is_control_class(nc_manager_class_id, class_id);
        }

        // is the given class_id a NcDeviceManager
        bool is_device_manager(const nc_class_id& class_id)
        {
            return details::is_control_class(nc_device_manager_class_id, class_id);
        }

        // is the given class_id a NcClassManager
        bool is_class_manager(const nc_class_id& class_id)
        {
            return details::is_control_class(nc_class_manager_class_id, class_id);
        }

        // is the given class_id a NcStatusMonitor
        bool is_status_monitor(const nc_class_id& class_id)
        {
            return details::is_control_class(nc_status_monitor_class_id, class_id);
        }

        // is the given class_id a NcStatusMonitor
        bool is_sender_monitor(const nc_class_id& class_id)
        {
            return details::is_control_class(nc_sender_monitor_class_id, class_id);
        }

        // construct NcClassId
        nc_class_id make_class_id(const nc_class_id& prefix, int32_t authority_key, const std::vector<int32_t>& suffix)
        {
            nc_class_id class_id = prefix;
            class_id.push_back(authority_key);
            class_id.insert(class_id.end(), suffix.begin(), suffix.end());
            return class_id;
        }
        nc_class_id make_class_id(const nc_class_id& prefix, const std::vector<int32_t>& suffix)
        {
            return make_class_id(prefix, 0, suffix);
        }

        // find control class property descriptor (NcPropertyDescriptor)
        web::json::value find_property_descriptor(const nc_property_id& property_id, const nc_class_id& class_id_, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
        {
            using web::json::value;

            auto class_id = class_id_;

            while (!class_id.empty())
            {
                const auto& control_class = get_control_protocol_class_descriptor(class_id);
                const auto& property_descriptors = control_class.property_descriptors.as_array();
                auto found = std::find_if(property_descriptors.begin(), property_descriptors.end(), [&property_id](const web::json::value& property_descriptor)
                    {
                        return (property_id == nc::details::parse_property_id(nmos::fields::nc::id(property_descriptor)));
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
                        if (is_block(nc::details::parse_class_id(nmos::fields::nc::class_id(member))))
                        {
                            // get resource based on the oid
                            const auto& oid = nmos::fields::nc::oid(member);
                            const auto found = find_resource(resources, utility::s2us(std::to_string(oid)));
                            if (resources.end() != found)
                            {
                                nc::get_member_descriptors(resources, *found, recurse, descriptors);
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
                        if (is_block(nc::details::parse_class_id(nmos::fields::nc::class_id(member))))
                        {
                            // get resource based on the oid
                            const auto& oid = nmos::fields::nc::oid(member);
                            const auto found = find_resource(resources, utility::s2us(std::to_string(oid)));
                            if (resources.end() != found)
                            {
                                nc::find_members_by_role(resources, *found, role, match_whole_string, case_sensitive, recurse, descriptors);
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
                            const auto& class_id = nc::details::parse_class_id(nmos::fields::nc::class_id(descriptor));

                            if (include_derived) { return class_id_.size() <= class_id.size() && std::equal(class_id_.begin(), class_id_.end(), class_id.begin()); }
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
                        if (is_block(nc::details::parse_class_id(nmos::fields::nc::class_id(member))))
                        {
                            // get resource based on the oid
                            const auto& oid = nmos::fields::nc::oid(member);
                            const auto found = find_resource(resources, utility::s2us(std::to_string(oid)));
                            if (resources.end() != found)
                            {
                                nc::find_members_by_class_id(resources, *found, class_id_, include_derived, recurse, descriptors);
                            }
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

            if (!is_block(details::parse_class_id(nmos::fields::nc::class_id(parent)))) throw std::logic_error("non-NcBlock cannot be nested");

            web::json::push_back(parent[nmos::fields::nc::members],
                details::make_block_member_descriptor(nmos::fields::description(child), nmos::fields::nc::role(child), nmos::fields::nc::oid(child), nmos::fields::nc::constant_oid(child), nc::details::parse_class_id(nmos::fields::nc::class_id(child)), nmos::fields::nc::user_label(child), nmos::fields::nc::oid(parent)));

            nc_block_resource.resources.push_back(resource);
        }

        // insert root block and all sub control protocol resources
        void insert_root(resources& resources, control_protocol_resource& root)
        {
            // note, model write lock should already be applied by the outer function, so access to control_protocol_resources is OK...

            std::function<void(nmos::resources& resources, nmos::control_protocol_resource& resource)> insert_resources;

            insert_resources = [&insert_resources](nmos::resources& resources, nmos::control_protocol_resource& resource)
            {
                for (auto& r : resource.resources)
                {
                    insert_resources(resources, r);
                    nmos::nc::insert_resource(resources, std::move(r));
                }
                resource.resources.clear();
            };

            insert_resources(resources, root);
            nmos::nc::insert_resource(resources, std::move(root));
        }

        // insert a control protocol resource
        std::pair<resources::iterator, bool> insert_resource(resources& resources, resource&& resource)
        {
            // set the creation and update timestamps, before inserting the resource
            resource.updated = resource.created = nmos::strictly_increasing_update(resources);

            auto result = resources.insert(std::move(resource));
            // replacement of a deleted or expired resource is also allowed
            // (currently, with no further checks on api_version, type, etc.)
            if (!result.second && !result.first->has_data())
            {
                // if the insertion was banned, resource has not been moved from
                result.second = resources.replace(result.first, std::move(resource));
            }
            return result;
        }

        // modify a control protocol resource, and insert notification event to all subscriptions
        bool modify_resource(resources& resources, const id& id, std::function<void(resource&)> modifier, const web::json::value& notification_event)
        {
            // note, model write lock should already be applied by the outer function, so access to control_protocol_resources is OK...

            auto found = resources.find(id);
            if (resources.end() == found || !found->has_data()) return false;

            auto pre = notification_event != web::json::value::null() ? found->data : web::json::value::null();

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

            if (web::json::value::null() != notification_event && result)
            {
                auto& modified = *found;
                nc::insert_notification_events(resources, modified.version, modified.downgrade_version, modified.type, pre, modified.data, notification_event);
            }

            if (modifier_exception)
            {
                std::rethrow_exception(modifier_exception);
            }

            return result;
        }

        // erase a control protocol resource
        resources::size_type erase_resource(resources& resources, const id& id)
        {
            // hmm, may be also erasing all it's member blocks?
            resources::size_type count = 0;
            auto found = resources.find(id);
            if (resources.end() != found && found->has_data())
            {
                resources.erase(found);
                ++count;
            }
            return count;
        }

        // find the control protocol resource which is assoicated with the given IS-04/IS-05/IS-08 resource id
        resources::const_iterator find_resource(resources& resources, type type, const id& resource_id)
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
                                return (resource_id == nmos::fields::nc::id(resource).as_string());
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
                details::method_parameter_constraints_validation(arguments.at(name), constraints, { nmos::nc::details::get_datatype_descriptor(type_name, get_control_protocol_datatype_descriptor), get_control_protocol_datatype_descriptor });
            }
        }

        // Validate that the resource has been correctly constructed according to the class_descriptor
        void validate_resource(const resource& resource, const experimental::control_class_descriptor& class_descriptor)
        {
            // Validate properties
            for (const auto& property_descriptor : class_descriptor.property_descriptors.as_array())
            {
                if ((resource.data.is_null()) || (!resource.data.has_field(nmos::fields::nc::name(property_descriptor))))
                {
                    throw control_protocol_exception("missing control resource property: " + utility::us2s(nmos::fields::nc::name(property_descriptor)));
                }
            }

            // Validate methods
            for (const auto& method_descriptor : class_descriptor.method_descriptors)
            {
                if (!method_descriptor.second)
                {
                    throw control_protocol_exception("method not implemented: " + utility::us2s(nmos::fields::nc::name(method_descriptor.first)));
                }
            }
        }

        resources::const_iterator find_resource_by_role_path(const resources& resources, const web::json::array& role_path_)
        {
            auto role_path = role_path_;
            auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(nmos::root_block_oid)));
            if (resources.end() != resource)
            {
                const auto role = nmos::fields::nc::role(resource->data);

                if (role_path.size() && role == web::json::front(role_path).as_string())
                {
                    role_path.erase(0);

                    if (role_path.size())
                    {
                        const auto& block_member_descriptor = details::get_nc_block_member_descriptor(resources, *resource, role_path);
                        if (!block_member_descriptor.is_null())
                        {
                            const auto& oid = nmos::fields::nc::oid(block_member_descriptor);
                            const auto found = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                            if (resources.end() != found)
                            {
                                return found;
                            }
                        }
                    }
                    else
                    {
                        return resource;
                    }
                }
            }
            return resources.end();
        }

        resources::const_iterator find_resource_by_role_path(const resources& resources, const utility::string_t& role_path_)
        {
            const auto& role_path = details::parse_role_path(role_path_);

            return find_resource_by_role_path(resources, role_path.as_array());
        }

        resources::const_iterator find_touchpoint_resource(const resources& resources, const resource& resource)
        {
            if (!resource.has_data())
            {
                return resources.end();
            }

            const auto& touchpoints = resource.data.at(nmos::fields::nc::touchpoints);

            if (touchpoints.size() == 0)
            {
                return resources.end();
            }

            // Hmmmmm we're only getting the first touchpoint resource. There could be more than one.
            const auto& touchpoint_uuid = nmos::fields::nc::id(nmos::fields::nc::resource(*touchpoints.as_array().begin()));
            return nmos::find_resource(resources, touchpoint_uuid.as_string());
        }

        // insert 'value changed', 'sequence item added', 'sequence item changed' or 'sequence item removed' notification events into all grains whose subscriptions match the specified version, type and "pre" or "post" values
        // this is used for the IS-12 propertry changed event
        void insert_notification_events(nmos::resources& resources, const nmos::api_version& version, const nmos::api_version& downgrade_version, const nmos::type& type, const web::json::value& pre, const web::json::value& post, const web::json::value& event)
        {
            using web::json::value;

            if (pre == post) return;

            auto& by_type = resources.get<tags::type>();
            const auto subscriptions = by_type.equal_range(nmos::details::has_data(nmos::types::subscription));

            for (auto it = subscriptions.first; subscriptions.second != it; ++it)
            {
                // for each subscription
                const auto& subscription = *it;

                // check whether the resource_path matches the resource type and the query parameters match either the "pre" or "post" resource

                const auto resource_path = nmos::fields::resource_path(subscription.data);
                const resource_query match(subscription.version, resource_path, nmos::fields::params(subscription.data));

                const bool pre_match = match(version, downgrade_version, type, pre, resources);
                const bool post_match = match(version, downgrade_version, type, post, resources);

                if (!pre_match && !post_match) continue;

                // add the event to the grain for each websocket connection to this subscription

                for (const auto& id : subscription.sub_resources)
                {
                    auto grain = find_resource(resources, { id, nmos::types::grain });
                    if (resources.end() == grain) continue; // check websocket connection is still open

                    resources.modify(grain, [&resources, &event](nmos::resource& grain)
                    {
                        auto& events = nmos::fields::message_grain_data(grain.data);
                        web::json::push_back(events, event);
                        grain.updated = strictly_increasing_update(resources);
                    });
                }
            }
        }

        web::json::value get_property(const resources& resources, nc_oid oid, const nc_property_id& property_id, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            // get resource based on the oid
            const auto found = find_resource(resources, utility::s2us(std::to_string(oid)));
            if (resources.end() != found)
            {
                // find the relevant nc_property_descriptor
                const auto& property = nc::find_property_descriptor(property_id, nc::details::parse_class_id(nmos::fields::nc::class_id(found->data)), get_control_protocol_class_descriptor);
                if (!property.is_null() && found->has_data() && found->data.has_field(nmos::fields::nc::name(property)))
                {
                    return found->data.at(nmos::fields::nc::name(property));
                }
            }
            // unknown property
            slog::log<slog::severities::error>(gate, SLOG_FLF) << U("unknown property: {level=") << property_id.level << U(", index=") << property_id.index << U("} to do Get");
            return web::json::value::null();
        }

        bool set_property_and_notify(resources& resources, nc_oid oid, const nc_property_id& property_id, const web::json::value& value, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto found = find_resource(resources, utility::s2us(std::to_string(oid)));
            if (resources.end() != found)
            {
                const auto& property = nc::find_property_descriptor(property_id, nc::details::parse_class_id(nmos::fields::nc::class_id(found->data)), get_control_protocol_class_descriptor);
                if (!property.is_null())
                {
                    try
                    {
                        // update property
                        nc::modify_resource(resources, found->id, [&](nmos::resource& resource)
                        {
                            resource.data[nmos::fields::nc::name(property)] = value;

                        }, make_property_changed_event(oid, { { property_id, nc_property_change_type::type::value_changed, value } }));

                        return true;
                    }
                    catch (const nmos::control_protocol_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set property: {level=" << property_id.level << ", index=" << property_id.index << "}  error: " << e.what();
                        return false;
                    }
                }

                // unknown property
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "unknown property: {level=" << property_id.level << ", index=" << property_id.index << "}  to do Set.";
                return false;
            }

            // unknown resource
            slog::log<slog::severities::error>(gate, SLOG_FLF) << "unknown control protocol resource: oid=" << oid;
            return false;
        }

        bool set_property(resources& resources, nc_oid oid, const utility::string_t& property_name, const web::json::value& value, slog::base_gate& gate)
        {
            try
            {
                nc::modify_resource(resources, utility::s2us(std::to_string(oid)), [&](nmos::resource& resource)
                {
                    resource.data[property_name] = value;
                });
                return true;
            }
            catch (const nmos::control_protocol_exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set property name : " << property_name.c_str() << " error: " << e.what();
                return false;
            }
        }

        web::json::value get_property(const resources& resources, nc_oid oid, const utility::string_t& property_name, slog::base_gate& gate)
        {
            // get resource based on the oid
            const auto found = find_resource(resources, utility::s2us(std::to_string(oid)));
            if (resources.end() != found)
            {
                // find the relevant nc_property_descriptor
                return found->data.at(property_name);
            }
            // unknown resource
            slog::log<slog::severities::error>(gate, SLOG_FLF) << "unknown control protocol resource: oid=" << oid;
            return web::json::value::null();
        }

        // Set link status and link status message
        bool set_receiver_monitor_link_status(resources& resources, nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            return nc::details::set_monitor_status(resources, oid, link_status, link_status_message,
                nc_receiver_monitor_link_status_property_id,
                nc_receiver_monitor_link_status_message_property_id,
                nc_receiver_monitor_link_status_transition_counter_property_id,
                nmos::fields::nc::link_status_pending_received_time,
                get_control_protocol_class_descriptor,
                gate);
        }

        bool set_receiver_monitor_link_status_with_delay(resources& resources, nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            return nc::details::set_monitor_status_with_delay(resources, oid, link_status, link_status_message,
                nc_receiver_monitor_link_status_property_id,
                nc_receiver_monitor_link_status_message_property_id,
                nc_receiver_monitor_link_status_transition_counter_property_id,
                nmos::fields::nc::link_status_pending,
                nmos::fields::nc::link_status_message_pending,
                nmos::fields::nc::link_status_pending_received_time,
                now_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        }

        bool set_receiver_monitor_connection_status(resources& resources, nc_oid oid, nmos::nc_connection_status::status connection_status, const utility::string_t& connection_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            return nc::details::set_monitor_status(resources, oid, connection_status, connection_status_message,
                nc_receiver_monitor_connection_status_property_id,
                nc_receiver_monitor_connection_status_message_property_id,
                nc_receiver_monitor_connection_status_transition_counter_property_id,
                nmos::fields::nc::connection_status_pending_received_time,
                get_control_protocol_class_descriptor,
                gate);
        }

        bool set_receiver_monitor_connection_status_with_delay(resources& resources, nc_oid oid, nmos::nc_connection_status::status connection_status, const utility::string_t& connection_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            return nc::details::set_monitor_status_with_delay(resources, oid, connection_status, connection_status_message,
                nc_receiver_monitor_connection_status_property_id,
                nc_receiver_monitor_connection_status_message_property_id,
                nc_receiver_monitor_connection_status_transition_counter_property_id,
                nmos::fields::nc::connection_status_pending,
                nmos::fields::nc::connection_status_message_pending,
                nmos::fields::nc::connection_status_pending_received_time,
                now_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        }

        // Set external synchronization status and external synchronization status message
        bool set_receiver_monitor_external_synchronization_status(resources& resources, nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            return nc::details::set_monitor_status(resources, oid, external_synchronization_status, external_synchronization_status_message,
                nc_receiver_monitor_external_synchronization_status_property_id,
                nc_receiver_monitor_external_synchronization_status_message_property_id,
                nc_receiver_monitor_external_synchronization_status_transition_counter_property_id,
                nmos::fields::nc::external_synchronization_status_pending_received_time,
                get_control_protocol_class_descriptor,
                gate);
        }

        bool set_receiver_monitor_external_synchronization_status_with_delay(resources& resources, nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            return nc::details::set_monitor_status_with_delay(resources, oid, external_synchronization_status, external_synchronization_status_message,
                nc_receiver_monitor_external_synchronization_status_property_id,
                nc_receiver_monitor_external_synchronization_status_message_property_id,
                nc_receiver_monitor_external_synchronization_status_transition_counter_property_id,
                nmos::fields::nc::external_synchronization_status_pending,
                nmos::fields::nc::external_synchronization_status_message_pending,
                nmos::fields::nc::external_synchronization_status_pending_received_time,
                now_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        }

        // Set stream status and stream status message
        bool set_receiver_monitor_stream_status(resources& resources, nc_oid oid, nmos::nc_stream_status::status stream_status, const utility::string_t& stream_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            return nc::details::set_monitor_status(resources, oid, stream_status, stream_status_message,
                nc_receiver_monitor_stream_status_property_id,
                nc_receiver_monitor_stream_status_message_property_id,
                nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending_received_time,
                get_control_protocol_class_descriptor,
                gate);
        }

        bool set_receiver_monitor_stream_status_with_delay(resources& resources, nc_oid oid, nmos::nc_stream_status::status stream_status, const utility::string_t& stream_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            return nc::details::set_monitor_status_with_delay(resources, oid, stream_status, stream_status_message,
                nc_receiver_monitor_stream_status_property_id,
                nc_receiver_monitor_stream_status_message_property_id,
                nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending,
                nmos::fields::nc::stream_status_message_pending,
                nmos::fields::nc::stream_status_pending_received_time,
                now_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        }

        // Set synchronization source id
        bool set_monitor_synchronization_source_id(resources& resources, nc_oid oid, const bst::optional<utility::string_t>& source_id_, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            web::json::value source_id = source_id_ ? web::json::value::string(*source_id_) : web::json::value{};
            return set_property_and_notify(resources, oid, nc_receiver_monitor_synchronization_source_id_property_id, source_id, get_control_protocol_class_descriptor, gate);
        }

        bool activate_monitor(resources& resources, nc_oid oid, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_method_descriptor_handler get_control_protocol_method_descriptor, slog::base_gate& gate)
        {
            // A monitor is expected to go through a period of instability upon activation. Therefore, on monitor activation
            // domain specific statuses offering an Inactive option MUST transition immediately to the Healthy state.
            // Furthermore, after activation, as long as the monitor isn't being deactivated, it MUST delay the reporting
            // of non Healthy states for the duration specified by statusReportingDelay, and then transition to any other appropriate state.
            const auto found = find_resource(resources, utility::s2us(std::to_string(oid)));
            if (resources.end() != found && nc::is_status_monitor(nc::details::parse_class_id(nmos::fields::nc::class_id(found->data))))
            {
                const auto& class_id = nc::details::parse_class_id(nmos::fields::nc::class_id(found->data));

                auto activation_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                auto succeed = set_property(resources, oid, nmos::fields::nc::monitor_activation_time, activation_time, gate);
                // If autoResetCountersAndMessages set to true then reset the transition counters
                bool auto_reset_monitor{false};

                if (nc::is_sender_monitor(class_id))
                {
                    if (succeed) succeed = set_sender_monitor_transmission_status(resources, oid, nmos::nc_transmission_status::status::healthy, U("Sender activated"), get_control_protocol_class_descriptor, gate);
                    if (succeed) succeed = set_sender_monitor_essence_status(resources, oid, nmos::nc_essence_status::status::healthy, U("Sender activated"), get_control_protocol_class_descriptor, gate);
                }
                else
                {
                    if (succeed) succeed = set_receiver_monitor_connection_status(resources, oid, nmos::nc_connection_status::status::healthy, U("Receiver activated"), get_control_protocol_class_descriptor, gate);
                    if (succeed) succeed = set_receiver_monitor_stream_status(resources, oid, nmos::nc_stream_status::status::healthy, U("Receiver activated"), get_control_protocol_class_descriptor, gate);
                }

                if (succeed)
                {
                    auto auto_reset_property_id = nc::is_sender_monitor(class_id) ? nmos::nc_sender_monitor_auto_reset_monitor_property_id : nmos::nc_receiver_monitor_auto_reset_monitor_property_id;
                    auto auto_reset_monitor_ = get_property(resources, oid, auto_reset_property_id, get_control_protocol_class_descriptor, gate);
                    if (auto_reset_monitor_.is_null()) succeed = false;
                    else auto_reset_monitor = auto_reset_monitor_.as_bool();
                }

                if (succeed && auto_reset_monitor)
                {
                    auto reset_monitor_method_id = nc::is_sender_monitor(class_id) ? nmos::nc_sender_monitor_reset_monitor_method_id : nmos::nc_receiver_monitor_reset_monitor_method_id;
                    // find the method_handler for the reset monitor method
                    auto method = get_control_protocol_method_descriptor(class_id, reset_monitor_method_id);
                    auto& nc_method_descriptor = method.first;
                    auto& reset_monitor = method.second;
                    if (reset_monitor)
                    {
                        // this callback should not throw exceptions
                        const auto method_result = reset_monitor(resources, *found, web::json::value::null(), nmos::fields::nc::is_deprecated(nc_method_descriptor), gate);
                        const auto status = nmos::fields::nc::status(method_result);
                        succeed = (nmos::nc_method_status::ok == status || nmos::nc_method_status::property_deprecated == status || nmos::nc_method_status::method_deprecated == status);
                    }
                }
                if (succeed) slog::log<slog::severities::info>(gate, SLOG_FLF) << "Activating monitor oid: " << oid;
                else slog::log<slog::severities::error>(gate, SLOG_FLF) << "Fail to activating monitor oid: " << oid;

                return succeed;
            }
            else
            {
                // should never happen
                slog::log<slog::severities::fatal>(gate, SLOG_FLF) << "Invalid logic found in activate monitor oid: " << oid;
            }
            return false;
        }

        bool deactivate_monitor(resources& resources, nc_oid oid, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto found = find_resource(resources, utility::s2us(std::to_string(oid)));
            if (resources.end() != found && nc::is_status_monitor(nc::details::parse_class_id(nmos::fields::nc::class_id(found->data))))
            {
                const auto& class_id = nc::details::parse_class_id(nmos::fields::nc::class_id(found->data));

                auto succeed = set_property(resources, oid, nmos::fields::nc::monitor_activation_time, web::json::value::number(0), gate);

                if (nc::is_sender_monitor(class_id))
                {
                    if (succeed) succeed = set_sender_monitor_transmission_status(resources, oid, nmos::nc_transmission_status::status::inactive, U("Sender deactivated"), get_control_protocol_class_descriptor, gate);
                    if (succeed) succeed = set_sender_monitor_essence_status(resources, oid, nmos::nc_essence_status::status::inactive, U("Sender deactivated"), get_control_protocol_class_descriptor, gate);
                }
                else
                {
                    if (succeed) succeed = set_receiver_monitor_connection_status(resources, oid, nmos::nc_connection_status::status::inactive, U("Receiver deactivated"), get_control_protocol_class_descriptor, gate);
                    if (succeed) succeed = set_receiver_monitor_stream_status(resources, oid, nmos::nc_stream_status::status::inactive, U("Receiver deactivated"), get_control_protocol_class_descriptor, gate);
                }
                return succeed;
            }
            else
            {
                // should never happen
                slog::log<slog::severities::fatal>(gate, SLOG_FLF) << "Invalid logic found in deactivate monitor oid: " << oid;
            }
            return false;
        }

        // Set link status and link status message
        bool set_sender_monitor_link_status(resources& resources, nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            return nc::details::set_monitor_status(resources, oid, link_status, link_status_message,
                nc_sender_monitor_link_status_property_id,
                nc_sender_monitor_link_status_message_property_id,
                nc_sender_monitor_link_status_transition_counter_property_id,
                nmos::fields::nc::link_status_pending_received_time,
                get_control_protocol_class_descriptor,
                gate);
        }
        // Set link status and status message and apply status reporting delay
        bool set_sender_monitor_link_status_with_delay(resources& resources, nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            return nc::details::set_monitor_status_with_delay(resources, oid, link_status, link_status_message,
                nc_sender_monitor_link_status_property_id,
                nc_sender_monitor_link_status_message_property_id,
                nc_sender_monitor_link_status_transition_counter_property_id,
                nmos::fields::nc::link_status_pending,
                nmos::fields::nc::link_status_message_pending,
                nmos::fields::nc::link_status_pending_received_time,
                now_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        }

        // Set transmission status and transmission status message
        bool set_sender_monitor_transmission_status(resources& resources, nc_oid oid, nmos::nc_transmission_status::status transmission_status, const utility::string_t& transmission_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
             return nc::details::set_monitor_status(resources, oid, transmission_status, transmission_status_message,
                nc_sender_monitor_transmission_status_property_id,
                nc_sender_monitor_transmission_status_message_property_id,
                nc_sender_monitor_transmission_status_transition_counter_property_id,
                nmos::fields::nc::connection_status_pending_received_time,
                get_control_protocol_class_descriptor,
                gate);
        }
        // Set transmission status and status message and apply status reporting delay
        bool set_sender_monitor_transmission_status_with_delay(resources& resources, nc_oid oid, nmos::nc_transmission_status::status transmission_status, const utility::string_t& transmission_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            return nc::details::set_monitor_status_with_delay(resources, oid, transmission_status, transmission_status_message,
                nc_sender_monitor_transmission_status_property_id,
                nc_sender_monitor_transmission_status_message_property_id,
                nc_sender_monitor_transmission_status_transition_counter_property_id,
                nmos::fields::nc::transmission_status_pending,
                nmos::fields::nc::transmission_status_message_pending,
                nmos::fields::nc::transmission_status_pending_received_time,
                now_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        }

        // Set external synchronization status and external synchronization status message
        bool set_sender_monitor_external_synchronization_status(resources& resources, nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            return nc::details::set_monitor_status(resources, oid, external_synchronization_status, external_synchronization_status_message,
                nc_sender_monitor_external_synchronization_status_property_id,
                nc_sender_monitor_external_synchronization_status_message_property_id,
                nc_sender_monitor_external_synchronization_status_transition_counter_property_id,
                nmos::fields::nc::external_synchronization_status_pending_received_time,
                get_control_protocol_class_descriptor,
                gate);
        }
        // Set external synchronization status and status message and apply status reporting delay
        bool set_sender_monitor_external_synchronization_status_with_delay(resources& resources, nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            return nc::details::set_monitor_status_with_delay(resources, oid, external_synchronization_status, external_synchronization_status_message,
                nc_sender_monitor_external_synchronization_status_property_id,
                nc_sender_monitor_external_synchronization_status_message_property_id,
                nc_sender_monitor_external_synchronization_status_transition_counter_property_id,
                nmos::fields::nc::external_synchronization_status_pending,
                nmos::fields::nc::external_synchronization_status_message_pending,
                nmos::fields::nc::external_synchronization_status_pending_received_time,
                now_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        }

        // Set essence status and stream status message
        bool set_sender_monitor_essence_status(resources& resources, nc_oid oid, nmos::nc_essence_status::status essence_status, const utility::string_t& essence_status_message, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            return nc::details::set_monitor_status(resources, oid, essence_status, essence_status_message,
                nc_sender_monitor_essence_status_property_id,
                nc_sender_monitor_essence_status_message_property_id,
                nc_sender_monitor_essence_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending_received_time,
                get_control_protocol_class_descriptor,
                gate);
        }
        // Set essence status and status message and apply status reporting delay
        bool set_sender_monitor_essence_status_with_delay(resources& resources, nc_oid oid, nmos::nc_essence_status::status essence_status, const utility::string_t& essence_status_message, monitor_status_pending_handler monitor_status_pending, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
        {
            const auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            return nc::details::set_monitor_status_with_delay(resources, oid, essence_status, essence_status_message,
                nc_sender_monitor_essence_status_property_id,
                nc_sender_monitor_essence_status_message_property_id,
                nc_sender_monitor_essence_status_transition_counter_property_id,
                nmos::fields::nc::essence_status_pending,
                nmos::fields::nc::essence_status_message_pending,
                nmos::fields::nc::essence_status_pending_received_time,
                now_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        }
    }

}

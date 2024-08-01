#include "nmos/control_protocol_state.h"

#include "cpprest/http_utils.h"
#include "nmos/control_protocol_methods.h"
#include "nmos/control_protocol_resource.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            // create control class descriptor
            //   where
            //     properties: vector of NcPropertyDescriptor where NcPropertyDescriptor can be constructed using make_control_class_property
            //     methods:    vector of NcMethodDescriptor vs assoicated method handler where NcMethodDescriptor can be constructed using make_nc_method_descriptor
            //     events:     vector of NcEventDescriptor where NcEventDescriptor can be constructed using make_nc_event_descriptor
            control_class_descriptor make_control_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const web::json::value& fixed_role, const std::vector<web::json::value>& properties_, const std::vector<nmos::experimental::method>& methods_, const std::vector<web::json::value>& events_)
            {
                using web::json::value;

                web::json::value properties = value::array();
                for (const auto& property : properties_) { web::json::push_back(properties, property); }
                web::json::value events = value::array();
                for (const auto& event : events_) { web::json::push_back(events, event); }

                return { description, class_id, name, fixed_role, properties, methods_, events };
            }
        }
        // create control class descriptor with fixed role
        //   where
        //     properties: vector of NcPropertyDescriptor where NcPropertyDescriptor can be constructed using make_control_class_property
        //     methods:    vector of NcMethodDescriptor where NcMethodDescriptor  can be constructed using make_nc_method_descriptor and the assoicated method handler
        //     events:     vector of NcEventDescriptor where NcEventDescriptor can be constructed using make_nc_event_descriptor
        control_class_descriptor make_control_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const utility::string_t& fixed_role, const std::vector<web::json::value>& properties, const std::vector<nmos::experimental::method>& methods, const std::vector<web::json::value>& events)
        {
            using web::json::value;

            return details::make_control_class_descriptor(description, class_id, name, value::string(fixed_role), properties, methods, events);
        }
        // create control class descriptor without fixed role
        //   where
        //     properties: vector of NcPropertyDescriptor where NcPropertyDescriptor can be constructed using make_control_class_property
        //     methods:    vector of NcMethodDescriptor where NcMethodDescriptor can be constructed using make_nc_method_descriptor and the assoicated method handler
        //     events:     vector of NcEventDescriptor where NcEventDescriptor can be constructed using make_nc_event_descriptor
        control_class_descriptor make_control_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const std::vector<web::json::value>& properties, const std::vector<nmos::experimental::method>& methods, const std::vector<web::json::value>& events)
        {
            using web::json::value;

            return details::make_control_class_descriptor(description, class_id, name, value::null(), properties, methods, events);
        }

        // create control class property descriptor
        web::json::value make_control_class_property_descriptor(const utility::string_t& description, const nc_property_id& id, const nc_name& name, const utility::string_t& type_name, bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints)
        {
            return nmos::details::make_nc_property_descriptor(description, id, name, type_name, is_read_only, is_nullable, is_sequence, is_deprecated, constraints);
        }

        // create control class method parameter descriptor
        web::json::value make_control_class_method_parameter_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints)
        {
            return nmos::details::make_nc_parameter_descriptor(description, name, type_name, is_nullable, is_sequence, constraints);
        }

        namespace details
        {
            web::json::value make_control_class_method_descriptor(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype, const std::vector<web::json::value>& parameters_, bool is_deprecated)
            {
                using web::json::value;

                value parameters = value::array();
                for (const auto& parameter : parameters_) { web::json::push_back(parameters, parameter); }

                return nmos::details::make_nc_method_descriptor(description, id, name, result_datatype, parameters, is_deprecated);
            }
        }
        // create control class method descriptor
        method make_control_class_method_descriptor(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype, const std::vector<web::json::value>& parameters, bool is_deprecated, control_protocol_method_handler method_handler)
        {
            return make_control_class_method(details::make_control_class_method_descriptor(description, id, name, result_datatype, parameters, is_deprecated), method_handler);
        }

        // create control class event descriptor
        web::json::value make_control_class_event_descriptor(const utility::string_t& description, const nc_event_id& id, const nc_name& name, const utility::string_t& event_datatype, bool is_deprecated)
        {
            return nmos::details::make_nc_event_descriptor(description, id, name, event_datatype, is_deprecated);
        }

        namespace details
        {
            nmos::experimental::control_protocol_method_handler make_nc_get_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
            {
                return [get_control_protocol_class_descriptor](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return get(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_set_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return set(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_sequence_item_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
            {
                return [get_control_protocol_class_descriptor](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return get_sequence_item(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_set_sequence_item_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return set_sequence_item(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_add_sequence_item_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return add_sequence_item(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_remove_sequence_item_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
            {
                return [get_control_protocol_class_descriptor](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return remove_sequence_item(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_sequence_length_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
            {
                return [get_control_protocol_class_descriptor](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return get_sequence_length(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_member_descriptors_handler()
            {
                return [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return get_member_descriptors(resources, resource, arguments, is_deprecated, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_find_members_by_path_handler()
            {
                return [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return find_members_by_path(resources, resource, arguments, is_deprecated, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_find_members_by_role_handler()
            {
                return [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return find_members_by_role(resources, resource, arguments, is_deprecated, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_find_members_by_class_id_handler()
            {
                return [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return find_members_by_class_id(resources, resource, arguments, is_deprecated, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_control_class_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
            {
                return [get_control_protocol_class_descriptor](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return get_control_class(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_datatype_handler(get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor)
            {
                return [get_control_protocol_datatype_descriptor](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return get_datatype(resources, resource, arguments, is_deprecated, get_control_protocol_datatype_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_properties_by_path_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, get_properties_by_path_handler get_properties_by_path)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_properties_by_path](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    bool recurse = nmos::fields::nc::recurse(arguments);

                    // Delegate to user defined handler

                    auto result = nmos::details::make_nc_method_result_error({ nmos::nc_method_status::method_not_implemented }, U("not implemented"));
                    if (get_properties_by_path)
                    {
                        result = get_properties_by_path(get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, resource, recurse);

                        const auto& status = nmos::fields::nc::status(result);
                        if (!web::http::is_error_status_code((web::http::status_code)status) && is_deprecated)
                        {
                            return nmos::details::make_nc_method_result({ nmos::nc_method_status::method_deprecated }, nmos::fields::nc::value(result));
                        }
                    }
                    return result;
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_validate_set_properties_by_path_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, validate_set_properties_by_path_handler validate_set_properties_by_path)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, validate_set_properties_by_path](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    bool recurse = nmos::fields::nc::recurse(arguments);
                    const auto& included_property_traits = nmos::fields::nc::included_property_traits(arguments);
                    const auto& data_set = nmos::fields::nc::data_set(arguments);

                    if (data_set.is_null())
                    {
                        return nmos::details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("Null dataSet parameter"));
                    }

                    auto result = nmos::details::make_nc_method_result_error({ nmos::nc_method_status::method_not_implemented }, U("not implemented"));
                    if (validate_set_properties_by_path)
                    {
                        result = validate_set_properties_by_path(get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, resource, data_set, recurse, included_property_traits);

                        const auto& status = nmos::fields::nc::status(result);
                        if (!web::http::is_error_status_code((web::http::status_code)status) && is_deprecated)
                        {
                            return nmos::details::make_nc_method_result({ nmos::nc_method_status::method_deprecated }, nmos::fields::nc::value(result));
                        }
                    }
                    return result;
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_set_properties_by_path_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, set_properties_by_path_handler set_properties_by_path)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, set_properties_by_path](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    bool recurse = nmos::fields::nc::recurse(arguments);
                    const auto& included_property_traits = nmos::fields::nc::included_property_traits(arguments);
                    const auto& data_set = nmos::fields::nc::data_set(arguments);

                    if (data_set.is_null())
                    {
                        return nmos::details::make_nc_method_result_error({ nc_method_status::parameter_error }, U("Null dataSet parameter"));
                    }

                    auto result = nmos::details::make_nc_method_result_error({ nmos::nc_method_status::method_not_implemented }, U("not implemented"));
                    if (set_properties_by_path)
                    {
                        result = set_properties_by_path(get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, resource, data_set, recurse, included_property_traits);

                        const auto& status = nmos::fields::nc::status(result);
                        if (!web::http::is_error_status_code((web::http::status_code)status) && is_deprecated)
                        {
                            return nmos::details::make_nc_method_result({ nmos::nc_method_status::method_deprecated }, nmos::fields::nc::value(result));
                        }
                    }
                    return result;
                };
            }
        }

        control_protocol_state::control_protocol_state(control_protocol_property_changed_handler property_changed, get_properties_by_path_handler get_properties_by_path, validate_set_properties_by_path_handler validate_set_properties_by_path, set_properties_by_path_handler set_properties_by_path)
        {
            auto to_vector = [](const web::json::value& data)
            {
                if (!data.is_null())
                {
                    return std::vector<web::json::value>(data.as_array().begin(), data.as_array().end());
                }
                return std::vector<web::json::value>{};
            };

            auto to_methods_vector = [](const web::json::value& nc_method_descriptors, const std::map<nmos::nc_method_id, const control_protocol_method_handler>& method_handlers)
            {
                // NcMethodDescriptor method handler array
                std::vector<method> methods;

                if (!nc_method_descriptors.is_null())
                {
                    for (const auto& nc_method_descriptor : nc_method_descriptors.as_array())
                    {
                        methods.push_back(make_control_class_method(nc_method_descriptor, method_handlers.at(nmos::details::parse_nc_method_id(nmos::fields::nc::id(nc_method_descriptor)))));
                    }
                }
                return methods;
            };

            auto get_control_protocol_class_descriptor = make_get_control_protocol_class_descriptor_handler(*this);
            auto get_control_protocol_datatype_descriptor = make_get_control_protocol_datatype_descriptor_handler(*this);

            // setup the standard control classes
            control_class_descriptors =
            {
                // Control class models
                // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/

                // NcObject
                { nc_object_class_id, make_control_class_descriptor(U("NcObject class descriptor"), nc_object_class_id, U("NcObject"),
                    // NcObject properties
                    to_vector(make_nc_object_properties()),
                    // NcObject methods
                    to_methods_vector(make_nc_object_methods(),
                    {
                        // link NcObject method_ids with method functions
                        { nc_object_get_method_id, details::make_nc_get_handler(get_control_protocol_class_descriptor) },
                        { nc_object_set_method_id, details::make_nc_set_handler(get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed) },
                        { nc_object_get_sequence_item_method_id, details::make_nc_get_sequence_item_handler(get_control_protocol_class_descriptor) },
                        { nc_object_set_sequence_item_method_id, details::make_nc_set_sequence_item_handler(get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed) },
                        { nc_object_add_sequence_item_method_id, details::make_nc_add_sequence_item_handler(get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed) },
                        { nc_object_remove_sequence_item_method_id, details::make_nc_remove_sequence_item_handler(get_control_protocol_class_descriptor) },
                        { nc_object_get_sequence_length_method_id, details::make_nc_get_sequence_length_handler(get_control_protocol_class_descriptor) }
                    }),
                    // NcObject events
                    to_vector(make_nc_object_events())) },
                // NcBlock
                { nc_block_class_id, make_control_class_descriptor(U("NcBlock class descriptor"), nc_block_class_id, U("NcBlock"),
                    // NcBlock properties
                    to_vector(make_nc_block_properties()),
                    // NcBlock methods
                    to_methods_vector(make_nc_block_methods(),
                    {
                        // link NcBlock method_ids with method functions
                        { nc_block_get_member_descriptors_method_id, details::make_nc_get_member_descriptors_handler() },
                        { nc_block_find_members_by_path_method_id, details::make_nc_find_members_by_path_handler() },
                        { nc_block_find_members_by_role_method_id, details::make_nc_find_members_by_role_handler() },
                        { nc_block_find_members_by_class_id_method_id, details::make_nc_find_members_by_class_id_handler() }
                    }),
                    // NcBlock events
                    to_vector(make_nc_block_events())) },
                // NcWorker
                { nc_worker_class_id, make_control_class_descriptor(U("NcWorker class descriptor"), nc_worker_class_id, U("NcWorker"),
                    // NcWorker properties
                    to_vector(make_nc_worker_properties()),
                    // NcWorker methods
                    to_methods_vector(make_nc_worker_methods(), {}),
                    // NcWorker events
                    to_vector(make_nc_worker_events())) },
                // NcManager
                { nc_manager_class_id, make_control_class_descriptor(U("NcManager class descriptor"), nc_manager_class_id, U("NcManager"),
                    // NcManager properties
                    to_vector(make_nc_manager_properties()),
                    // NcManager methods
                    to_methods_vector(make_nc_manager_methods(), {}),
                    // NcManager events
                    to_vector(make_nc_manager_events())) },
                // NcDeviceManager
                { nc_device_manager_class_id, make_control_class_descriptor(U("NcDeviceManager class descriptor"), nc_device_manager_class_id, U("NcDeviceManager"), U("DeviceManager"),
                    // NcDeviceManager properties
                    to_vector(make_nc_device_manager_properties()),
                    // NcDeviceManager methods
                    to_methods_vector(make_nc_device_manager_methods(), {}),
                    // NcDeviceManager events
                    to_vector(make_nc_device_manager_events())) },
                // NcClassManager
                { nc_class_manager_class_id, make_control_class_descriptor(U("NcClassManager class descriptor"), nc_class_manager_class_id, U("NcClassManager"), U("ClassManager"),
                    // NcClassManager properties
                    to_vector(make_nc_class_manager_properties()),
                    // NcClassManager methods
                    to_methods_vector(make_nc_class_manager_methods(),
                    {
                        // link NcClassManager method_ids with method functions
                        { nc_class_manager_get_control_class_method_id, details::make_nc_get_control_class_handler(get_control_protocol_class_descriptor) },
                        { nc_class_manager_get_datatype_method_id, details::make_nc_get_datatype_handler(get_control_protocol_datatype_descriptor) }
                    }),
                    // NcClassManager events
                    to_vector(make_nc_class_manager_events())) },
                // Identification feature set
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#control-classes
                // NcIdentBeacon
                { nc_ident_beacon_class_id, make_control_class_descriptor(U("NcIdentBeacon class descriptor"), nc_ident_beacon_class_id, U("NcIdentBeacon"),
                    // NcIdentBeacon properties
                    to_vector(make_nc_ident_beacon_properties()),
                    // NcIdentBeacon methods
                    to_methods_vector(make_nc_ident_beacon_methods(), {}),
                    // NcIdentBeacon events
                    to_vector(make_nc_ident_beacon_events())) },
                // Monitoring feature set
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#control-classes
                // NcReceiverMonitor
                { nc_receiver_monitor_class_id, make_control_class_descriptor(U("NcReceiverMonitor class descriptor"), nc_receiver_monitor_class_id, U("NcReceiverMonitor"),
                    // NcReceiverMonitor properties
                    to_vector(make_nc_receiver_monitor_properties()),
                    // NcReceiverMonitor methods
                    to_methods_vector(make_nc_receiver_monitor_methods(), {}),
                    // NcReceiverMonitor events
                    to_vector(make_nc_receiver_monitor_events())) },
                // NcReceiverMonitorProtected
                { nc_receiver_monitor_protected_class_id, make_control_class_descriptor(U("NcReceiverMonitorProtected class descriptor"), nc_receiver_monitor_protected_class_id, U("NcReceiverMonitorProtected"),
                    // NcReceiverMonitorProtected properties
                    to_vector(make_nc_receiver_monitor_protected_properties()),
                    // NcReceiverMonitorProtected methods
                    to_methods_vector(make_nc_receiver_monitor_protected_methods(), {}),
                    // NcReceiverMonitorProtected events
                    to_vector(make_nc_receiver_monitor_protected_events())) },
                // NcBulkPropertiesManager
                { nc_bulk_properties_manager_class_id, make_control_class_descriptor(U("NcBulkPropertiesManager class descriptor"), nc_bulk_properties_manager_class_id, U("NcBulkPropertiesManager"), U("BulkPropertiesManager"),
                    to_vector(make_nc_bulk_properties_manager_properties()),
                    to_methods_vector(make_nc_bulk_properties_manager_methods(),
                    {
                        { nc_bulk_properties_manager_get_properties_by_path_method_id, details::make_nc_get_properties_by_path_handler(make_get_control_protocol_class_descriptor_handler(*this), make_get_control_protocol_datatype_descriptor_handler(*this), get_properties_by_path)},
                        { nc_bulk_properties_manager_validate_set_properties_by_path_method_id, details::make_nc_validate_set_properties_by_path_handler(make_get_control_protocol_class_descriptor_handler(*this), make_get_control_protocol_datatype_descriptor_handler(*this), validate_set_properties_by_path) },
                        { nc_bulk_properties_manager_set_properties_by_path_method_id, details::make_nc_set_properties_by_path_handler(make_get_control_protocol_class_descriptor_handler(*this), make_get_control_protocol_datatype_descriptor_handler(*this), set_properties_by_path) }
                    }),
                    to_vector(make_nc_bulk_properties_manager_events())) }
            };

            // setup the standard datatypes
            datatype_descriptors =
            {
                // Datatype models
                // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/
                { U("NcBoolean"), {make_nc_boolean_datatype()} },
                { U("NcInt16"), {make_nc_int16_datatype()} },
                { U("NcInt32"), {make_nc_int32_datatype()} },
                { U("NcInt64"), {make_nc_int64_datatype()} },
                { U("NcUint16"), {make_nc_uint16_datatype()} },
                { U("NcUint32"), {make_nc_uint32_datatype()} },
                { U("NcUint64"), {make_nc_uint64_datatype()} },
                { U("NcFloat32"), {make_nc_float32_datatype()} },
                { U("NcFloat64"), {make_nc_float64_datatype()} },
                { U("NcString"), {make_nc_string_datatype()} },
                { U("NcClassId"), {make_nc_class_id_datatype()} },
                { U("NcOid"), {make_nc_oid_datatype()} },
                { U("NcTouchpoint"), {make_nc_touchpoint_datatype()} },
                { U("NcElementId"), {make_nc_element_id_datatype()} },
                { U("NcPropertyId"), {make_nc_property_id_datatype()} },
                { U("NcPropertyConstraints"), {make_nc_property_contraints_datatype()} },
                { U("NcMethodResultPropertyValue"), {make_nc_method_result_property_value_datatype()} },
                { U("NcMethodStatus"), {make_nc_method_status_datatype()} },
                { U("NcMethodResult"), {make_nc_method_result_datatype()} },
                { U("NcId"), {make_nc_id_datatype()} },
                { U("NcMethodResultId"), {make_nc_method_result_id_datatype()} },
                { U("NcMethodResultLength"), {make_nc_method_result_length_datatype()} },
                { U("NcPropertyChangeType"), {make_nc_property_change_type_datatype()} },
                { U("NcPropertyChangedEventData"), {make_nc_property_changed_event_data_datatype()} },
                { U("NcDescriptor"), {make_nc_descriptor_datatype()} },
                { U("NcBlockMemberDescriptor"), {make_nc_block_member_descriptor_datatype()} },
                { U("NcMethodResultBlockMemberDescriptors"), {make_nc_method_result_block_member_descriptors_datatype()} },
                { U("NcVersionCode"), {make_nc_version_code_datatype()} },
                { U("NcOrganizationId"), {make_nc_organization_id_datatype()} },
                { U("NcUri"), {make_nc_uri_datatype()} },
                { U("NcManufacturer"), {make_nc_manufacturer_datatype()} },
                { U("NcUuid"), {make_nc_uuid_datatype()} },
                { U("NcProduct"), {make_nc_product_datatype()} },
                { U("NcDeviceGenericState"), {make_nc_device_generic_state_datatype()} },
                { U("NcDeviceOperationalState"), {make_nc_device_operational_state_datatype()} },
                { U("NcResetCause"), {make_nc_reset_cause_datatype()} },
                { U("NcName"), {make_nc_name_datatype()} },
                { U("NcPropertyDescriptor"), {make_nc_property_descriptor_datatype()} },
                { U("NcParameterDescriptor"), {make_nc_parameter_descriptor_datatype()} },
                { U("NcMethodId"), {make_nc_method_id_datatype()} },
                { U("NcMethodDescriptor"), {make_nc_method_descriptor_datatype()} },
                { U("NcEventId"), {make_nc_event_id_datatype()} },
                { U("NcEventDescriptor"), {make_nc_event_descriptor_datatype()} },
                { U("NcClassDescriptor"), {make_nc_class_descriptor_datatype()} },
                { U("NcParameterConstraints"), {make_nc_parameter_constraints_datatype()} },
                { U("NcDatatypeType"), {make_nc_datatype_type_datatype()} },
                { U("NcDatatypeDescriptor"), {make_nc_datatype_descriptor_datatype()} },
                { U("NcMethodResultClassDescriptor"), {make_nc_method_result_class_descriptor_datatype()} },
                { U("NcMethodResultDatatypeDescriptor"), {make_nc_method_result_datatype_descriptor_datatype()} },
                { U("NcMethodResultError"), {make_nc_method_result_error_datatype()} },
                { U("NcDatatypeDescriptorEnum"), {make_nc_datatype_descriptor_enum_datatype()} },
                { U("NcDatatypeDescriptorPrimitive"), {make_nc_datatype_descriptor_primitive_datatype()} },
                { U("NcDatatypeDescriptorStruct"), {make_nc_datatype_descriptor_struct_datatype()} },
                { U("NcDatatypeDescriptorTypeDef"), {make_nc_datatype_descriptor_type_def_datatype()} },
                { U("NcEnumItemDescriptor"), {make_nc_enum_item_descriptor_datatype()} },
                { U("NcFieldDescriptor"), {make_nc_field_descriptor_datatype()} },
                { U("NcPropertyConstraintsNumber"), {make_nc_property_constraints_number_datatype()} },
                { U("NcPropertyConstraintsString"), {make_nc_property_constraints_string_datatype()} },
                { U("NcRegex"), {make_nc_regex_datatype()} },
                { U("NcRolePath"), {make_nc_role_path_datatype()} },
                { U("NcParameterConstraintsNumber"), {make_nc_parameter_constraints_number_datatype()} },
                { U("NcParameterConstraintsString"), {make_nc_parameter_constraints_string_datatype()} },
                { U("NcTimeInterval"), {make_nc_time_interval_datatype()} },
                { U("NcTouchpointNmos"), {make_nc_touchpoint_nmos_datatype()} },
                { U("NcTouchpointNmosChannelMapping"), {make_nc_touchpoint_nmos_channel_mapping_datatype()} },
                { U("NcTouchpointResource"), {make_nc_touchpoint_resource_datatype()} },
                { U("NcTouchpointResourceNmos"), {make_nc_touchpoint_resource_nmos_datatype()} },
                { U("NcTouchpointResourceNmosChannelMapping"), {make_nc_touchpoint_resource_nmos_channel_mapping_datatype()} },
                // Monitoring feature set
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#datatypes
                { U("NcConnectionStatus"), {make_nc_connection_status_datatype()} },
                { U("NcPayloadStatus"), {make_nc_payload_status_datatype()} },
                // Device configuration feature set
                // TODO: add link
                { U("NcPropertyTrait"), {make_nc_property_trait_datatype()} },
                { U("NcPropertyValueHolder"), {make_nc_property_value_holder_datatype()}},
                { U("NcObjectPropertiesHolder"), {make_nc_object_properties_holder_datatype()}},
                { U("NcBulkValuesHolder"), {make_nc_bulk_values_holder_datatype()}},
                { U("NcRestoreValidationStatus"), {make_nc_restore_validation_status_datatype()}},
                { U("NcObjectPropertiesSetValidation"), {make_nc_object_properties_set_validation_datatype()}},
                { U("NcMethodResultBulkValuesHolder"), {make_nc_method_result_bulk_values_holder_datatype()}},
                { U("NcMethodResultObjectPropertiesSetValidation"), {make_nc_method_result_object_properties_set_validation_datatype()}}
            };
        }

        // insert control class descriptor, false if class descriptor already inserted
        bool control_protocol_state::insert(const experimental::control_class_descriptor& control_class_descriptor)
        {
            auto lock = write_lock();

            if (control_class_descriptors.end() == control_class_descriptors.find(control_class_descriptor.class_id))
            {
                control_class_descriptors[control_class_descriptor.class_id] = control_class_descriptor;
                return true;
            }
            return false;
        }

        // erase control class descriptor of the given class id, false if the required class descriptor not found
        bool control_protocol_state::erase(nc_class_id class_id)
        {
            auto lock = write_lock();

            if (control_class_descriptors.end() != control_class_descriptors.find(class_id))
            {
                control_class_descriptors.erase(class_id);
                return true;
            }
            return false;
        }

        // insert datatype descriptor, false if datatype descriptor already inserted
        bool control_protocol_state::insert(const experimental::datatype_descriptor& datatype_descriptor)
        {
            const auto& name = nmos::fields::nc::name(datatype_descriptor.descriptor);

            auto lock = write_lock();

            if (datatype_descriptors.end() == datatype_descriptors.find(name))
            {
                datatype_descriptors[name] = datatype_descriptor;
                return true;
            }
            return false;
        }

        // erase datatype descriptor of the given datatype name, false if the required datatype descriptor not found
        bool control_protocol_state::erase(const utility::string_t& datatype_name)
        {
            auto lock = write_lock();

            if (datatype_descriptors.end() != datatype_descriptors.find(datatype_name))
            {
                datatype_descriptors.erase(datatype_name);
                return true;
            }
            return false;
        }
    }
}
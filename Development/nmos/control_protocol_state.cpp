#include "nmos/control_protocol_state.h"

#include "cpprest/http_utils.h"
#include "nmos/control_protocol_methods.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/configuration_methods.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            // create control class descriptor
            //   where
            //     properties: vector of NcPropertyDescriptor where NcPropertyDescriptor can be constructed using make_control_class_property
            //     methods:    vector of NcMethodDescriptor vs assoicated method handler where NcMethodDescriptor can be constructed using make_method_descriptor
            //     events:     vector of NcEventDescriptor where NcEventDescriptor can be constructed using make_event_descriptor
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
        //     methods:    vector of NcMethodDescriptor where NcMethodDescriptor  can be constructed using make_method_descriptor and the assoicated method handler
        //     events:     vector of NcEventDescriptor where NcEventDescriptor can be constructed using make_event_descriptor
        control_class_descriptor make_control_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const utility::string_t& fixed_role, const std::vector<web::json::value>& properties, const std::vector<nmos::experimental::method>& methods, const std::vector<web::json::value>& events)
        {
            using web::json::value;

            return details::make_control_class_descriptor(description, class_id, name, value::string(fixed_role), properties, methods, events);
        }
        // create control class descriptor without fixed role
        //   where
        //     properties: vector of NcPropertyDescriptor where NcPropertyDescriptor can be constructed using make_control_class_property
        //     methods:    vector of NcMethodDescriptor where NcMethodDescriptor can be constructed using make_method_descriptor and the assoicated method handler
        //     events:     vector of NcEventDescriptor where NcEventDescriptor can be constructed using make_event_descriptor
        control_class_descriptor make_control_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const std::vector<web::json::value>& properties, const std::vector<nmos::experimental::method>& methods, const std::vector<web::json::value>& events)
        {
            using web::json::value;

            return details::make_control_class_descriptor(description, class_id, name, value::null(), properties, methods, events);
        }

        // create control class property descriptor
        web::json::value make_control_class_property_descriptor(const utility::string_t& description, const nc_property_id& id, const nc_name& name, const utility::string_t& type_name, bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints)
        {
            return nc::details::make_property_descriptor(description, id, name, type_name, is_read_only, is_nullable, is_sequence, is_deprecated, constraints);
        }

        // create control class method parameter descriptor
        web::json::value make_control_class_method_parameter_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints)
        {
            return nc::details::make_parameter_descriptor(description, name, type_name, is_nullable, is_sequence, constraints);
        }

        namespace details
        {
            web::json::value make_control_class_method_descriptor(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype, const std::vector<web::json::value>& parameters_, bool is_deprecated)
            {
                using web::json::value;

                value parameters = value::array();
                for (const auto& parameter : parameters_) { web::json::push_back(parameters, parameter); }

                return nc::details::make_method_descriptor(description, id, name, result_datatype, parameters, is_deprecated);
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
            return nc::details::make_event_descriptor(description, id, name, event_datatype, is_deprecated);
        }

        namespace details
        {
            nmos::experimental::control_protocol_method_handler make_nc_get_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
            {
                return [get_control_protocol_class_descriptor](nmos::resources&, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::get(resource, arguments, is_deprecated, get_control_protocol_class_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_set_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::set(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_sequence_item_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
            {
                return [get_control_protocol_class_descriptor](nmos::resources&, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::get_sequence_item(resource, arguments, is_deprecated, get_control_protocol_class_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_set_sequence_item_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::set_sequence_item(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_add_sequence_item_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::add_sequence_item(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_remove_sequence_item_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, control_protocol_property_changed_handler property_changed)
            {
                return [get_control_protocol_class_descriptor, property_changed](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::remove_sequence_item(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, property_changed, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_sequence_length_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
            {
                return [get_control_protocol_class_descriptor](nmos::resources&, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::get_sequence_length(resource, arguments, is_deprecated, get_control_protocol_class_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_member_descriptors_handler()
            {
                return [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::get_member_descriptors(resources, resource, arguments, is_deprecated, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_find_members_by_path_handler()
            {
                return [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::find_members_by_path(resources, resource, arguments, is_deprecated, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_find_members_by_role_handler()
            {
                return [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::find_members_by_role(resources, resource, arguments, is_deprecated, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_find_members_by_class_id_handler()
            {
                return [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::find_members_by_class_id(resources, resource, arguments, is_deprecated, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_control_class_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
            {
                return [get_control_protocol_class_descriptor](nmos::resources&, const nmos::resource&, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::get_control_class(arguments, is_deprecated, get_control_protocol_class_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_datatype_handler(get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor)
            {
                return [get_control_protocol_datatype_descriptor](nmos::resources&, const nmos::resource&, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::get_datatype(arguments, is_deprecated, get_control_protocol_datatype_descriptor, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_properties_by_path_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, create_validation_fingerprint_handler create_validation_fingerprint)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, create_validation_fingerprint](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    bool recurse = nmos::fields::nc::recurse(arguments);
                    bool include_descriptors = nmos::fields::nc::include_descriptors(arguments);

                    return nmos::get_properties_by_path(resources, resource, recurse, include_descriptors, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, create_validation_fingerprint);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_validate_set_properties_by_path_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, validate_validation_fingerprint_handler validate_validation_fingerprint, get_read_only_modification_allow_list_handler get_read_only_modification_allow_list, remove_device_model_object_handler remove_device_model_object, create_device_model_object_handler create_device_model_object)
            {
                return [&get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, validate_validation_fingerprint, remove_device_model_object, create_device_model_object](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    bool recurse = nmos::fields::nc::recurse(arguments);
                    const auto& restore_mode = nmos::fields::nc::restore_mode(arguments);
                    const auto& data_set = nmos::fields::nc::data_set(arguments);

                    if (data_set.is_null())
                    {
                        return nc::details::make_method_result_error({ nc_method_status::parameter_error }, U("Null dataSet parameter"));
                    }

                    auto result = nc::details::make_method_result_error({ nmos::nc_method_status::method_not_implemented }, U("not implemented"));
                    if (get_read_only_modification_allow_list && remove_device_model_object && create_device_model_object)
                    {
                        result = validate_set_properties_by_path(resources, resource, data_set, recurse, static_cast<nmos::nc_restore_mode::restore_mode>(restore_mode), get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, validate_validation_fingerprint, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object);

                        const auto& status = nmos::fields::nc::status(result);
                        if (!web::http::is_error_status_code((web::http::status_code)status) && is_deprecated)
                        {
                            return nc::details::make_method_result({ nmos::nc_method_status::method_deprecated }, nmos::fields::nc::value(result));
                        }
                    }
                    return result;
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_set_properties_by_path_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, validate_validation_fingerprint_handler validate_validation_fingerprint, get_read_only_modification_allow_list_handler get_read_only_modification_allow_list, remove_device_model_object_handler remove_device_model_object, create_device_model_object_handler create_device_model_object)
            {
                return [get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, validate_validation_fingerprint, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    bool recurse = nmos::fields::nc::recurse(arguments);
                    const auto& restore_mode = nmos::fields::nc::restore_mode(arguments);
                    const auto& data_set = nmos::fields::nc::data_set(arguments);

                    if (data_set.is_null())
                    {
                        return nc::details::make_method_result_error({ nc_method_status::parameter_error }, U("Null dataSet parameter"));
                    }

                    auto result = nc::details::make_method_result_error({ nmos::nc_method_status::method_not_implemented }, U("callbacks not implemented"));
                    if (get_read_only_modification_allow_list && remove_device_model_object && create_device_model_object)
                    {
                        result = set_properties_by_path(resources, resource, data_set, recurse, static_cast<nmos::nc_restore_mode::restore_mode>(restore_mode), get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, validate_validation_fingerprint, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object);

                        const auto& status = nmos::fields::nc::status(result);
                        if (!web::http::is_error_status_code((web::http::status_code)status) && is_deprecated)
                        {
                            return nc::details::make_method_result({ nmos::nc_method_status::method_deprecated }, nmos::fields::nc::value(result));
                        }
                    }
                    return result;
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_lost_packet_counters_handler(get_packet_counters_handler get_lost_packet_counters)
            {
                return [get_lost_packet_counters](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::get_lost_packet_counters(resources, resource, arguments, is_deprecated, get_lost_packet_counters, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_get_late_packet_counters_handler(get_packet_counters_handler get_late_packet_counters)
            {
                return [get_late_packet_counters](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::get_late_packet_counters(resources, resource, arguments, is_deprecated, get_late_packet_counters, gate);
                };
            }
            nmos::experimental::control_protocol_method_handler make_nc_reset_monitor_handler(get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, control_protocol_property_changed_handler property_changed, reset_monitor_handler reset_monitor)
            {
                return [get_control_protocol_class_descriptor, property_changed, reset_monitor](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
                {
                    return nc::reset_monitor(resources, resource, arguments, is_deprecated, get_control_protocol_class_descriptor, property_changed, reset_monitor, gate);
                };
            }
        }

        control_protocol_state::control_protocol_state(control_protocol_property_changed_handler property_changed, create_validation_fingerprint_handler create_validation_fingerprint, validate_validation_fingerprint_handler validate_validation_fingerprint, get_read_only_modification_allow_list_handler get_read_only_modification_allow_list, remove_device_model_object_handler remove_device_model_object, create_device_model_object_handler create_device_model_object, get_packet_counters_handler get_lost_packet_counters, get_packet_counters_handler get_late_packet_counters, reset_monitor_handler reset_monitor)
        : monitor_status_pending(false)
        {
            using web::json::value;

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
                        methods.push_back(make_control_class_method(nc_method_descriptor, method_handlers.at(nc::details::parse_method_id(nmos::fields::nc::id(nc_method_descriptor)))));
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
                    to_vector(nc::make_object_properties()),
                    // NcObject methods
                    to_methods_vector(nc::make_object_methods(),
                    {
                        // link NcObject method_ids with method functions
                        { nc_object_get_method_id, details::make_nc_get_handler(get_control_protocol_class_descriptor) },
                        { nc_object_set_method_id, details::make_nc_set_handler(get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed) },
                        { nc_object_get_sequence_item_method_id, details::make_nc_get_sequence_item_handler(get_control_protocol_class_descriptor) },
                        { nc_object_set_sequence_item_method_id, details::make_nc_set_sequence_item_handler(get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed) },
                        { nc_object_add_sequence_item_method_id, details::make_nc_add_sequence_item_handler(get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, property_changed) },
                        { nc_object_remove_sequence_item_method_id, details::make_nc_remove_sequence_item_handler(get_control_protocol_class_descriptor, property_changed) },
                        { nc_object_get_sequence_length_method_id, details::make_nc_get_sequence_length_handler(get_control_protocol_class_descriptor) }
                    }),
                    // NcObject events
                    to_vector(nc::make_object_events())) },
                // NcBlock
                { nc_block_class_id, make_control_class_descriptor(U("NcBlock class descriptor"), nc_block_class_id, U("NcBlock"),
                    // NcBlock properties
                    to_vector(nc::make_block_properties()),
                    // NcBlock methods
                    to_methods_vector(nc::make_block_methods(),
                    {
                        // link NcBlock method_ids with method functions
                        { nc_block_get_member_descriptors_method_id, details::make_nc_get_member_descriptors_handler() },
                        { nc_block_find_members_by_path_method_id, details::make_nc_find_members_by_path_handler() },
                        { nc_block_find_members_by_role_method_id, details::make_nc_find_members_by_role_handler() },
                        { nc_block_find_members_by_class_id_method_id, details::make_nc_find_members_by_class_id_handler() }
                    }),
                    // NcBlock events
                    to_vector(nc::make_block_events())) },
                // NcWorker
                { nc_worker_class_id, make_control_class_descriptor(U("NcWorker class descriptor"), nc_worker_class_id, U("NcWorker"),
                    // NcWorker properties
                    to_vector(nc::make_worker_properties()),
                    // NcWorker methods
                    to_methods_vector(nc::make_worker_methods(), {}),
                    // NcWorker events
                    to_vector(nc::make_worker_events())) },
                // NcManager
                { nc_manager_class_id, make_control_class_descriptor(U("NcManager class descriptor"), nc_manager_class_id, U("NcManager"),
                    // NcManager properties
                    to_vector(nc::make_manager_properties()),
                    // NcManager methods
                    to_methods_vector(nc::make_manager_methods(), {}),
                    // NcManager events
                    to_vector(nc::make_manager_events())) },
                // NcDeviceManager
                { nc_device_manager_class_id, make_control_class_descriptor(U("NcDeviceManager class descriptor"), nc_device_manager_class_id, U("NcDeviceManager"), U("DeviceManager"),
                    // NcDeviceManager properties
                    to_vector(nc::make_device_manager_properties()),
                    // NcDeviceManager methods
                    to_methods_vector(nc::make_device_manager_methods(), {}),
                    // NcDeviceManager events
                    to_vector(nc::make_device_manager_events())) },
                // NcClassManager
                { nc_class_manager_class_id, make_control_class_descriptor(U("NcClassManager class descriptor"), nc_class_manager_class_id, U("NcClassManager"), U("ClassManager"),
                    // NcClassManager properties
                    to_vector(nc::make_class_manager_properties()),
                    // NcClassManager methods
                    to_methods_vector(nc::make_class_manager_methods(),
                    {
                        // link NcClassManager method_ids with method functions
                        { nc_class_manager_get_control_class_method_id, details::make_nc_get_control_class_handler(get_control_protocol_class_descriptor) },
                        { nc_class_manager_get_datatype_method_id, details::make_nc_get_datatype_handler(get_control_protocol_datatype_descriptor) }
                    }),
                    // NcClassManager events
                    to_vector(nc::make_class_manager_events())) },
                // Identification feature set
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#control-classes
                // NcIdentBeacon
                { nc_ident_beacon_class_id, make_control_class_descriptor(U("NcIdentBeacon class descriptor"), nc_ident_beacon_class_id, U("NcIdentBeacon"),
                    // NcIdentBeacon properties
                    to_vector(nc::make_ident_beacon_properties()),
                    // NcIdentBeacon methods
                    to_methods_vector(nc::make_ident_beacon_methods(), {}),
                    // NcIdentBeacon events
                    to_vector(nc::make_ident_beacon_events())) },
                // Monitoring feature set
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#control-classes
                // NcStatusMonitor
                { nc_status_monitor_class_id, make_control_class_descriptor(U("NcStatusMonitor class descriptor"), nc_status_monitor_class_id, U("NcStatusMonitor"),
                    // NcReceiverMonitor properties
                    to_vector(nc::make_status_monitor_properties()),
                    // NcReceiverMonitor methods
                    to_methods_vector(nc::make_status_monitor_methods(), {}),
                    // NcReceiverMonitor events
                    to_vector(nc::make_status_monitor_events())) },
                // NcReceiverMonitor
                { nc_receiver_monitor_class_id, make_control_class_descriptor(U("NcReceiverMonitor class descriptor"), nc_receiver_monitor_class_id, U("NcReceiverMonitor"),
                    // NcReceiverMonitor properties
                    to_vector(nc::make_receiver_monitor_properties()),
                    // NcReceiverMonitor methods
                    to_methods_vector(nc::make_receiver_monitor_methods(),
                    {
                        // link NcReceiverMonitor method_ids with method functions
                        { nc_receiver_monitor_get_lost_packet_counters_method_id, details::make_nc_get_lost_packet_counters_handler(get_lost_packet_counters)},
                        { nc_receiver_monitor_get_late_packet_counters_method_id, details::make_nc_get_late_packet_counters_handler(get_late_packet_counters)},
                        { nc_receiver_monitor_reset_monitor_method_id, details::make_nc_reset_monitor_handler(get_control_protocol_class_descriptor, property_changed, reset_monitor)}
                    }),
                    // NcReceiverMonitor events
                    to_vector(nc::make_receiver_monitor_events())) },
                // NcSenderMonitor
                { nc_sender_monitor_class_id, make_control_class_descriptor(U("NcSenderMonitor class descriptor"), nc_sender_monitor_class_id, U("NcSenderMonitor"),
                    // NcSenderMonitor properties
                    to_vector(nc::make_sender_monitor_properties()),
                    // NcSenderMonitor methods
                    to_methods_vector(nc::make_sender_monitor_methods(),
                    {
                        // link NcSenderMonitor method_ids with method functions
                        // TODO: implement actual GetTransmissionError and ResetCountersAndMessages function
                        { nc_sender_monitor_get_transmission_error_counters_method_id, details::make_nc_get_lost_packet_counters_handler(get_lost_packet_counters)},
                        { nc_sender_monitor_reset_monitor_method_id, details::make_nc_reset_monitor_handler(get_control_protocol_class_descriptor, property_changed, reset_monitor)}
                    }),
                    // NcSenderMonitor events
                    to_vector(nc::make_sender_monitor_events())) },
                // NcBulkPropertiesManager
                { nc_bulk_properties_manager_class_id, make_control_class_descriptor(U("NcBulkPropertiesManager class descriptor"), nc_bulk_properties_manager_class_id, U("NcBulkPropertiesManager"), U("BulkPropertiesManager"),
                    to_vector(nc::make_bulk_properties_manager_properties()),
                    to_methods_vector(nc::make_bulk_properties_manager_methods(),
                    {
                        { nc_bulk_properties_manager_get_properties_by_path_method_id, details::make_nc_get_properties_by_path_handler(make_get_control_protocol_class_descriptor_handler(*this), make_get_control_protocol_datatype_descriptor_handler(*this), create_validation_fingerprint)},
                        { nc_bulk_properties_manager_validate_set_properties_by_path_method_id, details::make_nc_validate_set_properties_by_path_handler(make_get_control_protocol_class_descriptor_handler(*this), make_get_control_protocol_datatype_descriptor_handler(*this), validate_validation_fingerprint, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object) },
                        { nc_bulk_properties_manager_set_properties_by_path_method_id, details::make_nc_set_properties_by_path_handler(make_get_control_protocol_class_descriptor_handler(*this), make_get_control_protocol_datatype_descriptor_handler(*this), validate_validation_fingerprint, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object) }
                    }),
                    to_vector(nc::make_bulk_properties_manager_events())) }
            };

            // setup the standard datatypes
            datatype_descriptors =
            {
                // Datatype models
                // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/
                { U("NcBoolean"), {nc::make_boolean_datatype()} },
                { U("NcInt16"), {nc::make_int16_datatype()} },
                { U("NcInt32"), {nc::make_int32_datatype()} },
                { U("NcInt64"), {nc::make_int64_datatype()} },
                { U("NcUint16"), {nc::make_uint16_datatype()} },
                { U("NcUint32"), {nc::make_uint32_datatype()} },
                { U("NcUint64"), {nc::make_uint64_datatype()} },
                { U("NcFloat32"), {nc::make_float32_datatype()} },
                { U("NcFloat64"), {nc::make_float64_datatype()} },
                { U("NcString"), {nc::make_string_datatype()} },
                { U("NcClassId"), {nc::make_class_id_datatype()} },
                { U("NcOid"), {nc::make_oid_datatype()} },
                { U("NcTouchpoint"), {nc::make_touchpoint_datatype()} },
                { U("NcElementId"), {nc::make_element_id_datatype()} },
                { U("NcPropertyId"), {nc::make_property_id_datatype()} },
                { U("NcPropertyConstraints"), {nc::make_property_contraints_datatype()} },
                { U("NcMethodResultPropertyValue"), {nc::make_method_result_property_value_datatype()} },
                { U("NcMethodStatus"), {nc::make_method_status_datatype()} },
                { U("NcMethodResult"), {nc::make_method_result_datatype()} },
                { U("NcId"), {nc::make_id_datatype()} },
                { U("NcMethodResultId"), {nc::make_method_result_id_datatype()} },
                { U("NcMethodResultLength"), {nc::make_method_result_length_datatype()} },
                { U("NcPropertyChangeType"), {nc::make_property_change_type_datatype()} },
                { U("NcPropertyChangedEventData"), {nc::make_property_changed_event_data_datatype()} },
                { U("NcDescriptor"), {nc::make_descriptor_datatype()} },
                { U("NcBlockMemberDescriptor"), {nc::make_block_member_descriptor_datatype()} },
                { U("NcMethodResultBlockMemberDescriptors"), {nc::make_method_result_block_member_descriptors_datatype()} },
                { U("NcVersionCode"), {nc::make_version_code_datatype()} },
                { U("NcOrganizationId"), {nc::make_organization_id_datatype()} },
                { U("NcUri"), {nc::make_uri_datatype()} },
                { U("NcManufacturer"), {nc::make_manufacturer_datatype()} },
                { U("NcUuid"), {nc::make_uuid_datatype()} },
                { U("NcProduct"), {nc::make_product_datatype()} },
                { U("NcDeviceGenericState"), {nc::make_device_generic_state_datatype()} },
                { U("NcDeviceOperationalState"), {nc::make_device_operational_state_datatype()} },
                { U("NcResetCause"), {nc::make_reset_cause_datatype()} },
                { U("NcName"), {nc::make_name_datatype()} },
                { U("NcPropertyDescriptor"), {nc::make_property_descriptor_datatype()} },
                { U("NcParameterDescriptor"), {nc::make_parameter_descriptor_datatype()} },
                { U("NcMethodId"), {nc::make_method_id_datatype()} },
                { U("NcMethodDescriptor"), {nc::make_method_descriptor_datatype()} },
                { U("NcEventId"), {nc::make_event_id_datatype()} },
                { U("NcEventDescriptor"), {nc::make_event_descriptor_datatype()} },
                { U("NcClassDescriptor"), {nc::make_class_descriptor_datatype()} },
                { U("NcParameterConstraints"), {nc::make_parameter_constraints_datatype()} },
                { U("NcDatatypeType"), {nc::make_datatype_type_datatype()} },
                { U("NcDatatypeDescriptor"), {nc::make_datatype_descriptor_datatype()} },
                { U("NcMethodResultClassDescriptor"), {nc::make_method_result_class_descriptor_datatype()} },
                { U("NcMethodResultDatatypeDescriptor"), {nc::make_method_result_datatype_descriptor_datatype()} },
                { U("NcMethodResultError"), {nc::make_method_result_error_datatype()} },
                { U("NcDatatypeDescriptorEnum"), {nc::make_datatype_descriptor_enum_datatype()} },
                { U("NcDatatypeDescriptorPrimitive"), {nc::make_datatype_descriptor_primitive_datatype()} },
                { U("NcDatatypeDescriptorStruct"), {nc::make_datatype_descriptor_struct_datatype()} },
                { U("NcDatatypeDescriptorTypeDef"), {nc::make_datatype_descriptor_type_def_datatype()} },
                { U("NcEnumItemDescriptor"), {nc::make_enum_item_descriptor_datatype()} },
                { U("NcFieldDescriptor"), {nc::make_field_descriptor_datatype()} },
                { U("NcPropertyConstraintsNumber"), {nc::make_property_constraints_number_datatype()} },
                { U("NcPropertyConstraintsString"), {nc::make_property_constraints_string_datatype()} },
                { U("NcRegex"), {nc::make_regex_datatype()} },
                { U("NcRolePath"), {nc::make_role_path_datatype()} },
                { U("NcParameterConstraintsNumber"), {nc::make_parameter_constraints_number_datatype()} },
                { U("NcParameterConstraintsString"), {nc::make_parameter_constraints_string_datatype()} },
                { U("NcTimeInterval"), {nc::make_time_interval_datatype()} },
                { U("NcTouchpointNmos"), {nc::make_touchpoint_nmos_datatype()} },
                { U("NcTouchpointNmosChannelMapping"), {nc::make_touchpoint_nmos_channel_mapping_datatype()} },
                { U("NcTouchpointResource"), {nc::make_touchpoint_resource_datatype()} },
                { U("NcTouchpointResourceNmos"), {nc::make_touchpoint_resource_nmos_datatype()} },
                { U("NcTouchpointResourceNmosChannelMapping"), {nc::make_touchpoint_resource_nmos_channel_mapping_datatype()} },
                // Monitoring feature set
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#datatypes
                { U("NcConnectionStatus"), {nc::make_connection_status_datatype()} },
                { U("NcCounter"), {nc::make_counter_datatype()} },
                { U("NcEssenceStatus"), {nc::make_essence_status_datatype()} },
                { U("NcLinkStatus"), {nc::make_link_status_datatype()} },
                { U("NcMethodResultCounters"), {nc::make_method_result_counters_datatype()} },
                { U("NcOverallStatus"), {nc::make_overall_status_datatype()} },
                { U("NcSynchronizationStatus"), {nc::make_synchronization_status_datatype()} },
                { U("NcStreamStatus"), {nc::make_stream_status_datatype()} },
                { U("NcTransmissionStatus"), {nc::make_transmission_status_datatype()} },
                // Device configuration feature set
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#datatypes
                { U("NcRestoreMode"), {nc::make_restore_mode_datatype()} },
                { U("NcPropertyHolder"), {nc::make_property_holder_datatype()} },
                { U("NcObjectPropertiesHolder"), {nc::make_object_properties_holder_datatype()} },
                { U("NcBulkPropertiesHolder"), {nc::make_bulk_properties_holder_datatype()} },
                { U("NcRestoreValidationStatus"), {nc::make_restore_validation_status_datatype()} },
                { U("NcPropertyRestoreNoticeType"), {nc::make_property_restore_notice_type_datatype()} },
                { U("NcPropertyRestoreNotice"), {nc::make_property_restore_notice_datatype()} },
                { U("NcObjectPropertiesSetValidation"), {nc::make_object_properties_set_validation_datatype()} },
                { U("NcMethodResultBulkPropertiesHolder"), {nc::make_method_result_bulk_properties_holder_datatype()} },
                { U("NcMethodResultObjectPropertiesSetValidation"), {nc::make_method_result_object_properties_set_validation_datatype()} }
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
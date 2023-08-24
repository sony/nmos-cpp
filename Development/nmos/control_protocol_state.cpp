#include "nmos/control_protocol_state.h"

#include "nmos/control_protocol_resource.h" // for nc_object_class_id, nc_block_class_id, nc_worker_class_id, nc_manager_class_id, nc_device_manager_class_id, nc_class_manager_class_id definitions

namespace nmos
{
    namespace experimental
    {
        // create control class property
        web::json::value make_control_class_property(const utility::string_t& description, const nc_property_id& id, const utility::string_t& name, const utility::string_t& type_name, bool is_read_only, bool is_nullable, bool is_sequence, bool is_deprecated, const web::json::value& constraints)
        {
            return nmos::details::make_nc_property_descriptor(description, id, name, type_name, is_read_only, is_nullable, is_sequence, is_deprecated, constraints);
        }

        namespace details
        {
            control_class make_control_class(const utility::string_t& description, const nc_class_id& class_id, const utility::string_t& name, const web::json::value& fixed_role, const std::vector<web::json::value>& properties_, const std::vector<web::json::value>& methods_, const std::vector<web::json::value>& events_)
            {
                using web::json::value;

                web::json::value properties = value::array();
                for (const auto& property : properties_) { web::json::push_back(properties, property); }
                web::json::value methods = value::array();
                for (const auto& method : methods_) { web::json::push_back(methods, method); }
                web::json::value events = value::array();
                for (const auto& event : events_) { web::json::push_back(events, event); }

                return { value::string(description), class_id, name, fixed_role, properties, methods, events };
            }
        }

        // create control class with fixed role
        control_class make_control_class(const utility::string_t& description, const nc_class_id& class_id, const utility::string_t& name, const utility::string_t& fixed_role, const std::vector<web::json::value>& properties, const std::vector<web::json::value>& methods, const std::vector<web::json::value>& events)
        {
            using web::json::value;

            return details::make_control_class(description, class_id, name, value::string(fixed_role), properties, methods, events);
        }
        // create control class with no fixed role
        control_class make_control_class(const utility::string_t& description, const nc_class_id& class_id, const utility::string_t& name, const std::vector<web::json::value>& properties, const std::vector<web::json::value>& methods, const std::vector<web::json::value>& events)
        {
            using web::json::value;

            return details::make_control_class(description, class_id, name, value::null(), properties, methods, events);
        }

        // create control class method parameter
        web::json::value make_control_class_method_parameter(const utility::string_t& description, const utility::string_t& name, const utility::string_t& type_name, bool is_nullable, bool is_sequence, const web::json::value& constraints)
        {
            return nmos::details::make_nc_parameter_descriptor(description, name, type_name, is_nullable, is_sequence, constraints);
        }

        // create control class method
        web::json::value make_control_class_method(const utility::string_t& description, const nc_method_id& id, const utility::string_t& name, const utility::string_t& result_datatype, const std::vector<web::json::value>& parameters_, bool is_deprecated)
        {
            using web::json::value;

            value parameters = value::array();
            for (const auto& parameter : parameters_) { web::json::push_back(parameters, parameter); }

            return nmos::details::make_nc_method_descriptor(description, id, name, result_datatype, parameters, is_deprecated);
        }

        // create control class event
        web::json::value make_control_class_event(const utility::string_t& description, const nc_event_id& id, const utility::string_t& name, const utility::string_t& event_datatype, bool is_deprecated)
        {
            return nmos::details::make_nc_event_descriptor(description, id, name, event_datatype, is_deprecated);
        }

        control_protocol_state::control_protocol_state()
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

            // setup the core control classes
            control_classes =
            {
                // Control class models
                // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/classes/#control-class-models-for-branch-v10-dev
                { nmos::details::make_nc_class_id(nc_object_class_id), make_control_class(U("NcObject class descriptor"), nc_object_class_id, U("NcObject"), to_vector(make_nc_object_properties()), to_vector(make_nc_object_methods()), to_vector(make_nc_object_events())) },
                { nmos::details::make_nc_class_id(nc_block_class_id), make_control_class(U("NcBlock class descriptor"), nc_block_class_id, U("NcBlock"), to_vector(make_nc_block_properties()), to_vector(make_nc_block_methods()), to_vector(make_nc_block_events())) },
                { nmos::details::make_nc_class_id(nc_worker_class_id), make_control_class(U("NcWorker class descriptor"), nc_worker_class_id, U("NcWorker"), to_vector(make_nc_worker_properties()), to_vector(make_nc_worker_methods()), to_vector(make_nc_worker_events())) },
                { nmos::details::make_nc_class_id(nc_manager_class_id), make_control_class(U("NcManager class descriptor"), nc_manager_class_id, U("NcManager"),to_vector(make_nc_manager_properties()), to_vector(make_nc_manager_methods()), to_vector(make_nc_manager_events())) },
                { nmos::details::make_nc_class_id(nc_device_manager_class_id), make_control_class(U("NcDeviceManager class descriptor"), nc_device_manager_class_id, U("NcDeviceManager"), U("DeviceManager"), to_vector(make_nc_device_manager_properties()), to_vector(make_nc_device_manager_methods()), to_vector(make_nc_device_manager_events())) },
                { nmos::details::make_nc_class_id(nc_class_manager_class_id), make_control_class(U("NcClassManager class descriptor"), nc_class_manager_class_id, U("NcClassManager"), U("ClassManager"), to_vector(make_nc_class_manager_properties()), to_vector(make_nc_class_manager_methods()), to_vector(make_nc_class_manager_events())) },
                // identification beacon model
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
                { nmos::details::make_nc_class_id(nc_ident_beacon_class_id), make_control_class(U("NcIdentBeacon class descriptor"), nc_ident_beacon_class_id, U("NcIdentBeacon"), to_vector(make_nc_ident_beacon_properties()), to_vector(make_nc_ident_beacon_methods()), to_vector(make_nc_ident_beacon_events())) },
                // Monitoring
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
                { nmos::details::make_nc_class_id(nc_receiver_monitor_class_id), make_control_class(U("NcReceiverMonitor class descriptor"), nc_receiver_monitor_class_id, U("NcReceiverMonitor"), to_vector(make_nc_receiver_monitor_properties()), to_vector(make_nc_receiver_monitor_methods()), to_vector(make_nc_receiver_monitor_events())) },
                { nmos::details::make_nc_class_id(nc_receiver_monitor_protected_class_id), make_control_class(U("NcReceiverMonitorProtected class descriptor"), nc_receiver_monitor_protected_class_id, U("NcReceiverMonitorProtected"), to_vector(make_nc_receiver_monitor_protected_properties()), to_vector(make_nc_receiver_monitor_protected_methods()), to_vector(make_nc_receiver_monitor_protected_events())) }
            };

            // setup the core datatypes
            datatypes =
            {
                // Dataype models
                // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/models/datatypes/#datatype-models-for-branch-v10-dev
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
                // Monitoring
                // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#datatypes
                { U("NcConnectionStatus"), {make_nc_connection_status_datatype()} },
                { U("NcPayloadStatus"), {make_nc_payload_status_datatype()} }
            };
        }

        // insert control class, false if class already presented
        bool control_protocol_state::insert(const experimental::control_class& control_class)
        {
            const auto& class_id = nmos::details::make_nc_class_id(control_class.class_id);

            auto lock = write_lock();

            if (control_classes.end() == control_classes.find(class_id))
            {
                control_classes[nmos::details::make_nc_class_id(control_class.class_id)] = control_class;
                return true;
            }
            return false;
        }

        // erase control class of the given class id, false if the required class not found
        bool control_protocol_state::erase(nc_class_id class_id_)
        {
            const auto& class_id = nmos::details::make_nc_class_id(class_id_);

            auto lock = write_lock();

            if (control_classes.end() != control_classes.find(class_id))
            {
                control_classes.erase(class_id);
                return true;
            }
            return false;
        }

        // insert datatype, false if datatype already presented
        bool control_protocol_state::insert(const experimental::datatype& datatype)
        {
            const auto& name = nmos::fields::nc::name(datatype.descriptor);

            auto lock = write_lock();

            if (datatypes.end() == datatypes.find(name))
            {
                datatypes[name] = datatype;
                return true;
            }
            return false;
        }

        // erase datatype of the given datatype name, false if the required datatype not found
        bool control_protocol_state::erase(const utility::string_t& name)
        {
            auto lock = write_lock();

            if (datatypes.end() != datatypes.find(name))
            {
                datatypes.erase(name);
                return true;
            }
            return false;
        }
    }
}
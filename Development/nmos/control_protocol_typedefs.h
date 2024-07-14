#ifndef NMOS_CONTROL_PROTOCOL_TYPEDEFS_H
#define NMOS_CONTROL_PROTOCOL_TYPEDEFS_H

#include "cpprest/basic_utils.h"
#include "cpprest/json_utils.h"
#include "nmos/control_protocol_nmos_channel_mapping_resource_type.h"
#include "nmos/control_protocol_nmos_resource_type.h"

namespace nmos
{
    // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html
    namespace ncp_message_type
    {
        enum type
        {
            command = 0,
            command_response = 1,
            notification = 2,
            subscription = 3,
            subscription_response = 4,
            error = 5
        };
    }

    // Method invokation status
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethodstatus
    namespace nc_method_status
    {
        enum status
        {
            ok = 200,                       // Method call was successful
            property_deprecated = 298,      // Method call was successful but targeted property is deprecated
            method_deprecated = 299,        // Method call was successful but method is deprecated
            bad_command_format = 400,       // Badly-formed command
            unauthorized = 401,              // Client is not authorized
            bad_oid = 404,                  // Command addresses a nonexistent object
            read_only = 405,                // Attempt to change read-only state
            invalid_request = 406,          // Method call is invalid in current operating context
            conflict = 409,                 // There is a conflict with the current state of the device
            buffer_overflow = 413,          // Something was too big
            index_out_of_bounds = 414,      // Index is outside the available range
            parameter_error = 417,          // Method parameter does not meet expectations
            locked = 423,                   // Addressed object is locked
            device_error = 500,             // Internal device error
            method_not_implemented = 501,   // Addressed method is not implemented by the addressed object
            property_not_implemented = 502, // Addressed property is not implemented by the addressed object
            not_ready = 503,                // The device is not ready to handle any commands
            timeout = 504,                  // Method call did not finish within the allotted time
        };
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethodresult
    struct nc_method_result
    {
        nc_method_status::status status;
    };

    // Datatype type
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdatatypetype
    namespace nc_datatype_type
    {
        enum type
        {
            Primitive = 0,
            Typedef = 1,
            Struct = 2,
            Enum = 3
        };
    }

    // Device generic operational state
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicegenericstate
    namespace nc_device_generic_state
    {
        enum state
        {
            unknown = 0,            // Unknown
            normal_operation = 1,   // Normal operation
            initializing = 2,       // Device is initializing
            updating = 3,           // Device is performing a software or firmware update
            licensing_error = 4,    // Device is experiencing a licensing error
            internal_error = 5      // Device is experiencing an internal error
        };
    }

    // Reset cause enum
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncresetcause
    namespace nc_reset_cause
    {
        enum cause
        {
            unknown = 0,            // Unknown
            power_on = 1,           // Power on
            internal_error = 2,      // Internal error
            upgrade = 3,            // Upgrade
            controller_request = 4, // Controller request
            manual_reset = 5         // Manual request from the front panel
        };
    }

    // NcConnectionStatus
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncconnectionstatus
    namespace nc_connection_status
    {
        enum status
        {
            undefined = 0,          // This is the value when there is no receiver
            connected = 1,          // Connected to a stream
            disconnected = 2,       // Not connected to a stream
            connection_error = 3     // A connection error was encountered
        };
    }

    // NcPayloadStatus
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncpayloadstatus
    namespace nc_payload_status
    {
        enum status
        {
            undefined = 0,                  // This is the value when there's no connection.
            payload_ok = 1,                  // Payload is being received without errors and is the correct type
            payload_format_unsupported = 2,   // Payload is being received but is of an unsupported type
            payloadError = 3                // A payload error was encountered
        };
    }

    // Device Configuration
    // NcPropertyTrait
    namespace nc_property_trait
    {
        enum trait
        {
            instance_specific = 1, // Property is instance specific
            ephemeral = 2,         // Property is ephemeral
            immutable = 3,         // Property is immutable
            device_generated = 4   // Property value is generated by the device
        };
    }

    // NcRestoreValidationStatus
    namespace nc_restore_validation_status
    {
        enum staus
        {
            ok = 200,                 // Restore was successful
            excluded = 204,           // Excluded from restore due to data provided in the request
            invalid_data = 400,       // Restore failed because relevant backup data set provided is invalid
            not_found = 404,          // Restore failed because the role path is not found in the device model or the device cannot create the role path from the data set
            missing_dependency = 424, // Restore failed because of missing dependency information in the relevant backup data set
            device_error = 500        // Restore failed due to an internal device error preventing the restore from happening
        };
    }

    // NcElementId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncelementid
    struct nc_element_id
    {
        uint16_t level;
        uint16_t index;

        nc_element_id(uint16_t level, uint16_t index)
            : level(level)
            , index(index)
        {}

        auto tied() const -> decltype(std::tie(level, index)) { return std::tie(level, index); }
        friend bool operator==(const nc_element_id& lhs, const nc_element_id& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const nc_element_id& lhs, const nc_element_id& rhs) { return !(lhs == rhs); }
        friend bool operator<(const nc_element_id& lhs, const nc_element_id& rhs) { return lhs.tied() < rhs.tied(); }
    };

    // NcEventId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nceventid
    typedef nc_element_id nc_event_id;
    // NcEventIds for NcObject
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
    const nc_event_id nc_object_property_changed_event_id(1, 1);

    // NcMethodId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmethodid
    typedef nc_element_id nc_method_id;
    // NcMethodIds for NcObject
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
    const nc_method_id nc_object_get_method_id(1, 1);
    const nc_method_id nc_object_set_method_id(1, 2);
    const nc_method_id nc_object_get_sequence_item_method_id(1, 3);
    const nc_method_id nc_object_set_sequence_item_method_id(1, 4);
    const nc_method_id nc_object_add_sequence_item_method_id(1, 5);
    const nc_method_id nc_object_remove_sequence_item_method_id(1, 6);
    const nc_method_id nc_object_get_sequence_length_method_id(1, 7);
    // NcMethodIds for NcBlock
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
    const nc_method_id nc_block_get_member_descriptors_method_id(2, 1);
    const nc_method_id nc_block_find_members_by_path_method_id(2, 2);
    const nc_method_id nc_block_find_members_by_role_method_id(2, 3);
    const nc_method_id nc_block_find_members_by_class_id_method_id(2, 4);
    // NcMethodIds for NcClassManager
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
    const nc_method_id nc_class_manager_get_control_class_method_id(3, 1);
    const nc_method_id nc_class_manager_get_datatype_method_id(3, 2);
    // NcMethodsIds for NcBulkPropertiesManager
    // TODO: add link
    const nc_method_id nc_bulk_properties_manager_get_properties_by_path_method_id(3, 1);
    const nc_method_id nc_bulk_properties_manager_validate_set_properties_by_path_method_id(3, 2);
    const nc_method_id nc_bulk_properties_manager_set_properties_by_path_method_id(3, 3);

    // NcPropertyId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertyid
    typedef nc_element_id nc_property_id;
    // NcPropertyIds for NcObject
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
    const nc_property_id nc_object_class_id_property_id(1, 1);
    const nc_property_id nc_object_oid_property_id(1, 2);
    const nc_property_id nc_object_constant_oid_property_id(1, 3);
    const nc_property_id nc_object_owner_property_id(1, 4);
    const nc_property_id nc_object_role_property_id(1, 5);
    const nc_property_id nc_object_user_label_property_id(1, 6);
    const nc_property_id nc_object_touchpoints_property_id(1, 7);
    const nc_property_id nc_object_runtime_property_constraints_property_id(1, 8);
    // NcPropertyIds for NcBlock
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
    const nc_property_id nc_block_enabled_property_id(2, 1);
    const nc_property_id nc_block_members_property_id(2, 2);
    // NcPropertyIds for NcWorker
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncworker
    const nc_property_id nc_worker_enabled_property_id(2, 1);
    // NcPropertyIds for NcDeviceManager
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
    const nc_property_id nc_device_manager_nc_version_property_id(3, 1);
    const nc_property_id nc_device_manager_manufacturer_property_id(3, 2);
    const nc_property_id nc_device_manager_product_property_id(3, 3);
    const nc_property_id nc_device_manager_serial_number_property_id(3, 4);
    const nc_property_id nc_device_manager_user_inventory_code_property_id(3, 5);
    const nc_property_id nc_device_manager_device_name_property_id(3, 6);
    const nc_property_id nc_device_manager_device_role_property_id(3, 7);
    const nc_property_id nc_device_manager_operational_state_property_id(3, 8);
    const nc_property_id nc_device_manager_reset_cause_property_id(3, 9);
    const nc_property_id nc_device_manager_message_property_id(3, 10);
    // NcPropertyIds for NcClassManager
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
    const nc_property_id nc_class_manager_control_classes_property_id(3, 1);
    const nc_property_id nc_class_manager_datatypes_property_id(3, 2);
    // NcPropertyids for NcReceiverMonitor
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    const nc_property_id nc_receiver_monitor_connection_status_property_id(3, 1);
    const nc_property_id nc_receiver_monitor_connection_status_message_property_id(3, 2);
    const nc_property_id nc_receiver_monitor_payload_status_property_id(3, 3);
    const nc_property_id nc_receiver_monitor_payload_status_message_property_id(3, 4);
    // NcPropertyids for NcReceiverMonitorProtected
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    const nc_property_id nc_receiver_monitor_protected_signal_protection_status_property_id(4, 1);
    // NcPropertyids for NcIdentBeacon
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    const nc_property_id nc_ident_beacon_active_property_id(3, 1);

    // NcId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncid
    typedef uint32_t nc_id;

    // NcName
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncname
    typedef utility::string_t nc_name;

    // NcOid
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncoid
    typedef uint32_t nc_oid;

    // NcUri
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncuri
    typedef utility::string_t nc_uri;

    // NcUuid
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncuuid
    typedef utility::string_t nc_uuid;

    // NcRegex
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncregex
    typedef utility::string_t nc_regex;

    // NcOrganizationId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncorganizationid
    typedef int32_t nc_organization_id;

    // NcClassId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassid
    typedef std::vector<int32_t> nc_class_id;
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncobject
    const nc_class_id nc_object_class_id({ 1 });
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncblock
    const nc_class_id nc_block_class_id({ 1, 1 });
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncworker
    const nc_class_id nc_worker_class_id({ 1, 2 });
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncmanager
    const nc_class_id nc_manager_class_id({ 1, 3 });
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
    const nc_class_id nc_device_manager_class_id({ 1, 3, 1 });
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
    const nc_class_id nc_class_manager_class_id({ 1, 3, 2 });
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    const nc_class_id nc_ident_beacon_class_id({ 1, 2, 2 });
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    const nc_class_id nc_receiver_monitor_class_id({ 1, 2, 3 });
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    const nc_class_id nc_receiver_monitor_protected_class_id({ 1, 2, 3, 1 });
    // TODO: add link
    const nc_class_id nc_bulk_properties_manager_class_id({ 1, 3, 3 });

    // NcTouchpoint
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpoint
    typedef utility::string_t nc_touch_point;

    // NcPropertyChangeType
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertychangetype
    namespace nc_property_change_type
    {
        enum type
        {
            value_changed = 0,          // Current value changed
            sequence_item_added = 1,    // Sequence item added
            sequence_item_changed = 2,  // Sequence item changed
            sequence_item_removed = 3   // Sequence item removed
        };
    }

    // NcPropertyChangedEventData
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncpropertychangedeventdata
    struct nc_property_changed_event_data
    {
        nc_property_id property_id;
        nc_property_change_type::type change_type;
        web::json::value value;
        web::json::value sequence_item_index; // nc_id, can be null

        nc_property_changed_event_data(nc_property_id property_id, nc_property_change_type::type change_type, web::json::value value, nc_id sequence_item_index)
            : property_id(std::move(property_id))
            , change_type(change_type)
            , value(std::move(value))
            , sequence_item_index(sequence_item_index)
        {}

        nc_property_changed_event_data(nc_property_id property_id, nc_property_change_type::type change_type, web::json::value value)
            : property_id(std::move(property_id))
            , change_type(change_type)
            , value(std::move(value))
            , sequence_item_index(web::json::value::null())
        {}

        nc_property_changed_event_data(nc_property_id property_id, nc_property_change_type::type change_type, nc_id sequence_item_index)
            : property_id(std::move(property_id))
            , change_type(change_type)
            , value(web::json::value::null())
            , sequence_item_index(sequence_item_index)
        {}

        auto tied() const -> decltype(std::tie(property_id, change_type, value, sequence_item_index)) { return std::tie(property_id, change_type, value, sequence_item_index); }
        friend bool operator==(const nc_property_changed_event_data& lhs, const nc_property_changed_event_data& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const nc_property_changed_event_data& lhs, const nc_property_changed_event_data& rhs) { return !(lhs == rhs); }
        friend bool operator<(const nc_property_changed_event_data& lhs, const nc_property_changed_event_data& rhs) { return lhs.tied() < rhs.tied(); }
    };

    // NcTouchpointResource
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointresource
    struct nc_touchpoint_resource
    {
        utility::string_t resource_type;

        nc_touchpoint_resource(const utility::string_t& resource_type)
            : resource_type(resource_type)
        {}

        auto tied() const -> decltype(std::tie(resource_type)) { return std::tie(resource_type); }
        friend bool operator==(const nc_touchpoint_resource& lhs, const nc_touchpoint_resource& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const nc_touchpoint_resource& lhs, const nc_touchpoint_resource& rhs) { return !(lhs == rhs); }
        friend bool operator<(const nc_touchpoint_resource& lhs, const nc_touchpoint_resource& rhs) { return lhs.tied() < rhs.tied(); }
    };

    // NcTouchpointResourceNmos
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointresourcenmos
    struct nc_touchpoint_resource_nmos : nc_touchpoint_resource
    {
        nc_uuid id;

        nc_touchpoint_resource_nmos(const utility::string_t& resource_type, nc_uuid id)
            : nc_touchpoint_resource(resource_type)
            , id(id)
        {}

        nc_touchpoint_resource_nmos(const ncp_nmos_resource_type& resource_type, nc_uuid id)
            : nc_touchpoint_resource(resource_type.name)
            , id(id)
        {}

        auto tied() const -> decltype(std::tie(resource_type, id)) { return std::tie(resource_type, id); }
        friend bool operator==(const nc_touchpoint_resource_nmos& lhs, const nc_touchpoint_resource_nmos& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const nc_touchpoint_resource_nmos& lhs, const nc_touchpoint_resource_nmos& rhs) { return !(lhs == rhs); }
        friend bool operator<(const nc_touchpoint_resource_nmos& lhs, const nc_touchpoint_resource_nmos& rhs) { return lhs.tied() < rhs.tied(); }
    };

    // NcTouchpointResourceNmosChannelMapping
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#nctouchpointresourcenmoschannelmapping
    struct nc_touchpoint_resource_nmos_channel_mapping : nc_touchpoint_resource_nmos
    {
        //ncp_nmos_channel_mapping_resource_type resource_type;
        nc_uuid io_id;

        nc_touchpoint_resource_nmos_channel_mapping(const ncp_nmos_channel_mapping_resource_type& resource_type, nc_uuid id, const utility::string_t& io_id)
            : nc_touchpoint_resource_nmos(resource_type.name, id)
            , io_id(io_id)
        {}

        auto tied() const -> decltype(std::tie(resource_type, id, io_id)) { return std::tie(resource_type, id, io_id); }
        friend bool operator==(const nc_touchpoint_resource_nmos_channel_mapping& lhs, const nc_touchpoint_resource_nmos_channel_mapping& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const nc_touchpoint_resource_nmos_channel_mapping& lhs, const nc_touchpoint_resource_nmos_channel_mapping& rhs) { return !(lhs == rhs); }
        friend bool operator<(const nc_touchpoint_resource_nmos_channel_mapping& lhs, const nc_touchpoint_resource_nmos_channel_mapping& rhs) { return lhs.tied() < rhs.tied(); }
    };

    // Root block specification
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Blocks.html
    const nc_oid root_block_oid{ 1 };
    const utility::string_t root_block_role{ U("root") };

    // Device Configuration
    const utility::string_t bulk_properties_manager_role{ root_block_role + U(".BulkPropertiesManager") };
}

#endif

#ifndef NMOS_CONTROL_PROTOCOL_TYPEDEFS_H
#define NMOS_CONTROL_PROTOCOL_TYPEDEFS_H

#include "cpprest/basic_utils.h"

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace nmos
{
    namespace nc_message_type
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
    namespace nc_method_status
    {
        enum status
        {
            ok = 200,                       // Method call was successful
            property_deprecated = 298,      // Method call was successful but targeted property is deprecated
            method_deprecated = 299,        // Method call was successful but method is deprecated
            bad_command_format = 400,       // Badly-formed command
            unathorized = 401,              // Client is not authorized
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
            property_version_error = 505    // Incompatible protocol version
        };
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmethodresult
    struct nc_method_result
    {
        nc_method_status::status status;
    };

    // Datatype type
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

    // NcElementId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncelementid
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
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#nceventid
    typedef nc_element_id nc_event_id;

    // NcMethodId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmethodid
    typedef nc_element_id nc_method_id;

    // NcPropertyId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncpropertyid
    typedef nc_element_id nc_property_id;

    // NcId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncid
    typedef uint32_t nc_id;

    // NcName
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncname
    typedef utility::string_t nc_name;

    // NcOid
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncoid
    typedef uint32_t nc_oid;
    const nc_oid root_block_oid{ 1 };

    // NcUri
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncuri
    typedef utility::string_t nc_uri;

    // NcUuid
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncuuid
    typedef utility::string_t nc_uuid;

    // NcClassId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassid
    typedef std::vector<int32_t> nc_class_id;
    const nc_class_id nc_object_class_id({ 1 });
    const nc_class_id nc_block_class_id({ 1, 1 });
    const nc_class_id nc_worker_class_id({ 1, 2 });
    const nc_class_id nc_manager_class_id({ 1, 3 });
    const nc_class_id nc_device_manager_class_id({ 1, 3, 1 });
    const nc_class_id nc_class_manager_class_id({ 1, 3, 2 });
    const nc_class_id nc_ident_beacon_class_id({ 1, 2, 2 });
    const nc_class_id nc_receiver_monitor_class_id({ 1, 2, 3 });
    const nc_class_id nc_receiver_monitor_protected_class_id({ 1, 2, 3, 1 });

    // NcTouchpoint
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#nctouchpoint
    typedef utility::string_t nc_touch_point;
}

#endif

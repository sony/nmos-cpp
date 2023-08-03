#ifndef NMOS_CONTROL_PROTOCOL_RESOURCES_H
#define NMOS_CONTROL_PROTOCOL_RESOURCES_H

#include <map>
#include "nmos/settings.h"

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace nmos
{
    struct resource;

    namespace details
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

        // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#command-response-message-type
        web::json::value make_control_protocol_error_response(int32_t handle, const nc_method_result& method_result, const utility::string_t& error_message);
        web::json::value make_control_protocol_response(int32_t handle, const nc_method_result& method_result);
        web::json::value make_control_protocol_response(int32_t handle, const nc_method_result& method_result, const web::json::value& value);

        // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#command-message-type
        web::json::value make_control_protocol_message_response(nc_message_type::type type, const web::json::value& responses);
        // See https://specs.amwa.tv/is-12/branches/v1.0-dev/docs/Protocol_messaging.html#error-messages
        web::json::value make_control_protocol_error_message(const nc_method_result& method_result, const utility::string_t& error_message);

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
                Unknown = 0,            // Unknown
                NormalOperation = 1,    // Normal operation
                Initializing = 2,       // Device is initializing
                Updating = 3,           // Device is performing a software or firmware update
                LicensingError = 4,     // Device is experiencing a licensing error
                InternalError = 5       // Device is experiencing an internal error
            };
        }

        // Reset cause enum
        namespace nc_reset_cause
        {
            enum cause
            {
                Unknown = 0,            // 0 Unknown
                Power_on = 1,           // 1 Power on
                InternalError = 2,      // 2 Internal error
                Upgrade = 3,            // 3 Upgrade
                Controller_request = 4, // 4 Controller request
                ManualReset = 5         // 5 Manual request from the front panel
            };
        }

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncid
        typedef uint32_t nc_id;

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncoid
        typedef uint32_t nc_oid;

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncuri
        typedef utility::string_t nc_uri;

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncuuid
        typedef utility::string_t nc_uuid;

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassid
        typedef std::vector<int32_t> nc_class_id;

        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#nctouchpoint
        typedef utility::string_t nc_touch_point;

        typedef std::map<web::json::value, utility::string_t> properties;

        typedef std::function<web::json::value(const properties& properties, int32_t handle, int32_t oid, const web::json::value& arguments)> method;
        typedef std::map<web::json::value, method> methods; // method_id vs method handler
    }

    nmos::resource make_device_manager(details::nc_oid oid, nmos::resource& root_block, const nmos::settings& settings);

    nmos::resource make_class_manager(details::nc_oid oid, nmos::resource& root_block);

    nmos::resource make_root_block();
}

#endif

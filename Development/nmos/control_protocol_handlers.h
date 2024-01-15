#ifndef NMOS_CONTROL_PROTOCOL_HANDLERS_H
#define NMOS_CONTROL_PROTOCOL_HANDLERS_H

#include <functional>
#include "nmos/control_protocol_typedefs.h"
#include "nmos/resources.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    namespace experimental
    {
        struct control_protocol_state;
        struct control_class;
        struct datatype;
    }

    // callback to retrieve a specific control protocol classe
    // this callback should not throw exceptions
    typedef std::function<experimental::control_class(const nc_class_id& class_id)> get_control_protocol_class_handler;

    // callback to add user control protocol class
    // this callback should not throw exceptions
    typedef std::function<bool(const nc_class_id& class_id, const experimental::control_class& control_class)> add_control_protocol_class_handler;

    // callback to retrieve a control protocol datatype
    // this callback should not throw exceptions
    typedef std::function<experimental::datatype(const nc_name& name)> get_control_protocol_datatype_handler;

    // a control_protocol_property_changed_handler is a notification that the specified (IS-12) property has changed
    // index is set to -1 for non-sequence property
    // this callback should not throw exceptions, as the relevant property will already has been changed and those changes will not be rolled back
    typedef std::function<void(const nmos::resource& resource, const utility::string_t& property_name, int index)> control_protocol_property_changed_handler;

    namespace experimental
    {
        // standard method handler definition
        typedef std::function<web::json::value(nmos::resources& resources, const nmos::resource& resource, int32_t handle, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler get_control_protocol_datatype, control_protocol_property_changed_handler property_changed, slog::base_gate& gate)> standard_method_handler;

        // non-standard method handler definition
        typedef std::function<web::json::value(nmos::resources& resources, const nmos::resource& resource, int32_t handle, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)> non_standard_method_handler;

        // method definition (NcMethodDescriptor vs method handler)
        typedef std::tuple<web::json::value, standard_method_handler, non_standard_method_handler> method;

        inline method make_control_class_standard_method(const web::json::value& nc_method_descriptor, standard_method_handler method_handler)
        {
            return std::make_tuple(nc_method_descriptor, method_handler, nullptr);
        }

        inline method make_control_class_non_standard_method(const web::json::value& nc_method_descriptor, non_standard_method_handler method_handler)
        {
            return std::make_tuple(nc_method_descriptor, nullptr, method_handler);
        }
    }

    // callback to retrieve a specific method
    // this callback should not throw exceptions
    typedef std::function<experimental::method(const nc_class_id& class_id, const nc_method_id& method_id)> get_control_protocol_method_handler;

    // construct callback to retrieve a specific control protocol class
    get_control_protocol_class_handler make_get_control_protocol_class_handler(experimental::control_protocol_state& control_protocol_state);

    // construct callback to retrieve a specific datatype
    get_control_protocol_datatype_handler make_get_control_protocol_datatype_handler(experimental::control_protocol_state& control_protocol_state);

    // construct callback to retrieve a specific method
    get_control_protocol_method_handler make_get_control_protocol_method_handler(experimental::control_protocol_state& control_protocol_state);

    // a control_protocol_connection_activation_handler is a notification that the active parameters for the specified (IS-05) sender/connection_sender or receiver/connection_receiver have changed
    // this callback should not throw exceptions
    typedef std::function<void(const nmos::resource& connection_resource)> control_protocol_connection_activation_handler;

    // construct callback for receiver monitor to process connection (de)activation
    control_protocol_connection_activation_handler make_receiver_monitor_connection_activation_handler(nmos::resources& resources);
}

#endif

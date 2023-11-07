#ifndef NMOS_CONTROL_PROTOCOL_HANDLERS_H
#define NMOS_CONTROL_PROTOCOL_HANDLERS_H

#include <functional>
#include <map>
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

    namespace experimental
    {
        // method handler defnition
        typedef std::function<web::json::value(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler get_control_protocol_datatype, slog::base_gate& gate)> method_handler;
        // methods defnition
        typedef std::map<nmos::nc_method_id, method_handler> methods; // method_id vs method handler
    }

    // callback to retrieve all the method handlers
    // this callback should not throw exceptions
    typedef std::function<std::map<nmos::nc_class_id, experimental::methods>()> get_control_protocol_methods_handler;

    // callback to retrieve a specific method handler
    // this callback should not throw exceptions
    typedef std::function<experimental::method_handler(const nc_class_id& class_id, const nc_method_id& method_id)> get_control_protocol_method_handler;

    // construct callback to retrieve a specific control protocol class
    get_control_protocol_class_handler make_get_control_protocol_class_handler(experimental::control_protocol_state& control_protocol_state);

    // construct callback to retrieve a specific datatype
    get_control_protocol_datatype_handler make_get_control_protocol_datatype_handler(experimental::control_protocol_state& control_protocol_state);

    // construct callback to retrieve a specific method handler
    get_control_protocol_method_handler make_get_control_protocol_method_handler(experimental::control_protocol_state& control_protocol_state);

    // a control_protocol_connection_activation_handler is a notification that the active parameters for the specified (IS-05) sender/connection_sender or receiver/connection_receiver have changed
    // this callback should not throw exceptions
    typedef std::function<void(const nmos::resource& connection_resource)> control_protocol_connection_activation_handler;

    // construct callback for receiver monitor to process connection (de)activation
    control_protocol_connection_activation_handler make_receiver_monitor_connection_activation_handler(nmos::resources& resources);
}

#endif

#ifndef NMOS_CONTROL_PROTOCOL_HANDLERS_H
#define NMOS_CONTROL_PROTOCOL_HANDLERS_H

#include <cstdint>
#include <functional>
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    namespace experimental
    {
        struct control_class;
        struct control_protocol_state;
    }

    // callback to retrieve all control protocol classes
    // this callback should not throw exceptions
    typedef std::function<experimental::control_classes()> get_control_protocol_classes_handler;

    // callback to retrieve a specific control protocol class
    // this callback should not throw exceptions
    typedef std::function<experimental::control_class(const details::nc_class_id& class_id)> get_control_protocol_class_handler;

    // callback to add user control protocol class
    // this callback should not throw exceptions
    typedef std::function<bool(const details::nc_class_id& class_id, const experimental::control_class& control_class)> add_control_protocol_class_handler;

    // construct callback to retrieve all control protocol classes
    get_control_protocol_classes_handler make_get_control_protocol_classes_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // construct callback to retrieve control protocol class
    get_control_protocol_class_handler make_get_control_protocol_class_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);

    // construct callback to add control protocol class
    add_control_protocol_class_handler make_add_control_protocol_class_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate);
}

#endif

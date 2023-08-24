#include "nmos/control_protocol_handlers.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/slog.h"

namespace nmos
{
    get_control_protocol_classes_handler make_get_control_protocol_classes_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&]()
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Retrieve all control classes from cache";

            auto lock = control_protocol_state.read_lock();

            return control_protocol_state.control_classes;
        };
    }

    add_control_protocol_class_handler make_add_control_protocol_class_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&](const nc_class_id& class_id, const experimental::control_class& control_class)
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Add control class to cache";

            auto lock = control_protocol_state.write_lock();

            auto class_id_data = details::make_nc_class_id(class_id);

            auto& control_classes = control_protocol_state.control_classes;
            if (control_classes.end() == control_classes.find(class_id_data))
            {
                return false;
            }

            control_classes[class_id_data] = control_class;
            return true;
        };
    }

    get_control_protocol_datatypes_handler make_get_control_protocol_datatypes_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&]()
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Retrieve all datatypes from cache";

            auto lock = control_protocol_state.read_lock();

            return control_protocol_state.datatypes;
        };
    }
}

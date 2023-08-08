#include "nmos/control_protocol_handlers.h"

#include "cpprest/basic_utils.h"
#include "nmos/control_protocol_state.h"
#include "nmos/slog.h"

namespace nmos
{
    get_control_protocol_classes_handler make_get_control_protocol_classes_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&]()
        {
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Retrieve all control classes from cache";

            auto lock = control_protocol_state.read_lock();

            return control_protocol_state.control_classes;
        };
    }

    get_control_protocol_class_handler make_get_control_protocol_class_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&](const details::nc_class_id& class_id)
        {
            using web::json::value;

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Retrieve control class from cache";

            auto lock = control_protocol_state.read_lock();

            auto class_id_data = details::make_nc_class_id(class_id);

            auto& control_classes = control_protocol_state.control_classes;
            auto found = control_classes.find(class_id_data);
            if (control_classes.end() != found)
            {
                return found->second;
            }

            return experimental::control_class{ value::array(), value::array(), value::array() };
        };
    }

    add_control_protocol_class_handler make_add_control_protocol_class_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&](const details::nc_class_id& class_id, const experimental::control_class& control_class)
        {
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Add control class to cache";

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
}

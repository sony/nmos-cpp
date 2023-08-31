#include "nmos/control_protocol_handlers.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/slog.h"

namespace nmos
{
    get_control_protocol_class_handler make_get_control_protocol_class_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&](const nc_class_id& class_id)
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Retrieve control class of class_id: " << nmos::details::make_nc_class_id(class_id).serialize() << " from cache";

            auto lock = control_protocol_state.read_lock();

            auto& control_classes = control_protocol_state.control_classes;
            auto found = control_classes.find(class_id);
            if (control_classes.end() != found)
            {
                return found->second;
            }
            return nmos::experimental::control_class{};
        };
    }

    add_control_protocol_class_handler make_add_control_protocol_class_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&](const nc_class_id& class_id, const experimental::control_class& control_class)
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Add control class to cache";

            auto lock = control_protocol_state.write_lock();

            auto& control_classes = control_protocol_state.control_classes;
            if (control_classes.end() == control_classes.find(class_id))
            {
                return false;
            }

            control_classes[class_id] = control_class;
            return true;
        };
    }

    get_control_protocol_datatype_handler make_get_control_protocol_datatype_handler(nmos::experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&](const nmos::nc_name& name)
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Retrieve datatype of name: " << name << " from cache";

            auto lock = control_protocol_state.read_lock();

            auto found = control_protocol_state.datatypes.find(name);
            if (control_protocol_state.datatypes.end() != found)
            {
                return found->second;
            }
            return nmos::experimental::datatype{};
        };
    }

    get_control_protocol_methods_handler make_get_control_protocol_methods_handler(experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        return [&]()
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Retrieve all method handlers from cache";

            std::map<nmos::nc_class_id, experimental::methods> methods;

            auto lock = control_protocol_state.read_lock();

            auto& control_classes = control_protocol_state.control_classes;

            for (const auto& control_class : control_classes)
            {
                methods[control_class.first] = control_class.second.method_handlers;
            }
            return methods;
        };
    }
}

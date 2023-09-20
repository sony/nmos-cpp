#include "nmos/control_protocol_handlers.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/slog.h"

namespace nmos
{
    get_control_protocol_class_handler make_get_control_protocol_class_handler(nmos::experimental::control_protocol_state& control_protocol_state)
    {
        return [&](const nc_class_id& class_id)
        {
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

    get_control_protocol_datatype_handler make_get_control_protocol_datatype_handler(nmos::experimental::control_protocol_state& control_protocol_state)
    {
        return [&](const nmos::nc_name& name)
        {
            auto lock = control_protocol_state.read_lock();

            auto found = control_protocol_state.datatypes.find(name);
            if (control_protocol_state.datatypes.end() != found)
            {
                return found->second;
            }
            return nmos::experimental::datatype{};
        };
    }

    get_control_protocol_methods_handler make_get_control_protocol_methods_handler(experimental::control_protocol_state& control_protocol_state)
    {
        return [&]()
        {
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

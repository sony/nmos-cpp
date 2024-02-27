#include "nmos/control_protocol_handlers.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
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

    get_control_protocol_method_handler make_get_control_protocol_method_handler(experimental::control_protocol_state& control_protocol_state)
    {
        return [&](const nc_class_id& class_id_, const nc_method_id& method_id)
        {
            auto class_id = class_id_;

            auto get_control_protocol_class = make_get_control_protocol_class_handler(control_protocol_state);

            auto lock = control_protocol_state.read_lock();

            while (!class_id.empty())
            {
                const auto& control_class = get_control_protocol_class(class_id);

                auto& methods = control_class.methods;
                auto method_found = std::find_if(methods.begin(), methods.end(), [&method_id](const experimental::method& method)
                {
                    return method_id == details::parse_nc_method_id(nmos::fields::nc::id(std::get<0>(method)));
                });
                if (methods.end() != method_found)
                {
                    return *method_found;
                }

                class_id.pop_back();
            }

            return experimental::method();
        };
    }

    // Example Receiver-Monitor Connection activation callback to perform application-specific operations to complete activation
    control_protocol_connection_activation_handler make_receiver_monitor_connection_activation_handler(resources& resources)
    {
        return [&resources](const resource& connection_resource)
        {
            auto found = find_control_protocol_resource(resources, nmos::types::nc_receiver_monitor, connection_resource.id);
            if (resources.end() != found && nc_receiver_monitor_class_id == details::parse_nc_class_id(nmos::fields::nc::class_id(found->data)))
            {
                // update receiver-monitor's connectionStatus propertry

                const auto active = nmos::fields::master_enable(nmos::fields::endpoint_active(connection_resource.data));
                const web::json::value value = active ? nc_connection_status::connected : nc_connection_status::disconnected;

                // hmm, maybe updating connectionStatusMessage, payloadStatus, and payloadStatusMessage too

                const auto property_changed_event = make_property_changed_event(nmos::fields::nc::oid(found->data),
                {
                    { nc_receiver_monitor_connection_status_property_id, nc_property_change_type::type::value_changed, value }
                });

                modify_control_protocol_resource(resources, found->id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::nc::connection_status] = value;

                }, property_changed_event);
            }
        };
    }
}

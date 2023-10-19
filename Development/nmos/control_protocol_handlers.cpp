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

    control_protocol_connection_activation_handler make_receiver_monitor_connection_activation_handler(resources& resources)
    {
        return [&resources](const resource& connection_resource)
        {
            auto found = find_control_protocol_resource(resources, nmos::types::nc_receiver_monitor, connection_resource.id);
            if (resources.end() != found && nc_receiver_monitor_class_id == details::parse_nc_class_id(nmos::fields::nc::class_id(found->data)))
            {
                // update receiver-monitor's connectionStatus propertry

                auto active = nmos::fields::master_enable(nmos::fields::endpoint_active(connection_resource.data));

                nc_property_id property_id = nc_receiver_monitor_connection_status_property_id;
                web::json::value val = active ? nc_connection_status::connected : nc_connection_status::disconnected;
                const nc_property_changed_event_data property_changed_event_data{ property_id, nc_property_change_type::type::value_changed, val };
                const auto notification = make_control_protocol_notification(nmos::fields::nc::oid(found->data), nc_object_property_changed_event_id, property_changed_event_data);
                const auto notification_event = make_control_protocol_notification(web::json::value_of({ notification }));

                modify_control_protocol_resource(resources, found->id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::nc::connection_status] = property_changed_event_data.value;
                    // hmm, maybe updating connectionStatusMessage, payloadStatus, and payloadStatusMessage too

                }, notification_event);
            }
        };
    }
}

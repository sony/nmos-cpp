#include "nmos/control_protocol_handlers.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/slog.h"

namespace nmos
{
    get_control_protocol_class_descriptor_handler make_get_control_protocol_class_descriptor_handler(nmos::experimental::control_protocol_state& control_protocol_state)
    {
        return [&](const nc_class_id& class_id)
        {
            auto lock = control_protocol_state.read_lock();

            auto& control_class_descriptors = control_protocol_state.control_class_descriptors;
            auto found = control_class_descriptors.find(class_id);
            if (control_class_descriptors.end() != found)
            {
                return found->second;
            }
            return nmos::experimental::control_class_descriptor{};
        };
    }

    get_control_protocol_datatype_descriptor_handler make_get_control_protocol_datatype_descriptor_handler(nmos::experimental::control_protocol_state& control_protocol_state)
    {
        return [&](const nmos::nc_name& name)
        {
            auto lock = control_protocol_state.read_lock();

            auto found = control_protocol_state.datatype_descriptors.find(name);
            if (control_protocol_state.datatype_descriptors.end() != found)
            {
                return found->second;
            }
            return nmos::experimental::datatype_descriptor{};
        };
    }

    get_control_protocol_method_descriptor_handler make_get_control_protocol_method_descriptor_handler(experimental::control_protocol_state& control_protocol_state)
    {
        return [&](const nc_class_id& class_id_, const nc_method_id& method_id)
        {
            auto class_id = class_id_;

            auto get_control_protocol_class_descriptor = make_get_control_protocol_class_descriptor_handler(control_protocol_state);

            auto lock = control_protocol_state.read_lock();

            while (!class_id.empty())
            {
                const auto& control_class_descriptor = get_control_protocol_class_descriptor(class_id);

                auto& method_descriptors = control_class_descriptor.method_descriptors;
                auto found = std::find_if(method_descriptors.begin(), method_descriptors.end(), [&method_id](const experimental::method& method)
                {
                    return method_id == details::parse_nc_method_id(nmos::fields::nc::id(std::get<0>(method)));
                });
                if (method_descriptors.end() != found)
                {
                    return *found;
                }

                class_id.pop_back();
            }

            return experimental::method();
        };
    }

    receiver_monitor_status_pending_handler make_receiver_monitor_status_pending_handler(experimental::control_protocol_state& control_protocol_state)
    {
        return [&control_protocol_state]()
        {
            auto lock = control_protocol_state.write_lock();
            control_protocol_state.receiver_monitor_status_pending = true;
        };
    }

    control_protocol_connection_activation_handler make_receiver_monitor_connection_activation_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
        auto get_control_protocol_method_descriptor = nmos::make_get_control_protocol_method_descriptor_handler(control_protocol_state);
        auto receiver_monitor_status_pending = nmos::make_receiver_monitor_status_pending_handler(control_protocol_state);

        return [&resources, receiver_monitor_status_pending, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, &gate](const nmos::resource& resource, const nmos::resource& connection_resource)
        {
            if (nmos::types::receiver != resource.type) return;

            const auto& endpoint_active = nmos::fields::endpoint_active(connection_resource.data);
            const bool active = nmos::fields::master_enable(endpoint_active);

            auto found = find_control_protocol_resource(resources, nmos::types::nc_receiver_monitor, connection_resource.id);
            if (resources.end() != found && nc_receiver_monitor_class_id == details::parse_nc_class_id(nmos::fields::nc::class_id(found->data)))
            {
                const auto& oid = nmos::fields::nc::oid(found->data);

                if (active)
                {
                    // Activate receiver monitor
                    activate_receiver_monitor(resources, oid, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);
                }
                else
                {
                    // deactivate receiver monitor
                    deactivate_receiver_monitor(resources, oid, get_control_protocol_class_descriptor, gate);
                }
            }
        };
    }

    get_control_protocol_property_handler make_get_control_protocol_property_handler(const resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);

        return [&resources, get_control_protocol_class_descriptor, &gate](nc_oid oid, const nc_property_id& property_id)
        {
            return get_control_protocol_property(resources, oid, property_id, get_control_protocol_class_descriptor, gate);
        };
    }

    set_control_protocol_property_handler make_set_control_protocol_property_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);

        return [&resources, get_control_protocol_class_descriptor, &gate](nc_oid oid, const nc_property_id& property_id, const web::json::value& value)
        {
            return set_control_protocol_property(resources, oid, property_id, value, get_control_protocol_class_descriptor, gate);
        };
    }

    set_receiver_monitor_link_status_handler make_set_receiver_monitor_link_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
        auto receiver_monitor_status_pending = nmos::make_receiver_monitor_status_pending_handler(control_protocol_state);

        return [&resources, receiver_monitor_status_pending, get_control_protocol_class_descriptor, &gate](nc_oid oid, nmos::nc_link_status::status link_status, const utility::string_t& link_status_message)
        {
            return set_receiver_monitor_link_status_with_delay(resources, oid, link_status, link_status_message, receiver_monitor_status_pending, get_control_protocol_class_descriptor, gate);
        };
    }

    // Set connection status and connection status message
    set_receiver_monitor_connection_status_handler make_set_receiver_monitor_connection_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
        auto receiver_monitor_status_pending = nmos::make_receiver_monitor_status_pending_handler(control_protocol_state);

        return [&resources, receiver_monitor_status_pending, get_control_protocol_class_descriptor, &gate](nc_oid oid, nmos::nc_connection_status::status connection_status, const utility::string_t& connection_status_message)
        {
            return set_receiver_monitor_connection_status_with_delay(resources, oid, connection_status, connection_status_message, receiver_monitor_status_pending, get_control_protocol_class_descriptor, gate);
        };
    }

    // Set external synchronization status and external synchronization status message
    set_receiver_monitor_external_synchronization_status_handler make_set_receiver_monitor_external_synchronization_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
        auto receiver_monitor_status_pending = nmos::make_receiver_monitor_status_pending_handler(control_protocol_state);

        return [&resources, receiver_monitor_status_pending, get_control_protocol_class_descriptor, &gate](nc_oid oid, nmos::nc_synchronization_status::status external_synchronization_status, const utility::string_t& external_synchronization_status_message)
        {
            return set_receiver_monitor_external_synchronization_status_with_delay(resources, oid, external_synchronization_status, external_synchronization_status_message, receiver_monitor_status_pending, get_control_protocol_class_descriptor, gate);
        };
    }

    // Set stream status and stream status message
    set_receiver_monitor_stream_status_handler make_set_receiver_monitor_stream_status_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
        auto receiver_monitor_status_pending = nmos::make_receiver_monitor_status_pending_handler(control_protocol_state);

        return [&resources, receiver_monitor_status_pending, get_control_protocol_class_descriptor, &gate](nc_oid oid, nmos::nc_stream_status::status stream_status, const utility::string_t& stream_status_message)
        {
            return set_receiver_monitor_stream_status_with_delay(resources, oid, stream_status, stream_status_message, receiver_monitor_status_pending, get_control_protocol_class_descriptor, gate);
        };
    }

    // Set synchronization source id
    set_receiver_monitor_synchronization_source_id_handler make_set_receiver_monitor_synchronization_source_id_handler(resources& resources, experimental::control_protocol_state& control_protocol_state, slog::base_gate& gate)
    {
        auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);

        return [&resources, get_control_protocol_class_descriptor, &gate](nc_oid oid, const bst::optional<utility::string_t>& source_id)
        {
            return set_receiver_monitor_synchronization_source_id(resources, oid, source_id, get_control_protocol_class_descriptor, gate);
        };
    }
}

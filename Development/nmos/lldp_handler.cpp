#include "nmos/lldp_handler.h"

#include "cpprest/json.h"
#include "nmos/json_fields.h"
#include "nmos/model.h"
#include "nmos/node_interfaces.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        // make an LLDP handler to update the node interfaces JSON data with attached_network_device details
        lldp::lldp_handler make_lldp_handler(nmos::model& model, const std::map<utility::string_t, node_interface>& interfaces, slog::base_gate& gate)
        {
            return [&model, interfaces, &gate](const std::string& interface_id, const lldp::lldp_data_unit& lldpdu)
            {
                auto interface = interfaces.find(utility::s2us(interface_id));
                if (interfaces.end() == interface)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "LLDP received - no matching network interface " << interface_id << " to update";
                    return;
                }

                const auto& interface_name = interface->second.name;

                using web::json::value_of;

                auto lock = model.write_lock();
                auto& resources = model.node_resources;

                auto resource = nmos::find_self_resource(resources);
                if (resources.end() != resource)
                {
                    auto& json_interfaces = nmos::fields::interfaces(resource->data);

                    const auto json_interface = std::find_if(json_interfaces.begin(), json_interfaces.end(), [&](const web::json::value& nv)
                    {
                        return nmos::fields::name(nv) == interface_name;
                    });

                    if (json_interfaces.end() == json_interface)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "LLDP received - no matching network interface " << interface_id << " to update";
                        return;
                    }

                    const auto attached_network_device = value_of({
                        { nmos::fields::chassis_id, utility::s2us(lldp::parse_chassis_id(lldpdu.chassis_id)) },
                        { nmos::fields::port_id, utility::s2us(lldp::parse_port_id(lldpdu.port_id)) }
                    });

                    // has the attached_network_device info changed?
                    if (nmos::fields::attached_network_device(*json_interface) != attached_network_device)
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "LLDP received - updating attached network device info for network interface " << interface->second.name;

                        nmos::modify_resource(resources, resource->id, [&interface_name, &attached_network_device](nmos::resource& resource)
                        {
                            resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());

                            auto& json_interfaces = nmos::fields::interfaces(resource.data);

                            const auto json_interface = std::find_if(json_interfaces.begin(), json_interfaces.end(), [&](const web::json::value& nv)
                            {
                                return nmos::fields::name(nv) == interface_name;
                            });

                            if (json_interfaces.end() != json_interface)
                            {
                                (*json_interface)[nmos::fields::attached_network_device] = attached_network_device;
                            }
                        });

                        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying attached network device info updated";
                        model.notify();
                    }
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Self resource not found!";
                }
            };
        }
    }
}

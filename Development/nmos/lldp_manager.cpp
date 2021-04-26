#ifdef HAVE_LLDP
#include "nmos/lldp_manager.h"

#include "lldp/lldp_manager.h"
#include "cpprest/json.h"
#include "nmos/json_fields.h"
#include "nmos/lldp_handler.h"
#include "nmos/model.h"
#include "nmos/node_interfaces.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        // make an LLDP manager configured to receive LLDP frames on the specified interfaces
        // and update the node interfaces JSON data with attached_network_device details
        // and optionally, transmit LLDP frames for these interfaces
        lldp::lldp_manager make_lldp_manager(nmos::model& model, const std::map<utility::string_t, node_interface>& interfaces, bool transmit, slog::base_gate& gate)
        {
            // use default configuration for now
            lldp::lldp_manager manager(gate);

            const auto status = transmit ? lldp::management_status::transmit_receive : lldp::management_status::receive;

            manager.set_handler(make_lldp_handler(model, interfaces, gate));

            for (const auto& interface : interfaces)
            {
                // data with default TTL
                const auto data = lldp::normal_data_unit(
                    lldp::make_chassis_id(utility::us2s(interface.second.chassis_id)),
                    lldp::make_port_id(utility::us2s(interface.second.port_id)),
                    { utility::us2s(interface.second.name) },
                    utility::us2s(nmos::get_host_name(model.settings))
                );

                auto configure = manager.configure_interface(utility::us2s(interface.first), status, data);
                // current configure_interface implementation is synchronous, but just to make clear...
                // for now, wait for it to complete
                configure.wait();
            }

            return manager;
        }
    }
}
#endif

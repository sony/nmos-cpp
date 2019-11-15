#include "nmos/node_interfaces.h"

#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/host_utils.h"
#include "nmos/json_fields.h"

namespace nmos
{
    // make node interfaces JSON data, for the specified map from local interface_id
    // no attached_network_device details are included
    web::json::value make_node_interfaces(const std::map<utility::string_t, node_interface>& interfaces)
    {
        using web::json::value_from_elements;
        using web::json::value_of;

        return value_from_elements(interfaces | boost::adaptors::transformed([](const std::map<utility::string_t, node_interface>::value_type& interface)
        {
            return value_of({
                { nmos::fields::chassis_id, interface.second.chassis_id },
                { nmos::fields::port_id, interface.second.port_id },
                { nmos::fields::name, interface.second.name }
            });
        }));
    }

    namespace experimental
    {
        // make a map from local interface_id to the (recommended) node interface details
        // cf. web::hosts::experimental::host_interfaces()
        std::map<utility::string_t, node_interface> node_interfaces()
        {
            const auto host_interfaces = web::hosts::experimental::host_interfaces();
            return boost::copy_range<std::map<utility::string_t, node_interface>>(host_interfaces | boost::adaptors::transformed([&](const web::hosts::experimental::host_interface& interface)
            {
                return std::map<utility::string_t, node_interface>::value_type{
                    interface.name,
                    { host_interfaces.front().physical_address, interface.physical_address, interface.name }
                };
            }));
        }
    }
}

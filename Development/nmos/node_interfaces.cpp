#include "nmos/node_interfaces.h"

#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/host_utils.h"
#include "nmos/json_fields.h"

namespace nmos
{
    namespace details
    {
        // Chassis ID may be a MAC address (recommended) or IPv4 or IPv6 address; empty string indicates null
        web::json::value make_node_interfaces_chassis_id(const utility::string_t& chassis_id)
        {
            using web::json::value;
            return !chassis_id.empty() ? value::string(chassis_id) : value::null();
        }

        // Port ID must be a MAC address
        web::json::value make_node_interfaces_port_id(const utility::string_t& port_id)
        {
            using web::json::value;
            // when no physical address is available, use the common null value of all zeros
            // see https://standards.ieee.org/content/dam/ieee-standards/standards/web/documents/tutorials/eui.pdf
            return value::string(!port_id.empty() ? port_id : U("00-00-00-00-00-00"));
        }
    }

    // make node interfaces JSON data, for the specified map from local interface_id
    // no attached_network_device details are included
    web::json::value make_node_interfaces(const std::map<utility::string_t, node_interface>& interfaces)
    {
        using web::json::value_from_elements;
        using web::json::value_of;

        return value_from_elements(interfaces | boost::adaptors::transformed([](const std::map<utility::string_t, node_interface>::value_type& interface)
        {
            return value_of({
                { nmos::fields::chassis_id, details::make_node_interfaces_chassis_id(interface.second.chassis_id) },
                { nmos::fields::port_id, details::make_node_interfaces_port_id(interface.second.port_id) },
                { nmos::fields::name, interface.second.name }
            });
        }));
    }

    namespace experimental
    {
        // make a map from local interface_id to the (recommended) node interface details for the specified host interfaces
        std::map<utility::string_t, node_interface> node_interfaces(const std::vector<web::hosts::experimental::host_interface>& host_interfaces)
        {
            return boost::copy_range<std::map<utility::string_t, node_interface>>(host_interfaces | boost::adaptors::transformed([&](const web::hosts::experimental::host_interface& interface)
            {
                return std::map<utility::string_t, node_interface>::value_type{
                    interface.name,
                    { host_interfaces.front().physical_address, interface.physical_address, interface.name }
                };
            }));
        }

        // make a map from local interface_id to the (recommended) node interface details
        std::map<utility::string_t, node_interface> node_interfaces()
        {
            return node_interfaces(web::hosts::experimental::host_interfaces());
        }
    }
}

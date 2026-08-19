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

        utility::string_t parse_node_interfaces_chassis_id(const web::json::value& chassis_id)
        {
            return chassis_id.is_null() ? utility::string_t{} : chassis_id.as_string();
        }

        bool is_valid_node_interfaces_port_id(const utility::string_t& port_id)
        {
            if (17 != port_id.size()) return false;

            for (size_t index = 0; index < port_id.size(); ++index)
            {
                const auto character = port_id[index];
                if (2 == index % 3)
                {
                    if (U('-') != character) return false;
                }
                else if (!((U('0') <= character && character <= U('9')) || (U('a') <= character && character <= U('f'))))
                {
                    return false;
                }
            }

            return true;
        }

        // Port ID must be a MAC address
        web::json::value make_node_interfaces_port_id(const utility::string_t& port_id)
        {
            using web::json::value;
            // IS-04 port_id requires the six-octet lowercase-hyphen form. Any other representation,
            // including uppercase or colon-separated MAC addresses, is not representable in this field
            // and uses the existing null-address fallback of all zeros
            // see https://standards.ieee.org/content/dam/ieee-standards/standards/web/documents/tutorials/eui.pdf
            return value::string(is_valid_node_interfaces_port_id(port_id) ? port_id : U("00-00-00-00-00-00"));
        }
    }

    // make node interface JSON data
    web::json::value make_node_interface(const node_interface& interface)
    {
        using web::json::value_of;

        const bool keep_order = true;

        const auto has_attached = !interface.attached_chassis_id.empty() && !interface.attached_port_id.empty();

        return value_of({
            { nmos::fields::chassis_id, details::make_node_interfaces_chassis_id(interface.chassis_id) },
            { nmos::fields::port_id, details::make_node_interfaces_port_id(interface.port_id) },
            { nmos::fields::name, interface.name },
            { has_attached ? nmos::fields::attached_network_device.key : U(""), value_of({
                { nmos::fields::chassis_id, interface.attached_chassis_id },
                { nmos::fields::port_id, interface.attached_port_id }
            }, keep_order) }
        }, keep_order);
    }

    // parse node interface JSON data
    node_interface parse_node_interface(const web::json::value& interface)
    {
        const auto& attached = nmos::fields::attached_network_device(interface);
        return {
            details::parse_node_interfaces_chassis_id(interface.at(nmos::fields::chassis_id)),
            nmos::fields::port_id(interface),
            nmos::fields::name(interface),
            attached.is_object() ? nmos::fields::chassis_id(attached) : utility::string_t{},
            attached.is_object() ? nmos::fields::port_id(attached) : utility::string_t{}
        };
    }

    // make node interfaces JSON data, for the specified map from local interface_id
    web::json::value make_node_interfaces(const std::map<utility::string_t, node_interface>& interfaces)
    {
        using web::json::value_from_elements;

        return value_from_elements(interfaces | boost::adaptors::transformed([](const std::map<utility::string_t, node_interface>::value_type& interface)
        {
            return make_node_interface(interface.second);
        }));
    }

    namespace experimental
    {
        // make a map from local interface_id to the (recommended) node interface details for the specified host interfaces
        // no attached_network_device details are included
        std::map<utility::string_t, node_interface> node_interfaces(const std::vector<web::hosts::experimental::host_interface>& host_interfaces)
        {
            return boost::copy_range<std::map<utility::string_t, node_interface>>(host_interfaces | boost::adaptors::transformed([&](const web::hosts::experimental::host_interface& interface)
            {
                return std::map<utility::string_t, node_interface>::value_type{
                    interface.name,
                    { host_interfaces.front().physical_address, interface.physical_address, interface.name, {}, {} }
                };
            }));
        }

        // make a map from local interface_id to the (recommended) node interface details
        // no attached_network_device details are included
        std::map<utility::string_t, node_interface> node_interfaces()
        {
            return node_interfaces(web::hosts::experimental::host_interfaces());
        }
    }
}

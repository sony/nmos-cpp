#include "lldp/lldp.h"

#include <iomanip>  // for std::setfill
#include <sstream>
#include <boost/asio.hpp>
#include "bst/regex.h"
#include "slog/all_in_one.h" // for slog::manip_function, etc.

namespace lldp
{
    namespace details
    {
        slog::manip_function<std::istream> const_char_of(std::initializer_list<char> cs)
        {
            return slog::manip_function<std::istream>([cs](std::istream& is)
            {
                if (cs.end() == std::find(cs.begin(), cs.end(), is.get())) is.setstate(std::ios_base::failbit);
            });
        }
    }

    // return empty if invalid; non-throwing
    std::vector<uint8_t> make_mac_address(const std::string& mac_address)
    {
        std::vector<uint8_t> data;

        // mac_address is xx-xx-xx-xx-xx-xx
        const size_t mac_size(6);
        std::istringstream is(mac_address);
        
        is >> std::hex;
        uint32_t d = 0;

        for (size_t i = 0; i < mac_size; ++i)
        {
            if (0 != i) is >> details::const_char_of({ '-', ':' });
            is >> std::setw(2) >> d;
            data.push_back((uint8_t)d);
        }
        return !is.fail() ? data : std::vector<uint8_t>{};
    }

    // return empty if invalid; non-throwing
    std::string parse_mac_address(const std::vector<uint8_t>& data, char separator)
    {
        const size_t mac_size(6);

        if (data.size() < mac_size) return std::string{};

        std::ostringstream os;
        os << std::setfill('0') << std::hex << std::nouppercase;

        for (size_t i = 0; i < mac_size; ++i)
        {
            if (0 != i) os << separator;
            os << std::setw(2) << (uint32_t)data[i];
        }
        return os.str();
    }

    // make a network address, i.e. address_family and address data
    // address can be an IPv4 or IPv6 or MAC address or DNS name
    // return empty if invalid; non-throwing
    std::vector<uint8_t> make_network_address(const std::string& address)
    {
        std::vector<uint8_t> data;

        // see https://stackoverflow.com/a/3824105
        static const bst::regex dns_regex(R"(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*\.?)");

        static const bst::regex mac_regex(R"([0-9a-fA-F]{2}-){5}([0-9a-fA-F]{2})");

        boost::system::error_code ec;
        auto addr = boost::asio::ip::address::from_string(address, ec);

        if (addr.is_v4())
        {
            data.push_back(network_address_family_numbers::ipv4);
            const auto network_address = addr.to_v4().to_bytes();
            data.insert(data.end(), network_address.begin(), network_address.end());
        }
        else if (addr.is_v6())
        {
            data.push_back(network_address_family_numbers::ipv6);
            const auto network_address = addr.to_v6().to_bytes();
            data.insert(data.end(), network_address.begin(), network_address.end());
        }
        else if (bst::regex_match(address, mac_regex))
        {
            data.push_back(network_address_family_numbers::mac);
            const auto mac_address = make_mac_address(address);
            data.insert(data.end(), mac_address.begin(), mac_address.end());
        }
        else if (bst::regex_match(address, dns_regex))
        {
            data.push_back(network_address_family_numbers::dns);
            data.insert(data.end(), address.begin(), address.end());
        }

        return data;
    }

    // data must be an IPv4 or IPv6 or MAC address or DNS name
    // return empty if invalid; non-throwing
    std::string parse_network_address(const std::vector<uint8_t>& data)
    {
        std::string address;

        auto len = data.size();
        if (len)
        {
            int idx = 0;

            // IANA Address Family Numbers enum value byte
            network_address_family_number network_address_family = data[idx++];

            auto address_data = data.data() + idx;

            if (network_address_family_numbers::ipv4 == network_address_family)
            {
                const size_t ipv4_size(4);
                if (len > ipv4_size)
                {
                    boost::asio::ip::address_v4::bytes_type addr4_bytes;
                    std::copy_n(address_data, addr4_bytes.size(), addr4_bytes.begin());
                    address = boost::asio::ip::address_v4(addr4_bytes).to_string();
                }
            }
            else if (network_address_family_numbers::ipv6 == network_address_family)
            {
                const size_t ipv6_size(8);
                if (len > ipv6_size)
                {
                    boost::asio::ip::address_v6::bytes_type addr6_bytes;
                    std::copy_n(address_data, addr6_bytes.size(), addr6_bytes.begin());
                    address = boost::asio::ip::address_v6(addr6_bytes).to_string();
                }
            }
            else if (network_address_family_numbers::mac == network_address_family)
            {
                address = parse_mac_address({ data.begin() + idx, data.end() });
            }
            else if (network_address_family_numbers::dns == network_address_family)
            {
                address = std::string{ data.begin() + idx, data.end() };
            }            
        }

        return address;
    }

    // make a Chassis ID with the specified MAC address, or IPv4 or IPv6 address or DNS name
    // return reserved otherwise; non-throwing
    chassis_id make_chassis_id(const std::string& chassis_id)
    {
        auto data = make_mac_address(chassis_id);
        if (!data.empty()) return{ chassis_id_subtypes::mac_address, std::move(data) };
        data = make_network_address(chassis_id);
        if (!data.empty()) return{ chassis_id_subtypes::network_address, std::move(data) };
        return{ chassis_id_subtypes::reserved, {} };
    }

    // parse a Chassis ID
    // return empty if invalid; non-throwing
    std::string parse_chassis_id(const chassis_id& chassis_id)
    {
        switch (chassis_id.subtype)
        {
        case chassis_id_subtypes::mac_address:
            return parse_mac_address(chassis_id.data);
        case chassis_id_subtypes::network_address:
            return parse_network_address(chassis_id.data);        
        case chassis_id_subtypes::chassis_component:
        case chassis_id_subtypes::port_component:
        case chassis_id_subtypes::interface_alias:
        case chassis_id_subtypes::interface_name:
        case chassis_id_subtypes::locally_assigned:
            return std::string{ chassis_id.data.begin(), chassis_id.data.end() };
        case chassis_id_subtypes::reserved:
        default:
            return{};
        }
    }

    // make a Chassis ID with the specified MAC address
    // may throw
    chassis_id make_mac_address_chassis_id(const std::string& mac_address)
    {
        auto data = make_mac_address(mac_address);
        if (data.empty()) throw lldp_exception("invalid MAC address for Chassis ID");
        return{ chassis_id_subtypes::mac_address, std::move(data) };
    }

    // parse a MAC address subtype Chassis ID
    // may throw
    std::string parse_mac_address_chassis_id(const chassis_id& chassis_id)
    {
        if (chassis_id_subtypes::mac_address != chassis_id.subtype) throw lldp_exception("wrong Chassis ID subtype");
        auto mac_address = parse_mac_address(chassis_id.data);
        if (mac_address.empty()) throw lldp_exception("invalid MAC address for Chassis ID");
        return mac_address;
    }

    // make a Chassis ID with the specified IPv4 or IPv6 address
    // may throw
    chassis_id make_network_address_chassis_id(const std::string& address)
    {
        auto data = make_network_address(address);
        if (data.empty()) throw lldp_exception("invalid network address for Chassis ID");
        return{ chassis_id_subtypes::network_address, std::move(data) };
    }

    // parse a network address subtype Chassis ID
    // may throw
    std::string parse_network_address_chassis_id(const chassis_id& chassis_id)
    {
        if (chassis_id_subtypes::network_address != chassis_id.subtype) throw lldp_exception("wrong Chassis ID subtype");
        auto address = parse_network_address(chassis_id.data);
        if (address.empty()) throw lldp_exception("invalid network address for Chassis ID");
        return address;
    }

    // make a Port ID with the specified MAC address, or IPv4 or IPv6 address
    // return reserved otherwise; non-throwing
    port_id make_port_id(const std::string& port_id)
    {
        auto data = make_mac_address(port_id);
        if (!data.empty()) return{ port_id_subtypes::mac_address, std::move(data) };
        data = make_network_address(port_id);
        if (!data.empty()) return{ port_id_subtypes::network_address, std::move(data) };
        return{ port_id_subtypes::reserved,{} };
    }

    // forward declaration
    agent_circuit_id parse_agent_circuit_id(const std::vector<uint8_t>& data);

    // parse a Port ID
    // return empty if invalid; non-throwing
    std::string parse_port_id(const port_id& port_id)
    {
        switch (port_id.subtype)
        {
        case port_id_subtypes::mac_address:
            return parse_mac_address(port_id.data);
        case port_id_subtypes::network_address:
            return parse_network_address(port_id.data);
        case port_id_subtypes::agent_circuit_id:
            // lossy...
            return parse_agent_circuit_id(port_id.data).agent_id;
        case port_id_subtypes::port_component:
        case port_id_subtypes::interface_alias:
        case port_id_subtypes::interface_name:
        case port_id_subtypes::locally_assigned:
            // but then so are these...
            return std::string{ port_id.data.begin(), port_id.data.end() };
        case port_id_subtypes::reserved:
        default:
            return{};
        }
    }

    // make a Port ID with the specified MAC address
    // may throw
    port_id make_mac_address_port_id(const std::string& mac_address)
    {
        auto data = make_mac_address(mac_address);
        if (data.empty()) throw lldp_exception("invalid MAC address for Port ID");
        return{ port_id_subtypes::mac_address, std::move(data) };
    }

    // parse a MAC address subtype Port ID
    // may throw
    std::string parse_mac_address_port_id(const port_id& port_id)
    {
        if (port_id_subtypes::mac_address != port_id.subtype) throw lldp_exception("wrong Port ID subtype");
        auto mac_address = parse_mac_address(port_id.data);
        if (mac_address.empty()) throw lldp_exception("invalid MAC address for Port ID");
        return mac_address;
    }

    // make a Port ID with the specified IPv4 or IPv6 address
    // may throw
    port_id make_network_address_port_id(const std::string& address)
    {
        auto data = make_network_address(address);
        if (data.empty()) throw lldp_exception("invalid network address for Port ID");
        return{ port_id_subtypes::network_address, std::move(data) };
    }

    // parse a network address subtype Port ID
    // may throw
    std::string parse_network_address_port_id(const port_id& port_id)
    {
        if (port_id_subtypes::network_address != port_id.subtype) throw lldp_exception("wrong Port ID subtype");
        auto address = parse_network_address(port_id.data);
        if (address.empty()) throw lldp_exception("invalid network address for Port ID");
        return address;
    }

    std::vector<uint8_t> make_agent_circuit_id(const lldp::agent_circuit_id& agent_circuit_id)
    {
        std::vector<uint8_t> data;
        data.push_back(agent_circuit_id.subopt);
        if (0xFF < agent_circuit_id.agent_id.size())
        {
            throw lldp_exception("invalid agent circuit ID length for Port ID");
        }
        data.push_back((uint8_t)agent_circuit_id.agent_id.size());
        data.insert(data.end(), agent_circuit_id.agent_id.begin(), agent_circuit_id.agent_id.end());
        return data;
    }

    agent_circuit_id parse_agent_circuit_id(const std::vector<uint8_t>& data)
    {
        agent_circuit_id agent_circuit_id;
        const size_t min_agent_circuit_id_port_id_size(2);
        if (data.size() >= min_agent_circuit_id_port_id_size)
        {
            int idx = 0;
            // subopt
            agent_circuit_id.subopt = data.at(idx++);
            if (agent_circuit_id_subopts::local == agent_circuit_id.subopt || agent_circuit_id_subopts::remote == agent_circuit_id.subopt)
            {
                // circuit length
                size_t circuit_id_len = data.at(idx++);
                // agent circuit id
                if ((data.size() - idx) >= circuit_id_len)
                {
                    agent_circuit_id.agent_id = std::string((const char*)data.data() + idx, circuit_id_len);
                }
            }
        }
        return agent_circuit_id;
    }

    // make a Port ID with the specified agent circuit ID
    // may throw
    port_id make_agent_circuit_id_port_id(const agent_circuit_id& agent_circuit_id)
    {
        return{ port_id_subtypes::agent_circuit_id, make_agent_circuit_id(agent_circuit_id) };
    }

    // parse a agent circuit ID subtype Port ID
    // may throw
    agent_circuit_id parse_agent_circuit_id_port_id(const port_id& port_id)
    {
        if (port_id_subtypes::agent_circuit_id != port_id.subtype)
        {
            throw lldp_exception("wrong Port ID subtype");
        }
        return parse_agent_circuit_id(port_id.data);
    }

    network_address_family_number management_address::address_family() const
    {
        return !network_address.empty() ? network_address[0] : network_address_family_numbers::reserved;
    }

    // make a Management Address
    // address must be an IPv4 or IPv6 address, a MAC address or DNS name
    // may throw
    management_address make_management_address(const std::string& address, interface_numbering_subtype interface_numbering_subtype, uint32_t interface_number, const std::vector<uint8_t>& object_identifier)
    {
        auto data = make_network_address(address);
        if (data.empty()) throw lldp_exception("invalid network address for Management Address");
        return{
            std::move(data),
            interface_numbering_subtype,
            interface_number,
            object_identifier
        };
    }
}

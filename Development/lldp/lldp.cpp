#include "lldp/lldp.h"

#include <iomanip>  // for std::setfill
#include <sstream>
#include <boost/asio.hpp>
#include "bst/regex.h"
#include "slog/all_in_one.h" // for slog::manip_function, etc.

namespace lldp
{
    static lldp_exception lldp_format_error(std::string message)
    {
        return{ "lldp format error - " + std::move(message) };
    }

    static lldp_exception lldp_parse_error(std::string message)
    {
        return{ "lldp parse error - " + std::move(message) };
    }

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

    const size_t ipv4_size(boost::asio::ip::address_v4::bytes_type().size());
    const size_t ipv6_size(boost::asio::ip::address_v6::bytes_type().size());
    const size_t mac_size(6);

    // return empty if invalid; non-throwing
    std::vector<uint8_t> make_mac_address(const std::string& mac_address)
    {
        std::vector<uint8_t> data;

        // mac_address is xx-xx-xx-xx-xx-xx
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

    bool is_valid_network_address_data_size(network_address_family_number address_family, size_t data_size)
    {
        switch (address_family)
        {
        case network_address_family_numbers::ipv4:
            return ipv4_size == data_size;
        case network_address_family_numbers::ipv6:
            return ipv6_size == data_size;
        case network_address_family_numbers::mac:
            return mac_size == data_size;
        case network_address_family_numbers::dns:
        case network_address_family_numbers::reserved:
        default:
            return true;
        }
    }

    // make a network address, i.e. address_family and address data
    // address can be an IPv4 or IPv6 or MAC address or DNS name
    // return empty if invalid; non-throwing
    std::vector<uint8_t> make_network_address(const std::string& address)
    {
        std::vector<uint8_t> data;

        // see https://stackoverflow.com/a/3824105
        static const bst::regex dns_regex(R"(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*\.?)");

        static const bst::regex mac_regex(R"(([0-9a-fA-F]{2}-){5}([0-9a-fA-F]{2}))");

        boost::system::error_code ec;
        auto addr = boost::asio::ip::address::from_string(address, ec);

        if (!ec)
        {
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

        if (!data.empty())
        {
            // IANA Address Family Numbers enum value byte
            network_address_family_number address_family(*data.begin());
            auto first = std::next(data.begin()), last = data.end();

            if (is_valid_network_address_data_size(address_family, std::distance(first, last)))
            {
                if (network_address_family_numbers::ipv4 == address_family)
                {
                    boost::asio::ip::address_v4::bytes_type addr4_bytes;
                    std::copy_n(first, addr4_bytes.size(), addr4_bytes.begin());
                    address = boost::asio::ip::address_v4(addr4_bytes).to_string();
                }
                else if (network_address_family_numbers::ipv6 == address_family)
                {
                    boost::asio::ip::address_v6::bytes_type addr6_bytes;
                    std::copy_n(first, addr6_bytes.size(), addr6_bytes.begin());
                    address = boost::asio::ip::address_v6(addr6_bytes).to_string();
                }
                else if (network_address_family_numbers::mac == address_family)
                {
                    address = parse_mac_address({ first, last });
                }
                else if (network_address_family_numbers::dns == address_family)
                {
                    address = std::string{ first, last };
                }
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
        if (data.empty()) throw lldp_format_error("invalid MAC address for Chassis ID");
        return{ chassis_id_subtypes::mac_address, std::move(data) };
    }

    // parse a MAC address subtype Chassis ID
    // may throw
    std::string parse_mac_address_chassis_id(const chassis_id& chassis_id)
    {
        if (chassis_id_subtypes::mac_address != chassis_id.subtype) throw lldp_parse_error("wrong Chassis ID subtype");
        auto mac_address = parse_mac_address(chassis_id.data);
        if (mac_address.empty()) throw lldp_parse_error("invalid MAC address for Chassis ID");
        return mac_address;
    }

    // make a Chassis ID with the specified IPv4 or IPv6 address
    // may throw
    chassis_id make_network_address_chassis_id(const std::string& address)
    {
        auto data = make_network_address(address);
        if (data.empty()) throw lldp_format_error("invalid network address for Chassis ID");
        return{ chassis_id_subtypes::network_address, std::move(data) };
    }

    // parse a network address subtype Chassis ID
    // may throw
    std::string parse_network_address_chassis_id(const chassis_id& chassis_id)
    {
        if (chassis_id_subtypes::network_address != chassis_id.subtype) throw lldp_parse_error("wrong Chassis ID subtype");
        auto address = parse_network_address(chassis_id.data);
        if (address.empty()) throw lldp_parse_error("invalid network address for Chassis ID");
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
        case port_id_subtypes::port_component:
        case port_id_subtypes::interface_alias:
        case port_id_subtypes::interface_name:
        case port_id_subtypes::locally_assigned:
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
        if (data.empty()) throw lldp_format_error("invalid MAC address for Port ID");
        return{ port_id_subtypes::mac_address, std::move(data) };
    }

    // parse a MAC address subtype Port ID
    // may throw
    std::string parse_mac_address_port_id(const port_id& port_id)
    {
        if (port_id_subtypes::mac_address != port_id.subtype) throw lldp_parse_error("wrong Port ID subtype");
        auto mac_address = parse_mac_address(port_id.data);
        if (mac_address.empty()) throw lldp_parse_error("invalid MAC address for Port ID");
        return mac_address;
    }

    // make a Port ID with the specified IPv4 or IPv6 address
    // may throw
    port_id make_network_address_port_id(const std::string& address)
    {
        auto data = make_network_address(address);
        if (data.empty()) throw lldp_format_error("invalid network address for Port ID");
        return{ port_id_subtypes::network_address, std::move(data) };
    }

    // parse a network address subtype Port ID
    // may throw
    std::string parse_network_address_port_id(const port_id& port_id)
    {
        if (port_id_subtypes::network_address != port_id.subtype) throw lldp_parse_error("wrong Port ID subtype");
        auto address = parse_network_address(port_id.data);
        if (address.empty()) throw lldp_parse_error("invalid network address for Port ID");
        return address;
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
        if (data.empty()) throw lldp_format_error("invalid network address for Management Address");
        return{
            std::move(data),
            interface_numbering_subtype,
            interface_number,
            object_identifier
        };
    }
}

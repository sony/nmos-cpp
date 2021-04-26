#ifndef LLDP_LLDP_H
#define LLDP_LLDP_H

#include <chrono>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace lldp
{
    struct lldp_exception : std::runtime_error
    {
        lldp_exception(const std::string& message) : std::runtime_error(message) {}
    };

    // indicates the status of an interface according to an LLDP manager
    // i.e. the Administrative Status, if only one manager in the system
    enum management_status
    {
        unmanaged = 0x0,
        transmit = 0x1,
        receive = 0x2,
        transmit_receive = transmit + receive
    };

    // MAC address functions and constants

    // return empty if invalid; non-throwing
    std::vector<uint8_t> make_mac_address(const std::string& mac_address);

    // return empty if invalid; non-throwing
    std::string parse_mac_address(const std::vector<uint8_t>& data, char separator = '-');

    // Group MAC addresses used by LLDP
    namespace group_mac_addresses
    {
        // constrained to a single physical link; stopped by all types of bridge
        const std::string nearest_bridge("01-80-C2-00-00-0E");

        // constrained by all bridges other than TPMRs; intended for use within provider bridged networks
        const std::string nearest_non_tpmr_bridge("01-80-C2-00-00-03");

        // constrained by customer bridges; this gives the same coverage as a customer-customer MACSec connection
        const std::string nearest_customer_bridge("01-80-C2-00-00-00");
    }

    // Network address functions and constants

    // IANA Address Family Numbers enumeration value
    // see https://www.iana.org/assignments/address-family-numbers/address-family-numbers.xhtml
    typedef uint8_t network_address_family_number;
    namespace network_address_family_numbers
    {
        const network_address_family_number reserved = 0;
        const network_address_family_number ipv4 = 1;
        const network_address_family_number ipv6 = 2;
        const network_address_family_number mac = 6;
        const network_address_family_number dns = 16;
    }

    bool is_valid_network_address_data_size(network_address_family_number address_family, size_t data_size);

    // make a network address, i.e. address_family and address data
    // address must be an IPv4 or IPv6 address, a MAC address or DNS name
    // return empty if invalid; non-throwing
    std::vector<uint8_t> make_network_address(const std::string& address);

    // data must be an IPv4 or IPv6 or MAC address or DNS name
    // return empty if invalid; non-throwing
    std::string parse_network_address(const std::vector<uint8_t>& data);

    // LLDP TLV (type, length, value) fields

    // Chassis ID

    // Chassis ID subtype definitions
    typedef uint8_t chassis_id_subtype;
    namespace chassis_id_subtypes
    {
        const chassis_id_subtype reserved = 0; // indicates default
        const chassis_id_subtype chassis_component = 1;
        const chassis_id_subtype interface_alias = 2;
        const chassis_id_subtype port_component = 3;
        const chassis_id_subtype mac_address = 4;
        const chassis_id_subtype network_address = 5;
        const chassis_id_subtype interface_name = 6;
        const chassis_id_subtype locally_assigned = 7;
    }

    struct chassis_id
    {
        chassis_id_subtype subtype;
        std::vector<uint8_t> data;

        chassis_id(chassis_id_subtype subtype = chassis_id_subtypes::reserved, const std::vector<uint8_t>& data = {})
            : subtype(subtype)
            , data(data)
        {}

        auto tied() const -> decltype(std::tie(subtype, data)) { return std::tie(subtype, data); }
        friend bool operator==(const chassis_id& lhs, const chassis_id& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const chassis_id& lhs, const chassis_id& rhs) { return !(lhs == rhs); }
    };

    // make a Chassis ID with the specified MAC address or network address (e.g. IPv4 or IPv6 address, or DNS name)
    // otherwise return reserved; non-throwing
    chassis_id make_chassis_id(const std::string& chassis_id);

    // parse a Chassis ID
    // return empty if invalid; non-throwing
    std::string parse_chassis_id(const chassis_id& chassis_id);

    // make a Chassis ID with the specified MAC address
    // may throw
    chassis_id make_mac_address_chassis_id(const std::string& mac_address);

    // parse a MAC address subtype Chassis ID
    // may throw
    std::string parse_mac_address_chassis_id(const chassis_id& chassis_id);

    // make a Chassis ID with the specified network address (e.g. IPv4 or IPv6 address, or DNS name)
    // may throw
    chassis_id make_network_address_chassis_id(const std::string& address);

    // parse a network address subtype Chassis ID
    // may throw
    std::string parse_network_address_chassis_id(const chassis_id& chassis_id);

    // Port ID

    // Port ID subtype definitions
    typedef uint8_t port_id_subtype;
    namespace port_id_subtypes
    {
        const port_id_subtype reserved = 0;
        const port_id_subtype interface_alias = 1;
        const port_id_subtype port_component = 2;
        const port_id_subtype mac_address = 3;
        const port_id_subtype network_address = 4;
        const port_id_subtype interface_name = 5;
        const port_id_subtype agent_circuit_id = 6;
        const port_id_subtype locally_assigned = 7;
    }

    struct port_id
    {
        port_id_subtype subtype;
        std::vector<uint8_t> data;

        port_id(port_id_subtype subtype = port_id_subtypes::reserved, const std::vector<uint8_t>& data = {})
            : subtype(subtype)
            , data(data)
        {}

        auto tied() const -> decltype(std::tie(subtype, data)) { return std::tie(subtype, data); }
        friend bool operator==(const port_id& lhs, const port_id& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const port_id& lhs, const port_id& rhs) { return !(lhs == rhs); }
    };

    // make a Port ID with the specified MAC address or network address (e.g. IPv4 or IPv6 address, or DNS name)
    // otherwise return reserved; non-throwing
    port_id make_port_id(const std::string& port_id);

    // parse a Port ID
    // return empty if invalid; non-throwing
    std::string parse_port_id(const port_id& port_id);

    // make a Port ID with the specified MAC address
    // may throw
    port_id make_mac_address_port_id(const std::string& mac_address);

    // parse a MAC address subtype Port ID
    // may throw
    std::string parse_mac_address_port_id(const port_id& port_id);

    // make a Port ID with the specified network address (e.g. IPv4 or IPv6 address, or DNS name)
    // may throw
    port_id make_network_address_port_id(const std::string& address);

    // parse a network address subtype Port ID
    // may throw
    std::string parse_network_address_port_id(const port_id& port_id);

    // System Capabilities

    typedef uint16_t capability_bitmap;
    namespace capability_bitmaps
    {
        const capability_bitmap other = 0x1;
        const capability_bitmap repeater = 0x2;
        const capability_bitmap bridge = 0x4;
        const capability_bitmap wlan_access_point = 0x8;
        const capability_bitmap router = 0x10;
        const capability_bitmap telephone = 0x20;
        const capability_bitmap docsis_cable_device = 0x40;
        const capability_bitmap station_only = 0x80;
        const capability_bitmap c_vlan_component = 0x100;
        const capability_bitmap s_vlan_component = 0x200;
        const capability_bitmap tpmr = 0x400;
    }

    struct system_capabilities
    {
        capability_bitmap system;
        capability_bitmap enabled;

        system_capabilities(capability_bitmap system = capability_bitmaps::other, capability_bitmap enabled = capability_bitmaps::other)
            : system(system)
            , enabled(enabled)
        {}

        static system_capabilities station_only() { return{ capability_bitmaps::station_only, capability_bitmaps::station_only }; }

        auto tied() const -> decltype(std::tie(system, enabled)) { return std::tie(system, enabled); }
        friend bool operator==(const system_capabilities& lhs, const system_capabilities& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const system_capabilities& lhs, const system_capabilities& rhs) { return !(lhs == rhs); }
    };

    // Management Addresses

    typedef uint8_t interface_numbering_subtype;
    namespace interface_numbering_subtypes
    {
        const interface_numbering_subtype unknown = 0x1;
        const interface_numbering_subtype if_index = 0x2;
        const interface_numbering_subtype system_port_number = 0x3;
    }

    struct management_address
    {
        std::vector<uint8_t> network_address; // i.e. address_family and address data
        interface_numbering_subtype interface_numbering;
        uint32_t interface_number;
        std::vector<uint8_t> object_identifier;

        management_address(const std::vector<uint8_t>& network_address = {}, interface_numbering_subtype interface_numbering = interface_numbering_subtypes::unknown, uint32_t interface_number = 0, const std::vector<uint8_t>& object_identifier = {})
            : network_address(network_address)
            , interface_numbering(interface_numbering)
            , interface_number(interface_number)
            , object_identifier(object_identifier)
        {}

        network_address_family_number address_family() const;

        auto tied() const -> decltype(std::tie(network_address, interface_numbering, interface_number, object_identifier)) { return std::tie(network_address, interface_numbering, interface_number, object_identifier); }
        friend bool operator==(const management_address& lhs, const management_address& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const management_address& lhs, const management_address& rhs) { return !(lhs == rhs); }
    };

    // make a Management Address
    // address must be an IPv4 or IPv6 address, a MAC address or DNS name
    // may throw
    management_address make_management_address(const std::string& address, interface_numbering_subtype interface_numbering_subtype, uint32_t interface_number, const std::vector<uint8_t>& object_identifier);

    // LLDP Data Unit

    struct lldp_data_unit
    {
        lldp::chassis_id chassis_id;
        lldp::port_id port_id;
        std::chrono::seconds time_to_live;
        std::vector<std::string> port_descriptions;
        std::string system_name;
        std::string system_description;
        lldp::system_capabilities system_capabilities;
        std::vector<lldp::management_address> management_addresses;

        lldp_data_unit(const lldp::chassis_id& chassis_id = {}, const lldp::port_id& port_id = {}, const std::chrono::seconds& time_to_live = {}, const std::vector<std::string>& port_descriptions = {}, const std::string& system_name = {}, const std::string& system_description = {}, const lldp::system_capabilities& system_capabilities = {}, const std::vector<lldp::management_address>& management_addresses = {})
            : chassis_id(chassis_id)
            , port_id(port_id)
            , time_to_live(time_to_live)
            , port_descriptions(port_descriptions)
            , system_name(system_name)
            , system_description(system_description)
            , system_capabilities(system_capabilities)
            , management_addresses(management_addresses)
        {}

        auto tied() const -> decltype(std::tie(chassis_id, port_id, time_to_live, port_descriptions, system_name, system_description, system_capabilities, management_addresses)) { return std::tie(chassis_id, port_id, time_to_live, port_descriptions, system_name, system_description, system_capabilities, management_addresses); }
        friend bool operator==(const lldp_data_unit& lhs, const lldp_data_unit& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator!=(const lldp_data_unit& lhs, const lldp_data_unit& rhs) { return !(lhs == rhs); }
    };

    inline lldp_data_unit normal_data_unit(const lldp::chassis_id& chassis_id, const lldp::port_id& port_id, const std::vector<std::string>& port_descriptions = {}, const std::string& system_name = {}, const std::string& system_description = {}, const lldp::system_capabilities& system_capabilities = {}, const std::vector<lldp::management_address>& management_addresses = {})
    {
        return{ chassis_id, port_id, std::chrono::seconds(121), port_descriptions, system_name, system_description, system_capabilities, management_addresses };
    }

    inline lldp_data_unit shutdown_data_unit(const lldp::chassis_id& chassis_id, const lldp::port_id& port_id)
    {
        return{ chassis_id, port_id };
    }

    // LLDP Handler
    typedef std::function<void(const std::string& interface_id, const lldp_data_unit& data)> lldp_handler;
}

#endif

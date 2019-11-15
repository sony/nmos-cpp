#ifndef LLDP_LLDP_FRAME_H
#define LLDP_LLDP_FRAME_H

#include "lldp/lldp.h"

namespace lldp
{
    namespace details
    {
        // LLDP EtherType
        const uint16_t lldp_ether_type = 0x88CC;

        struct ether_header
        {
            uint16_t ether_type;
            std::string src_mac_address;
            std::string dest_mac_address;

            ether_header(const std::string& src_mac_address = {}, const std::string& dest_mac_address = group_mac_addresses::nearest_bridge)
                : ether_type(lldp_ether_type)
                , src_mac_address(src_mac_address)
                , dest_mac_address(dest_mac_address)
            {}

            auto tied() const -> decltype(std::tie(ether_type, src_mac_address, dest_mac_address)) { return std::tie(ether_type, src_mac_address, dest_mac_address); }
            friend bool operator==(const ether_header& lhs, const ether_header& rhs) { return lhs.tied() == rhs.tied(); }
            friend bool operator!=(const ether_header& lhs, const ether_header& rhs) { return !(lhs == rhs); }
        };

        struct lldp_frame
        {
            ether_header header;
            lldp_data_unit lldpdu;

            lldp_frame(const ether_header& header = {}, const lldp_data_unit& lldpdu = {})
                : header(header)
                , lldpdu(lldpdu)
            {}

            auto tied() const -> decltype(std::tie(header, lldpdu)) { return std::tie(header, lldpdu); }
            friend bool operator==(const lldp_frame& lhs, const lldp_frame& rhs) { return lhs.tied() == rhs.tied(); }
            friend bool operator!=(const lldp_frame& lhs, const lldp_frame& rhs) { return !(lhs == rhs); }
        };

        // make a LLDP farme byte array
        // may throw
        std::vector<uint8_t> make_lldp_frame(const lldp_frame& lldp_frame);

        // make a LLDP frame with the given byte array
        // may throw
        lldp_frame parse_lldp_frame(const uint8_t* data, size_t len);
    }
}

#endif

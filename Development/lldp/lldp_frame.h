#ifndef LLDP_LLDP_FRAME_H
#define LLDP_LLDP_FRAME_H

#include "lldp/lldp.h"

namespace lldp
{
    namespace details
    {
        // LLDP EtherType
        const uint16_t lldp_ether_type = 0x88CC;

        struct lldp_frame
        {
            std::vector<uint8_t> destination_mac_address;
            std::vector<uint8_t> source_mac_address;

            lldp_data_unit lldpdu;

            lldp_frame(std::vector<uint8_t> destination_mac_address = {}, std::vector<uint8_t> source_mac_address = {}, lldp_data_unit lldpdu = {})
                : destination_mac_address(std::move(destination_mac_address))
                , source_mac_address(std::move(source_mac_address))
                , lldpdu(std::move(lldpdu))
            {}

            auto tied() const -> decltype(std::tie(destination_mac_address, source_mac_address, lldpdu)) { return std::tie(destination_mac_address, source_mac_address, lldpdu); }
            friend bool operator==(const lldp_frame& lhs, const lldp_frame& rhs) { return lhs.tied() == rhs.tied(); }
            friend bool operator!=(const lldp_frame& lhs, const lldp_frame& rhs) { return !(lhs == rhs); }
        };

        // encode an LLDP frame
        // may throw
        std::vector<uint8_t> make_lldp_frame(const lldp_frame& lldp_frame);

        // decode an LLDP frame
        // may throw
        lldp_frame parse_lldp_frame(const uint8_t* data, size_t len);
    }
}

#endif

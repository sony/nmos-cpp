#include "lldp/lldp_frame.h"

namespace lldp
{
    static lldp_exception lldp_parse_error(std::string message)
    {
        return{ "lldp parse error - " + std::move(message) };
    }

    namespace details
    {
        // make an EtherType byte array
        // non-throwing
        std::vector<uint8_t> make_ether_type(uint16_t ether_type)
        {
            return{ (uint8_t)((ether_type >> 8) & 0xFF), (uint8_t)(ether_type & 0xFF) };
        }

        // make an EtherType with the given byte array
        // may throw
        uint16_t parse_ether_type(const std::vector<uint8_t>& data)
        {
            const size_t ether_type_size(2);
            if (data.size() >= ether_type_size)
            {
                return uint16_t((data[0] << 8) | data[1]);
            }
            throw lldp_parse_error("invalid EtherType");
        }

        // make an Ethernet frame header byte array
        // may throw
        std::vector<uint8_t> make_ether_header(const std::vector<uint8_t>& dest_mac, const std::vector<uint8_t>& src_mac)
        {
            std::vector<uint8_t> data;
            // Destination MAC
            data.insert(data.end(), dest_mac.begin(), dest_mac.end());
            // Source MAC
            data.insert(data.end(), src_mac.begin(), src_mac.end());
            // EtherType
            const auto ether_type = make_ether_type(lldp_ether_type);
            data.insert(data.end(), ether_type.begin(), ether_type.end());
            return data;
        }

        // make an Ethernet frame header with the given byte array
        // returns the number of bytes consumed, may throw
        size_t parse_ether_header(const uint8_t* data, size_t len, std::vector<uint8_t>& dest_mac, std::vector<uint8_t>& src_mac)
        {
            size_t consumed{ 0 };

            const size_t mac_size(6);
            const size_t ether_type_size(2);
            if (mac_size + mac_size + ether_type_size > len)
            {
                throw lldp_parse_error("invalid length of Ethernet frame header");
            }

            // Destination MAC
            dest_mac = { data, data + mac_size };
            data += mac_size;
            consumed += mac_size;

            // Source MAC
            src_mac = { data, data + mac_size };
            data += mac_size;
            consumed += mac_size;

            // EtherType
            const uint16_t ether_type = parse_ether_type({ data, data + ether_type_size });
            if (lldp_ether_type != ether_type)
            {
                throw lldp_parse_error("unexpected EtherType found in Ethernet frame header");
            }
            consumed += ether_type_size;

            return consumed;
        }

        // LLDP TLV type definitions
        typedef uint8_t tlv_type;
        namespace tlv_types
        {
            const tlv_type end_of_LLDPDU = 0;
            const tlv_type chassis_id = 1;
            const tlv_type port_id = 2;
            const tlv_type time_to_live = 3;
            const tlv_type port_description = 4;
            const tlv_type system_name = 5;
            const tlv_type system_description = 6;
            const tlv_type system_capabilities = 7;
            const tlv_type management_address = 8;
        }

        struct tlv
        {
            tlv_type type;
            std::vector<uint8_t> value;

            tlv(tlv_type type = tlv_types::end_of_LLDPDU, const std::vector<uint8_t>& value = {})
                : type(type)
                , value(value) {}
        };

        // make a TLV byte array
        // non-throwing
        std::vector<uint8_t> make_tlv(const tlv& tlv)
        {
            std::vector<uint8_t> data;

            auto value_len = tlv.value.size();
            data.push_back((tlv.type << 1) | ((value_len >> 8) & 0x01));
            data.push_back(value_len & 0xFF);
            if (value_len)
            {
                data.insert(data.end(), tlv.value.begin(), tlv.value.end());
            }
            return data;
        }

        // make a TLV with given byte array
        // returns the number of bytes consumed, may throw
        size_t parse_tlv(const uint8_t* data, size_t len, tlv& tlv)
        {
            const auto min_tlv_size(2);

            if (data && len >= min_tlv_size)
            {
                const auto mask(0x01FF);

                tlv.type = (data[0] >> 1);
                uint16_t value_len = ((data[0] << 8 | data[1]) & mask);

                const size_t tlv_len = min_tlv_size + value_len;
                if (len >= tlv_len)
                {
                    tlv.value.insert(tlv.value.end(), &data[2], &data[2] + value_len);
                    return tlv_len;
                }
                throw lldp_parse_error("TLV value field is shorter than expected");
            }
            throw lldp_parse_error("no data value for TLV");
        }

        // make a Chassis ID byte array
        // non-throwing
        std::vector<uint8_t> make_chassis_id(const chassis_id& chassis_id)
        {
            std::vector<uint8_t> data;
            data.push_back(chassis_id.subtype);
            data.insert(data.end(), chassis_id.data.begin(), chassis_id.data.end());
            return data;
        }

        // make a Chassis ID with the given byte array
        // may throw
        chassis_id parse_chassis_id(const std::vector<uint8_t>& value)
        {
            if (value.size())
            {
                chassis_id chassis_id{ value.at(0), {value.begin() + 1, value.end()} };

                switch (chassis_id.subtype)
                {
                case chassis_id_subtypes::reserved:
                case chassis_id_subtypes::chassis_component:
                case chassis_id_subtypes::interface_alias:
                case chassis_id_subtypes::port_component:
                case chassis_id_subtypes::interface_name:
                case chassis_id_subtypes::locally_assigned:
                    break;
                case chassis_id_subtypes::mac_address:
                    // vertify MAC address Chassis ID
                    parse_mac_address_chassis_id(chassis_id);
                    break;
                case chassis_id_subtypes::network_address:
                    // verify network address Chassis ID
                    parse_network_address_chassis_id(chassis_id);
                    break;
                default:
                    // invalid Chassis ID subtype
                    throw lldp_parse_error("invalid Chassis ID subtype");
                }
                return chassis_id;
            }
            else
            {
                throw lldp_parse_error("missing Chassis ID subtype");
            }
        }

        // make a Port ID byte array
        // non-throwing
        std::vector<uint8_t> make_port_id(const port_id& port_id)
        {
            std::vector<uint8_t> data;
            data.push_back(port_id.subtype);
            data.insert(data.end(), port_id.data.begin(), port_id.data.end());
            return data;
        }

        // make a Port ID with the given byte array
        // may throw
        port_id parse_port_id(const std::vector<uint8_t>& value)
        {
            if (value.size())
            {
                port_id port_id{ value.at(0),{ value.begin() + 1, value.end() } };

                switch (port_id.subtype)
                {
                case port_id_subtypes::reserved:
                case port_id_subtypes::interface_alias:
                case port_id_subtypes::port_component:
                    break;
                case port_id_subtypes::mac_address:
                    // vertify MAC address Port ID
                    parse_mac_address_port_id(port_id);
                    break;
                case port_id_subtypes::network_address:
                    // verify network address Port ID
                    parse_network_address_port_id(port_id);
                    break;
                case port_id_subtypes::interface_name:
                case port_id_subtypes::agent_circuit_id:
                case port_id_subtypes::locally_assigned:
                    break;
                default:
                    throw lldp_parse_error("invalid Port ID subtype");
                }
                return port_id;
            }
            else
            {
                throw lldp_parse_error("missing Port ID subtype");
            }
        }

        // make a Time-To-Live byte array
        // non-throwing
        std::vector<uint8_t> make_time_to_live(std::chrono::seconds time_to_live)
        {
            return{ (uint8_t)((time_to_live.count()) >> 8), (uint8_t)(time_to_live.count() & 0xFF) };
        }

        // make a Time-To-Live with the given byte array
        // may throw
        std::chrono::seconds parse_time_to_live(const std::vector<uint8_t>& value)
        {
            const auto time_to_live_size(2);
            if (value.size() >= time_to_live_size)
            {
                return std::chrono::seconds((uint16_t)value[0] << 8 | value[1]);
            }
            throw lldp_parse_error("insufficient bytes for Time To Live");
        }

        // make a display string byte array
        // non-throwing
        inline std::vector<uint8_t> make_display_string(const std::string& value)
        {
            return std::vector<uint8_t>(value.begin(), value.end());
        }

        // make a display string with the given byte array
        // non-throwing
        inline std::string parse_display_string(const std::vector<uint8_t>& value)
        {
            return std::string(value.begin(), value.end());
        }

        // make a Port Description byte array
        // non-throwing
        std::vector<uint8_t> make_port_description(const std::string& value)
        {
            return make_display_string(value);
        }

        // make a Port Description with the given byte array
        // non-throwing
        std::string parse_port_description(const std::vector<uint8_t>& value)
        {
            return parse_display_string(value);
        }

        // make a System Name byte array
        // non-throwing
        std::vector<uint8_t> make_system_name(const std::string& value)
        {
            return make_display_string(value);
        }

        // make a System Name with the given byte array
        // non-throwing
        std::string parse_system_name(const std::vector<uint8_t>& value)
        {
            return parse_display_string(value);
        }

        // make a System Description byte array
        // non-throwing
        std::vector<uint8_t> make_system_description(const std::string& value)
        {
            return make_display_string(value);
        }

        // make a System Description with the given byte array
        // non-throwing
        std::string parse_system_description(const std::vector<uint8_t>& value)
        {
            return parse_display_string(value);
        }

        // make a System Capabilities byte array
        // non-throwing
        std::vector<uint8_t> make_system_capabilities(const system_capabilities& system_capabilities)
        {
            return{
                (uint8_t)(system_capabilities.system >> 8), (uint8_t)(system_capabilities.system & 0xFF),
                (uint8_t)(system_capabilities.enabled >> 8), (uint8_t)(system_capabilities.enabled & 0xFF) };
        }

        // make a System Capabilities with the given byte array
        // may throw
        system_capabilities parse_system_capabilities(const std::vector<uint8_t>& value)
        {
            const size_t system_capabilities_size(4);
            if (system_capabilities_size > value.size())
            {
                throw lldp_parse_error("invalid length for System Capabilities");
            }
            return{ (capability_bitmap)(((uint16_t)(value.at(0)) << 8) | value.at(1)), (capability_bitmap)(((uint16_t)(value.at(2)) << 8) | value.at(3)) };
        }

        // make a Management Address byte array
        // non-throwing
        std::vector<uint8_t> make_management_address(const management_address& management_address)
        {
            std::vector<uint8_t> data;

            // management address string length
            data.push_back(uint8_t(management_address.network_address.size()));

            // management address
            data.insert(data.end(), management_address.network_address.begin(), management_address.network_address.end());

            // interface numbering subtype
            data.push_back(management_address.interface_numbering);

            // interface number
            data.push_back((management_address.interface_number >> 24) & 0xFF);
            data.push_back((management_address.interface_number >> 16) & 0xFF);
            data.push_back((management_address.interface_number >> 8) & 0xFF);
            data.push_back(management_address.interface_number & 0xFF);

            // OID string length
            data.push_back(uint8_t(management_address.object_identifier.size()));

            // object identifier
            data.insert(data.end(), management_address.object_identifier.begin(), management_address.object_identifier.end());

            return data;
        }

        // See IEEE 802.1AB:2016 Figure 8-11 Management Address TLV Format
        bool is_valid_management_address_size(network_address_family_number address_family, size_t m)
        {
            const size_t max_size(31);
            const size_t min_size(1);
            return max_size >= m && m >= min_size && is_valid_network_address_data_size(address_family, m);
        }

        // make a Management Address with the given byte array
        // may throw
        management_address parse_management_address(const std::vector<uint8_t>& value)
        {
            management_address management_address;

            auto len = value.size();
            int idx{ 0 };

            // management address string length
            if (len < 1)
            {
                throw lldp_parse_error("missing address string length for Management Address");
            }
            const auto management_address_string_length = value.at(idx++);
            --len;
            if (management_address_string_length > len)
            {
                throw lldp_parse_error("invalid address string length for Management Address");
            }

            // management address subtype
            if (len < 1)
            {
                throw lldp_parse_error("missing address subtype for Management Address");
            }
            const network_address_family_number address_family = value.at(idx++);
            --len;
            const auto m = management_address_string_length - 1;
            if (!is_valid_management_address_size(address_family, m))
            {
                throw lldp_parse_error("invalid address string length or subtype for Management Address");
            }

            // management address
            switch (address_family)
            {
            case network_address_family_numbers::ipv4:
            case network_address_family_numbers::ipv6:
            case network_address_family_numbers::mac:
            case network_address_family_numbers::dns:
                management_address.network_address.assign(1, address_family);
                management_address.network_address.insert(management_address.network_address.end(), value.begin() + idx, value.begin() + idx + m);
                break;
            case network_address_family_numbers::reserved:
            default:
                throw lldp_parse_error("invalid address subtype for Management Address");
            }
            if (parse_network_address(management_address.network_address).empty())
            {
                throw lldp_parse_error("invalid address string for Management Address");
            }
            idx += m;
            len -= m;

            // interface numbering subtype
            if (len < 1)
            {
                throw lldp_parse_error("missing interface numbering subtype for Management Address");
            }
            management_address.interface_numbering = value.at(idx++);
            --len;

            // interface number
            const size_t interface_number_size(4);
            if (interface_number_size > len)
            {
                throw lldp_parse_error("invalid interface number for Management Address");
            }
            management_address.interface_number = (((uint32_t)value.at(idx) << 24) | ((uint32_t)value.at(idx + 1) << 16) | ((uint32_t)value.at(idx + 2) << 8) | value.at(idx + 3));
            idx += interface_number_size;
            len -= interface_number_size;

            // verify interface numbering subtype & interface number
            if ((interface_numbering_subtypes::unknown == management_address.interface_numbering) && (management_address.interface_number != 0))
            {
                throw lldp_parse_error("invalid Unknown interface numbering subtype for Management Address");
            }

            // OID string length
            if (len < 1)
            {
                throw lldp_parse_error("missing OID string length for Management Address");
            }
            const auto oid_string_length = value.at(idx++);
            --len;

            // object identifier
            management_address.object_identifier = { value.begin() + idx, value.end() };
            if (oid_string_length != management_address.object_identifier.size())
            {
                throw lldp_parse_error("invalid OID string length for Management Address");
            }

            return management_address;
        }

        // make a LLDP data unit byte array
        // non-throwing
        std::vector<uint8_t> make_lldp_data_uint(const lldp_data_unit& lldpdu)
        {
            std::vector<uint8_t> data;

            // Chassis ID TLV
            const auto chassis_id_tlv = make_tlv({ tlv_types::chassis_id, make_chassis_id(lldpdu.chassis_id) });
            data.insert(data.end(), chassis_id_tlv.begin(), chassis_id_tlv.end());

            // Port ID TLV
            const auto port_id_tlv = make_tlv({ tlv_types::port_id, make_port_id(lldpdu.port_id) });
            data.insert(data.end(), port_id_tlv.begin(), port_id_tlv.end());

            // Time to live TLV
            const auto ttl_tlv = make_tlv({ tlv_types::time_to_live, make_time_to_live(lldpdu.time_to_live) });
            data.insert(data.end(), ttl_tlv.begin(), ttl_tlv.end());

            // Optional TLVs...

            // Port Description TLV(s)
            for (const auto& port_description : lldpdu.port_descriptions)
            {
                const auto port_description_tlv = make_tlv({ tlv_types::port_description, make_port_description(port_description) });
                data.insert(data.end(), port_description_tlv.begin(), port_description_tlv.end());
            }

            // System Name TLV(s)
            if (!lldpdu.system_name.empty())
            {
                const auto system_name_tlv = make_tlv({ tlv_types::system_name, make_system_name(lldpdu.system_name) });
                data.insert(data.end(), system_name_tlv.begin(), system_name_tlv.end());
            }

            // System Description TLV(s)
            if (!lldpdu.system_description.empty())
            {
                const auto system_description_tlv = make_tlv({ tlv_types::system_description, make_system_description(lldpdu.system_description) });
                data.insert(data.end(), system_description_tlv.begin(), system_description_tlv.end());
            }

            // System Capabilities TLV(s)
            if (lldpdu.system_capabilities != system_capabilities{})
            {
                const auto system_capabilities_tlv = make_tlv({ tlv_types::system_capabilities, make_system_capabilities(lldpdu.system_capabilities) });
                data.insert(data.end(), system_capabilities_tlv.begin(), system_capabilities_tlv.end());
            }

            // Management Address TLV(s)
            for (const auto& management_address : lldpdu.management_addresses)
            {
                const auto management_address_tlv = make_tlv({ tlv_types::management_address, make_management_address(management_address) });
                data.insert(data.end(), management_address_tlv.begin(), management_address_tlv.end());
            }

            // End of LLDPDU TLV
            const auto end_of_lldpdu_tlv = make_tlv({ tlv_types::end_of_LLDPDU });
            data.insert(data.end(), end_of_lldpdu_tlv.begin(), end_of_lldpdu_tlv.end());

            return data;
        }

        // make a LLDP data unit with the given byte array
        // may throw
        lldp_data_unit parse_lldp_data_uint(const uint8_t* data, size_t len)
        {
            lldp_data_unit lldpdu;

            // "The LLDPDU shall contain the following ordered sequence of three mandatory TLVs followed by zero or more optional TLVs
            // Three mandatory TLVs shall be included at the beginning of each LLDPDU and shall be in the order shown.
            //    1) Chassis ID TLV
            //    2) Port ID TLV
            //    3) Time To Live TLV"
            // See IEEE Std 802.1AB-2016 8.2 LLDPDU format
            std::vector<tlv_type> ordered_mandatory_TLVs{ tlv_types::chassis_id, tlv_types::port_id, tlv_types::time_to_live };

            // parse the mandatory TLVs
            for (auto it = ordered_mandatory_TLVs.begin(); it != ordered_mandatory_TLVs.end(); ++it)
            {
                tlv tlv;
                auto consumed = parse_tlv(data, len, tlv);
                if (*it != tlv.type)
                {
                    throw lldp_parse_error("the three mandatory TLVs are not in the expected order");
                }

                switch (tlv.type)
                {
                case tlv_types::chassis_id:
                    lldpdu.chassis_id = parse_chassis_id(tlv.value);
                    break;
                case tlv_types::port_id:
                    lldpdu.port_id = parse_port_id(tlv.value);
                    break;
                case tlv_types::time_to_live:
                    lldpdu.time_to_live = parse_time_to_live(tlv.value);
                    break;
                }
                data += consumed;
                len -= consumed;
            }

            // Optional TLVs may be inserted in any order
            int system_name_count{ 0 };
            int system_description_count{ 0 };
            int system_capabilities_count{ 0 };

            // parse the optional TLVs
            while (len > 0)
            {
                tlv tlv;
                auto consumed = parse_tlv(data, len, tlv);

                switch (tlv.type)
                {
                case tlv_types::end_of_LLDPDU:
                    return lldpdu;
                case tlv_types::chassis_id:
                    // "An LLDPDU shall contain exactly one Chassis ID TLV."
                    // See IEEE Std 802.1AB-2016 8.5.2.4 Chassis ID TLV usage rules
                    throw lldp_parse_error("found more than 1 Chassis ID TLV");
                case tlv_types::port_id:
                    // "An LLDPDU shall contain exactly one Port ID TLV."
                    // See IEEE Std 802.1AB-2016 8.5.3.4 Port ID TLV usage rules
                    throw lldp_parse_error("found more than 1 Port ID TLV");
                case tlv_types::time_to_live:
                    // "An LLDPDU shall contain exactly one Time To Live TLV."
                    // See IEEE Std 802.1AB-2016 8.5.4.2 Time To Live TLV usage rules
                    throw lldp_parse_error("found more than 1 Time To Live TLV");
                case tlv_types::port_description:
                    // "An LLDPDU should not contain more than one Port Description TLV."
                    // See IEEE Std 802.1AB-2016 8.5.5.3 Port Description TLV usage rules
                    lldpdu.port_descriptions.push_back(parse_port_description(tlv.value));
                    break;
                case tlv_types::system_name:
                    // "An LLDPDU shall not contain more than one System Name TLV."
                    // See IEEE Std 802.1AB-2016 8.5.6.3 System Name TLV usage rules
                    if (system_name_count++)
                    {
                        throw lldp_parse_error("found more than 1 System Name TLV");
                    }
                    lldpdu.system_name = parse_system_name(tlv.value);
                    break;
                case tlv_types::system_description:
                    // "An LLDPDU shall not contain more than one System Description TLV."
                    // See IEEE Std 802.1AB-2016 8.5.7.3 System Description TLV usage rules
                    if (system_description_count++)
                    {
                        throw lldp_parse_error("found more than 1 System Description TLV");
                    }
                    lldpdu.system_description = parse_system_description(tlv.value);
                    break;
                case tlv_types::system_capabilities:
                    // "An LLDPDU shall not contain more than one System Capabilities TLV."
                    // See IEEE Std 802.1AB-2016 8.5.8.3 System Capabilities TLV usage rules
                    if (system_capabilities_count++)
                    {
                        throw lldp_parse_error("found more than 1 System Capabilities TLV");
                    }
                    lldpdu.system_capabilities = parse_system_capabilities(tlv.value);
                    break;
                case tlv_types::management_address:
                    // "a) At least one Management Address TLV should be included in every LLDPDU.
                    // b) Since there are typically a number of different addresses associated with a MSAP identifier, an
                    //    individual LLDPDU may contain more than one Management Address TLV.
                    // c) When Management Address TLV(s) are included in an LLDPDU, the included address(es) should
                    //    be the address(es) offering the best management capability.
                    // d) If more than one Management Address TLV is included in an LLDPDU, each management address
                    //    shall be different from the management address in any other management address TLV in the LLDPDU.
                    // e) If an OID is included in the TLV, it shall be reachable by the management address.
                    // f) In a properly formed Management Address TLV, the TLV information string length is equal to :
                    //    (management address string length) + (OID string length) + 7.
                    //    If the TLV information string length in a received Management Address TLV is incorrect, then it is
                    //    ignored and processing of that LLDPDU is terminated."
                    // See IEEE Std 802.1AB-2016 8.5.9.9 Management Address TLV usage rules
                    lldpdu.management_addresses.push_back(parse_management_address(tlv.value));
                    break;
                default:
                    // ignore
                    break;
                }
                data += consumed;
                len -= consumed;
            }
            return lldpdu;
        }

        // encode an LLDP frame
        // may throw
        std::vector<uint8_t> make_lldp_frame(const lldp_frame& lldp_frame)
        {
            // hm, less copying here and elsewhere would be better, but at least LLDP frames are sent and received only infrequently...
            std::vector<uint8_t> data;

            // Ethernet header
            const auto ether_header = make_ether_header(lldp_frame.destination_mac_address, lldp_frame.source_mac_address);
            data.insert(data.end(), ether_header.begin(), ether_header.end());

            // LLDPDU (sequence of TLVs)
            const auto lldpdu = make_lldp_data_uint(lldp_frame.lldpdu);
            data.insert(data.end(), lldpdu.begin(), lldpdu.end());

            return data;
        }

        // make a LLDP frame with the given byte array
        // may throw
        lldp_frame parse_lldp_frame(const uint8_t* data, size_t len)
        {
            lldp_frame frame;

            // Ethernet header
            auto consumed = parse_ether_header(data, len, frame.destination_mac_address, frame.source_mac_address);

            // LLDPDU (sequence of TLVs)
            frame.lldpdu = parse_lldp_data_uint(data + consumed, len - consumed);

            return frame;
        }
    }
}

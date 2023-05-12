#ifndef NMOS_SDP_ATTRIBUTES_H
#define NMOS_SDP_ATTRIBUTES_H

#include "sdp/json.h"

namespace nmos
{
    namespace sdp_attributes
    {
        // RTP Header Extensions
        // See https://tools.ietf.org/html/rfc5285#section-5
        struct extmap
        {
            uint64_t local_id;
            sdp::direction direction;
            utility::string_t uri;
            utility::string_t ext_attributes;

            extmap() : local_id() {}
            extmap(uint64_t local_id, const utility::string_t& uri) : local_id(local_id), uri(uri) {}
            extmap(uint64_t local_id, const sdp::direction& direction, const utility::string_t& uri) : local_id(local_id), direction(direction), uri(uri) {}
            extmap(uint64_t local_id, const utility::string_t& uri, const utility::string_t& ext_attributes) : local_id(local_id), uri(uri), ext_attributes(ext_attributes) {}
            extmap(uint64_t local_id, const sdp::direction& direction, const utility::string_t& uri, const utility::string_t& ext_attributes) : local_id(local_id), direction(direction), uri(uri), ext_attributes(ext_attributes) {}
        };

        web::json::value make_extmap(const extmap& extmap);
        extmap parse_extmap(const web::json::value& extmap);

        // HDCP Key Exchange Protocol (HKEP) Signalling
        // See VSF TR-10-5:2022 Internet Protocol Media Experience (IPMX): HDCP Key Exchange Protocol, Section 10
        // at https://videoservicesforum.com/download/technical_recommendations/VSF_TR-10-5_2022-03-22.pdf
        struct hkep
        {
            uint64_t port;
            utility::string_t unicast_address;
            utility::string_t node_id;
            utility::string_t port_id;

            hkep() : port() {}
            hkep(uint64_t port, const utility::string_t& unicast_address, const utility::string_t& node_id, const utility::string_t& port_id) : port(port), unicast_address(unicast_address), node_id(node_id), port_id(port_id) {}
        };

        web::json::value make_hkep(const hkep& hkep);
        hkep parse_hkep(const web::json::value& hkep);
    }
}

#endif

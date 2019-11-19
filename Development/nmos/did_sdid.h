#ifndef NMOS_DID_SDID_H
#define NMOS_DID_SDID_H

#include <tuple>
#include "cpprest/details/basic_types.h"

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace nmos
{
    // SMPTE ST 291 Data Identification Words
    struct did_sdid
    {
        // Data Identifier
        uint8_t did;
        // Secondary Data Identifier
        uint8_t sdid;

        did_sdid(uint8_t did = 0, uint8_t sdid = 0) : did(did), sdid(sdid) {}

        auto tied() const -> decltype(std::tie(did, sdid)) { return std::tie(did, sdid); }
        friend bool operator==(const did_sdid& lhs, const did_sdid& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator< (const did_sdid& lhs, const did_sdid& rhs) { return lhs.tied() <  rhs.tied(); }
        friend bool operator> (const did_sdid& lhs, const did_sdid& rhs) { return rhs < lhs; }
        friend bool operator!=(const did_sdid& lhs, const did_sdid& rhs) { return !(lhs == rhs); }
        friend bool operator<=(const did_sdid& lhs, const did_sdid& rhs) { return !(rhs < lhs); }
        friend bool operator>=(const did_sdid& lhs, const did_sdid& rhs) { return !(lhs < rhs); }
    };

    // SMPTE ST 291 Data Identification Word Assignments for Registered DIDs
    // See https://smpte-ra.org/smpte-ancillary-data-smpte-st-291
    namespace did_sdids
    {
        // extensible enum
    }

    // Data identification and Secondary data identification words
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_sdianc_data.json
    utility::string_t make_did_or_sdid(const uint8_t& did_or_sdid);
    uint8_t parse_did_or_sdid(const utility::string_t& did_or_sdid);

    // Data identification and Secondary data identification words
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_sdianc_data.json
    web::json::value make_did_sdid(const nmos::did_sdid& did_sdid);
    nmos::did_sdid parse_did_sdid(const web::json::value& did_sdid);

    // DID_SDID (Data Identification and Secondary Data Identification)
    // Grammar defined by ABNF as follows:
    // TwoHex = "0x" 1*2(HEXDIG)
    // DidSdid = "DID_SDID={" TwoHex "," TwoHex "}"
    // See https://tools.ietf.org/html/rfc8331
    utility::string_t make_fmtp_did_sdid(const nmos::did_sdid& did_sdid);
    nmos::did_sdid parse_fmtp_did_sdid(const utility::string_t& did_sdid);
}

#endif

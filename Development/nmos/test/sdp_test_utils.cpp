#include "nmos/test/sdp_test_utils.h"

#include "bst/test/test.h"
#include "nmos/sdp_utils.h"

namespace nmos
{
    typedef std::multimap<utility::string_t, utility::string_t> comparable_fmtp_t;

    inline comparable_fmtp_t comparable_fmtp(const nmos::sdp_parameters::fmtp_t& fmtp)
    {
        return comparable_fmtp_t{ fmtp.begin(), fmtp.end() };
    }

    void check_sdp_parameters(const nmos::sdp_parameters& lhs, const nmos::sdp_parameters& rhs)
    {
        BST_REQUIRE_EQUAL(lhs.session_name, rhs.session_name);
        BST_REQUIRE_EQUAL(lhs.rtpmap.payload_type, rhs.rtpmap.payload_type);
        BST_REQUIRE_EQUAL(lhs.rtpmap.encoding_name, rhs.rtpmap.encoding_name);
        BST_REQUIRE_EQUAL(lhs.rtpmap.clock_rate, rhs.rtpmap.clock_rate);
        if (0 != lhs.rtpmap.encoding_parameters)
            BST_REQUIRE_EQUAL(lhs.rtpmap.encoding_parameters, rhs.rtpmap.encoding_parameters);
        else
            BST_REQUIRE((0 == rhs.rtpmap.encoding_parameters || 1 == rhs.rtpmap.encoding_parameters));
        BST_REQUIRE_EQUAL(comparable_fmtp(lhs.fmtp), comparable_fmtp(rhs.fmtp));
        BST_REQUIRE_EQUAL(lhs.packet_time, rhs.packet_time);
        BST_REQUIRE_EQUAL(lhs.max_packet_time, rhs.max_packet_time);
        BST_REQUIRE_EQUAL(lhs.bandwidth.bandwidth_type, rhs.bandwidth.bandwidth_type);
        BST_REQUIRE_EQUAL(lhs.bandwidth.bandwidth, rhs.bandwidth.bandwidth);
    }
}

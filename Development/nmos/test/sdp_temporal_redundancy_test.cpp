#include "nmos/sdp_utils.h"

#include "bst/test/test.h"
#include "nmos/json_fields.h"
#include "sdp/sdp.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testInterpretationOfSdpFilesTemporalRedundancy)
{
    using web::json::value_of;

    // See https://specs.amwa.tv/is-05/releases/v1.1.1/docs/4.1._Behaviour_-_RTP_Transport_Type.html#temporal-redundancy

    const std::string test_sdp = R"(v=0
o=ali 1122334455 1122334466 IN IP4 dup.example.com
s=Delayed Duplication
t=0 0
m=video 30000 RTP/AVP 100
c=IN IP4 233.252.0.1/127
a=source-filter:incl IN IP4 233.252.0.1 198.51.100.1
a=rtpmap:100 MP2T/90000
a=ssrc:1000 cname:ch1a@example.com
a=ssrc:1010 cname:ch1a@example.com
a=ssrc-group:DUP 1000 1010
a=duplication-delay:50
a=mid:Ch1
)";

    const auto leg = value_of({
        { nmos::fields::source_ip, U("198.51.100.1") },
        { nmos::fields::multicast_ip, U("233.252.0.1") },
        { nmos::fields::interface_ip, U("auto") },
        { nmos::fields::destination_port, 30000 },
        { nmos::fields::rtp_enabled, true }
    });
    const auto test_params = value_of({ leg, leg });

    const auto session_description = sdp::parse_session_description(test_sdp);
    BST_REQUIRE(test_params == nmos::get_session_description_transport_params(session_description));

    const auto sdp_params = nmos::get_session_description_sdp_parameters(session_description);
    BST_REQUIRE_EQUAL(2, sdp_params.temporal_redundancy.synchronization_sources.size());
    BST_REQUIRE_EQUAL(1000, sdp_params.temporal_redundancy.synchronization_sources.at(0).id);
    BST_REQUIRE_EQUAL(U("ch1a@example.com"), sdp_params.temporal_redundancy.synchronization_sources.at(1).cname);
    BST_REQUIRE(bool(sdp_params.temporal_redundancy.duplication_delay));
    BST_REQUIRE_EQUAL(50, *sdp_params.temporal_redundancy.duplication_delay);
    BST_REQUIRE_EQUAL(U("Ch1"), sdp_params.temporal_redundancy.media_stream_id);

    auto invalid_sdp = test_sdp;
    const std::string ssrc_group = "a=ssrc-group:DUP 1000 1010";
    invalid_sdp.replace(invalid_sdp.find(ssrc_group), ssrc_group.size(), "a=ssrc-group:DUP");
    BST_REQUIRE_THROW(sdp::parse_session_description(invalid_sdp), std::runtime_error);

    invalid_sdp = test_sdp;
    const std::string ssrc = "a=ssrc:1000 cname:ch1a@example.com";
    invalid_sdp.replace(invalid_sdp.find(ssrc), ssrc.size(), "a=ssrc:1000");
    BST_REQUIRE_THROW(sdp::parse_session_description(invalid_sdp), std::runtime_error);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpTransportParamsTemporalRedundancyRoundtrip)
{
    using web::json::value_of;

    auto test_sdp_params = nmos::sdp_parameters(U("Delayed Duplication"), nmos::sdp_parameters::mux_t{}, 100);
    test_sdp_params.temporal_redundancy = {
        {
            { 1000, U("ch1a@example.com") },
            { 1010, U("ch1a@example.com") }
        },
        50,
        U("Ch1")
    };

    const auto sender_leg = value_of({
        { nmos::fields::source_ip, U("198.51.100.1") },
        { nmos::fields::destination_ip, U("233.252.0.1") },
        { nmos::fields::source_port, 30000 },
        { nmos::fields::destination_port, 30000 },
        { nmos::fields::rtp_enabled, true }
    });
    const auto receiver_leg = value_of({
        { nmos::fields::source_ip, U("198.51.100.1") },
        { nmos::fields::multicast_ip, U("233.252.0.1") },
        { nmos::fields::interface_ip, U("auto") },
        { nmos::fields::destination_port, 30000 },
        { nmos::fields::rtp_enabled, true }
    });

    const auto session_description = nmos::make_session_description(test_sdp_params, value_of({ sender_leg, sender_leg }));
    BST_REQUIRE_EQUAL(1, sdp::fields::media_descriptions(session_description).size());
    BST_REQUIRE(value_of({ receiver_leg, receiver_leg }) == nmos::get_session_description_transport_params(session_description));

    const auto text = sdp::make_session_description(session_description);
    BST_REQUIRE(std::string::npos != text.find("a=ssrc-group:DUP 1000 1010"));
    BST_REQUIRE(std::string::npos != text.find("a=duplication-delay:50"));
    BST_REQUIRE(value_of({ receiver_leg, receiver_leg }) == nmos::get_session_description_transport_params(sdp::parse_session_description(text)));

    auto different_leg = sender_leg;
    different_leg[nmos::fields::destination_port] = web::json::value::number(30002);
    BST_REQUIRE_THROW(nmos::make_session_description(test_sdp_params, value_of({ sender_leg, different_leg })), std::logic_error);
}

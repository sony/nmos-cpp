// The first "test" is of course whether the header compiles standalone
#include "nmos/video_jxsv.h"

#include "bst/test/test.h"
#include "nmos/json_fields.h"
#include "sdp/sdp.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpParametersRoundtripVideoJpegXs)
{
    using web::json::value;

    // typical SDP data for JPEG XS, based on BCP-006-01 example file
    // see https://specs.amwa.tv/bcp-006-01/branches/v1.0-dev/examples/jpeg-xs.html
    const std::string test_sdp = R"(v=0
o=- 1443716955 1443716955 IN IP4 192.168.1.2
s=SMPTE ST2110-22 JPEG XS
t=0 0
m=video 30000 RTP/AVP 112
c=IN IP4 224.1.1.1/64
b=AS:116000
a=ts-refclk:localmac=40-a3-6b-a0-2b-d2
a=mediaclk:direct=0
a=source-filter: incl IN IP4 224.1.1.1 192.168.1.2
a=rtpmap:112 jxsv/90000
a=fmtp:112 packetmode=0; profile=High444.12; level=1k-1; sublevel=Sublev3bpp; depth=10; width=1280; height=720; exactframerate=60000/1001; sampling=YCbCr-4:2:2; colorimetry=BT709; TCS=SDR; RANGE=FULL; SSN=ST2110-22:2019; TP=2110TPN
)";

    auto test_description = sdp::parse_session_description(test_sdp);
    const auto test_params = nmos::parse_session_description(test_description);

    auto& s = test_params.first;
    auto& t = test_params.second;

    const auto params = nmos::get_video_jxsv_parameters(s);

    auto s2 = nmos::make_sdp_parameters(s.session_name, params, s.rtpmap.payload_type, s.group.media_stream_ids, s.ts_refclk);
    // nmos::make_sdp_parameters always generates a new origin (o=) line and the connection data (c=) ttl value is hard-coded
    s2.origin = s.origin;
    s2.connection_data.ttl = s.connection_data.ttl;

    auto t2 = t;
    t2[0][nmos::fields::interface_ip] = value::string(U("192.168.1.2"));

    auto session_description = nmos::make_session_description(s2, t2);

    auto test_sdp2 = sdp::make_session_description(session_description);
    std::istringstream expected(test_sdp), actual(test_sdp2);
    do
    {
        std::string expected_line, actual_line;
        std::getline(expected, expected_line);
        std::getline(actual, actual_line);
        // CR cannot appear in a raw string literal, so remove it from the actual line
        if (!actual_line.empty() && '\r' == actual_line.back()) actual_line.pop_back();
        BST_CHECK_EQUAL(expected_line, actual_line);
    } while (!expected.fail() && !actual.fail());
}

// The first "test" is of course whether the header compiles standalone
#include "nmos/video_jxsv.h"

#include "bst/test/test.h"
#include "nmos/json_fields.h"
#include "nmos/test/sdp_test_utils.h"
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

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpParametersVideoJpegXs)
{
    std::pair<nmos::sdp_parameters, nmos::video_jxsv_parameters> example{
        {
            U("example"),
            sdp::media_types::video,
            {
                112,
                U("jxsv"),
                90000
            },
            {
                { U("packetmode"), U("0") },
                { U("profile"), U("High444.12") },
                { U("level"), U("1k-1") },
                { U("sublevel"), U("Sublev3bpp") },
                { U("sampling"), U("YCbCr-4:2:2") },
                { U("width"), U("1280") },
                { U("height"), U("720") },
                { U("exactframerate"), U("60000/1001") },
                { U("depth"), U("10") },
                { U("colorimetry"), U("BT709") },
                { U("TCS"), U("SDR") },
                { U("RANGE"), U("FULL") },
                { U("SSN"), U("ST2110-22:2019") },
                { U("TP"), U("2110TPN") }
            },
            {
                116000
            }
        },
        {
            sdp::video_jxsv::packetization_mode::codestream,
            sdp::video_jxsv::transmission_mode::sequential,
            sdp::video_jxsv::profiles::High444_12,
            sdp::video_jxsv::levels::Level1k_1,
            sdp::video_jxsv::sublevels::Sublev3bpp,
            sdp::samplings::YCbCr_4_2_2,
            10,
            1280,
            720,
            nmos::rates::rate59_94,
            false,
            false,
            sdp::transfer_characteristic_systems::SDR,
            sdp::colorimetries::BT709,
            sdp::ranges::FULL,
            sdp::smpte_standard_numbers::ST2110_22_2019,
            sdp::type_parameters::type_N,
            {},
            {},
            {},
            {},
            {},
            116000
        }
    };

    std::pair<nmos::sdp_parameters, nmos::video_jxsv_parameters> wacky{
        {
            U("example"),
            sdp::media_types::video,
            {
                123,
                U("jxsv"),
                90000
            },
            {
                { U("packetmode"), U("0") },
                { U("profile"), U("High444.12") },
                { U("level"), U("1k-1") },
                { U("sublevel"), U("Sublev3bpp") },
                { U("sampling"), U("UNSPECIFIED") },
                { U("width"), U("9999") },
                { U("height"), U("6666") },
                { U("exactframerate"), U("123") },
                { U("depth"), U("16") },
                { U("colorimetry"), U("BT2100") },
                { U("TCS"), U("UNSPECIFIED") },
                { U("RANGE"), U("FULL") },
                { U("SSN"), U("ST2110-20:2022") },
                { U("TP"), U("2110TPW") },
                { U("TROFF"), U("0") },
                { U("CMAX"), U("42") },
                { U("MAXUDP"), U("57") },
                { U("TSMODE"), U("SAMP") },
                { U("TSDELAY"), U("0") }
            },
            {
                200000
            }
        },
        {
            sdp::video_jxsv::packetization_mode::codestream,
            sdp::video_jxsv::transmission_mode::sequential,
            sdp::video_jxsv::profiles::High444_12,
            sdp::video_jxsv::levels::Level1k_1,
            sdp::video_jxsv::sublevels::Sublev3bpp,
            sdp::samplings::UNSPECIFIED,
            16,
            9999,
            6666,
            { 123, 1 },
            false,
            false,
            sdp::transfer_characteristic_systems::UNSPECIFIED,
            sdp::colorimetries::BT2100,
            sdp::ranges::FULL,
            sdp::smpte_standard_numbers::ST2110_20_2022,
            sdp::type_parameters::type_W,
            0,
            42,
            57,
            sdp::timestamp_modes::SAMP,
            0,
            200000
        }
    };

    for (auto& test : { example, wacky })
    {
        auto made = nmos::make_video_jxsv_sdp_parameters(test.first.session_name, test.second, test.first.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, made);
        auto roundtripped = nmos::make_video_jxsv_sdp_parameters(made.session_name, nmos::get_video_jxsv_parameters(made), made.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, roundtripped);
    }
}

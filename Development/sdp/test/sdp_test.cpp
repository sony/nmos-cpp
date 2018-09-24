// The first "test" is of course whether the header compiles standalone
#include "sdp/sdp.h"

#include "bst/test/test.h"
#include "sdp/json.h"

#include <iostream>

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpRoundtrip)
{
    // a black box style test/example of SDP conversion

    const std::string test_sdp = R"(v=0
o=- 2358175406662690538 2358175406662690538 IN IP4 192.168.9.142
s=Example Sender 1 (Video)
t=0 0
a=group:DUP PRIMARY SECONDARY
m=video 50020/3 RTP/AVP 96
c=IN IP4 239.22.142.1/32
a=ts-refclk:ptp=IEEE1588-2008:traceable
a=source-filter: incl IN IP4 239.22.142.1 192.168.9.142
a=rtpmap:96 raw/90000
a=fmtp:96 colorimetry=BT709; exactframerate=30000/1001; depth=10; TCS=SDR; sampling=YCbCr-4:2:2; width=1920; interlace; TP=2110TPN; PM=2110GPM; height=1080; SSN=ST2110-20:2017; 
a=mediaclk:direct=0
a=mid:PRIMARY
m=video 50120 RTP/AVP 96
c=IN IP4 239.122.142.1/32
a=ts-refclk:ptp=IEEE1588-2008:traceable
a=source-filter: incl IN IP4 239.122.142.1 192.168.109.142
a=rtpmap:96 raw/90000
a=fmtp:96 colorimetry=BT709; exactframerate=30000/1001; depth=10; TCS=SDR; sampling=YCbCr-4:2:2; width=1920; interlace; TP=2110TPN; PM=2110GPM; height=1080; SSN=ST2110-20:2017; 
a=mediaclk:direct=0
a=mid:SECONDARY
)";

    auto session_description = sdp::parse_session_description(test_sdp);
    auto& media_descriptions = sdp::fields::media_descriptions(session_description);
    auto& media_description = media_descriptions.at(0);
    auto& media = sdp::fields::media(media_description);

    BST_REQUIRE_EQUAL(sdp::media_types::video, sdp::media_type{ sdp::fields::media_type(media) });
    BST_REQUIRE_EQUAL(50020, sdp::fields::port(media));
    BST_REQUIRE_EQUAL(sdp::protocols::RTP_AVP, sdp::protocol{ sdp::fields::protocol(media) });
    BST_REQUIRE_EQUAL(U("96"), sdp::fields::formats(media).at(0).as_string());

    auto& attributes = sdp::fields::attributes(media_description).as_array();

    auto source_filter = sdp::find_name(attributes, sdp::attributes::source_filter);
    BST_REQUIRE(attributes.end() != source_filter);
    BST_REQUIRE_EQUAL(U("239.22.142.1"), sdp::fields::destination_address(sdp::fields::value(*source_filter)));

    auto fmtp = sdp::find_name(attributes, sdp::attributes::fmtp);
    BST_REQUIRE(attributes.end() != fmtp);

    auto& params = sdp::fields::format_specific_parameters(sdp::fields::value(*fmtp));

    auto width_param = sdp::find_name(params, U("width"));
    BST_REQUIRE(params.end() != width_param);
    BST_REQUIRE_EQUAL(U("1920"), sdp::fields::value(*width_param).as_string());

    auto test_sdp2 = sdp::make_session_description(session_description);
    std::istringstream expected(test_sdp), actual(test_sdp2);
    do
    {
        std::string expected_line, actual_line;
        std::getline(expected, expected_line);
        // CR cannot appear in a raw string literal, so add it
        if (!expected_line.empty()) expected_line.push_back('\r');
        std::getline(actual, actual_line);
        BST_CHECK_EQUAL(expected_line, actual_line);
    } while (!expected.fail() && !actual.fail());

    auto session_description2 = sdp::parse_session_description(test_sdp2);
    BST_REQUIRE_EQUAL(session_description, session_description2);

    // beginning of example constructing json representation from scratch...

    auto session_description3 = web::json::value_of({
        { sdp::fields::protocol_version, 0 },
        { sdp::fields::origin, web::json::value_of({
            { sdp::fields::user_name, web::json::value::string(U("-")) },
            { sdp::fields::session_id, 2358175406662690538 },
            { sdp::fields::session_version, 2358175406662690538 },
            { sdp::fields::network_type, web::json::value::string(sdp::network_types::IN.name) },
            { sdp::fields::address_type, web::json::value::string(sdp::address_types::IP4.name) },
            { sdp::fields::unicast_address, web::json::value::string(U("192.168.9.142")) }
        }, true) },
        { sdp::fields::session_name, web::json::value::string(U("Example Sender 1 (Video)")) },
        { sdp::fields::time_descriptions, web::json::value_of({
            web::json::value_of({
                { sdp::fields::timing, web::json::value_of({
                    { sdp::fields::start_time, 0 },
                    { sdp::fields::stop_time, 0 }
                }) }
            })
        }) },
        { sdp::fields::attributes, web::json::value_of({
            web::json::value_of({
                { sdp::fields::name, web::json::value::string(sdp::attributes::group) },
                { sdp::fields::value, web::json::value_of({
                    { sdp::fields::semantics, web::json::value::string(U("DUP")) },
                    { sdp::fields::mids, web::json::value_of({
                        web::json::value::string(U("PRIMARY")),
                        web::json::value::string(U("SECONDARY"))
                    }) }
                }, true) }
            })
        }) }
    }, true);

    session_description2.erase(sdp::fields::media_descriptions);

    BST_CHECK_EQUAL(session_description3, session_description2);
}

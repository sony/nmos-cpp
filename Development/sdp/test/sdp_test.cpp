// The first "test" is of course whether the header compiles standalone
#include "sdp/sdp.h"

#include "bst/test/test.h"
#include "sdp/json.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpRoundtrip)
{
    // a black box style test/example of SDP conversion

    const std::string test_sdp = R"(v=0
o=- 3745911798 3745911798 IN IP4 192.168.9.142
s=Example Sender 1 (Video)
t=0 0
a=group:DUP PRIMARY SECONDARY
a=extmap:1 http://example.com/082005/ext.htm#ttime
a=extmap:2/sendrecv http://example.com/082005/ext.htm#xmeta short
a=extmap:3/sendonly http://example.com/082005/ext.htm#xmeta
a=extmap:4 http://example.com/082005/ext.htm#ttime SHORT
a=hkep:9000 IN IP4 192.168.9.142 db31de40-19ad-450a-afb9-f4105be7b564 01-02-03-04-05-06
a=hkep:9001 IN IP4 192.168.9.142 db31de40-19ad-450a-afb9-f4105be7b564 01-02-03-04-05-06
m=video 50020 RTP/AVP 96
c=IN IP4 239.22.142.1/32
a=ts-refclk:ptp=IEEE1588-2008:traceable
a=source-filter: incl IN IP4 239.22.142.1 192.168.9.142
a=rtpmap:96 raw/90000
a=fmtp:96 colorimetry=BT709; exactframerate=30000/1001; depth=10; TCS=SDR; sampling=YCbCr-4:2:2; width=1920; interlace; TP=2110TPN; PM=2110GPM; height=1080; SSN=ST2110-20:2017
a=mediaclk:direct=0
a=mid:PRIMARY
m=video 50120 RTP/AVP 96
c=IN IP4 239.122.142.1/32
a=ts-refclk:ptp=IEEE1588-2008:traceable
a=source-filter: incl IN IP4 239.122.142.1 192.168.109.142
a=rtpmap:96 raw/90000
a=fmtp:96 colorimetry=BT709; exactframerate=30000/1001; depth=10; TCS=SDR; sampling=YCbCr-4:2:2; width=1920; interlace; TP=2110TPN; PM=2110GPM; height=1080; SSN=ST2110-20:2017
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
        std::getline(actual, actual_line);
        // CR cannot appear in a raw string literal, so remove it from the actual line
        if (!actual_line.empty() && '\r' == actual_line.back()) actual_line.pop_back();
        BST_CHECK_EQUAL(expected_line, actual_line);
    } while (!expected.fail() && !actual.fail());

    auto session_description2 = sdp::parse_session_description(test_sdp2);
    BST_REQUIRE_EQUAL(session_description, session_description2);

    // example constructing json representation from scratch...

    auto format_specific_parameters = web::json::value_of({
        sdp::named_value(sdp::fields::colorimetry, sdp::colorimetries::BT709.name),
        sdp::named_value(sdp::fields::exactframerate, U("30000/1001")),
        sdp::named_value(sdp::fields::depth, U("10")),
        sdp::named_value(sdp::fields::transfer_characteristic_system, sdp::transfer_characteristic_systems::SDR.name),
        sdp::named_value(sdp::fields::sampling, sdp::samplings::YCbCr_4_2_2.name),
        sdp::named_value(sdp::fields::width, U("1920")),
        sdp::named_value(sdp::fields::interlace),
        sdp::named_value(sdp::fields::type_parameter, sdp::type_parameters::type_N.name),
        sdp::named_value(sdp::fields::packing_mode, sdp::packing_modes::general.name),
        sdp::named_value(sdp::fields::height, U("1080")),
        sdp::named_value(sdp::fields::smpte_standard_number, sdp::smpte_standard_numbers::ST2110_20_2017.name)
    });

    const bool keep_order = true;

    auto session_description3 = web::json::value_of({
        { sdp::fields::protocol_version, 0 },
        { sdp::fields::origin, web::json::value_of({
            { sdp::fields::user_name, U("-") },
            { sdp::fields::session_id, 3745911798u },
            { sdp::fields::session_version, 3745911798u },
            { sdp::fields::network_type, sdp::network_types::internet.name },
            { sdp::fields::address_type, sdp::address_types::IP4.name },
            { sdp::fields::unicast_address, U("192.168.9.142") }
        }, keep_order) },
        { sdp::fields::session_name, U("Example Sender 1 (Video)") },
        { sdp::fields::time_descriptions, web::json::value_of({
            web::json::value_of({
                { sdp::fields::timing, web::json::value_of({
                    { sdp::fields::start_time, 0 },
                    { sdp::fields::stop_time, 0 }
                }) }
            }, keep_order)
        }) },
        { sdp::fields::attributes, web::json::value_of({
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::group },
                { sdp::fields::value, web::json::value_of({
                    { sdp::fields::semantics, sdp::group_semantics::duplication.name },
                    { sdp::fields::mids, web::json::value_of({
                        U("PRIMARY"),
                        U("SECONDARY")
                    }) }
                }, keep_order) },
            }, keep_order),
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::extmap },
                { sdp::fields::value, web::json::value_of({
                    { sdp::fields::local_id, 1 },
                    { sdp::fields::uri, U("http://example.com/082005/ext.htm#ttime") }
                }, keep_order) },
            }, keep_order),
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::extmap },
                { sdp::fields::value, web::json::value_of({
                    { sdp::fields::local_id, 2 },
                    { sdp::fields::direction, sdp::directions::sendrecv.name },
                    { sdp::fields::uri, U("http://example.com/082005/ext.htm#xmeta") },
                    { sdp::fields::extensionattributes, U("short") }
                }, keep_order) },
            }, keep_order),
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::extmap },
                { sdp::fields::value, web::json::value_of({
                    { sdp::fields::local_id, 3 },
                    { sdp::fields::direction, sdp::directions::sendonly.name },
                    { sdp::fields::uri, U("http://example.com/082005/ext.htm#xmeta") }
                }, keep_order) },
            }, keep_order),
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::extmap },
                { sdp::fields::value, web::json::value_of({
                    { sdp::fields::local_id, 4 },
                    { sdp::fields::uri, U("http://example.com/082005/ext.htm#ttime") },
                    { sdp::fields::extensionattributes, U("SHORT") }
                }, keep_order) },
            }, keep_order),
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::hkep },
                { sdp::fields::value, web::json::value_of({
                    { sdp::fields::port, 9000 },
                    { sdp::fields::network_type, sdp::network_types::internet.name },
                    { sdp::fields::address_type, sdp::address_types::IP4.name },
                    { sdp::fields::unicast_address, U("192.168.9.142") },
                    { sdp::fields::node_id, U("db31de40-19ad-450a-afb9-f4105be7b564") },
                    { sdp::fields::port_id, U("01-02-03-04-05-06") }
                }, keep_order) },
            }, keep_order),
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::hkep },
                { sdp::fields::value, web::json::value_of({
                    { sdp::fields::port, 9001 },
                    { sdp::fields::network_type, sdp::network_types::internet.name },
                    { sdp::fields::address_type, sdp::address_types::IP4.name },
                    { sdp::fields::unicast_address, U("192.168.9.142") },
                    { sdp::fields::node_id, U("db31de40-19ad-450a-afb9-f4105be7b564") },
                    { sdp::fields::port_id, U("01-02-03-04-05-06") }
                }, keep_order) },
            }, keep_order)
        }) },
        { sdp::fields::media_descriptions, web::json::value_of({
            web::json::value_of({
                { sdp::fields::media, web::json::value_of({
                    { sdp::fields::media_type, sdp::media_types::video.name },
                    { sdp::fields::port, 50020 },
                    { sdp::fields::protocol, sdp::protocols::RTP_AVP.name },
                    { sdp::fields::formats, web::json::value_of({
                        { U("96") }
                    }) }
                }, keep_order) },
                { sdp::fields::connection_data, web::json::value_of({
                    web::json::value_of({
                        { sdp::fields::network_type, sdp::network_types::internet.name },
                        { sdp::fields::address_type, sdp::address_types::IP4.name },
                        { sdp::fields::connection_address, U("239.22.142.1/32") }
                    }, keep_order)
                }) },
                { sdp::fields::attributes, web::json::value_of({
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::ts_refclk },
                        { sdp::fields::value, web::json::value_of({
                            { sdp::fields::clock_source, sdp::ts_refclk_sources::ptp.name },
                            { sdp::fields::ptp_version, sdp::ptp_versions::IEEE1588_2008.name },
                            { sdp::fields::traceable, true }
                        }, keep_order) }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::source_filter },
                        { sdp::fields::value, web::json::value_of({
                            { sdp::fields::filter_mode, sdp::filter_modes::incl.name },
                            { sdp::fields::network_type, sdp::network_types::internet.name },
                            { sdp::fields::address_types, sdp::address_types::IP4.name },
                            { sdp::fields::destination_address, U("239.22.142.1") },
                            { sdp::fields::source_addresses, web::json::value_of({
                                { U("192.168.9.142") }
                            }) }
                        }, keep_order) }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::rtpmap },
                        { sdp::fields::value, web::json::value_of({
                            { sdp::fields::payload_type, 96 },
                            { sdp::fields::encoding_name, U("raw") },
                            { sdp::fields::clock_rate, 90000 }
                        }, keep_order) }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::fmtp },
                        { sdp::fields::value, web::json::value_of({
                            { sdp::fields::format, U("96") },
                            { sdp::fields::format_specific_parameters, format_specific_parameters }
                        }, keep_order) }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::mediaclk },
                        { sdp::fields::value, U("direct=0") }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::mid },
                        { sdp::fields::value, U("PRIMARY") }
                    }, keep_order)
                }) }
            }, keep_order),
            web::json::value_of({
                { sdp::fields::media, web::json::value_of({
                    { sdp::fields::media_type, sdp::media_types::video.name },
                    { sdp::fields::port, 50120 },
                    { sdp::fields::protocol, sdp::protocols::RTP_AVP.name },
                    { sdp::fields::formats, web::json::value_of({
                        { U("96") }
                    }) }
                }, keep_order) },
                { sdp::fields::connection_data, web::json::value_of({
                    web::json::value_of({
                        { sdp::fields::network_type, sdp::network_types::internet.name },
                        { sdp::fields::address_type, sdp::address_types::IP4.name },
                        { sdp::fields::connection_address, U("239.122.142.1/32") }
                    }, keep_order)
                }) },
                { sdp::fields::attributes, web::json::value_of({
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::ts_refclk },
                        { sdp::fields::value, web::json::value_of({
                            { sdp::fields::clock_source, sdp::ts_refclk_sources::ptp.name },
                            { sdp::fields::ptp_version, sdp::ptp_versions::IEEE1588_2008.name },
                            { sdp::fields::traceable, true }
                        }, keep_order) }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::source_filter },
                        { sdp::fields::value, web::json::value_of({
                            { sdp::fields::filter_mode, sdp::filter_modes::incl.name },
                            { sdp::fields::network_type, sdp::network_types::internet.name },
                            { sdp::fields::address_types, sdp::address_types::IP4.name },
                            { sdp::fields::destination_address, U("239.122.142.1") },
                            { sdp::fields::source_addresses, web::json::value_of({
                                { U("192.168.109.142") }
                            }) }
                        }, keep_order) }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::rtpmap },
                        { sdp::fields::value, web::json::value_of({
                            { sdp::fields::payload_type, 96 },
                            { sdp::fields::encoding_name, U("raw") },
                            { sdp::fields::clock_rate, 90000 }
                        }, keep_order) }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::fmtp },
                        { sdp::fields::value, web::json::value_of({
                            { sdp::fields::format, U("96") },
                            { sdp::fields::format_specific_parameters, format_specific_parameters }
                        }, keep_order) }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::mediaclk },
                        { sdp::fields::value, U("direct=0") }
                    }, keep_order),
                    web::json::value_of({
                        { sdp::fields::name, sdp::attributes::mid },
                        { sdp::fields::value, U("SECONDARY") }
                    }, keep_order)
                }) }
            }, keep_order)
        }) }
    }, keep_order);

    BST_CHECK_EQUAL(session_description3, session_description2);
    BST_CHECK_EQUAL(session_description3.serialize(), session_description2.serialize());
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpNumbers)
{
    const bool keep_order = true;

    const std::string test_sdp = R"(v=0
o=- 0 0 IN IP4 192.0.2.0
s=Awkward frame rate values
t=0 0
a=framerate:23.98
a=framerate:25
a=framerate:29.97
a=framerate:59.94
)";

    auto session_description = sdp::parse_session_description(test_sdp);

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

    auto session_description2 = web::json::value_of({
        { sdp::fields::protocol_version, 0 },
        { sdp::fields::origin, web::json::value_of({
            { sdp::fields::user_name, U("-") },
            { sdp::fields::session_id, 0 },
            { sdp::fields::session_version, 0 },
            { sdp::fields::network_type, sdp::network_types::internet.name },
            { sdp::fields::address_type, sdp::address_types::IP4.name },
            { sdp::fields::unicast_address, U("192.0.2.0") }
        }, keep_order) },
        { sdp::fields::session_name, U("Awkward frame rate values") },
        { sdp::fields::time_descriptions, web::json::value_of({
            web::json::value_of({
                { sdp::fields::timing, web::json::value_of({
                    { sdp::fields::start_time, 0 },
                    { sdp::fields::stop_time, 0 }
                }, keep_order) }
            })
        }) },
        { sdp::fields::attributes, web::json::value_of({
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::framerate },
                { sdp::fields::value, 23.98 }
            }, keep_order),
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::framerate },
                { sdp::fields::value, 25 }
            }, keep_order),
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::framerate },
                { sdp::fields::value, 29.97 }
            }, keep_order),
            web::json::value_of({
                { sdp::fields::name, sdp::attributes::framerate },
                { sdp::fields::value, 59.94 }
            }, keep_order)
        }) }
    }, keep_order);

    BST_REQUIRE_EQUAL(session_description, session_description2);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpDigits)
{
    const std::string test_sdp = R"(v=000
o=- 007 0987654321098765432 IN IP4 192.0.2.0
s=Leading zeros
b=AS:09876543210
t=0 0
m=video 000000000000000000050020 RTP/AVP 0096
)";

    const std::string expected_sdp = R"(v=0
o=- 7 987654321098765432 IN IP4 192.0.2.0
s=Leading zeros
b=AS:9876543210
t=0 0
m=video 50020 RTP/AVP 0096
)";

    auto session_description = sdp::parse_session_description(test_sdp);

    auto test_sdp2 = sdp::make_session_description(session_description);
    std::istringstream expected(expected_sdp), actual(test_sdp2);
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
BST_TEST_CASE(testSdpParseErrors)
{
    // an empty SDP file results in "sdp parse error - expected a line for protocol_version at line 1"
    // hmm, could do with BST_REQUIRE_THROW_WITH to be able to check the message
    BST_REQUIRE_THROW(sdp::parse_session_description(""), std::runtime_error);

    const std::string not_enough = "v=0\r\no=- 42 42 IN IP4 10.0.0.1\r\ns= \r\n";

    // an incomplete SDP file results in e.g. "sdp parse error - expected a line for timing at line 4"
    BST_REQUIRE_THROW(sdp::parse_session_description(not_enough), std::runtime_error);

    const std::string enough = not_enough + "t=0 0\r\n";

    // a complete (though minimal!) SDP file parses successfully
    BST_REQUIRE_EQUAL(4, sdp::parse_session_description(enough).size());

    // appending just a single 'a' results in an "sdp parse error - expected '=' at line 5"
    BST_REQUIRE_THROW(sdp::parse_session_description(enough + "a"), std::runtime_error);
    // appending the '=' as well results in an "sdp parse error - expected an attribute name at line 5"
    BST_REQUIRE_THROW(sdp::parse_session_description(enough + "a="), std::runtime_error);
    // appending a valid "a=<flag>" form line (even without a final CRLF) parses successfully
    BST_REQUIRE_EQUAL(5, sdp::parse_session_description(enough + "a=foo").size());
    // appending an invalid "a=<attribute>:<value>" form line results in an "sdp parse error - expected an attribute value after ':' at line 5"
    BST_REQUIRE_THROW(sdp::parse_session_description(enough + "a=foo:"), std::runtime_error);
    // appending a valid "a=<attribute>:<value>" line also parses successfully
    BST_REQUIRE_EQUAL(5, sdp::parse_session_description(enough + "a=foo:bar").size());

    // appending an invalid type character results in "sdp parse error - unexpected characters before end-of-file at line 5"
    BST_REQUIRE_THROW(sdp::parse_session_description(enough + "x"), std::runtime_error);
    // ... even if the whole line has valid syntax
    BST_REQUIRE_THROW(sdp::parse_session_description(enough + "x=foo"), std::runtime_error);

    // appending an empty line also results in "sdp parse error - unexpected characters before end-of-file at line 5"
    BST_REQUIRE_THROW(sdp::parse_session_description(enough + "\r\n"), std::runtime_error);
    // ... even if there's a complete valid line after it
    BST_REQUIRE_THROW(sdp::parse_session_description(enough + "\r\na=foo"), std::runtime_error);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpNumberErrors)
{
    const std::string before = "v=0\r\no=- 42 42 IN IP4 10.0.0.1\r\ns= \r\n";
    BST_REQUIRE_NO_THROW(sdp::parse_session_description(before + "t=0 0"));
    BST_REQUIRE_NO_THROW(sdp::parse_session_description(before + "t=0 0\r\na=ptime:0.125"));
    BST_REQUIRE_NO_THROW(sdp::parse_session_description(before + "t=0 0\r\na=ptime:1"));
    // an invalid time results in "sdp parse error - expected a number at line 4"
    BST_REQUIRE_THROW(sdp::parse_session_description(before + "t=foo 0"), std::runtime_error);
    // an invalid packet time results in "sdp parse error - expected a number at line 5"
    BST_REQUIRE_THROW(sdp::parse_session_description(before + "t=0 0\r\na=ptime:foo"), std::runtime_error);
    //BST_REQUIRE_THROW(sdp::parse_session_description(before + "t=0 0\r\na=ptime:+1.25e-1"), std::runtime_error);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpDigitsErrors)
{
    const std::string after = "\r\no=- 42 42 IN IP4 10.0.0.1\r\ns= \r\nt=0 0\r\n";
    BST_REQUIRE_NO_THROW(sdp::parse_session_description("v=0" + after));
    BST_REQUIRE_NO_THROW(sdp::parse_session_description("v=0000000000" + after));
    // an invalid protocol version results in "sdp parse error - expected a sequence of digits at line 1"
    BST_REQUIRE_THROW(sdp::parse_session_description("v=" + after), std::runtime_error);
    BST_REQUIRE_THROW(sdp::parse_session_description("v=foo" + after), std::runtime_error);
    BST_REQUIRE_THROW(sdp::parse_session_description("v=0foo" + after), std::runtime_error);
    BST_REQUIRE_THROW(sdp::parse_session_description("v=0.0" + after), std::runtime_error);
    BST_REQUIRE_THROW(sdp::parse_session_description("v=0x0" + after), std::runtime_error);
    //BST_REQUIRE_THROW(sdp::parse_session_description("v=+0" + after), std::runtime_error);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpFmtp)
{
    const std::string test_sdp = R"(v=0
o=- 0 0 IN IP4 192.0.2.1
s=Example Sender 1 (Video)
t=0 0
m=video 5004 RTP/AVP 96
c=IN IP4 233.252.0.1/32
a=fmtp:96)";

    const std::vector<std::pair<size_t, std::string>> fmtp_params = {
        { 0, "" },
        { 0, " " },
        { 1, " foo=meow" },
        { 1, " foo=meow;" },
        { 1, " foo=meow; " },
        { 2, " foo=meow;bar=purr" },
        { 2, " bar=purr; foo=meow;" },
        { 2, "  bar=purr ; foo=meow " },
        { 2, "  bar=purr ; foo=meow ; " }
    };

    for (const auto& fp : fmtp_params)
    {
        auto session_description = sdp::parse_session_description(test_sdp + fp.second);
        auto& media_descriptions = sdp::fields::media_descriptions(session_description);
        auto& media_description = media_descriptions.at(0);
        auto& attributes = sdp::fields::attributes(media_description).as_array();

        auto fmtp = sdp::find_name(attributes, sdp::attributes::fmtp);
        BST_REQUIRE(attributes.end() != fmtp);

        auto& params = sdp::fields::format_specific_parameters(sdp::fields::value(*fmtp));
        BST_REQUIRE_EQUAL(fp.first, params.size());

        if (0 != fp.first)
        {
            auto foo_param = sdp::find_name(params, U("foo"));
            BST_REQUIRE(params.end() != foo_param);
            BST_REQUIRE_EQUAL(U("meow"), sdp::fields::value(*foo_param).as_string());
        }
    }

    const std::vector<std::pair<size_t, std::string>> fail_params = {
        // invalid case... whitespace on its own isn't a separator
        // but nor is it allowed in the <name> or <value>
        { 2, "  bar=purr ; foo=meow baz=hiss" }
    };

    for (const auto& fp : fail_params)
    {
        auto session_description = sdp::parse_session_description(test_sdp + fp.second);
        auto& media_descriptions = sdp::fields::media_descriptions(session_description);
        auto& media_description = media_descriptions.at(0);
        auto& attributes = sdp::fields::attributes(media_description).as_array();

        auto fmtp = sdp::find_name(attributes, sdp::attributes::fmtp);
        BST_REQUIRE(attributes.end() != fmtp);

        auto& params = sdp::fields::format_specific_parameters(sdp::fields::value(*fmtp));
        BST_REQUIRE_EQUAL(fp.first, params.size());

        if (0 != fp.first)
        {
            auto foo_param = sdp::find_name(params, U("foo"));
            BST_REQUIRE(params.end() != foo_param);
            BST_REQUIRE_NE(U("meow"), sdp::fields::value(*foo_param).as_string());
        }
    }

    const std::vector<std::string> bad_params = {
        " ;",
        " ; foo=meow",
        " foo=",
        " foo=meow;;",
        " bar=; foo=meow",
        " bar=purr; ; foo=meow"
    };

    for (const auto& bp : bad_params)
    {
        BST_REQUIRE_THROW(sdp::parse_session_description(test_sdp + bp), std::runtime_error);
    }
}

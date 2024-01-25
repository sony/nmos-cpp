// The first "test" is of course whether the header compiles standalone
#include "nmos/sdp_utils.h"

#include "bst/test/test.h"
#include "nmos/capabilities.h"
#include "nmos/components.h"
#include "nmos/format.h"
#include "nmos/interlace_mode.h"
#include "nmos/json_fields.h"
#include "nmos/media_type.h"
#include "nmos/random.h"
#include "nmos/test/sdp_test_utils.h"
#include "sdp/sdp.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeComponentsMakeSampling)
{
    using web::json::value_of;

    // use the older function to test the newer function
    BST_REQUIRE(nmos::make_components(nmos::YCbCr422, 1920, 1080, 8) == nmos::make_components(sdp::samplings::YCbCr_4_2_2, 1920, 1080, 8));
    BST_REQUIRE(nmos::make_components(nmos::RGB444, 3840, 2160, 12) == nmos::make_components(sdp::samplings::RGB, 3840, 2160, 12));

    const std::vector<sdp::sampling> samplings{
        // Red-Green-Blue-Alpha
        sdp::samplings::RGBA,
        // Red-Green-Blue
        sdp::samplings::RGB,
        // Non-constant luminance YCbCr
        sdp::samplings::YCbCr_4_4_4,
        sdp::samplings::YCbCr_4_2_2,
        sdp::samplings::YCbCr_4_2_0,
        sdp::samplings::YCbCr_4_1_1,
        // Constant luminance YCbCr
        sdp::samplings::CLYCbCr_4_4_4,
        sdp::samplings::CLYCbCr_4_2_2,
        sdp::samplings::CLYCbCr_4_2_0,
        // Constant intensity ICtCp
        sdp::samplings::ICtCp_4_4_4,
        sdp::samplings::ICtCp_4_2_2,
        sdp::samplings::ICtCp_4_2_0,
        // XYZ
        sdp::samplings::XYZ,
        // Key signal represented as a single component
        sdp::samplings::KEY,
        // Sampling signaled by the payload
        sdp::samplings::UNSPECIFIED
    };

    const std::vector<std::pair<uint32_t, uint32_t>> dims{
        { 3840, 2160 },
        { 1920, 1080 },
        { 1280,  720 }
    };

    nmos::details::seed_generator seeder;
    std::default_random_engine gen(seeder);

    for (const auto& sampling : samplings)
    {
        for (const auto& dim : dims)
        {
            auto components = nmos::make_components(sampling, dim.first, dim.second, 10);
            BST_REQUIRE(sampling == nmos::details::make_sampling(components.as_array()));
            std::shuffle(components.as_array().begin(), components.as_array().end(), gen);
            BST_REQUIRE(sampling == nmos::details::make_sampling(components.as_array()));
        }
    }

    const auto test_no_YCbCr_3_1_1 = value_of({
        nmos::make_component(nmos::component_names::Y, 1440, 1080, 8),
        nmos::make_component(nmos::component_names::Cb, 480, 1080, 8),
        nmos::make_component(nmos::component_names::Cr, 480, 1080, 8)
    });
    BST_CHECK_THROW(nmos::details::make_sampling(test_no_YCbCr_3_1_1.as_array()), std::logic_error);

    const auto test_no_integer_divisor = value_of({
        nmos::make_component(nmos::component_names::Y, 100, 100, 8),
        nmos::make_component(nmos::component_names::Cb, 40, 40, 8),
        nmos::make_component(nmos::component_names::Cr, 40, 40, 8)
    });
    BST_REQUIRE_THROW(nmos::details::make_sampling(test_no_integer_divisor.as_array()), std::logic_error);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testValidateSdpParameters)
{
    using web::json::value;
    using web::json::value_of;

    {
        // omitting TCS should be treated as "SDR"
        const sdp::transfer_characteristic_system omit_tcs;

        // an unimplemented parameter constraint should be ignored
        const utility::string_t unimplemented_parameter_constraint{ U("urn:x-nmos:cap:unimplemented") };

        nmos::video_raw_parameters params{
            1920, 1080, nmos::rates::rate29_97, true, false, sdp::samplings::YCbCr_4_2_2, 10,
            omit_tcs, sdp::colorimetries::BT2020, sdp::type_parameters::type_N
        };
        auto sdp_params = nmos::make_video_raw_sdp_parameters(U("-"), params, nmos::details::payload_type_video_default);

        // only format and caps are used to validate SDP parameters
        auto receiver = value_of({
            { nmos::fields::format, nmos::formats::video.name },
            { nmos::fields::caps, value_of({
                { nmos::fields::media_types, value_of({ nmos::media_types::video_raw.name }) },
                { nmos::fields::constraint_sets, value_of({
                    value_of({
                        { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::video_raw.name }) },
                        { nmos::caps::format::grain_rate, nmos::make_caps_rational_constraint({ nmos::rates::rate25, nmos::rates::rate29_97 }) },
                        { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({ 1920 }) },
                        { nmos::caps::format::frame_height, nmos::make_caps_integer_constraint({ 1080 }) },
                        { nmos::caps::format::color_sampling, nmos::make_caps_string_constraint({ sdp::samplings::YCbCr_4_2_2.name }) },
                        { nmos::caps::format::interlace_mode, nmos::make_caps_string_constraint({ nmos::interlace_modes::interlaced_bff.name, nmos::interlace_modes::interlaced_tff.name, nmos::interlace_modes::interlaced_psf.name }) },
                        { nmos::caps::format::colorspace, nmos::make_caps_string_constraint({ sdp::colorimetries::BT2020.name, sdp::colorimetries::BT709.name }) },
                        { nmos::caps::format::transfer_characteristic, nmos::make_caps_string_constraint({ sdp::transfer_characteristic_systems::SDR.name }) },
                        { nmos::caps::format::component_depth, nmos::make_caps_integer_constraint({}, 8, 12) },
                        { nmos::caps::transport::st2110_21_sender_type, nmos::make_caps_string_constraint({ sdp::type_parameters::type_N.name }) },
                        { unimplemented_parameter_constraint, nmos::make_caps_string_constraint({ U("ignored") }) }
                    })
                }) }
            }) }
        });

        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));

        receiver[nmos::fields::caps][nmos::fields::media_types] = value_of({ U("foo/meow"), U("foo/purr") });
        BST_REQUIRE_THROW(nmos::validate_sdp_parameters(receiver, sdp_params), std::runtime_error);

        receiver[nmos::fields::caps].erase(nmos::fields::media_types);
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));

        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::format::media_type] = nmos::make_caps_string_constraint({ U("foo/meow") });
        BST_REQUIRE_THROW(nmos::validate_sdp_parameters(receiver, sdp_params), std::runtime_error);

        // empty parameter constraint is always satisfied
        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::format::media_type] = value::object();
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));

        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::format::grain_rate] = nmos::make_caps_rational_constraint({ nmos::rates::rate50 });
        BST_REQUIRE_THROW(nmos::validate_sdp_parameters(receiver, sdp_params), std::runtime_error);

        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0].erase(nmos::caps::format::grain_rate);
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));

        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::format::component_depth] = nmos::make_caps_integer_constraint({}, 10);
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));
        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::format::component_depth] = nmos::make_caps_integer_constraint({}, nmos::no_minimum<int64_t>(), 10);
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));
        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::format::component_depth] = nmos::make_caps_integer_constraint({}, 10, 10);
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));

        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::format::component_depth] = nmos::make_caps_integer_constraint({}, 11);
        BST_REQUIRE_THROW(nmos::validate_sdp_parameters(receiver, sdp_params), std::runtime_error);
        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::format::component_depth] = nmos::make_caps_integer_constraint({}, nmos::no_minimum<int64_t>(), 9);
        BST_REQUIRE_THROW(nmos::validate_sdp_parameters(receiver, sdp_params), std::runtime_error);
        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::format::component_depth] = nmos::make_caps_integer_constraint({ 9 }, 8, 12);
        BST_REQUIRE_THROW(nmos::validate_sdp_parameters(receiver, sdp_params), std::runtime_error);

        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0].erase(nmos::caps::format::component_depth);
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));

        // empty enabled constraint set is always satisfied
        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0] = value::object();
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));

        // empty disabled constraint set is not considered and when no constraint set is satisfied, the constraint sets altogether are not satisfied
        receiver[nmos::fields::caps][nmos::fields::constraint_sets][0][nmos::caps::meta::enabled] = value::boolean(false);
        BST_REQUIRE_THROW(nmos::validate_sdp_parameters(receiver, sdp_params), std::runtime_error);

        // when there are no (enabled) constraint sets, the constraint sets altogether are not satisfied
        receiver[nmos::fields::caps][nmos::fields::constraint_sets] = value::array();
        BST_REQUIRE_THROW(nmos::validate_sdp_parameters(receiver, sdp_params), std::runtime_error);

        // when constraint sets aren't in use, that's valid!
        receiver[nmos::fields::caps].erase(nmos::fields::constraint_sets);
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));
    }

    {
        nmos::audio_L_parameters params{ 4, 16, 48000, {}, 1 };
        auto sdp_params = nmos::make_audio_L_sdp_parameters(U("-"), params, nmos::details::payload_type_audio_default);

        // only format and caps are used to validate SDP parameters
        auto receiver = value_of({
            { nmos::fields::format, nmos::formats::audio.name },
            { nmos::fields::caps, value_of({
                { nmos::fields::media_types, value_of({ nmos::media_types::audio_L(16).name, nmos::media_types::audio_L(24).name }) },
                { nmos::fields::constraint_sets, value_of({
                    value_of({
                        { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::audio_L(16).name }) },
                        { nmos::caps::format::channel_count, nmos::make_caps_integer_constraint({}, 1, 8) },
                        { nmos::caps::format::sample_rate, nmos::make_caps_rational_constraint({ 48000 }) },
                        { nmos::caps::format::sample_depth, nmos::make_caps_integer_constraint({ 16 }) },
                        { nmos::caps::transport::packet_time, nmos::make_caps_number_constraint({ 0.125, 1 }) },
                        { nmos::caps::transport::max_packet_time, nmos::make_caps_number_constraint({ 0.125, 1 }) }
                    }),
                    value_of({
                        { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::audio_L(24).name }) }
                    })
                }) }
            }) }
        });

        // because the SDP parameters don't include 'maxptime', the 'max_packet_time' parameter constraint will be ignored

        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));

        params.channel_count = 16;
        sdp_params = nmos::make_audio_L_sdp_parameters(U("-"), params, nmos::details::payload_type_audio_default);
        BST_REQUIRE_THROW(nmos::validate_sdp_parameters(receiver, sdp_params), std::runtime_error);

        params.bit_depth = 24;
        sdp_params = nmos::make_audio_L_sdp_parameters(U("-"), params, nmos::details::payload_type_audio_default);
        BST_REQUIRE_NO_THROW(nmos::validate_sdp_parameters(receiver, sdp_params));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpParametersRoundtrip)
{
    using web::json::value;

    const std::string test_sdp = R"(v=0
o=- 1643910985 1643910985 IN IP4 192.0.2.0
s=SDP Example
t=0 0
m=video 5000 RTP/AVP 96
c=IN IP4 233.252.0.0/32
a=source-filter: incl IN IP4 233.252.0.0 192.0.2.0
a=rtpmap:96 raw/90000
)";

    auto test_description = sdp::parse_session_description(test_sdp);
    auto params = nmos::parse_session_description(test_description);
    params.second[0][nmos::fields::interface_ip] = value::string(U("192.0.2.0"));
    auto session_description = nmos::make_session_description(params.first, params.second);

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
BST_TEST_CASE(testInterpretationOfSdpFilesUnicast)
{
    using web::json::value;
    using web::json::value_of;

    // See https://specs.amwa.tv/is-05/releases/v1.1.1/docs/4.1._Behaviour_-_RTP_Transport_Type.html#unicast

    const std::string test_sdp = R"(v=0
o=- 2890844526 2890842807 IN IP4 10.47.16.5
s=SDP Example
c=IN IP4 10.46.16.34/127
t=2873397496 2873404696
a=recvonly
m=video 51372 RTP/AVP 99
a=rtpmap:99 h263-1998/90000
)";

    const auto test_params = value_of({
        value_of({
            { nmos::fields::source_ip, value::null() },
            { nmos::fields::multicast_ip, value::null() },
            { nmos::fields::interface_ip, U("10.46.16.34") },
            { nmos::fields::destination_port, 51372 },
            { nmos::fields::rtp_enabled, true },
        })
    });

    auto session_description = sdp::parse_session_description(test_sdp);
    auto transport_params = nmos::get_session_description_transport_params(session_description);

    BST_REQUIRE(test_params == transport_params);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testInterpretationOfSdpFilesSourceSpecificMulticast)
{
    using web::json::value;
    using web::json::value_of;

    // See https://specs.amwa.tv/is-05/releases/v1.1.1/docs/4.1._Behaviour_-_RTP_Transport_Type.html#source-specific-multicast

    const std::string test_sdp = R"(v=0
o=- 1497010742 1497010742 IN IP4 172.29.26.24
s=SDP Example
t=2873397496 2873404696
m=video 5000 RTP/AVP 103
c=IN IP4 232.21.21.133/32
a=source-filter:incl IN IP4 232.21.21.133 172.29.226.24
a=rtpmap:103 raw/90000
)";

    const auto test_params = value_of({
        value_of({
            { nmos::fields::source_ip, U("172.29.226.24") },
            { nmos::fields::multicast_ip, U("232.21.21.133") },
            { nmos::fields::interface_ip, U("auto") },
            { nmos::fields::destination_port, 5000 },
            { nmos::fields::rtp_enabled, true },
        })
    });

    auto session_description = sdp::parse_session_description(test_sdp);
    auto transport_params = nmos::get_session_description_transport_params(session_description);

    BST_REQUIRE(test_params == transport_params);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testInterpretationOfSdpFilesSeparateSourceAddresses)
{
    using web::json::value;
    using web::json::value_of;

    // See https://specs.amwa.tv/is-05/releases/v1.1.1/docs/4.1._Behaviour_-_RTP_Transport_Type.html#separate-source-addresses

    const std::string test_sdp = R"(v=0
o=ali 1122334455 1122334466 IN IP4 dup.example.com
s=DUP Grouping Semantics
t=0 0
m=video 30000 RTP/AVP 100
c=IN IP4 233.252.0.1/127
a=source-filter:incl IN IP4 233.252.0.1 198.51.100.1 198.51.100.2
a=rtpmap:100 MP2T/90000
a=ssrc:1000 cname:ch1@example.com
a=ssrc:1010 cname:ch1@example.com
a=ssrc-group:DUP 1000 1010
a=mid:Ch1
)";

    const auto test_params = value_of({
        value_of({
            { nmos::fields::source_ip, U("198.51.100.1") },
            { nmos::fields::multicast_ip, U("233.252.0.1") },
            { nmos::fields::interface_ip, U("auto") },
            { nmos::fields::destination_port, 30000 },
            { nmos::fields::rtp_enabled, true },
        }),
        value_of({
            { nmos::fields::source_ip, U("198.51.100.2") },
            { nmos::fields::multicast_ip, U("233.252.0.1") },
            { nmos::fields::interface_ip, U("auto") },
            { nmos::fields::destination_port, 30000 },
            { nmos::fields::rtp_enabled, true },
        })
    });

    auto session_description = sdp::parse_session_description(test_sdp);
    auto transport_params = nmos::get_session_description_transport_params(session_description);

    BST_REQUIRE(test_params == transport_params);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testInterpretationOfSdpFilesSeparateDestinationAddresses)
{
    using web::json::value;
    using web::json::value_of;

    // See https://specs.amwa.tv/is-05/releases/v1.1.1/docs/4.1._Behaviour_-_RTP_Transport_Type.html#separate-destination-addresses

    const std::string test_sdp = R"(v=0
o=ali 1122334455 1122334466 IN IP4 dup.example.com
s=DUP Grouping Semantics
t=0 0
a=group:DUP S1a S1b
m=video 30000 RTP/AVP 100
c=IN IP4 233.252.0.1/127
a=source-filter:incl IN IP4 233.252.0.1 198.51.100.1
a=rtpmap:100 MP2T/90000
a=mid:S1a
m=video 30000 RTP/AVP 101
c=IN IP4 233.252.0.2/127
a=source-filter:incl IN IP4 233.252.0.2 198.51.100.1
a=rtpmap:101 MP2T/90000
a=mid:S1b
)";

    const auto test_params = value_of({
        value_of({
            { nmos::fields::source_ip, U("198.51.100.1") },
            { nmos::fields::multicast_ip, U("233.252.0.1") },
            { nmos::fields::interface_ip, U("auto") },
            { nmos::fields::destination_port, 30000 },
            { nmos::fields::rtp_enabled, true },
        }),
        value_of({
            { nmos::fields::source_ip, U("198.51.100.1") },
            { nmos::fields::multicast_ip, U("233.252.0.2") },
            { nmos::fields::interface_ip, U("auto") },
            { nmos::fields::destination_port, 30000 },
            { nmos::fields::rtp_enabled, true },
        })
    });

    auto session_description = sdp::parse_session_description(test_sdp);
    auto transport_params = nmos::get_session_description_transport_params(session_description);

    BST_REQUIRE(test_params == transport_params);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpTransportParamsUnicast)
{
    using web::json::value;
    using web::json::value_of;

    const auto test_params = nmos::sdp_parameters(U("SDP Example"), nmos::sdp_parameters::video_t{}, 99);

    const auto test_sender_params = value_of({
        value_of({
            { nmos::fields::source_ip, U("10.46.116.34") },
            { nmos::fields::destination_ip, U("10.46.16.34") },
            { nmos::fields::source_port, 5004 },
            { nmos::fields::destination_port, 51372 },
            { nmos::fields::rtp_enabled, true }
        })
    });

    for (int i = 0; i < 2; ++i)
    {
        const bool source_filters = i == 0;

        const auto test_receiver_params = value_of({
            value_of({
                { nmos::fields::source_ip, source_filters ? value::string(U("10.46.116.34")) : value::null() },
                { nmos::fields::multicast_ip, value::null() },
                { nmos::fields::interface_ip, U("10.46.16.34") },
                { nmos::fields::destination_port, 51372 },
                { nmos::fields::rtp_enabled, true },
            })
        });

        const auto sender_sdp = nmos::make_session_description(test_params, test_sender_params, source_filters);
        auto receiver_params = nmos::get_session_description_transport_params(sender_sdp);
        BST_REQUIRE(test_receiver_params == receiver_params);

        const auto receiver_sdp = nmos::make_session_description(test_params, receiver_params);
        receiver_params = nmos::get_session_description_transport_params(receiver_sdp);
        BST_REQUIRE(test_receiver_params == receiver_params);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpTransportParamsMulticast)
{
    using web::json::value;
    using web::json::value_of;

    const auto test_params = nmos::sdp_parameters(U("SDP Example"), nmos::sdp_parameters::video_t{}, 103);

    const auto test_sender_params = value_of({
        value_of({
            { nmos::fields::source_ip, U("172.29.226.24") },
            { nmos::fields::destination_ip, U("232.21.21.133") },
            { nmos::fields::source_port, 5004 },
            { nmos::fields::destination_port, 5000 },
            { nmos::fields::rtp_enabled, true }
        })
    });

    for (int i = 0; i < 2; ++i)
    {
        // i.e. source-specific multicast then any-source multicast
        const bool source_filters = i == 0;

        const auto test_receiver_params = value_of({
            value_of({
                { nmos::fields::source_ip, source_filters ? value::string(U("172.29.226.24")) : value::null() },
                { nmos::fields::multicast_ip, U("232.21.21.133") },
                { nmos::fields::interface_ip, U("auto") },
                { nmos::fields::destination_port, 5000 },
                { nmos::fields::rtp_enabled, true },
            })
        });

        const auto sender_sdp = nmos::make_session_description(test_params, test_sender_params, source_filters);
        auto receiver_params = nmos::get_session_description_transport_params(sender_sdp);
        BST_REQUIRE(test_receiver_params == receiver_params);

        // replace "auto"
        receiver_params[0][nmos::fields::interface_ip] = value::string(U("172.29.126.24"));

        const auto receiver_sdp = nmos::make_session_description(test_params, receiver_params);
        receiver_params = nmos::get_session_description_transport_params(receiver_sdp);
        BST_REQUIRE(test_receiver_params == receiver_params);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpParametersVideoRaw)
{
    // a=fmtp:96 colorimetry=BT709; exactframerate=30000/1001; depth=10; TCS=SDR; sampling=YCbCr-4:2:2; width=1920; interlace; TP=2110TPN; PM=2110GPM; height=1080; SSN=ST2110-20:2017
    // cf. testSdpRoundtrip in sdp/test/sdp_test.cpp

    std::pair<nmos::sdp_parameters, nmos::video_raw_parameters> example{
        {
            U("example"),
            sdp::media_types::video,
            {
                96,
                U("raw"),
                90000
            },
            {
                { U("colorimetry"), U("BT709") },
                { U("exactframerate"), U("30000/1001") },
                { U("depth"), U("10") },
                { U("TCS"), U("SDR") },
                { U("sampling"), U("YCbCr-4:2:2") },
                { U("width"), U("1920") },
                { U("interlace"), {} },
                { U("TP"), U("2110TPN") },
                { U("PM"), U("2110GPM") },
                { U("height"), U("1080") },
                { U("SSN"), U("ST2110-20:2017") }
            }
        },
        {
            sdp::samplings::YCbCr_4_2_2,
            10,
            1920,
            1080,
            nmos::rates::rate29_97,
            true,
            false,
            sdp::transfer_characteristic_systems::SDR,
            sdp::colorimetries::BT709,
            {},
            {},
            sdp::packing_modes::general,
            sdp::smpte_standard_numbers::ST2110_20_2017,
            sdp::type_parameters::type_N,
            {},
            {},
            {},
            {},
            {}
        }
    };

    std::pair<nmos::sdp_parameters, nmos::video_raw_parameters> wacky{
        {
            U("wacky"),
            sdp::media_types::video,
            {
                123,
                U("raw"),
                90000
            },
            {
                { U("sampling"), U("UNSPECIFIED") },
                { U("depth"), U("16") },
                { U("width"), U("9999") },
                { U("height"), U("6666") },
                { U("exactframerate"), U("123") },
                { U("interlace"), {} },
                { U("segmented"), {} },
                { U("TCS"), U("ST2115LOGS3") },
                { U("colorimetry"), U("BT2100") },
                { U("RANGE"), U("FULLPROTECT") },
                { U("PAR"), U("12:11") },
                { U("PM"), U("2110BPM") },
                { U("SSN"), U("ST2110-20:2022") },
                { U("TP"), U("2110TPW") },
                { U("TROFF"), U("0") },
                { U("CMAX"), U("42") },
                { U("MAXUDP"), U("57") },
                { U("TSMODE"), U("SAMP") },
                { U("TSDELAY"), U("0") }
            }
        },
        {
            sdp::samplings::UNSPECIFIED,
            16,
            9999,
            6666,
            { 123, 1 },
            true,
            true,
            sdp::transfer_characteristic_systems::ST2115LOGS3,
            sdp::colorimetries::BT2100,
            sdp::ranges::FULLPROTECT,
            { 12, 11 },
            sdp::packing_modes::block,
            sdp::smpte_standard_numbers::ST2110_20_2022,
            sdp::type_parameters::type_W,
            uint32_t(0),
            42,
            57,
            sdp::timestamp_modes::SAMP,
            uint32_t(0)
        }
    };

    for (auto& test : { example, wacky })
    {
        auto made = nmos::make_video_raw_sdp_parameters(test.first.session_name, test.second, test.first.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, made);
        auto roundtripped = nmos::make_video_raw_sdp_parameters(made.session_name, nmos::get_video_raw_parameters(made), made.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, roundtripped);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpParametersAudioL)
{
    std::pair<nmos::sdp_parameters, nmos::audio_L_parameters> example{
        {
            U("example"),
            sdp::media_types::audio,
            {
                97,
                U("L24"),
                48000,
                8
            },
            {
                { U("channel-order"), U("SMPTE2110.(51,ST)") }
            },
            {},
            0.125
        },
        {
            8,
            24,
            48000,
            U("SMPTE2110.(51,ST)"), // not testing nmos::make_fmtp_channel_order here
            {},
            {},
            0.125
        }
    };

    std::pair<nmos::sdp_parameters, nmos::audio_L_parameters> wacky{
        {
            U("wacky"),
            sdp::media_types::audio,
            {
                123,
                U("L16"),
                96000
            },
            {
                { U("channel-order"), U("SMPTE2110.(M,M,M,M,ST,U02)") },
                { U("TSMODE"), U("SAMP") },
                { U("TSDELAY"), U("0") }
            },
            {},
            0.333
        },
        {
            1,
            16,
            96000,
            U("SMPTE2110.(M,M,M,M,ST,U02)"), // not testing nmos::make_fmtp_channel_order here
            sdp::timestamp_modes::SAMP,
            uint32_t(0),
            0.333
        }
    };

    for (auto& test : { example, wacky })
    {
        auto made = nmos::make_audio_L_sdp_parameters(test.first.session_name, test.second, test.first.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, made);
        auto roundtripped = nmos::make_audio_L_sdp_parameters(made.session_name, nmos::get_audio_L_parameters(made), made.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, roundtripped);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpParametersVideoSmpte291)
{
    std::pair<nmos::sdp_parameters, nmos::video_smpte291_parameters> example{
        {
            U("example"),
            sdp::media_types::video,
            {
                100,
                U("smpte291"),
                90000
            },
            {
                { U("DID_SDID"), U("{0x41,0x01}") },
                { U("VPID_Code"), U("133") }
            }
        },
        {
            { { 0x41, 0x01 } },
            nmos::vpid_codes::vpid_1_5Gbps_1080_line,
            {},
            {},
            {},
            {},
            {},
            {}
        }
    };

    std::pair<nmos::sdp_parameters, nmos::video_smpte291_parameters> wacky{
        {
            U("wacky"),
            sdp::media_types::video,
            {
                123,
                U("smpte291"),
                90000
            },
            {
                { U("DID_SDID"), U("{0xAB,0xCD}") },
                { U("DID_SDID"), U("{0xEF,0x01}") },
                { U("VPID_Code"), U("132") },
                { U("exactframerate"), U("60000/1001") },
                { U("TM"), U("CTM") },
                { U("SSN"), U("ST2110-40:2021") },
                { U("TROFF"), U("0") },
                { U("TSMODE"), U("SAMP") },
                { U("TSDELAY"), U("0") }
            }
        },
        {
            { { 0xAB, 0xCD }, { 0xEF, 0x01 } },
            nmos::vpid_codes::vpid_1_5Gbps_720_line,
            nmos::rates::rate59_94,
            sdp::transmission_models::compatible,
            sdp::smpte_standard_numbers::ST2110_40_2023,
            uint32_t(0),
            sdp::timestamp_modes::SAMP,
            uint32_t(0)
        }
    };

    for (auto& test : { example, wacky })
    {
        auto made = nmos::make_video_smpte291_sdp_parameters(test.first.session_name, test.second, test.first.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, made);
        auto roundtripped = nmos::make_video_smpte291_sdp_parameters(made.session_name, nmos::get_video_smpte291_parameters(made), made.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, roundtripped);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpParametersVideoSmpte2022_6)
{
    std::pair<nmos::sdp_parameters, nmos::video_SMPTE2022_6_parameters> example{
        {
            U("example"),
            sdp::media_types::video,
            {
                98,
                U("SMPTE2022-6"),
                27000000
            },
            {
                { U("TP"), U("2110TPN") }
            }
        },
        {
            sdp::type_parameters::type_N,
            {}
        }
    };

    std::pair<nmos::sdp_parameters, nmos::video_SMPTE2022_6_parameters> wacky{
        {
            U("wacky"),
            sdp::media_types::video,
            {
                123,
                U("SMPTE2022-6"),
                27000000
            },
            {
                { U("TP"), U("2110TPW") },
                { U("TROFF"), U("0") }
            }
        },
        {
             sdp::type_parameters::type_W,
             uint32_t(0)
        }
    };

    for (auto& test : { example, wacky })
    {
        auto made = nmos::make_video_SMPTE2022_6_sdp_parameters(test.first.session_name, test.second, test.first.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, made);
        auto roundtripped = nmos::make_video_SMPTE2022_6_sdp_parameters(made.session_name, nmos::get_video_SMPTE2022_6_parameters(made), made.rtpmap.payload_type);
        sdp_test::check_sdp_parameters(test.first, roundtripped);
    }
}

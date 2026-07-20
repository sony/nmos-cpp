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
#include "sdp/ntp.h"
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
BST_TEST_CASE(testNumericString)
{
    // Default-constructed value matches uint64_t{}, i.e. the string "0",
    // so a default-constructed origin_t still produces a well-formed SDP o= line
    {
        nmos::numeric_string ns;
        BST_REQUIRE_EQUAL(U("0"), static_cast<const utility::string_t&>(ns));
    }

    // Integer assignment produces the corresponding minimal-width decimal string.
    // Other integer-ish right-hand sides (signed int, char, bool, etc.) implicitly
    // convert to uint64_t (the only operator= overload that takes a number) and
    // therefore produce a decimal string in the same way; we don't enumerate them
    // here because that would trigger any implicit-conversion compiler warnings
    // (the same as a uint64_t member would produce).
    {
        nmos::numeric_string ns;
        ns = uint64_t{ 0 };
        BST_REQUIRE_EQUAL(U("0"), static_cast<const utility::string_t&>(ns));
        ns = uint64_t{ 1779263096 };
        BST_REQUIRE_EQUAL(U("1779263096"), static_cast<const utility::string_t&>(ns));
        ns = (std::numeric_limits<uint64_t>::max)();
        BST_REQUIRE_EQUAL(U("18446744073709551615"), static_cast<const utility::string_t&>(ns));
    }

    // String assignment preserves any leading zeros from the input
    {
        nmos::numeric_string ns;
        ns = U("0001779263096");
        BST_REQUIRE_EQUAL(U("0001779263096"), static_cast<const utility::string_t&>(ns));
    }

    // Empty and non-numeric strings are rejected
    {
        nmos::numeric_string ns;
        BST_REQUIRE_THROW(ns = U(""), std::invalid_argument);
        BST_REQUIRE_THROW(ns = U("abc"), std::invalid_argument);
        BST_REQUIRE_THROW(ns = U("12a"), std::invalid_argument);
        BST_REQUIRE_THROW(ns = U("-1"), std::invalid_argument);
        BST_REQUIRE_THROW(ns = U(" 1"), std::invalid_argument);
        BST_REQUIRE_THROW(ns = U("1.0"), std::invalid_argument);
        BST_REQUIRE_THROW(nmos::numeric_string{ U("") }, std::invalid_argument);
        BST_REQUIRE_THROW(nmos::numeric_string{ U("abc") }, std::invalid_argument);
    }

    // The historic footgun: assigning an integer to an origin_t numeric
    // string member must produce a multi-digit decimal string
    {
        nmos::sdp_parameters::origin_t o;
        o.session_version = sdp::ntp_now() >> 32;
        const utility::string_t& sv = o.session_version;
        BST_REQUIRE(sv.size() > 1);
        BST_REQUIRE(std::all_of(sv.begin(), sv.end(), [](utility::char_t c) { return std::isdigit(c, std::locale::classic()); }));
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
BST_TEST_CASE(testInterpretationOfSdpFilesFec)
{
    using web::json::value;
    using web::json::value_of;

    // See https://specs.amwa.tv/is-05/releases/v1.1.1/docs/4.1._Behaviour_-_RTP_Transport_Type.html#operation-with-smpte-2022-5
    const std::string test_sdp = R"(v=0
o=ali 1122334455 1122334466 IN IP4 172.29.26.24
s=FEC Framework Examples
t=0 0
a=group:FEC-FR S1 R1
m=video 30000 RTP/AVP 100
c=IN IP4 233.252.0.1/127
a=rtpmap:100 MP2T/90000
a=fec-source-flow: id=0
a=mid:S1
m=application 30000 UDP/FEC
c=IN IP4 233.252.0.2/127
a=fec-repair-flow: encoding-id=10; ss-fssi=n:7,k:5
a=mid:R1
)";

    const auto expected_transport_params = value_of({
        value_of({
            { nmos::fields::source_ip, value::null() },
            { nmos::fields::multicast_ip, U("233.252.0.1") },
            { nmos::fields::interface_ip, U("auto") },
            { nmos::fields::destination_port, 30000 },
            { nmos::fields::rtp_enabled, true },
            { nmos::fields::fec_enabled, true },
            { nmos::fields::fec_mode, U("auto") },
            { nmos::fields::fec_destination_ip, U("233.252.0.2") },
            { nmos::fields::fec1D_destination_port, 30000 },
            { nmos::fields::fec2D_destination_port, value::null() }
        })
    });

    const auto session_description = sdp::parse_session_description(test_sdp);
    auto parsed = nmos::parse_session_description(session_description);
    BST_REQUIRE(expected_transport_params == parsed.second);

    BST_REQUIRE_EQUAL(1, parsed.first.fec.size());
    const auto& fec = parsed.first.fec.at(0);
    BST_CHECK_EQUAL(0, fec.source_id);
    BST_CHECK_EQUAL(U("S1"), fec.media_stream_id);
    BST_REQUIRE_EQUAL(1, fec.repair_flows.size());
    BST_CHECK_EQUAL(10, fec.repair_flows.at(0).encoding_id);
    BST_CHECK_EQUAL(U("R1"), fec.repair_flows.at(0).media_stream_id);
    BST_REQUIRE_EQUAL(2, fec.repair_flows.at(0).sender_side_scheme_specific.size());
    BST_CHECK_EQUAL(U("n"), fec.repair_flows.at(0).sender_side_scheme_specific.at(0).first);
    BST_CHECK_EQUAL(U("7"), fec.repair_flows.at(0).sender_side_scheme_specific.at(0).second);

    // A Receiver must resolve interface_ip before using the parsed parameters to create SDP.
    parsed.second[0][nmos::fields::interface_ip] = value::string(U("172.29.26.24"));
    const auto made_sdp = nmos::make_session_description(parsed.first, parsed.second);
    const auto roundtripped = nmos::parse_session_description(sdp::parse_session_description(sdp::make_session_description(made_sdp)));
    BST_REQUIRE(expected_transport_params == roundtripped.second);
    BST_REQUIRE_EQUAL(1, roundtripped.first.fec.size());
    BST_CHECK_EQUAL(U("R1"), roundtripped.first.fec.at(0).repair_flows.at(0).media_stream_id);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpTransportParamsFec2D)
{
    using web::json::value_of;

    nmos::sdp_parameters sdp_params{
        U("FEC 2D"),
        sdp::media_types::video,
        { 100, U("MP2T"), 90000 }
    };
    sdp_params.connection_data.ttl = 127;
    nmos::sdp_parameters::fec_t::repair_flow_t first_repair{ 10, U("R1"), { { U("n"), U("7") }, { U("k"), U("5") } } };
    first_repair.repair_window = nmos::sdp_parameters::fec_t::repair_window_t{ 200, sdp::repair_window_units::milliseconds };
    nmos::sdp_parameters::fec_t::repair_flow_t second_repair{ 11, U("R2"), { { U("t"), U("3") } }, {}, 1 };
    second_repair.repair_window = nmos::sdp_parameters::fec_t::repair_window_t{ 150500, sdp::repair_window_units::microseconds };
    sdp_params.fec.push_back({ 0, U("S1"), { first_repair, second_repair } });

    const auto sender_transport_params = value_of({
        value_of({
            { nmos::fields::source_ip, U("192.0.2.1") },
            { nmos::fields::destination_ip, U("233.252.0.1") },
            { nmos::fields::source_port, 5004 },
            { nmos::fields::destination_port, 30000 },
            { nmos::fields::rtp_enabled, true },
            { nmos::fields::fec_enabled, true },
            { nmos::fields::fec_mode, U("2D") },
            { nmos::fields::fec_destination_ip, U("233.252.0.2") },
            { nmos::fields::fec1D_destination_port, 30002 },
            { nmos::fields::fec2D_destination_port, 30004 }
        })
    });

    const auto session_description = sdp::parse_session_description(sdp::make_session_description(nmos::make_session_description(sdp_params, sender_transport_params)));
    const auto& media_descriptions = sdp::fields::media_descriptions(session_description);
    BST_REQUIRE_EQUAL(3, media_descriptions.size());

    const auto parsed = nmos::parse_session_description(session_description);
    BST_REQUIRE_EQUAL(1, parsed.second.size());
    BST_CHECK(nmos::fields::fec_enabled(parsed.second.at(0)));
    BST_CHECK_EQUAL(U("233.252.0.2"), nmos::fields::fec_destination_ip(parsed.second.at(0)).as_string());
    BST_CHECK_EQUAL(30002, nmos::fields::fec1D_destination_port(parsed.second.at(0)).as_integer());
    BST_CHECK_EQUAL(30004, nmos::fields::fec2D_destination_port(parsed.second.at(0)).as_integer());
    BST_REQUIRE_EQUAL(2, parsed.first.fec.at(0).repair_flows.size());
    BST_REQUIRE((bool)parsed.first.fec.at(0).repair_flows.at(1).repair_window);
    BST_CHECK_EQUAL(150500, parsed.first.fec.at(0).repair_flows.at(1).repair_window->size);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSdpTransportParamsFecUnsupportedAndMalformedGroups)
{
    const std::string before = R"(v=0
o=- 0 0 IN IP4 192.0.2.1
s=FEC
t=0 0
)";
    const std::string source = R"(m=video 30000 RTP/AVP 100
c=IN IP4 233.252.0.1/127
a=rtpmap:100 MP2T/90000
a=fec-source-flow: id=0
a=mid:S1
)";
    const std::string repair1 = R"(m=application 30002 UDP/FEC
c=IN IP4 233.252.0.2/127
a=fec-repair-flow: encoding-id=10
a=mid:R1
)";
    const std::string repair2_different_address = R"(m=application 30004 UDP/FEC
c=IN IP4 233.252.0.3/127
a=fec-repair-flow: encoding-id=11
a=mid:R2
)";

    // Multiple-source RFC 6364 groups are valid at the sdp/ layer but cannot be represented by IS-05 transport parameters.
    const auto unsupported = sdp::parse_session_description(before + "a=group:FEC-FR S1 S2 R1\n" + source + repair1);
    const auto unsupported_transport_params = nmos::get_session_description_transport_params(unsupported);
    BST_REQUIRE_EQUAL(1, unsupported_transport_params.size());
    BST_CHECK(!unsupported_transport_params.at(0).has_field(nmos::fields::fec_enabled));

    const auto missing_media = sdp::parse_session_description(before + "a=group:FEC-FR S1 MISSING\n" + source);
    BST_REQUIRE_THROW(nmos::get_session_description_transport_params(missing_media), std::runtime_error);

    const auto different_addresses = sdp::parse_session_description(before
        + "a=group:FEC-FR S1 R1\n"
        + "a=group:FEC-FR S1 R2\n"
        + source + repair1 + repair2_different_address);
    BST_REQUIRE_THROW(nmos::get_session_description_transport_params(different_addresses), std::runtime_error);
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
                { U("TROFF"), U("37") },
                { U("CMAX"), U("42") },
                { U("MAXUDP"), U("57") },
                { U("TSMODE"), U("SAMP") },
                { U("TSDELAY"), U("82") }
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
            37,
            42,
            57,
            sdp::timestamp_modes::SAMP,
            82
        }
    };

    std::pair<nmos::sdp_parameters, nmos::video_raw_parameters> zero_troff_tsdelay{
        {
            U("zero_troff_tsdelay"),
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
            0U,
            42,
            57,
            sdp::timestamp_modes::SAMP,
            0U
        }
    };

    for (auto& test : { example, wacky, zero_troff_tsdelay })
    {
        auto made = nmos::make_video_raw_sdp_parameters(test.first.session_name, test.second, test.first.rtpmap.payload_type);
        nmos::check_sdp_parameters(test.first, made);
        auto roundtripped = nmos::make_video_raw_sdp_parameters(made.session_name, nmos::get_video_raw_parameters(made), made.rtpmap.payload_type);
        nmos::check_sdp_parameters(test.first, roundtripped);
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
                { U("TSDELAY"), U("82") }
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
            82,
            0.333
        }
    };

    std::pair<nmos::sdp_parameters, nmos::audio_L_parameters> zero_tsdelay{
        {
            U("zero_tsdelay"),
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
            0U,
            0.333
        }
    };

    for (auto& test : { example, wacky, zero_tsdelay })
    {
        auto made = nmos::make_audio_L_sdp_parameters(test.first.session_name, test.second, test.first.rtpmap.payload_type);
        nmos::check_sdp_parameters(test.first, made);
        auto roundtripped = nmos::make_audio_L_sdp_parameters(made.session_name, nmos::get_audio_L_parameters(made), made.rtpmap.payload_type);
        nmos::check_sdp_parameters(test.first, roundtripped);
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
                { U("TROFF"), U("37") },
                { U("TSMODE"), U("SAMP") },
                { U("TSDELAY"), U("82") }
            }
        },
        {
            { { 0xAB, 0xCD }, { 0xEF, 0x01 } },
            nmos::vpid_codes::vpid_1_5Gbps_720_line,
            nmos::rates::rate59_94,
            sdp::transmission_models::compatible,
            sdp::smpte_standard_numbers::ST2110_40_2023,
            37,
            sdp::timestamp_modes::SAMP,
            82
        }
    };

    std::pair<nmos::sdp_parameters, nmos::video_smpte291_parameters> zero_troff_tsdelay{
        {
            U("zero_troff_tsdelay"),
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
            0U,
            sdp::timestamp_modes::SAMP,
            0U
        }
    };

    for (auto& test : { example, wacky, zero_troff_tsdelay })
    {
        auto made = nmos::make_video_smpte291_sdp_parameters(test.first.session_name, test.second, test.first.rtpmap.payload_type);
        nmos::check_sdp_parameters(test.first, made);
        auto roundtripped = nmos::make_video_smpte291_sdp_parameters(made.session_name, nmos::get_video_smpte291_parameters(made), made.rtpmap.payload_type);
        nmos::check_sdp_parameters(test.first, roundtripped);
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
                { U("TROFF"), U("37") }
            }
        },
        {
             sdp::type_parameters::type_W,
             37
        }
    };

    std::pair<nmos::sdp_parameters, nmos::video_SMPTE2022_6_parameters> zero_troff{
        {
            U("zero_troff"),
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
             0U
        }
    };

    for (auto& test : { example, wacky, zero_troff })
    {
        auto made = nmos::make_video_SMPTE2022_6_sdp_parameters(test.first.session_name, test.second, test.first.rtpmap.payload_type);
        nmos::check_sdp_parameters(test.first, made);
        auto roundtripped = nmos::make_video_SMPTE2022_6_sdp_parameters(made.session_name, nmos::get_video_SMPTE2022_6_parameters(made), made.rtpmap.payload_type);
        nmos::check_sdp_parameters(test.first, roundtripped);
    }
}

// The first "test" is of course whether the header compiles standalone
#include "nmos/sdp_utils.h"

#include "bst/test/test.h"
#include "nmos/components.h"
#include "nmos/json_fields.h"
#include "nmos/random.h"
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

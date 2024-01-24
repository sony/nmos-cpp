// The first "test" is of course whether the header compiles standalone
#include "nmos/api_downgrade.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "bst/test/test.h"
#include "cpprest/json_validator.h" // for web::json::experimental::json_validator
#include "nmos/clock_name.h"  // for nmos::make_internal_clock
#include "nmos/channels.h" // for nmos::channel
#include "nmos/colorspace.h" // for nmos::colorspace
#include "nmos/components.h" // for nmos::chroma_subsampling
#include "nmos/interlace_mode.h" // for nmos::interlace_mode
#include "nmos/is04_versions.h" // for nmos::is04_versions
#include "nmos/json_schema.h" // for nmos::experimental::load_json_schema
#include "nmos/format.h" // for nmos::format
#include "nmos/node_resource.h" // for nmos::make_node
#include "nmos/node_resources.h" // for nmos::make_device
#include "nmos/resource.h" // for nmos::resource
#include "nmos/transfer_characteristic.h" // for nmos::transfer_characteristic
#include "nmos/transport.h" // for nmos::transport
#include "nmos/video_jxsv.h" // for nmos::profile
#include "sdp/json.h" // for sdp::sampling

namespace
{
    using web::json::value_of;

    const auto lower_version = nmos::is04_versions::all.cbegin();
    const auto higher_version = --nmos::is04_versions::all.end();

    const auto node_id = nmos::make_id();
    const auto device_id = nmos::make_id();
    const auto source_id = nmos::make_id();
    const auto flow_id = nmos::make_id();
    const auto sender_id = nmos::make_id();
    const auto receiver_id = nmos::make_id();
    const auto sender_ids = { sender_id };
    const auto receiver_ids = { receiver_id };
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testNodeDowngrade)
{
    const web::json::experimental::json_validator validator
    {
        nmos::experimental::load_json_schema,
        boost::copy_range<std::vector<web::uri>>(nmos::is04_versions::all | boost::adaptors::transformed(nmos::experimental::make_node_schema_uri))
    };

    const auto settings = value_of({
        {U("label"), U("Test node")},
        {U("description"), U("This is a test node")},
        {U("host_name"), U("127.0.0.1")}
    });
    const auto clocks = value_of({
        nmos::make_internal_clock(nmos::clock_names::clk0)
    });
    const auto interfaces = value_of({
        value_of({
            {U("name"), U("{0BAFC19E-7B18-4C51-A7B1-9670E0B1CFE1}")},
            {U("chassis_id"), U("3c-52-82-6f-c2-3e")},
            {U("port_id"), U("3c-52-82-6f-c2-3e")}
        }, true)
    });
    auto resource = nmos::make_node(node_id, clocks, interfaces, settings);

    // tesing downgrade on all supported versions from the lower version to 1 less than the highest version (the current resource version)
    for (auto version = lower_version; higher_version != version; ++version)
    {
        const auto& downgraded = nmos::downgrade(resource, *version);
        validator.validate(downgraded, nmos::experimental::make_node_schema_uri(*version));
        BST_REQUIRE(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testDeviceDowngrade)
{
    const web::json::experimental::json_validator validator
    {
        nmos::experimental::load_json_schema,
        boost::copy_range<std::vector<web::uri>>(nmos::is04_versions::all | boost::adaptors::transformed(nmos::experimental::make_device_schema_uri))
    };

    const auto settings = value_of({
        {U("label"), U("Test device")},
        {U("description"), U("This is a test device")},
        {U("host_name"), U("127.0.0.1")}
    });

    auto resource = nmos::make_device(device_id, node_id, sender_ids, receiver_ids, settings);

    // tesing downgrade on all supported versions from the lower version to 1 less than the highest version (the current resource version)
    for (auto version = lower_version; higher_version != version; ++version)
    {
        const auto& downgraded = nmos::downgrade(resource, *version);
        validator.validate(downgraded, nmos::experimental::make_device_schema_uri(*version));
        BST_REQUIRE(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSourceDowngrade)
{
    const web::json::experimental::json_validator validator
    {
        nmos::experimental::load_json_schema,
        boost::copy_range<std::vector<web::uri>>(nmos::is04_versions::all | boost::adaptors::transformed(nmos::experimental::make_source_schema_uri))
    };

    const auto settings = value_of({
        {U("label"), U("Test source")},
        {U("description"), U("This is a test source")}
    });

    // list of different sources to test
    std::vector<nmos::resource> sources =
    {
        nmos::make_video_source(source_id, device_id, nmos::clock_names::clk0, { 25, 1 }, settings),
        nmos::make_audio_source(source_id, device_id, nmos::clock_names::clk0, { 25, 1 }, { {U("Mono"), nmos::channel_symbols::M1} }, settings),
        nmos::make_data_source(source_id, device_id, nmos::clock_names::clk0, { 25, 1 }, settings),
        nmos::make_mux_source(source_id, device_id, nmos::clock_names::clk0, { 25, 1 }, settings)
    };

    for (const auto& resource : sources)
    {
        // tesing downgrade on all supported versions from the lower version to 1 less than the highest version (the current resource version)
        for (auto version = lower_version; higher_version != version; ++version)
        {
            const auto& downgraded = nmos::downgrade(resource, *version);
            validator.validate(downgraded, nmos::experimental::make_source_schema_uri(*version));
            BST_REQUIRE(true);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testFlowDowngrade)
{
    const web::json::experimental::json_validator validator
    {
        nmos::experimental::load_json_schema,
        boost::copy_range<std::vector<web::uri>>(nmos::is04_versions::all | boost::adaptors::transformed(nmos::experimental::make_flow_schema_uri))
    };

    const auto settings = value_of({
        {U("label"), U("Test flow")},
        {U("description"), U("This is a test flow")}
    });
    // test UNSPECIFIED using in transfer characteristic, it can only be used from v1.3 flow
    const auto unspecified_transfer_characteristic = nmos::transfer_characteristic{ U("UNSPECIFIED")};

    // list of different flows to test
    std::vector<nmos::resource> flows =
    {
        nmos::make_raw_video_flow(flow_id, source_id, device_id, { 25, 1 }, 1920, 1080, nmos::interlace_modes::interlaced_tff, nmos::colorspaces::BT709, unspecified_transfer_characteristic, nmos::YCbCr422, 10, settings),
        nmos::make_raw_video_flow(flow_id, source_id, device_id, { 25, 1 }, 1920, 1080, nmos::interlace_modes::interlaced_tff, nmos::colorspaces::BT709, nmos::transfer_characteristics::SDR, nmos::YCbCr422, 10, settings),
        nmos::make_video_jxsv_flow(flow_id, source_id, device_id,{ 25, 1 }, 1920, 1080, nmos::interlace_modes::interlaced_tff,nmos::colorspaces::BT709, unspecified_transfer_characteristic, sdp::samplings::YCbCr_4_2_2, 10, nmos::profiles::High444_12, nmos::levels::Level1k_1, nmos::sublevels::Sublev3bpp, 2.0, settings),
        nmos::make_coded_video_flow(flow_id, source_id, device_id,{ 25, 1 }, 1920, 1080, nmos::interlace_modes::interlaced_tff,nmos::colorspaces::BT709, unspecified_transfer_characteristic, sdp::samplings::YCbCr_4_2_2, 10, nmos::media_types::video_raw, settings),
        nmos::make_raw_audio_flow(flow_id, source_id, device_id, 48000, 24, settings),
        nmos::make_sdianc_data_flow(flow_id, source_id, device_id, { 0x60, 0x60 }, settings),
        nmos::make_mux_flow(flow_id, source_id, device_id, settings)
    };

    for (const auto& resource : flows)
    {
        // tesing downgrade on all supported versions from the lower version to 1 less than the highest version (the current resource version)
        for (auto version = lower_version; higher_version != version; ++version)
        {
            const auto& downgraded = nmos::downgrade(resource, *version);
            validator.validate(downgraded, nmos::experimental::make_flow_schema_uri(*version));
            BST_REQUIRE(true);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSenderDowngrade)
{
    const web::json::experimental::json_validator validator
    {
        nmos::experimental::load_json_schema,
        boost::copy_range<std::vector<web::uri>>(nmos::is04_versions::all | boost::adaptors::transformed(nmos::experimental::make_sender_schema_uri))
    };

    const auto settings = value_of({
        {U("label"), U("Left Channel")},
        {U("description"), U("This is a test sender")}
    });
    const std::vector<utility::string_t> interfaces = { U("{0BAFC19E-7B18-4C51-A7B1-9670E0B1CFE1}") };
    const nmos::transport rtp_transport{ nmos::transports::rtp };

    // test sender downgrade
    const auto manifest_href = nmos::experimental::make_manifest_api_manifest(sender_id, settings);

    // list of different downgradable senders to test
    std::vector<nmos::resource> senders =
    {
        nmos::make_sender(sender_id, flow_id, rtp_transport, device_id, manifest_href.to_string(), interfaces, settings),
    };

    for (const auto& resource : senders)
    {
        // tesing downgrade on all supported versions from the lower version to 1 less than the highest version (the current resource version)
        for (auto version = lower_version; higher_version != version; ++version)
        {
            const auto& downgraded = nmos::downgrade(resource, *version);
            validator.validate(downgraded, nmos::experimental::make_sender_schema_uri(*version));
            BST_REQUIRE(true);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testReceiverDowngrade)
{
    const web::json::experimental::json_validator validator
    {
        nmos::experimental::load_json_schema,
        boost::copy_range<std::vector<web::uri>>(nmos::is04_versions::all | boost::adaptors::transformed(nmos::experimental::make_receiver_schema_uri))
    };

    const auto settings = value_of({
        {U("label"), U("Test receiver")},
        {U("description"), U("This is a test receiver")}
    });
    const nmos::transport transport{ nmos::transports::rtp };
    const std::vector<utility::string_t> interfaces = { U("{0BAFC19E-7B18-4C51-A7B1-9670E0B1CFE1}") };

    // list of different receivers to test
    std::vector<nmos::resource> receivers =
    {
        nmos::make_receiver(receiver_id, device_id, transport, interfaces, nmos::formats::video, { nmos::media_types::video_raw }, settings),
        nmos::make_audio_receiver(receiver_id, device_id, transport, interfaces, 24, settings),
        nmos::make_sdianc_data_receiver(receiver_id, device_id, transport, interfaces, settings),
        nmos::make_mux_receiver(receiver_id, device_id, transport, interfaces, settings)
    };

    for (const auto& resource : receivers)
    {
        // tesing downgrade on all supported versions from the lower version to 1 less than the highest version (the current resource version)
        for (auto version = lower_version; higher_version != version; ++version)
        {
            const auto& downgraded = nmos::downgrade(resource, *version);
            validator.validate(downgraded, nmos::experimental::make_receiver_schema_uri(*version));
            BST_REQUIRE(true);
        }
    }
}

#include "nmos/node_resources.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/api_utils.h" // for nmos::http_scheme
#include "nmos/channels.h"
#include "nmos/clock_name.h"
#include "nmos/colorspace.h"
#include "nmos/components.h"
#include "nmos/device_type.h"
#include "nmos/did_sdid.h"
#include "nmos/event_type.h"
#include "nmos/format.h"
#include "nmos/interlace_mode.h"
#include "nmos/is04_versions.h"
#include "nmos/is05_versions.h"
#include "nmos/is07_versions.h"
#include "nmos/is08_versions.h"
#include "nmos/media_type.h"
#include "nmos/resource.h"
#include "nmos/transfer_characteristic.h"
#include "nmos/transport.h"
#include "nmos/version.h"
//DPB case conversion
#include <boost/algorithm/string/case_conv.hpp>


namespace nmos
{
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/device.json
    nmos::resource make_device(const nmos::id& id, const nmos::id& node_id, const std::vector<nmos::id>& senders, const std::vector<nmos::id>& receivers, const nmos::settings& settings)
    {
        using web::json::value;
        using web::json::value_of;
        using web::json::value_from_elements;

        auto data = details::make_resource_core(id, settings);

        data[U("type")] = value::string(nmos::device_types::generic.name);
        data[U("node_id")] = value::string(node_id);
        data[U("senders")] = value_from_elements(senders);
        data[U("receivers")] = value_from_elements(receivers);

        const auto hosts = nmos::get_hosts(settings);

        if (0 <= nmos::fields::connection_port(settings))
        {
            for (const auto& version : nmos::is05_versions::from_settings(settings))
            {
                auto connection_uri = web::uri_builder()
                    .set_scheme(nmos::http_scheme(settings))
                    .set_port(nmos::fields::connection_port(settings))
                    .set_path(U("/x-nmos/connection/") + make_api_version(version));
                auto type = U("urn:x-nmos:control:sr-ctrl/") + make_api_version(version);

                for (const auto& host : hosts)
                {
                    web::json::push_back(data[U("controls")], value_of({
                        { U("href"), connection_uri.set_host(host).to_uri().to_string() },
                        { U("type"), type }
                    }));
                }
            }
        }

        if (0 <= nmos::fields::events_port(settings))
        {
            for (const auto& version : nmos::is07_versions::from_settings(settings))
            {
                auto events_uri = web::uri_builder()
                    .set_scheme(nmos::http_scheme(settings))
                    .set_port(nmos::fields::events_port(settings))
                    .set_path(U("/x-nmos/events/") + make_api_version(version));
                auto type = U("urn:x-nmos:control:events/") + make_api_version(version);

                for (const auto& host : hosts)
                {
                    web::json::push_back(data[U("controls")], value_of({
                        { U("href"), events_uri.set_host(host).to_uri().to_string() },
                        { U("type"), type }
                    }));
                }
            }
        }

        if (0 <= nmos::fields::channelmapping_port(settings))
        {
            // At the moment, it doesn't seem necessary to enable support multiple API instances via the API selector mechanism
            // so therefore just a single Channel Mapping API instance is mounted directly at /x-nmos/channelmapping/{version}/
            // If it becomes necessary, each device could associated with a specific API selector
            // See https://github.com/AMWA-TV/nmos-audio-channel-mapping/blob/v1.0.x/docs/2.0.%20APIs.md#api-paths

            for (const auto& version : nmos::is08_versions::from_settings(settings))
            {
                auto channelmapping_uri = web::uri_builder()
                    .set_scheme(nmos::http_scheme(settings))
                    .set_port(nmos::fields::channelmapping_port(settings))
                    .set_path(U("/x-nmos/channelmapping/") + make_api_version(version));
                auto type = U("urn:x-nmos:control:cm-ctrl/") + make_api_version(version);

                for (const auto& host : hosts)
                {
                    web::json::push_back(data[U("controls")], value_of({
                        { U("href"), channelmapping_uri.set_host(host).to_uri().to_string() },
                        { U("type"), type }
                    }));
                }
            }
        }

        if (0 <= nmos::experimental::fields::manifest_port(settings))
        {
            // See https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/device-control-types/manifest-base.md
            // and nmos::experimental::make_manifest_api_manifest
            auto manifest_uri = web::uri_builder()
                .set_scheme(nmos::http_scheme(settings))
                .set_port(nmos::experimental::fields::manifest_port(settings))
                .set_path(U("/x-manifest/"));
            auto type = U("urn:x-nmos:control:manifest-base/v1.0");

            for (const auto& host : hosts)
            {
                web::json::push_back(data[U("controls")], value_of({
                    { U("href"), manifest_uri.set_host(host).to_uri().to_string() },
                    { U("type"), type }
                }));
            }
        }

        return{ is04_versions::v1_3, types::device, std::move(data), false };
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_core.json
    nmos::resource make_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        using web::json::value;

        auto data = details::make_resource_core(id, settings);

        if (0 != grain_rate) data[U("grain_rate")] = nmos::make_rational(grain_rate); // optional
        data[U("caps")] = value::object();
        data[U("device_id")] = value::string(device_id);
        data[U("parents")] = value::array();
        data[U("clock_name")] = !clk.name.empty() ? value::string(clk.name) : value::null();

        return{ is04_versions::v1_3, types::source, std::move(data), false };
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_generic.json
    nmos::resource make_generic_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::format& format, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_source(id, device_id, clk, grain_rate, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(format.name);

        return resource;
    }

    nmos::resource make_generic_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::format& format, const nmos::settings& settings)
    {
        return make_generic_source(id, device_id, {}, grain_rate, format, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_generic.json
    nmos::resource make_video_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        return make_generic_source(id, device_id, clk, grain_rate, nmos::formats::video, settings);
    }

    nmos::resource make_video_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        return make_video_source(id, device_id, {}, grain_rate, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_generic.json
    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        return make_generic_source(id, device_id, clk, grain_rate, nmos::formats::data, settings);
    }

    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        return make_data_source(id, device_id, {}, grain_rate, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3/APIs/schemas/source_data.json
    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::event_type& event_type, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_data_source(id, device_id, clk, grain_rate, settings);
        auto& data = resource.data;

        data[U("event_type")] = value::string(event_type.name);

        return resource;
    }

    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::event_type& event_type, const nmos::settings& settings)
    {
        return make_data_source(id, device_id, {}, grain_rate, event_type, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_audio.json
    nmos::resource make_audio_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const std::vector<channel>& channels, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_source(id, device_id, clk, grain_rate, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::audio.name);

        auto channels_data = value::array();
        for (auto channel : channels)
        {
            web::json::push_back(channels_data, make_channel(channel));
        }
        data[U("channels")] = std::move(channels_data);

        return resource;
    }

    nmos::resource make_audio_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const std::vector<channel>& channels, const nmos::settings& settings)
    {
        return make_audio_source(id, device_id, {}, grain_rate, channels, settings);
    }

    nmos::resource make_mux_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        return make_generic_source(id, device_id, clk, grain_rate, nmos::formats::mux, settings);
    }

    nmos::resource make_mux_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        return make_mux_source(id, device_id, {}, grain_rate, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_core.json
    nmos::resource make_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        using web::json::value;

        auto data = details::make_resource_core(id, settings);

        if (0 != grain_rate) data[U("grain_rate")] = nmos::make_rational(grain_rate); // optional

        data[U("source_id")] = value::string(source_id);
        data[U("device_id")] = value::string(device_id);
        data[U("parents")] = value::array();

        return{ is04_versions::v1_3, types::flow, std::move(data), false };
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video.json
    nmos::resource make_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode, const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_flow(id, source_id, device_id, grain_rate, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::video.name);
        data[U("frame_width")] = frame_width;
        data[U("frame_height")] = frame_height;
        if (interlace_modes::none != interlace_mode) data[U("interlace_mode")] = value::string(interlace_mode.name); // optional
        data[U("colorspace")] = value::string(colorspace.name);
        if (transfer_characteristics::none != transfer_characteristic) data[U("transfer_characteristic")] = value::string(transfer_characteristic.name); // optional

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video_raw.json
    //DPB
    nmos::resource make_raw_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode,
      const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, chroma_subsampling chroma_subsampling, unsigned int bit_depth, const nmos::settings& settings, std::string txmedia_type)
    {
        using web::json::value;

        auto resource = make_video_flow(id, source_id, device_id, grain_rate, frame_width, frame_height, interlace_mode, colorspace, transfer_characteristic, settings);
        auto& data = resource.data;

        //DPB fix for coded video typoe, due to error in make_coded_video_flow()
        auto media_type = boost::algorithm::to_upper_copy(txmedia_type);
        if (media_type == "H264" || media_type == "H.264") { data[U("media_type")] = value::string(nmos::media_types::video_h264.name); }
        else if (media_type == "H265" || media_type == "H.265") { data[U("media_type")] = value::string(nmos::media_types::video_h265.name); }
        else if (media_type == "VC2") { data[U("media_type")] = value::string(nmos::media_types::video_vc2.name); }
        else if (media_type == "JXSV") { data[U("media_type")] = value::string(nmos::media_types::video_jxsv.name); }
        else { data[U("media_type")] = value::string(nmos::media_types::video_raw.name); }

        //data[U("media_type")] = value::string(nmos::media_types::video_raw.name);
        //data[U("media_type")] = value::string(nmos::media_types::video_h264.name);
        data[U("components")] = make_components(chroma_subsampling, frame_width, frame_height, bit_depth);

        return resource;
    }

    nmos::resource make_raw_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings, std::string txmedia_type)
    {
        return make_raw_video_flow(id, source_id, device_id, {}, 1920, 1080, nmos::interlace_modes::interlaced_bff, nmos::colorspaces::BT709, nmos::transfer_characteristics::SDR, YCbCr422, 10, settings, txmedia_type);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video_coded.json
    // (media_type must *not* be nmos::media_types::video_raw; cf. nmos::make_raw_video_flow)
    nmos::resource make_coded_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode, const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, const nmos::media_type& media_type, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_video_flow(id, source_id, device_id, grain_rate, frame_width, frame_height, interlace_mode, colorspace, transfer_characteristic, settings);
        auto& data = resource.data;

        data[U("media_type")] = value::string(media_type.name);

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_audio.json
    nmos::resource make_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_flow(id, source_id, device_id, {}, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::audio.name);
        data[U("sample_rate")] = make_rational(sample_rate);

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_audio_raw.json
    //DPB
    nmos::resource make_raw_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, unsigned int bit_depth, const nmos::settings& settings, std::string txmedia_type)
    //nmos::resource make_raw_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, std::string txmedia_type, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_audio_flow(id, source_id, device_id, sample_rate, settings);
        auto& data = resource.data;
        //DPB changed audio format form fixed to programmable
        //Coded format
        auto media_type = boost::algorithm::to_upper_copy(txmedia_type);
        if (media_type == "AAC") { data[U("media_type")] = value::string(nmos::media_types::audio_aac.name); data[U("bit_depth")] = bit_depth; }
        else if (media_type == "MP2") { data[U("media_type")] = value::string(nmos::media_types::audio_mp2.name); data[U("bit_depth")] = bit_depth; }
        else if (media_type == "MP3") { data[U("media_type")] = value::string(nmos::media_types::audio_mp3.name); data[U("bit_depth")] = bit_depth; }
        else if (media_type == "M4A") { data[U("media_type")] = value::string(nmos::media_types::audio_m4a.name); data[U("bit_depth")] = bit_depth; }
        else if (media_type == "OPUS") { data[U("media_type")] = value::string(nmos::media_types::audio_opus.name); data[U("bit_depth")] = bit_depth; }
        //Uncoded format
        //Defaults to L24, bit depth set ny bit_depth=24 in called function
        else if (media_type == "L8") { data[U("media_type")] = value::string(nmos::media_types::audio_L(8).name); data[U("bit_depth")] = 8; }
        else if (media_type == "L16") { data[U("media_type")] = value::string(nmos::media_types::audio_L(16).name); data[U("bit_depth")] = 16; }
        else if (media_type == "L20") { data[U("media_type")] = value::string(nmos::media_types::audio_L(20).name); data[U("bit_depth")] = 20; }
        else { data[U("media_type")] = value::string(nmos::media_types::audio_L(bit_depth).name); data[U("bit_depth")] = bit_depth; }

        return resource;
    }

    nmos::resource make_raw_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, unsigned int bit_depth, const nmos::settings& settings, std::string txmedia_type)
    {
        return make_raw_audio_flow(id, source_id, device_id, 48000, 24, settings, txmedia_type);

    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_audio_coded.json
    // (media_type must *not* be nmos::media_types::audio_L(bit_depth); cf. nmos::make_raw_audio_flow)

    //DPB //nmos::resource make_coded_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, const nmos::media_type& media_type, const nmos::settings& settings);
    nmos::resource make_coded_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, std::string txmedia_type, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_audio_flow(id, source_id, device_id, sample_rate, settings);
        auto& data = resource.data;

        //data[U("media_type")] = value::string(media_type.name);
        if (txmedia_type == "aac") { data[U("media_type")] = value::string(nmos::media_types::audio_aac.name); }
        else { data[U("media_type")] = value::string(nmos::media_types::audio_m4a.name); }

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_sdianc_data.json
    nmos::resource make_sdianc_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const std::vector<nmos::did_sdid>& did_sdids, const nmos::settings& settings)
    {
        using web::json::value;
        using web::json::value_from_elements;

        auto resource = make_flow(id, source_id, device_id, {}, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::data.name);
        data[U("media_type")] = value::string(nmos::media_types::video_smpte291.name);
        if (!did_sdids.empty())
        {
            data[U("DID_SDID")] = value_from_elements(did_sdids | boost::adaptors::transformed([](const nmos::did_sdid& did_sdid)
            {
                return nmos::make_did_sdid(did_sdid);
            }));
        }

        return resource;
    }

    nmos::resource make_sdianc_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings)
    {
        return make_sdianc_data_flow(id, source_id, device_id, {}, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3/APIs/schemas/flow_json_data.json
    nmos::resource make_json_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::event_type& event_type, const nmos::settings& settings)
    {
        using web::json::value;
        using web::json::value_from_elements;

        auto resource = make_flow(id, source_id, device_id, {}, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::data.name);
        data[U("media_type")] = value::string(nmos::media_types::application_json.name);
        if (!event_type.name.empty())
        {
            data[U("event_type")] = value::string(event_type.name);
        }

        return resource;
    }

    nmos::resource make_json_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings)
    {
        return make_json_data_flow(id, source_id, device_id, {}, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_data.json
    // (media_type must *not* be nmos::media_types::video_smpte291 or nmos::media_types::application_json; cf. nmos::make_sdianc_data_flow and nmos::make_json_data_flow)
    nmos::resource make_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::media_type& media_type, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_flow(id, source_id, device_id, {}, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::data.name);
        data[U("media_type")] = value::string(media_type.name);

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_mux.json
    nmos::resource make_mux_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::media_type& media_type, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_flow(id, source_id, device_id, {}, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::mux.name);
        data[U("media_type")] = value::string(media_type.name);

        return resource;
    }

    nmos::resource make_mux_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings)
    {
        return make_mux_flow(id, source_id, device_id, nmos::media_types::video_SMPTE2022_6, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/sender.json
    nmos::resource make_sender(const nmos::id& id, const nmos::id& flow_id, const nmos::transport& transport, const nmos::id& device_id, const utility::string_t& manifest_href,
      const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
        using web::json::value;

        auto data = details::make_resource_core(id, settings);

        //data[U("caps")] = value::object(); // optional

        data[U("flow_id")] = !flow_id.empty() ? value::string(flow_id) : value::null();
        data[U("transport")] = value::string(transport.name);
        data[U("device_id")] = value::string(device_id);
        // "Permit a Sender's 'manifest_href' to be null when the transport type does not require a transport file" from IS-04 v1.3
        // See https://github.com/AMWA-TV/nmos-discovery-registration/pull/97
        data[U("manifest_href")] = !manifest_href.empty() ? value::string(manifest_href) : value::null();

        //DPB modified to change interface bindingds to be remote interface for receiver if OGC_remote is set
        auto remote_int_name = nmos::fields::remote_int_name(settings);
        auto OGC_remote = nmos::fields::OGC_remote(settings);
        auto& interface_bindings = data[U("interface_bindings")] = value::array();

        if (OGC_remote == true)
        {
            web::json::push_back(interface_bindings, remote_int_name);
        }
        else
        {
            for (const auto& interface : interfaces)
            {
                web::json::push_back(interface_bindings, interface);
            }
        }

        value subscription;
        subscription[U("receiver_id")] = value::null();
        subscription[U("active")] = value::boolean(false);
        data[U("subscription")] = std::move(subscription);

        nmos::resource result{ is04_versions::v1_3, types::sender, std::move(data), false };

        // only RTP Senders are permitted prior to v1.3, so specify an appropriate minimum API version
        // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3/docs/2.1.%20APIs%20-%20Common%20Keys.md#transport
        result.downgrade_version = nmos::transports::rtp == nmos::transport_base(transport)
            ? is04_versions::v1_0
            : is04_versions::v1_3;

        return result;
    }

    namespace experimental
    {
        web::uri make_manifest_api_manifest(const nmos::id& sender_id, const nmos::settings& settings)
        {
            return web::uri_builder()
                .set_scheme(nmos::http_scheme(settings))
                .set_host(nmos::get_host(settings))
                .set_port(nmos::experimental::fields::manifest_port(settings))
                .set_path(U("/x-manifest/senders/") + sender_id + U("/manifest"))
                .to_uri();
        }
    }

    //DPB mcast or ucast settings cahnged rtp_mcast to rtp_ucast

    nmos::resource make_sender(const nmos::id& id, const nmos::id& flow_id, const nmos::transport& transport, const nmos::id& device_id, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
            //return make_sender(id, flow_id, nmos::transports::rtp_ucast, device_id, experimental::make_manifest_api_manifest(id, settings).to_string(), interfaces, settings);
            return make_sender(id, flow_id, transport, device_id, experimental::make_manifest_api_manifest(id, settings).to_string(), interfaces, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_core.json
    nmos::resource make_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
        using web::json::value;

        auto data = details::make_resource_core(id, settings);

        data[U("device_id")] = value::string(device_id);
        data[U("transport")] = value::string(transport.name);

        //DPB modified to change interface bindingds to be remote interface for receiver if OGC_remote is set
        auto remote_int_name = nmos::fields::remote_int_name(settings);
        auto OGC_remote = nmos::fields::OGC_remote(settings);
        auto& interface_bindings = data[U("interface_bindings")] = value::array();

        if (OGC_remote == true)
        {
            web::json::push_back(interface_bindings, remote_int_name);
        }
        else
        {
            for (const auto& interface : interfaces)
            {
                web::json::push_back(interface_bindings, interface);
            }
        }

        std::cout << "\nreceiver_interfaces: " << interface_bindings << "\n";

        value subscription;
        subscription[U("sender_id")] = value::null();
        subscription[U("active")] = value::boolean(false);
        data[U("subscription")] = std::move(subscription);

        nmos::resource result{ is04_versions::v1_3, types::receiver, std::move(data), false };

        // only RTP Receivers are permitted prior to v1.3, so specify an appropriate minimum API version
        // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3/docs/2.1.%20APIs%20-%20Common%20Keys.md#transport
        result.downgrade_version = nmos::transports::rtp == nmos::transport_base(transport)
            ? is04_versions::v1_0
            : is04_versions::v1_3;

        return result;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_video.json
    nmos::resource make_video_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings, int index)
    {
        using web::json::value;

        auto resource = make_receiver(id, device_id, transport, interfaces, settings);
        auto& data = resource.data;

        //DPB Change format from defalt (raw) to value in receiver conf
        int i = 0;
        //const auto OGC_remote = nmos::fields::OGC_remote(settings);
        const auto& conf_receivers = nmos::fields::receivers_list(settings);
        auto rxmedia_type = nmos::media_types::video_raw.name;
        auto receiver_conf_format = nmos::media_types::video_raw.name;

        for (const auto& conf_receiver : conf_receivers)
        {
            if (index == i)
                rxmedia_type  = boost::algorithm::to_upper_copy(nmos::fields::conf_format(conf_receiver));
            i++;
        }
        //DPB fix for case sensitive names
        if (rxmedia_type == "H264"  || rxmedia_type == "H.264") {  receiver_conf_format = nmos::media_types::video_h264.name; }
        else if (rxmedia_type == "H265"  || rxmedia_type == "H.265") { receiver_conf_format = nmos::media_types::video_h265.name; }
        else if (rxmedia_type == "VC2") { receiver_conf_format = nmos::media_types::video_vc2.name; }
        else if (rxmedia_type == "JXSV") { receiver_conf_format = nmos::media_types::video_jxsv.name; }
        else { receiver_conf_format = nmos::media_types::video_raw.name; }

        std::cout << "Video format: " << nmos::formats::video.name << " " << nmos::media_types::video_raw.name << " ConfForm: "  << receiver_conf_format << '\n';

        data[U("format")] = value::string(nmos::formats::video.name);
        //data[U("caps")][U("media_types")][0] = value::string(nmos::media_types::video_raw.name);
        data[U("caps")][U("media_types")][0] = value::string(receiver_conf_format);
        //data[U("caps")][U("media_types")][0] = "H264");
        //DPB nmos-js does not match to h264 sender //web::json::push_back(data[U("caps")][U("media_types")], "video/H264");
        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_audio.json
    nmos::resource make_audio_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const std::vector<unsigned int>& bit_depths, const nmos::settings& settings, int index)
    {
        using web::json::value;

        auto resource = make_receiver(id, device_id, transport, interfaces, settings);
        auto& data = resource.data;

        //DPB Change format from defalt (raw) to value in receiver conf
        int i = 0;
        //const auto OGC_remote = nmos::fields::OGC_remote(settings);
        const auto& conf_receivers = nmos::fields::receivers_list(settings);
        std::string rxmedia_type = " ";

        for (const auto& conf_receiver : conf_receivers)
        {
            if (index == i)
                rxmedia_type  = boost::algorithm::to_upper_copy(nmos::fields::conf_format(conf_receiver));
            i++;
        }
        //DPB fix for case sensitive names
        if (rxmedia_type == "AAC") {  web::json::push_back(data[U("caps")][U("media_types")], nmos::media_types::audio_aac.name); }
        else if (rxmedia_type == "MP2") {  web::json::push_back(data[U("caps")][U("media_types")], nmos::media_types::audio_mp2.name); }
        else if (rxmedia_type == "MP3") {  web::json::push_back(data[U("caps")][U("media_types")], nmos::media_types::audio_mp3.name); }
        else if (rxmedia_type == "M4A") {  web::json::push_back(data[U("caps")][U("media_types")], nmos::media_types::audio_m4a.name); }
        else if (rxmedia_type == "OPUS") {  web::json::push_back(data[U("caps")][U("media_types")], nmos::media_types::audio_opus.name); }

        else if (rxmedia_type == "L8") { web::json::push_back(data[U("caps")][U("media_types")], value::string(nmos::media_types::audio_L(8).name)); }
        else if (rxmedia_type == "L16") { web::json::push_back(data[U("caps")][U("media_types")], value::string(nmos::media_types::audio_L(16).name)); }
        else if (rxmedia_type == "L20") { web::json::push_back(data[U("caps")][U("media_types")], value::string(nmos::media_types::audio_L(20).name)); }
        else { web::json::push_back(data[U("caps")][U("media_types")], value::string(nmos::media_types::audio_L(24).name)); }  //L24, bit_depths = 24

        data[U("format")] = value::string(nmos::formats::audio.name);

        for (const auto& bit_depth : bit_depths) {
            std::cout << "Paylaod: " << nmos::formats::audio.name  <<  " ConfForm: "  << rxmedia_type << " " << bit_depth << '\n';
        }

        return resource;
    }

    nmos::resource make_audio_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, unsigned int bit_depth, const nmos::settings& settings, int index)
    {
        return make_audio_receiver(id, device_id, transport, interfaces, std::vector<unsigned int>{ bit_depth }, settings, index);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_data.json
    nmos::resource make_sdianc_data_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_receiver(id, device_id, transport, interfaces, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::data.name);
        data[U("caps")][U("media_types")][0] = value::string(nmos::media_types::video_smpte291.name);

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_data.json
    // (media_type must *not* be nmos::media_types::video_smpte291; cf. nmos::make_sdianc_data_receiver)
    nmos::resource make_data_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::media_type& media_type, const std::vector<nmos::event_type>& event_types, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_receiver(id, device_id, transport, interfaces, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::data.name);
        data[U("caps")][U("media_types")][0] = value::string(media_type.name);
        for (const auto& event_type : event_types)
        {
            web::json::push_back(data[U("caps")][U("event_types")], value::string(event_type.name));
        }

        return resource;
    }

    nmos::resource make_data_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::media_type& media_type, const nmos::settings& settings)
    {
        return make_data_receiver(id, device_id, transport, interfaces, media_type, {}, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_mux.json
    nmos::resource make_mux_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::media_type& media_type, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_receiver(id, device_id, transport, interfaces, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::mux.name);
        data[U("caps")][U("media_types")][0] = value::string(media_type.name);

        return resource;
    }

    nmos::resource make_mux_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
        return make_mux_receiver(id, device_id, transport, interfaces, nmos::media_types::video_SMPTE2022_6, settings);
    }
}

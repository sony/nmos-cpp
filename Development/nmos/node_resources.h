#ifndef NMOS_NODE_RESOURCES_H
#define NMOS_NODE_RESOURCES_H

#include "nmos/id.h"
#include "nmos/rational.h"
#include "nmos/settings.h"

namespace web
{
    class uri;
}

namespace nmos
{
    // IS-04 Node API resources

    struct channel;
    struct clock_name;
    struct colorspace;
    enum chroma_subsampling : int;
    struct did_sdid;
    struct event_type;
    struct format;
    struct interlace_mode;
    struct media_type;
    struct resource;
    struct transfer_characteristic;
    struct transport;

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/device.json
    nmos::resource make_device(const nmos::id& id, const nmos::id& node_id, const std::vector<nmos::id>& senders, const std::vector<nmos::id>& receivers, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_core.json
    nmos::resource make_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_generic.json
    nmos::resource make_generic_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::format& format, const nmos::settings& settings);
    nmos::resource make_generic_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::format& format, const nmos::settings& settings);
    nmos::resource make_video_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::settings& settings);
    nmos::resource make_video_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_generic.json
    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::settings& settings);
    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3/APIs/schemas/source_data.json
    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::event_type& event_type, const nmos::settings& settings);
    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::event_type& event_type, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_audio.json
    nmos::resource make_audio_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const std::vector<nmos::channel>& channels, const nmos::settings& settings);
    nmos::resource make_audio_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const std::vector<nmos::channel>& channels, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_generic.json
    nmos::resource make_mux_source(const nmos::id& id, const nmos::id& device_id, const nmos::clock_name& clk, const nmos::rational& grain_rate, const nmos::settings& settings);
    nmos::resource make_mux_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_core.json
    nmos::resource make_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video.json
    nmos::resource make_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode, const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video_raw.json
    nmos::resource make_raw_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode, const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, chroma_subsampling chroma_subsampling, unsigned int bit_depth, const nmos::settings& settings);
    nmos::resource make_raw_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video_coded.json
    // (media_type must *not* be nmos::media_types::video_raw; cf. nmos::make_raw_video_flow)
    nmos::resource make_coded_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode, const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, const nmos::media_type& media_type, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_audio.json
    nmos::resource make_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_audio_raw.json
    nmos::resource make_raw_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, unsigned int bit_depth, const nmos::settings& settings);
    nmos::resource make_raw_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_audio_coded.json
    // (media_type must *not* be nmos::media_types::audio_L(bit_depth); cf. nmos::make_raw_audio_flow)
    nmos::resource make_coded_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, const nmos::media_type& media_type, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_sdianc_data.json
    nmos::resource make_sdianc_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const std::vector<nmos::did_sdid>& did_sdids, const nmos::settings& settings);
    nmos::resource make_sdianc_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3/APIs/schemas/flow_json_data.json
    nmos::resource make_json_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::event_type& event_type, const nmos::settings& settings);
    nmos::resource make_json_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_data.json
    // (media_type must *not* be nmos::media_types::video_smpte291 or nmos::media_types::application_json; cf. nmos::make_sdianc_data_flow and nmos::make_json_data_flow)
    nmos::resource make_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::media_type& media_type, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_mux.json
    nmos::resource make_mux_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::media_type& media_type, const nmos::settings& settings);
    nmos::resource make_mux_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/sender.json
    nmos::resource make_sender(const nmos::id& id, const nmos::id& flow_id, const nmos::transport& transport, const nmos::id& device_id, const utility::string_t& manifest_href, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);

    namespace experimental
    {
        web::uri make_manifest_api_manifest(const nmos::id& sender_id, const nmos::settings& settings);
    }
    
    nmos::resource make_sender(const nmos::id& id, const nmos::id& flow_id, const nmos::id& device_id, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_core.json
    nmos::resource make_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_video.json
    nmos::resource make_video_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_audio.json
    nmos::resource make_audio_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const std::vector<unsigned int>& bit_depths, const nmos::settings& settings);
    nmos::resource make_audio_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, unsigned int bit_depth, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_data.json
    nmos::resource make_sdianc_data_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3/APIs/schemas/receiver_data.json
    // (media_type must *not* be nmos::media_types::video_smpte291; cf. nmos::make_sdianc_data_receiver)
    nmos::resource make_data_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::media_type& media_type, const std::vector<nmos::event_type>& event_types, const nmos::settings& settings);
    nmos::resource make_data_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::media_type& media_type, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_mux.json
    nmos::resource make_mux_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::media_type& media_type, const nmos::settings& settings);
    nmos::resource make_mux_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);
}

#endif

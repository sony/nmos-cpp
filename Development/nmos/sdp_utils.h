#ifndef NMOS_SDP_UTILS_H
#define NMOS_SDP_UTILS_H

#include "bst/optional.h"
#include "cpprest/basic_utils.h"
#include "sdp/json.h"
#include "sdp/ntp.h"
#include "nmos/did_sdid.h"
#include "nmos/rational.h"
#include "nmos/vpid_code.h"

namespace nmos
{
    struct sdp_parameters;

    // Sender helper functions

    web::json::value make_components(const sdp::sampling& sampling, uint32_t width, uint32_t height, uint32_t depth);

    namespace details
    {
        sdp::sampling make_sampling(const web::json::array& components);

        // Payload identifiers 96-127 are used for payloads defined dynamically during a session
        // 96 and 97 are suitable for video and audio encodings not covered by the IANA registry
        // See https://tools.ietf.org/html/rfc3551#section-3
        // and https://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-1
        const uint64_t payload_type_video_default = 96;
        const uint64_t payload_type_audio_default = 97;
        const uint64_t payload_type_data_default = 100;
        // Payload type 98 is recommended for "High bit rate media transport / 27-MHz Clock"
        // Payload type 99 is recommended for "High bit rate media transport FEC / 27-MHz Clock"
        // "Alternatively, payload types may be set by other means in accordance with RFC 3550."
        // See SMPTE ST 2022-6:2012 Section 6.3 RTP/UDP/IP Header
        const uint64_t payload_type_mux_default = 98;
    }

    // Construct SDP parameters from the IS-04 resources, using default values for unspecified items
    sdp_parameters make_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain);

    // deprecated, provided for backwards compatibility, because it may be necessary to also specify the PTP domain to generate an RFC 7273 'ts-refclk' attribute that meets the additional constraints of ST 2110-10
    sdp_parameters make_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids);

    // Construct SDP parameters for the specified format from the IS-04 resources, using default values for unspecified items
    sdp_parameters make_video_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<sdp::type_parameter> tp);
    sdp_parameters make_audio_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<double> packet_time);
    sdp_parameters make_data_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<nmos::vpid_code> vpid_code);
    sdp_parameters make_mux_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<sdp::type_parameter> tp);

    // Sender/Receiver helper functions

    // Make a json representation of an SDP file, e.g. for sdp::make_session_description, from the specified parameters; explicitly specify whether 'source-filter' attributes are included to override the default behaviour
    web::json::value make_session_description(const sdp_parameters& sdp_params, const web::json::value& transport_params, bst::optional<bool> source_filters = bst::nullopt);

    // Receiver helper functions

    // Get IS-05 transport parameters from the json representation of an SDP file, e.g. from sdp::parse_session_description
    web::json::value get_session_description_transport_params(const web::json::value& session_description);

    // Get other SDP parameters from the json representation of an SDP file, e.g. from sdp::parse_session_description
    sdp_parameters get_session_description_sdp_parameters(const web::json::value& session_description);

    // Get SDP parameters from the json representation of an SDP file, e.g. from sdp::parse_session_description
    std::pair<sdp_parameters, web::json::value> parse_session_description(const web::json::value& session_description);

    void validate_sdp_parameters(const web::json::value& receiver, const sdp_parameters& sdp_params);

    // Format-specific types

    struct video_raw_parameters;
    struct audio_L_parameters;
    struct video_smpte291_parameters;
    struct video_SMPTE2022_6_parameters;

    // sdp_parameters represents the additional (non-transport) SDP parameters.
    // It does not cover the case of multiple media types in a single SDP file because NMOS associates an SDP file
    // with each RTP sender and receiver.
    // When redundancy is being used, the media description and media-level attributes for each stream are assumed
    // to be identical except for the values corresponding to the IS-05 transport parameters for each leg.
    struct sdp_parameters
    {
        // Origin ("o=")
        // See https://tools.ietf.org/html/rfc4566#section-5.2
        struct origin_t
        {
            utility::string_t user_name;
            uint64_t session_id;
            uint64_t session_version;

            origin_t() : session_id(), session_version() {}
            origin_t(const utility::string_t& user_name, uint64_t session_id, uint64_t session_version)
                : user_name(user_name)
                , session_id(session_id)
                , session_version(session_version)
            {}
            origin_t(const utility::string_t& user_name, uint64_t session_id_version)
                : user_name(user_name)
                , session_id(session_id_version)
                , session_version(session_id_version)
            {}
        } origin;

        // Session Name ("s=")
        // See https://tools.ietf.org/html/rfc4566#section-5.3
        utility::string_t session_name;

        // Connection Data ("c=")
        // See https://tools.ietf.org/html/rfc4566#section-5.7
        struct connection_data_t
        {
            // most fields in the "c=" line have corresponding values in the IS-05 transport parameters
            uint32_t ttl;

            connection_data_t() : ttl() {}
            connection_data_t(uint32_t ttl) : ttl(ttl) {}
        } connection_data;

        // Timing ("t=")
        // See https://tools.ietf.org/html/rfc4566#section-5.9
        struct timing_t
        {
            uint64_t start_time;
            uint64_t stop_time;
            timing_t() : start_time(), stop_time() {}
            timing_t(uint64_t start_time, uint64_t stop_time) : start_time(start_time), stop_time(stop_time) {}
        } timing;

        // Grouping Framework ("a=mid:" and "a=group:")
        // See https://tools.ietf.org/html/rfc5888
        struct group_t
        {
            sdp::group_semantics_type semantics;
            // stream identifiers for each leg when redundancy is being used, in the appropriate order
            std::vector<utility::string_t> media_stream_ids;

            group_t() {}
            group_t(const sdp::group_semantics_type& semantics, const std::vector<utility::string_t>& media_stream_ids) : semantics(semantics), media_stream_ids(media_stream_ids) {}
        } group;

        // Media ("m=")
        // See https://tools.ietf.org/html/rfc4566#section-5.14
        struct media_t
        {
            sdp::media_type media_type;
            sdp::protocol protocol;
            // formats are specified by the RTP payload mapping

            media_t() {}
            media_t(const sdp::media_type& media_type, const sdp::protocol& protocol) : media_type(media_type), protocol(protocol) {}
        } media;

        // Bandwidth ("b=") (e.g. for "video/jxsv")
        // See https://tools.ietf.org/html/rfc4566#section-5.8
        struct bandwidth_t
        {
            sdp::bandwidth_type bandwidth_type;
            uint64_t bandwidth;

            bandwidth_t() : bandwidth() {}
            bandwidth_t(const sdp::bandwidth_type& bandwidth_type, uint64_t bandwidth) : bandwidth_type(bandwidth_type), bandwidth(bandwidth) {}
        };
        std::vector<bandwidth_t> bandwidth;

        // Packet Time ("a=ptime:") (e.g. for "audio/L16")
        // See https://tools.ietf.org/html/rfc4566#section-6
        double packet_time;

        // Maximum Packet Time ("a=maxptime:") (e.g. for "audio/L16")
        // See https://tools.ietf.org/html/rfc4566#section-6
        double max_packet_time;

        // RTP Payload Mapping ("a=rtpmap:")
        // See https://tools.ietf.org/html/rfc4566#section-6
        struct rtpmap_t
        {
            uint64_t payload_type;
            // encoding-name is the media subtype
            // e.g. "raw" for video, "L24" or "L16" for audio, "smpte291" for data, "SMPTE2022-6" for mux
            utility::string_t encoding_name;
            uint64_t clock_rate;
            // encoding-parameters optionally indicates channel count for audio
            uint64_t encoding_parameters;

            rtpmap_t() : payload_type(), clock_rate(), encoding_parameters() {}
            rtpmap_t(uint64_t payload_type, const utility::string_t& encoding_name, uint64_t clock_rate, uint64_t encoding_parameters = {})
                : payload_type(payload_type)
                , encoding_name(encoding_name)
                , clock_rate(clock_rate)
                , encoding_parameters(encoding_parameters)
            {}
        } rtpmap;

        // Frame Rate ("a=framerate:") (e.g. for "video/SMPTE2022-6")
        // See https://tools.ietf.org/html/rfc4566#section-6
        double framerate;

        // Format-specific Parameters ("a=fmtp:")
        // The payload type is specified by the RTP payload mapping
        // See https://tools.ietf.org/html/rfc4566#section-6
        typedef std::vector<std::pair<utility::string_t, utility::string_t>> fmtp_t;
        fmtp_t fmtp;

        // For now, only the default payload format is covered.
        //std::vector<std::pair<rtpmap_t, fmtp_t>> alternative_rtpmap_fmtp;
        
        // Timestamp Reference Clock Source Signalling ("a=ts-refclk:")
        // See https://tools.ietf.org/html/rfc7273#section-4
        struct ts_refclk_t
        {
            sdp::ts_refclk_source clock_source;
            // for sdp::ts_refclk_sources::ptp
            sdp::ptp_version ptp_version;
            utility::string_t ptp_server;
            // for sdp::ts_refclk_sources::local_mac
            utility::string_t mac_address;

            // ptp-server = ptp-gmid [":" ptp-domain]
            static ts_refclk_t ptp(const sdp::ptp_version& ptp_version, const utility::string_t& ptp_server)
            {
                return{ sdp::ts_refclk_sources::ptp, ptp_version, ptp_server, {} };
            }
            // ptp-server = ptp-gmid [":" ptp-domain]
            static ts_refclk_t ptp(const utility::string_t& ptp_server)
            {
                return{ sdp::ts_refclk_sources::ptp, sdp::ptp_versions::IEEE1588_2008, ptp_server, {} };
            }
            // ptp-server = "traceable"
            static ts_refclk_t ptp_traceable(const sdp::ptp_version& ptp_version = sdp::ptp_versions::IEEE1588_2008)
            {
                return{ sdp::ts_refclk_sources::ptp, ptp_version, {}, {} };
            }
            static ts_refclk_t local_mac(const utility::string_t& mac_address)
            {
                return{ sdp::ts_refclk_sources::local_mac, {}, {}, mac_address };
            }
            ts_refclk_t() {}
            ts_refclk_t(const sdp::ts_refclk_source& clock_source, const sdp::ptp_version& ptp_version, const utility::string_t& ptp_server, const utility::string_t& mac_address)
                : clock_source(clock_source)
                , ptp_version(ptp_version)
                , ptp_server(ptp_server)
                , mac_address(mac_address)
            {}
        };
        std::vector<sdp_parameters::ts_refclk_t> ts_refclk; // hm, this is one for each leg

        // Media Clock Source Signalling
        // See https://tools.ietf.org/html/rfc7273#section-5
        struct mediaclk_t
        {
            sdp::mediaclk_source clock_source;
            utility::string_t clock_parameters;

            mediaclk_t() {}
            mediaclk_t(const sdp::mediaclk_source& clock_source, const utility::string_t& clock_parameters = {})
                : clock_source(clock_source)
                , clock_parameters(clock_parameters)
            {}
        } mediaclk;

        // construct null SDP parameters
        sdp_parameters() : packet_time(), max_packet_time(), framerate() {}

        // construct SDP parameters with sensible defaults for unspecified fields
        sdp_parameters(const utility::string_t& session_name, const sdp::media_type& media_type, const rtpmap_t& rtpmap, const fmtp_t& fmtp = {}, uint64_t bandwidth = {}, double packet_time = {}, double max_packet_time = {}, double framerate = {}, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<ts_refclk_t>& ts_refclk = {})
            : origin(U("-"), sdp::ntp_now() >> 32)
            , session_name(session_name)
            , connection_data(32)
            , timing()
            , group(!media_stream_ids.empty() ? group_t{ sdp::group_semantics::duplication, media_stream_ids } : group_t{})
            , media(media_type, sdp::protocols::RTP_AVP)
            , bandwidth(0 != bandwidth ? std::vector<bandwidth_t>{ bandwidth_t{ sdp::bandwidth_types::application_specific, bandwidth } } : std::vector<bandwidth_t>{})
            , packet_time(packet_time)
            , max_packet_time(max_packet_time)
            , rtpmap(rtpmap)
            , framerate(framerate)
            , fmtp(fmtp)
            , ts_refclk(ts_refclk)
            , mediaclk(sdp::mediaclk_sources::direct, U("0"))
        {}

        // deprecated, provided to slightly simplify updating code to use fmtp
        typedef video_raw_parameters video_t;
        typedef audio_L_parameters audio_t;
        typedef video_smpte291_parameters data_t;
        typedef video_SMPTE2022_6_parameters mux_t;

        // deprecated, construct "video/raw" SDP parameters with sensible defaults for unspecified fields
        sdp_parameters(const utility::string_t& session_name, const video_t& video, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<ts_refclk_t>& ts_refclk = {});

        // deprecated, construct "audio/L" SDP parameters with sensible defaults for unspecified fields
        sdp_parameters(const utility::string_t& session_name, const audio_t& audio, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<ts_refclk_t>& ts_refclk = {});

        // deprecated, construct "video/smpte291" SDP parameters with sensible defaults for unspecified fields
        sdp_parameters(const utility::string_t& session_name, const data_t& data, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<ts_refclk_t>& ts_refclk = {});

        // deprecated, construct "video/SMPTE2022-6" SDP parameters with sensible defaults for unspecified fields
        sdp_parameters(const utility::string_t& session_name, const mux_t& mux, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<ts_refclk_t>& ts_refclk = {});
    };

    // Format-specific types and functions

    // Additional "video/raw" parameters
    // See SMPTE ST 2110-20:2017
    // and https://www.iana.org/assignments/media-types/video/raw
    struct video_raw_parameters
    {
        // fmtp indicates format
        uint32_t width;
        uint32_t height;
        nmos::rational exactframerate;
        bool interlace;
        bool segmented;
        sdp::sampling sampling;
        uint32_t depth;
        sdp::transfer_characteristic_system tcs; // nmos::transfer_characteristic is a subset
        sdp::colorimetry colorimetry; // nmos::colorspace is a subset
        sdp::type_parameter tp;

        video_raw_parameters() : width(), height(), interlace(), segmented(), depth() {}
        video_raw_parameters(uint32_t width, uint32_t height, const nmos::rational& exactframerate, bool interlace, bool segmented, const sdp::sampling& sampling, uint32_t depth, const sdp::transfer_characteristic_system& tcs, const sdp::colorimetry& colorimetry, const sdp::type_parameter& tp)
            : width(width)
            , height(height)
            , exactframerate(exactframerate)
            , interlace(interlace)
            , segmented(segmented)
            , sampling(sampling)
            , depth(depth)
            , tcs(tcs)
            , colorimetry(colorimetry)
            , tp(tp)
        {}
    };

    // Additional "audio/L" parameters
    // See SMPTE ST 2110-30:2017
    // and https://www.iana.org/assignments/media-types/audio/L24
    // and https://www.iana.org/assignments/media-types/audio/L16
    // and https://www.iana.org/assignments/media-types/audio/L8
    struct audio_L_parameters
    {
        // rtpmap encoding-parameters indicates channel_count
        uint32_t channel_count;
        // rtpmap encoding-name (e.g. "L24") indicates bit_depth
        uint32_t bit_depth;
        // rtpmap clock-rate indicates sample_rate
        uint64_t sample_rate;

        // fmtp indicates channel-order (e.g. "SMPTE2110.(ST)")
        utility::string_t channel_order;

        // ptime
        double packet_time;

        audio_L_parameters() : channel_count(), bit_depth(), sample_rate(), packet_time() {}
        audio_L_parameters(uint32_t channel_count, uint32_t bit_depth, uint64_t sample_rate, const utility::string_t& channel_order, double packet_time)
            : channel_count(channel_count)
            , bit_depth(bit_depth)
            , sample_rate(sample_rate)
            , channel_order(channel_order)
            , packet_time(packet_time)
        {}
    };

    // Additional "video/smpte291" data payload parameters
    // See SMPTE ST 2110-40:2018
    // and https://www.iana.org/assignments/media-types/video/smpte291
    // and https://tools.ietf.org/html/rfc8331
    struct video_smpte291_parameters
    {
        // fmtp optionally indicates multiple DID_SDID parameters
        std::vector<nmos::did_sdid> did_sdids;
        // fmtp optionally indicates VPID Code of the source interface
        nmos::vpid_code vpid_code;

        video_smpte291_parameters(const std::vector<nmos::did_sdid>& did_sdids = {}, nmos::vpid_code vpid_code = {})
            : did_sdids(did_sdids)
            , vpid_code(vpid_code)
        {}
    };

    // Additional "video/SMPTE2022-6" multiplexed payload parameters
    // See SMPTE ST 2022-8:2019
    struct video_SMPTE2022_6_parameters
    {
        sdp::type_parameter tp;

        video_SMPTE2022_6_parameters() {}
        video_SMPTE2022_6_parameters(const sdp::type_parameter& tp)
            : tp(tp)
        {}
    };

    void set_video_raw_parameters(sdp_parameters& sdp_params, const video_raw_parameters& params, uint64_t payload_type);
    video_raw_parameters get_video_raw_parameters(const sdp_parameters& sdp_params);

    void set_audio_L_parameters(sdp_parameters& sdp_params, const audio_L_parameters& params, uint64_t payload_type);
    audio_L_parameters get_audio_L_parameters(const sdp_parameters& sdp_params);

    void set_video_smpte291_parameters(sdp_parameters& sdp_params, const video_smpte291_parameters& params, uint64_t payload_type);
    video_smpte291_parameters get_video_smpte291_parameters(const sdp_parameters& sdp_params);

    void set_video_SMPTE2022_6_parameters(sdp_parameters& sdp_params, const video_SMPTE2022_6_parameters& params, uint64_t payload_type);
    video_SMPTE2022_6_parameters get_video_SMPTE2022_6_parameters(const sdp_parameters& sdp_params);

    // deprecated, construct "video/raw" SDP parameters with sensible defaults for unspecified fields
    inline sdp_parameters::sdp_parameters(const utility::string_t& session_name, const video_t& video, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<ts_refclk_t>& ts_refclk)
        : sdp_parameters(session_name, sdp::media_types::video, {}, {}, {}, {}, {}, {}, media_stream_ids, ts_refclk)
    {
        set_video_raw_parameters(*this, video, payload_type);
    }

    // deprecated, construct "audio/L" SDP parameters with sensible defaults for unspecified fields
    inline sdp_parameters::sdp_parameters(const utility::string_t& session_name, const audio_t& audio, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<ts_refclk_t>& ts_refclk)
        : sdp_parameters(session_name, sdp::media_types::audio, {}, {}, {}, {}, {}, {}, media_stream_ids, ts_refclk)
    {
        set_audio_L_parameters(*this, audio, payload_type);
    }

    // deprecated, construct "video/smpte291" SDP parameters with sensible defaults for unspecified fields
    inline sdp_parameters::sdp_parameters(const utility::string_t& session_name, const data_t& data, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<ts_refclk_t>& ts_refclk)
        : sdp_parameters(session_name, sdp::media_types::video, {}, {}, {}, {}, {}, {}, media_stream_ids, ts_refclk)
    {
        set_video_smpte291_parameters(*this, data, payload_type);
    }

    // deprecated, construct "video/SMPTE2022-6" SDP parameters with sensible defaults for unspecified fields
    inline sdp_parameters::sdp_parameters(const utility::string_t& session_name, const mux_t& mux, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<ts_refclk_t>& ts_refclk)
        : sdp_parameters(session_name, sdp::media_types::video, {}, {}, {}, {}, {}, {}, media_stream_ids, ts_refclk)
    {
        set_video_SMPTE2022_6_parameters(*this, mux, payload_type);
    }
}

#endif

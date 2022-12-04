#ifndef NMOS_SDP_UTILS_H
#define NMOS_SDP_UTILS_H

#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include "bst/any.h"
#include "bst/optional.h"
#include "sdp/json.h"
#include "sdp/ntp.h"
#include "nmos/did_sdid.h"
#include "nmos/rational.h"
#include "nmos/vpid_code.h"

namespace nmos
{
    struct format;
    struct media_type;

    struct sdp_parameters; // defined below

    // Sender helper functions

    web::json::value make_components(const sdp::sampling& sampling, uint32_t width, uint32_t height, uint32_t depth);

    // Construct SDP parameters from the IS-04 resources for "video/raw", "audio/L", "video/smpte291" and "video/SMPTE2022-6"
    // using default values for unspecified items

    // deprecated, use format-specific make_<media type/subtype>_parameters and then make_<media type/subtype>_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain);
    // deprecated, provided for backwards compatibility, because it may be necessary to also specify the PTP domain to generate an RFC 7273 'ts-refclk' attribute that meets the additional constraints of ST 2110-10
    sdp_parameters make_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids);

    // Construct SDP parameters for the specified format from the IS-04 resources, using default values for unspecified items

    // deprecated, use make_video_raw_parameters and then make_video_raw_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_video_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<sdp::type_parameter> tp);
    // deprecated, use make_audio_L_parameters and then make_audio_L_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_audio_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<double> packet_time);
    // deprecated, use make_video_smpte291_parameters and then make_video_smpte291_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_data_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<nmos::vpid_code> vpid_code);
    // deprecated, use make_video_SMPTE2022_6_parameters and then make_video_SMPTE2022_6_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_mux_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<sdp::type_parameter> tp);

    // Sender/Receiver helper functions

    // Make a json representation of an SDP file, e.g. for sdp::make_session_description, from the specified parameters; explicitly specify whether 'source-filter' attributes are included to override the default behaviour
    web::json::value make_session_description(const sdp_parameters& sdp_params, const web::json::value& transport_params, bst::optional<bool> source_filters = bst::nullopt);

    // Receiver helper functions

    // Get IS-05 transport parameters from the json representation of an SDP file, e.g. from sdp::parse_session_description
    web::json::value get_session_description_transport_params(const web::json::value& session_description);

    // Get the additional (non-transport) SDP parameters from the json representation of an SDP file, e.g. from sdp::parse_session_description
    sdp_parameters get_session_description_sdp_parameters(const web::json::value& session_description);

    // Get SDP parameters from the json representation of an SDP file, e.g. from sdp::parse_session_description
    std::pair<sdp_parameters, web::json::value> parse_session_description(const web::json::value& session_description);

    // Validate the SDP parameters against a receiver for "video/raw", "audio/L", "video/smpte291" or "video/SMPTE2022-6"
    void validate_sdp_parameters(const web::json::value& receiver, const sdp_parameters& sdp_params);

    // Check the specified SDP parameters against the specified constraint sets
    // for "video/raw", "audio/L", "video/smpte291" or "video/SMPTE2022-6"
    bool match_sdp_parameters_constraint_sets(const web::json::array& constraint_sets, const sdp_parameters& sdp_params);

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
        sdp::media_type media_type;
        sdp::protocol protocol;

        // Bandwidth ("b=") (e.g. for "video/jxsv")
        // See https://tools.ietf.org/html/rfc4566#section-5.8
        struct bandwidth_t
        {
            sdp::bandwidth_type bandwidth_type;
            uint64_t bandwidth;

            bandwidth_t() : bandwidth() {}
            bandwidth_t(const sdp::bandwidth_type& bandwidth_type, uint64_t bandwidth) : bandwidth_type(bandwidth_type), bandwidth(bandwidth) {}
        } bandwidth;

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
            , media_type(media_type)
            , protocol(sdp::protocols::RTP_AVP)
            , bandwidth(0 != bandwidth ? bandwidth_t{ sdp::bandwidth_types::application_specific, bandwidth } : bandwidth_t{})
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

        // deprecated, use make_video_raw_sdp_parameters or equivalent overload of make_sdp_parameters
        sdp_parameters(const utility::string_t& session_name, const video_t& video, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<ts_refclk_t>& ts_refclk = {});

        // deprecated, use make_audio_L_sdp_parameters or equivalent overload of make_sdp_parameters
        sdp_parameters(const utility::string_t& session_name, const audio_t& audio, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<ts_refclk_t>& ts_refclk = {});

        // deprecated, use make_video_smpte291_sdp_parameters or equivalent overload of make_sdp_parameters
        sdp_parameters(const utility::string_t& session_name, const data_t& data, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<ts_refclk_t>& ts_refclk = {});

        // deprecated, use make_video_SMPTE2022_6_sdp_parameters or equivalent overload of make_sdp_parameters
        sdp_parameters(const utility::string_t& session_name, const mux_t& mux, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<ts_refclk_t>& ts_refclk = {});
    };

    // "<sdp_params.media_type>/<sdp_params.rtpmap.encoding_name>"
    media_type get_media_type(const sdp_parameters& sdp_params);

    // Format-specific types and functions

    // Additional "video/raw" parameters
    // See SMPTE ST 2110-20:2022
    // and SMPTE ST 2110-10:2022
    // and SMPTE ST 2110-21:2022
    // and https://www.iana.org/assignments/media-types/video/raw
    // and https://tools.ietf.org/html/rfc4175
    struct video_raw_parameters
    {
        // fmtp indicates format
        sdp::sampling sampling;
        uint32_t depth;
        uint32_t width;
        uint32_t height;
        nmos::rational exactframerate;
        bool interlace;
        bool segmented;
        sdp::transfer_characteristic_system tcs; // if omitted (empty), assume sdp::transfer_characteristic_systems::SDR; nmos::transfer_characteristic is compatible
        sdp::colorimetry colorimetry; // nmos::colorspace is compatible
        sdp::range range; // if omitted (empty), assume sdp::ranges::NARROW
        nmos::rational par; // if omitted (zero), assume 1:1
        sdp::packing_mode pm;
        sdp::smpte_standard_number ssn;

        // additional fmtp parameters from ST 2110-21:2022
        sdp::type_parameter tp;
        uint32_t troff; // if omitted (zero), assume default
        uint32_t cmax; // if omitted (zero), assume max defined for tp

        // additional fmtp parameters from ST 2110-10:2022
        uint32_t maxudp; // if omitted (zero), assume the Standard UP Size Limit
        sdp::timestamp_mode tsmode; // if omitted (empty), assume sdp::timestamp_modes::NEW
        uint32_t tsdelay;

        video_raw_parameters() : depth(), width(), height(), interlace(), segmented(), troff(), cmax(), maxudp(), tsdelay() {}

        video_raw_parameters(
            sdp::sampling sampling,
            uint32_t depth,
            uint32_t width,
            uint32_t height,
            nmos::rational exactframerate,
            bool interlace,
            bool segmented,
            sdp::transfer_characteristic_system tcs,
            sdp::colorimetry colorimetry,
            sdp::range range,
            nmos::rational par,
            sdp::packing_mode pm,
            sdp::smpte_standard_number ssn,
            sdp::type_parameter tp,
            uint32_t troff,
            uint32_t cmax,
            uint32_t maxudp,
            sdp::timestamp_mode tsmode,
            uint32_t tsdelay
        )
            : sampling(std::move(sampling))
            , depth(depth)
            , width(width)
            , height(height)
            , exactframerate(exactframerate)
            , interlace(interlace)
            , segmented(segmented)
            , tcs(std::move(tcs))
            , colorimetry(std::move(colorimetry))
            , range(std::move(range))
            , par(par)
            , pm(std::move(pm))
            , ssn(std::move(ssn))
            , tp(std::move(tp))
            , troff(troff)
            , cmax(cmax)
            , maxudp(maxudp)
            , tsmode(tsmode)
            , tsdelay(tsdelay)
        {}

        // deprecated
        video_raw_parameters(uint32_t width, uint32_t height, const nmos::rational& exactframerate, bool interlace, bool segmented, const sdp::sampling& sampling, uint32_t depth, const sdp::transfer_characteristic_system& tcs, const sdp::colorimetry& colorimetry, const sdp::type_parameter& tp)
            : video_raw_parameters(sampling, depth, width, height, exactframerate, interlace, segmented, tcs, colorimetry, {}, {}, sdp::packing_modes::general, sdp::smpte_standard_numbers::ST2110_20_2017, tp, {}, {}, {}, {}, {})
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

        // additional fmtp parameters from ST 2110-10:2022
        sdp::timestamp_mode tsmode; // if omitted (empty), assume sdp::timestamp_modes::NEW
        uint32_t tsdelay;

        // ptime
        double packet_time;

        audio_L_parameters() : channel_count(), bit_depth(), sample_rate(), tsdelay(), packet_time() {}

        audio_L_parameters(
            uint32_t channel_count,
            uint32_t bit_depth,
            uint64_t sample_rate,
            utility::string_t channel_order,
            sdp::timestamp_mode tsmode,
            uint32_t tsdelay,
            double packet_time
        )
            : channel_count(channel_count)
            , bit_depth(bit_depth)
            , sample_rate(sample_rate)
            , channel_order(std::move(channel_order))
            , tsmode(std::move(tsmode))
            , tsdelay(tsdelay)
            , packet_time(packet_time)
        {}

        // deprecated
        audio_L_parameters(uint32_t channel_count, uint32_t bit_depth, uint64_t sample_rate, const utility::string_t& channel_order, double packet_time)
            : audio_L_parameters(channel_count, bit_depth, sample_rate, channel_order, {}, {}, packet_time)
        {}
    };

    // Additional "video/smpte291" data payload parameters
    // See SMPTE ST 2110-40:2022
    // and https://www.iana.org/assignments/media-types/video/smpte291
    // and https://tools.ietf.org/html/rfc8331
    struct video_smpte291_parameters
    {
        // fmtp optionally indicates multiple DID_SDID parameters
        std::vector<nmos::did_sdid> did_sdids;
        // fmtp optionally indicates VPID Code of the source interface
        nmos::vpid_code vpid_code;
        // fmtp is required to indicate frame rate, since ST 2110-40:2022
        nmos::rational exactframerate;
        // fmtp optionally indicates TM, since ST 2110-40:2022
        sdp::transmission_model tm; // if omitted (empty), assume sdp::transmission_models::CTM
        // fmtp is required to indicate SSN, since ST 2110-40:2022
        sdp::smpte_standard_number ssn;

        // additional fmtp parameters from ST 2110-21:2022
        uint32_t troff; // if omitted (zero), assume default

        // additional fmtp parameters from ST 2110-10:2022
        sdp::timestamp_mode tsmode; // if omitted (empty), assume sdp::timestamp_modes::NEW
        uint32_t tsdelay;

        video_smpte291_parameters() : vpid_code(), troff(), tsdelay() {}

        video_smpte291_parameters(
            std::vector<nmos::did_sdid> did_sdids,
            nmos::vpid_code vpid_code,
            nmos::rational exactframerate,
            sdp::transmission_model tm,
            sdp::smpte_standard_number ssn,
            uint32_t troff,
            sdp::timestamp_mode tsmode,
            uint32_t tsdelay
        )
            : did_sdids(std::move(did_sdids))
            , vpid_code(vpid_code)
            , exactframerate(exactframerate)
            , tm(std::move(tm))
            , ssn(std::move(ssn))
            , troff(troff)
            , tsmode(std::move(tsmode))
            , tsdelay(tsdelay)
        {}

        // deprecated
        video_smpte291_parameters(const std::vector<nmos::did_sdid>& did_sdids, const nmos::vpid_code& vpid_code = {})
            : video_smpte291_parameters(did_sdids, vpid_code, {}, {}, {}, {}, {}, {})
        {}
    };

    // Additional "video/SMPTE2022-6" multiplexed payload parameters
    // See SMPTE ST 2022-8:2019
    struct video_SMPTE2022_6_parameters
    {
        // additional fmtp parameters from ST 2110-21:2017
        sdp::type_parameter tp;
        uint32_t troff; // if omitted (zero), assume default

        video_SMPTE2022_6_parameters() : troff() {}

        video_SMPTE2022_6_parameters(
            sdp::type_parameter tp,
            uint32_t troff
        )
            : tp(std::move(tp))
            , troff(troff)
        {}

        // deprecated
        video_SMPTE2022_6_parameters(const sdp::type_parameter& tp)
            : video_SMPTE2022_6_parameters(tp, {})
        {}
    };

    // Construct additional "video/raw" parameters from the IS-04 resources, using default values for unspecified items
    video_raw_parameters make_video_raw_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<sdp::type_parameter> tp);
    // Construct SDP parameters for "video/raw", with sensible defaults for unspecified fields
    sdp_parameters make_video_raw_sdp_parameters(const utility::string_t& session_name, const video_raw_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {});
    // Get additional "video/raw" parameters from the SDP parameters
    video_raw_parameters get_video_raw_parameters(const sdp_parameters& sdp_params);

    // Construct additional "audio/L" parameters from the IS-04 resources, using default values for unspecified items
    audio_L_parameters make_audio_L_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<double> packet_time);
    // Construct SDP parameters for "audio/L", with sensible defaults for unspecified fields
    sdp_parameters make_audio_L_sdp_parameters(const utility::string_t& session_name, const audio_L_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {});
    // Get additional "audio/L" parameters from the SDP parameters
    audio_L_parameters get_audio_L_parameters(const sdp_parameters& sdp_params);

    // Construct additional "video/smpte291" parameters from the IS-04 resources, using default values for unspecified items
    video_smpte291_parameters make_video_smpte291_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<nmos::vpid_code> vpid_code, bst::optional<sdp::transmission_model> tm = bst::nullopt);
    // Construct SDP parameters for "video/smpte291", with sensible defaults for unspecified fields
    sdp_parameters make_video_smpte291_sdp_parameters(const utility::string_t& session_name, const video_smpte291_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {});
    // Get additional "video/smpte291" parameters from the SDP parameters
    video_smpte291_parameters get_video_smpte291_parameters(const sdp_parameters& sdp_params);

    // Construct additional "video/SMPTE2022-6" parameters from the IS-04 resources, using default values for unspecified items
    video_SMPTE2022_6_parameters make_video_SMPTE2022_6_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<sdp::type_parameter> tp);
    // Construct SDP parameters for "video/SMPTE2022-6", with sensible defaults for unspecified fields
    sdp_parameters make_video_SMPTE2022_6_sdp_parameters(const utility::string_t& session_name, const video_SMPTE2022_6_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {});
    // Get additional "video/SMPTE2022-6" parameters from the SDP parameters
    video_SMPTE2022_6_parameters get_video_SMPTE2022_6_parameters(const sdp_parameters& sdp_params);

    // Construct SDP parameters for "video/raw", with sensible defaults for unspecified fields
    inline sdp_parameters make_sdp_parameters(const utility::string_t& session_name, const video_raw_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {})
    {
        return make_video_raw_sdp_parameters(session_name, params, payload_type, media_stream_ids, ts_refclk);
    }
    // Construct SDP parameters for "audio/L", with sensible defaults for unspecified fields
    inline sdp_parameters make_sdp_parameters(const utility::string_t& session_name, const audio_L_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {})
    {
        return make_audio_L_sdp_parameters(session_name, params, payload_type, media_stream_ids, ts_refclk);
    }
    // Construct SDP parameters for "video/smpte291", with sensible defaults for unspecified fields
    inline sdp_parameters make_sdp_parameters(const utility::string_t& session_name, const video_smpte291_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {})
    {
        return make_video_smpte291_sdp_parameters(session_name, params, payload_type, media_stream_ids, ts_refclk);
    }
    // Construct SDP parameters for "video/SMPTE2022-6", with sensible defaults for unspecified fields
    inline sdp_parameters make_sdp_parameters(const utility::string_t& session_name, const video_SMPTE2022_6_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {})
    {
        return make_video_SMPTE2022_6_sdp_parameters(session_name, params, payload_type, media_stream_ids, ts_refclk);
    }

    // deprecated, use make_video_raw_sdp_parameters or equivalent overload of make_sdp_parameters
    inline sdp_parameters::sdp_parameters(const utility::string_t& session_name, const video_t& video, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<ts_refclk_t>& ts_refclk)
        : sdp_parameters(make_sdp_parameters(session_name, video, payload_type, media_stream_ids, ts_refclk))
    {}

    // deprecated, use make_audio_L_sdp_parameters or equivalent overload of make_sdp_parameters
    inline sdp_parameters::sdp_parameters(const utility::string_t& session_name, const audio_t& audio, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<ts_refclk_t>& ts_refclk)
        : sdp_parameters(make_sdp_parameters(session_name, audio, payload_type, media_stream_ids, ts_refclk))
    {}

    // deprecated, use make_video_smpte291_sdp_parameters or equivalent overload of make_sdp_parameters
    inline sdp_parameters::sdp_parameters(const utility::string_t& session_name, const data_t& data, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<ts_refclk_t>& ts_refclk)
        : sdp_parameters(make_sdp_parameters(session_name, data, payload_type, media_stream_ids, ts_refclk))
    {}

    // deprecated, use make_video_SMPTE2022_6_sdp_parameters or equivalent overload of make_sdp_parameters
    inline sdp_parameters::sdp_parameters(const utility::string_t& session_name, const mux_t& mux, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<ts_refclk_t>& ts_refclk)
        : sdp_parameters(make_sdp_parameters(session_name, mux, payload_type, media_stream_ids, ts_refclk))
    {}

    // Helper functions for implementing format-specific functions
    namespace details
    {
        inline std::logic_error sdp_creation_error(const std::string& message)
        {
            return std::logic_error{ "sdp creation error - " + message };
        }

        inline std::runtime_error sdp_processing_error(const std::string& message)
        {
            return std::runtime_error{ "sdp processing error - " + message };
        }

        inline sdp_parameters::fmtp_t::const_iterator find_fmtp(const sdp_parameters::fmtp_t& fmtp, const utility::string_t& name)
        {
            return std::find_if(fmtp.begin(), fmtp.end(), [&](const sdp_parameters::fmtp_t::value_type& param)
            {
                return param.first == name;
            });
        }
        inline sdp_parameters::fmtp_t::iterator find_fmtp(sdp_parameters::fmtp_t& fmtp, const utility::string_t& name)
        {
            return std::find_if(fmtp.begin(), fmtp.end(), [&](const sdp_parameters::fmtp_t::value_type& param)
            {
                return param.first == name;
            });
        }

        // type-erased format-specific parameters
        // e.g. can hold a video_raw_parameters, an audio_L_parameters, etc.
        typedef bst::any format_parameters;

        template <typename FormatParameters>
        inline const FormatParameters* get(const format_parameters* any)
        {
            return bst::any_cast<FormatParameters>(any);
        }

        // a function to check the specified SDP parameters and format-specific parameters
        // against the specified parameter constraint value, see nmos/capabilities.h
        typedef std::function<bool(const sdp_parameters& sdp_params, const format_parameters& format_params, const web::json::value& constraint)> sdp_parameter_constraint;

        // a map from parameter constraint URNs to parameter constraint functions
        typedef std::map<utility::string_t, sdp_parameter_constraint> sdp_parameter_constraints;

        // Check the specified SDP interlace and segmented parameters against the specified interlace_mode constraint
        bool match_interlace_mode_constraint(bool interlace, bool segmented, const web::json::value& constraint);

        // Check the specified SDP parameters and format-specific parameters against the specified constraint set
        // using the specified parameter constraint functions
        bool match_sdp_parameters_constraint_set(const sdp_parameter_constraints& constraints, const sdp_parameters& sdp_params, const format_parameters& format_params, const web::json::value& constraint_set);

        // Validate the specified SDP parameters and format-specific parameters against the specified receiver
        // using the specified parameter constraint functions
        void validate_sdp_parameters(const sdp_parameter_constraints& constraints, const sdp_parameters& sdp_params, const format& format, const format_parameters& format_params, const web::json::value& receiver);

        // Construct ts-refclk attributes for each leg based on the IS-04 resources
        std::vector<sdp_parameters::ts_refclk_t> make_ts_refclk(const web::json::value& node, const web::json::value& source, const web::json::value& sender, bst::optional<int> ptp_domain);

        // Construct simple media stream ids based on the sender's number of legs
        std::vector<utility::string_t> make_media_stream_ids(const web::json::value& sender);

        // cf. nmos::make_components
        sdp::sampling make_sampling(const web::json::array& components);

        // Exact Frame Rate
        // "Integer frame rates shall be signaled as a single decimal number (e.g. "25") whilst non-integer frame rates shall be
        // signaled as a ratio of two integer decimal numbers separated by a "forward-slash" character (e.g. "30000/1001"),
        // utilizing the numerically smallest numerator value possible."
        // See ST 2110-20:2017 Section 7.2 Required Media Type Parameters
        utility::string_t make_exactframerate(const nmos::rational& exactframerate);
        nmos::rational parse_exactframerate(const utility::string_t& exactframerate);

        // Pixel Aspect Ratio
        // "PAR shall be signaled as a ratio of two integer decimal numbers separated by a "colon" character (e.g. "12:11")."
        // See ST 2110-20:2017 Section 7.3 Media Type Parameters with default values
        utility::string_t make_pixel_aspect_ratio(const nmos::rational& par);
        nmos::rational parse_pixel_aspect_ratio(const utility::string_t& par);

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
}

#endif

#ifndef NMOS_VIDEO_H264_H
#define NMOS_VIDEO_H264_H

#include "nmos/media_type.h"
#include "nmos/sdp_utils.h"

namespace sdp
{
    // Packetization Mode
    // See https://www.iana.org/assignments/media-types/video/H264
    // and https://tools.ietf.org/html/rfc6184#section-6
    enum packetization_mode
    {
        // Single NAL Unit Mode
        single_nal_unit_mode = 0,
        // Non-Interleaved Mode
        non_interleaved_mode = 1,
        // Interleaved Mode
        interleaved_mode = 2
    };

    namespace fields
    {
        // See https://www.iana.org/assignments/media-types/video/H264
        // and https://tools.ietf.org/html/rfc6184#section-6
        const web::json::field_as_string_or profile_level_id{ U("profile-level-id"), {} }; // hex
        const web::json::field_with_default<uint32_t> packetization_mode{ U("packetization-mode"), (uint32_t)single_nal_unit_mode };
        const web::json::field_as_string_or sprop_parameter_sets{ U("sprop-parameter-sets"), {} };
    }
}

namespace nmos
{
    namespace media_types
    {
        // H.264 Video
        // See https://www.iana.org/assignments/media-types/video/H264
        // and https://tools.ietf.org/html/rfc6184
        const media_type video_H264{ U("video/H264") };
    }

    // Packetization Mode
    // See [TBC in NMOS Parameter Registers]
    // and https://www.iana.org/assignments/media-types/video/H264
    // and https://tools.ietf.org/html/rfc6184#section-6
    DEFINE_STRING_ENUM(packetization_mode)
    namespace packetization_modes
    {
        // Single NAL Unit Mode
        const packetization_mode single_nal_unit_mode{ U("single_nal_unit_mode") };
        // Non-Interleaved Mode
        const packetization_mode non_interleaved_mode{ U("non_interleaved_mode") };
        // Interleaved Mode
        const packetization_mode interleaved_mode{ U("interleaved_mode") };
    }

    sdp::packetization_mode make_sdp_packetization_mode(const packetization_mode& packetization_mode);
    packetization_mode parse_sdp_packetization_mode(const sdp::packetization_mode& packetization_mode);

    namespace fields
    {
        // See [TBC in NMOS Parameter Registers]

        // flow_video_coded
        const web::json::field<uint32_t> profile_level_id{ U("profile_level_id") };
        // sender
        const web::json::field_as_string packetization_mode{ U("packetization_mode") };
    }

    namespace caps
    {
        // See [TBC in NMOS Parameter Registers]

        namespace format
        {
            const web::json::field_as_value_or profile_level_id{ U("urn:x-nmos:cap:format:profile_level_id"), {} }; // number
        }

        namespace transport
        {
            const web::json::field_as_value_or packetization_mode{ U("urn:x-nmos:cap:transport:packetization_mode"), {} }; // number
        }
    }

    // Additional "video/H264" parameters
    // See https://www.iana.org/assignments/media-types/video/H264
    // and https://tools.ietf.org/html/rfc6184
    struct video_H264_parameters
    {
        uint32_t profile_level_id;
        sdp::packetization_mode packetization_mode;
        utility::string_t sprop_parameter_sets;

        video_H264_parameters() : profile_level_id(), packetization_mode(sdp::single_nal_unit_mode) {}
        video_H264_parameters(uint32_t profile_level_id, sdp::packetization_mode packetization_mode, const utility::string_t& sprop_parameter_sets)
            : profile_level_id(profile_level_id)
            , packetization_mode(packetization_mode)
            , sprop_parameter_sets(sprop_parameter_sets)
        {}
    };

    // Construct additional "video/H264" parameters from the IS-04 resources
    video_H264_parameters make_video_H264_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const utility::string_t& sprop_parameter_sets);
    // Construct SDP parameters for "video/H264"
    sdp_parameters make_video_H264_sdp_parameters(const utility::string_t& session_name, const video_H264_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {});
    // Get additional "video/H264" parameters from the SDP parameters
    video_H264_parameters get_video_H264_parameters(const sdp_parameters& sdp_params);

    // Construct SDP parameters for "video/H264"
    inline sdp_parameters make_sdp_parameters(const utility::string_t& session_name, const video_H264_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {})
    {
        return make_video_H264_sdp_parameters(session_name, params, payload_type, media_stream_ids, ts_refclk);
    }
}

#endif

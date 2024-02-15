#ifndef NMOS_VIDEO_JXSV_H
#define NMOS_VIDEO_JXSV_H

#include "nmos/media_type.h"
#include "nmos/node_resources.h"
#include "nmos/sdp_utils.h"

namespace sdp
{
    namespace video_jxsv
    {
        namespace fields
        {
            // See https://www.iana.org/assignments/media-types/video/jxsv
            // and https://tools.ietf.org/html/rfc9134#section-7

            // pacKetization mode (K) bit
            const web::json::field<uint32_t> packetmode{ U("packetmode") }; // cf. sdp::video_jxsv::packetization_mode
            // Transmission mode (T) bit
            const web::json::field_with_default<uint32_t> transmode{ U("transmode"), 1 }; // sequential, cf. sdp::video_jxsv::transmission_mode

            const web::json::field_as_string profile{ U("profile") }; // cf. sdp::video_jxsv::profile
            const web::json::field_as_string level{ U("level") }; // cf. sdp::video_jxsv::level
            const web::json::field_as_string sublevel{ U("sublevel") }; // cf. sdp::video_jxsv::sublevel
        }

        // pacKetization mode (K) bit
        // See https://tools.ietf.org/html/rfc9134
        enum packetization_mode
        {
            codestream = 0,
            slice = 1
        };

        // Transmission mode (T) bit
        // See https://tools.ietf.org/html/rfc9134
        enum transmission_mode
        {
            out_of_order = 0,
            sequential = 1
        };

        // JPEG XS Profile
        // "The JPEG XS profile [ISO21122-2] in use. Any white space Unicode character in the profile name SHALL be omitted."
        // See https://tools.ietf.org/html/rfc9134
        DEFINE_STRING_ENUM(profile)
        namespace profiles
        {
            const profile HighBayer{ U("HighBayer") };
            const profile MainBayer{ U("MainBayer") };
            const profile LightBayer{ U("LightBayer") };
            const profile High4444_12{ U("High4444.12") };
            const profile Main4444_12{ U("Main4444.12") };
            const profile High444_12{ U("High444.12") };
            const profile Main444_12{ U("Main444.12") };
            const profile Light444_12{ U("Light444.12") };
            const profile Main422_10{ U("Main422.10") };
            const profile Light422_10{ U("Light422.10") };
            const profile Light_Subline422_10{ U("Light-Subline422.10") };
            const profile MLS_12{ U("MLS.12") };
            const profile High420_12{ U("High420.12") };
            const profile Main420_12{ U("Main420.12") };
        }

        // JPEG XS Level
        // "The JPEG XS level [ISO21122-2] in use. Any white space Unicode character in the level name SHALL be omitted."
        // See https://tools.ietf.org/html/rfc9134
        DEFINE_STRING_ENUM(level)
        namespace levels
        {
            const level Level1k_1{ U("1k-1") };
            const level Bayer2k_1{ U("Bayer2k-1") };
            const level Level2k_1{ U("2k-1") };
            const level Bayer4k_1{ U("Bayer4k-1") };
            const level Level4k_1{ U("4k-1") };
            const level Bayer8k_1{ U("Bayer8k-1") };
            const level Level4k_2{ U("4k-2") };
            const level Bayer8k_2{ U("Bayer8k-2") };
            const level Level4k_3{ U("4k-3") };
            const level Bayer8k_3{ U("Bayer8k-3") };
            const level Level8k_1{ U("8k-1") };
            const level Bayer16k_1{ U("Bayer16k-1") };
            const level Level8k_2{ U("8k-2") };
            const level Bayer16k_2{ U("Bayer16k-2") };
            const level Level8k_3{ U("8k-3") };
            const level Bayer16k_3{ U("Bayer16k-3") };
            const level Level10k_1{ U("10k-1") };
            const level Bayer20k_1{ U("Bayer20k-1") };
        }

        // Calculate the lowest possible JPEG XS level from the specified frame rate and dimensions
        level get_level(const nmos::rational& frame_rate, uint32_t frame_width, uint32_t frame_height);

        // JPEG XS Sublevel
        // "The JPEG XS sublevel [ISO21122-2] in use. Any white space Unicode character in the sublevel name SHALL be omitted."
        // See https://tools.ietf.org/html/rfc9134
        DEFINE_STRING_ENUM(sublevel)
        namespace sublevels
        {
            const sublevel Full{ U("Full") };
            const sublevel Sublev12bpp{ U("Sublev12bpp") };
            const sublevel Sublev9bpp{ U("Sublev9bpp") };
            const sublevel Sublev6bpp{ U("Sublev6bpp") };
            const sublevel Sublev4bpp{ U("Sublev4bpp") };
            const sublevel Sublev3bpp{ U("Sublev3bpp") };
            const sublevel Sublev2bpp{ U("Sublev2bpp") };
        }
    }
}

namespace nmos
{
    namespace media_types
    {
        // JPEG XS
        // See https://www.iana.org/assignments/media-types/video/jxsv
        // and https://tools.ietf.org/html/rfc9134
        const media_type video_jxsv{ U("video/jxsv") };
    }

    namespace fields
    {
        // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/#bit-rate
        // and https://specs.amwa.tv/nmos-parameter-registers/branches/main/sender-attributes/#bit-rate
        const web::json::field_as_integer bit_rate{ U("bit_rate") };

        // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/#profile
        const web::json::field_as_string profile{ U("profile") };

        // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/#level
        const web::json::field_as_string level{ U("level") };

        // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/#sublevel
        const web::json::field_as_string sublevel{ U("sublevel") };

        // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/sender-attributes/#packet-transmission-mode
        const web::json::field_as_string_or packet_transmission_mode{ U("packet_transmission_mode"), U("codestream") };
    }

    namespace caps
    {
        namespace format
        {
            // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/capabilities/#format-bit-rate
            const web::json::field_as_value_or bit_rate{ U("urn:x-nmos:cap:format:bit_rate"), {} }; // number

            // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/capabilities/#profile
            const web::json::field_as_value_or profile{ U("urn:x-nmos:cap:format:profile"), {} }; // string

            // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/capabilities/#level
            const web::json::field_as_value_or level{ U("urn:x-nmos:cap:format:level"), {} }; // string

            // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/capabilities/#sublevel
            const web::json::field_as_value_or sublevel{ U("urn:x-nmos:cap:format:sublevel"), {} }; // string
        }

        namespace transport
        {
            // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/capabilities/#transport-bit-rate
            const web::json::field_as_value_or bit_rate{ U("urn:x-nmos:cap:transport:bit_rate"), {} }; // number

            // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/capabilities/#packet-transmission-mode
            const web::json::field_as_value_or packet_transmission_mode{ U("urn:x-nmos:cap:transport:packet_transmission_mode"), {} }; // string
        }
    }

    // Profile
    // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/#profile
    DEFINE_STRING_ENUM(profile)
    namespace profiles
    {
        // JPEG XS Profile
        // "The JPEG XS profile [ISO21122-2] in use. Any white space Unicode character in the profile name SHALL be omitted."
        // See https://tools.ietf.org/html/rfc9134
        // and https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/flow_video_jxsv_register.html
        const profile HighBayer{ U("HighBayer") };
        const profile MainBayer{ U("MainBayer") };
        const profile LightBayer{ U("LightBayer") };
        const profile High4444_12{ U("High4444.12") };
        const profile Main4444_12{ U("Main4444.12") };
        const profile High444_12{ U("High444.12") };
        const profile Main444_12{ U("Main444.12") };
        const profile Light444_12{ U("Light444.12") };
        const profile Main422_10{ U("Main422.10") };
        const profile Light422_10{ U("Light422.10") };
        const profile Light_Subline422_10{ U("Light-Subline422.10") };
        const profile MLS_12{ U("MLS.12") };
        const profile High420_12{ U("High420.12") };
        const profile Main420_12{ U("Main420.12") };
    }

    // Level
    // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/#level
    DEFINE_STRING_ENUM(level)
    namespace levels
    {
        // JPEG XS Level
        // "The JPEG XS level [ISO21122-2] in use. Any white space Unicode character in the level name SHALL be omitted."
        // See https://tools.ietf.org/html/rfc9134
        // and https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/flow_video_jxsv_register.html
        const level Level1k_1{ U("1k-1") };
        const level Bayer2k_1{ U("Bayer2k-1") };
        const level Level2k_1{ U("2k-1") };
        const level Bayer4k_1{ U("Bayer4k-1") };
        const level Level4k_1{ U("4k-1") };
        const level Bayer8k_1{ U("Bayer8k-1") };
        const level Level4k_2{ U("4k-2") };
        const level Bayer8k_2{ U("Bayer8k-2") };
        const level Level4k_3{ U("4k-3") };
        const level Bayer8k_3{ U("Bayer8k-3") };
        const level Level8k_1{ U("8k-1") };
        const level Bayer16k_1{ U("Bayer16k-1") };
        const level Level8k_2{ U("8k-2") };
        const level Bayer16k_2{ U("Bayer16k-2") };
        const level Level8k_3{ U("8k-3") };
        const level Bayer16k_3{ U("Bayer16k-3") };
        const level Level10k_1{ U("10k-1") };
        const level Bayer20k_1{ U("Bayer20k-1") };
    }

    // Calculate the lowest possible JPEG XS level from the specified frame rate and dimensions
    inline nmos::level get_video_jxsv_level(const nmos::rational& grain_rate, uint32_t frame_width, uint32_t frame_height)
    {
        return nmos::level{ sdp::video_jxsv::get_level(grain_rate, frame_width, frame_height).name };
    }

    // Sublevel
    // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/#sublevel
    DEFINE_STRING_ENUM(sublevel)
    namespace sublevels
    {
        // JPEG XS Sublevel
        // "The JPEG XS sublevel [ISO21122-2] in use. Any white space Unicode character in the sublevel name SHALL be omitted."
        // See https://tools.ietf.org/html/rfc9134
        // and https://specs.amwa.tv/nmos-parameter-registers/branches/main/flow-attributes/flow_video_jxsv_register.html
        const sublevel Full{ U("Full") };
        const sublevel Sublev12bpp{ U("Sublev12bpp") };
        const sublevel Sublev9bpp{ U("Sublev9bpp") };
        const sublevel Sublev6bpp{ U("Sublev6bpp") };
        const sublevel Sublev4bpp{ U("Sublev4bpp") };
        const sublevel Sublev3bpp{ U("Sublev3bpp") };
        const sublevel Sublev2bpp{ U("Sublev2bpp") };
    }

    // Packet Transmission Mode
    // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/sender-attributes/#packet-transmission-mode
    DEFINE_STRING_ENUM(packet_transmission_mode)
    namespace packet_transmission_modes
    {
        // JPEG XS packetization and transmission mode
        // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/sender-attributes/#packet-transmission-mode
        // and https://specs.amwa.tv/nmos-parameter-registers/branches/main/sender-attributes/sender_register.html
        const packet_transmission_mode codestream{ U("codestream") };
        const packet_transmission_mode slice_sequential{ U("slice_sequential") };
        const packet_transmission_mode slice_out_of_order{ U("slice_out_of_order") };
    }

    std::pair<sdp::video_jxsv::packetization_mode, sdp::video_jxsv::transmission_mode> make_packet_transmission_mode(const nmos::packet_transmission_mode& mode);
    nmos::packet_transmission_mode parse_packet_transmission_mode(sdp::video_jxsv::packetization_mode packetmode, sdp::video_jxsv::transmission_mode transmode);

    // Additional "video/jxsv" parameters
    // See https://www.iana.org/assignments/media-types/video/jxsv
    // and https://tools.ietf.org/html/rfc9134
    struct video_jxsv_parameters
    {
        // fmtp indicates format
        sdp::video_jxsv::packetization_mode packetmode;
        sdp::video_jxsv::transmission_mode transmode;
        sdp::video_jxsv::profile profile; // nmos::profile has compatible values
        sdp::video_jxsv::level level; // nmos::level has compatible values
        sdp::video_jxsv::sublevel sublevel; // nmos::sublevel has compatible values
        sdp::sampling sampling;
        uint32_t depth;
        uint32_t width;
        uint32_t height;
        nmos::rational exactframerate;
        bool interlace;
        bool segmented;
        sdp::transfer_characteristic_system tcs; // nmos::transfer_characteristic is compatible
        sdp::colorimetry colorimetry; // nmos::colorspace is compatible
        sdp::range range; // if omitted (empty), assume sdp::ranges::NARROW
        sdp::smpte_standard_number ssn;

        // additional fmtp parameters from ST 2110-21:2022
        sdp::type_parameter tp;
        bst::optional<uint32_t> troff; // if omitted, assume default
        uint32_t cmax; // if omitted (zero), assume max defined for tp

        // additional fmtp parameters from ST 2110-10:2022
        uint32_t maxudp; // if omitted (zero), assume the Standard UP Size Limit
        sdp::timestamp_mode tsmode; // if omitted (empty), assume sdp::timestamp_modes::NEW
        bst::optional<uint32_t> tsdelay;

        // bandwidth
        uint64_t bit_rate; // transport bit rate

        video_jxsv_parameters() : depth(), width(), height(), interlace(), segmented(), troff(), cmax(), maxudp(), tsdelay() {}

        video_jxsv_parameters(
            sdp::video_jxsv::packetization_mode packetmode,
            sdp::video_jxsv::transmission_mode transmode,
            sdp::video_jxsv::profile profile,
            sdp::video_jxsv::level level,
            sdp::video_jxsv::sublevel sublevel,
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
            sdp::smpte_standard_number ssn,
            sdp::type_parameter tp,
            bst::optional<uint32_t> troff,
            uint32_t cmax,
            uint32_t maxudp,
            sdp::timestamp_mode tsmode,
            bst::optional<uint32_t> tsdelay,
            uint64_t bit_rate
        )
            : packetmode(packetmode)
            , transmode(transmode)
            , profile(std::move(profile))
            , level(std::move(level))
            , sublevel(std::move(sublevel))
            , sampling(std::move(sampling))
            , depth(depth)
            , width(width)
            , height(height)
            , exactframerate(exactframerate)
            , interlace(interlace)
            , segmented(segmented)
            , tcs(std::move(tcs))
            , colorimetry(std::move(colorimetry))
            , range(std::move(range))
            , ssn(std::move(ssn))
            , tp(std::move(tp))
            , troff(troff)
            , cmax(cmax)
            , maxudp(maxudp)
            , tsmode(std::move(tsmode))
            , tsdelay(tsdelay)
            , bit_rate(bit_rate)
        {}
    };

    // Construct additional "video/jxsv" parameters from the IS-04 resources
    video_jxsv_parameters make_video_jxsv_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender);
    // Construct SDP parameters for "video/jxsv"
    sdp_parameters make_video_jxsv_sdp_parameters(const utility::string_t& session_name, const video_jxsv_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {});
    // Get additional "video/jxsv" parameters from the SDP parameters
    video_jxsv_parameters get_video_jxsv_parameters(const sdp_parameters& sdp_params);

    // Construct SDP parameters for "video/jxsv"
    inline sdp_parameters make_sdp_parameters(const utility::string_t& session_name, const video_jxsv_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids = {}, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk = {})
    {
        return make_video_jxsv_sdp_parameters(session_name, params, payload_type, media_stream_ids, ts_refclk);
    }

    // Validate SDP parameters for "video/jxsv" against IS-04 receiver capabilities
    void validate_video_jxsv_sdp_parameters(const web::json::value& receiver, const nmos::sdp_parameters& sdp_params);

    // Calculate the format bit rate (kilobits/second) from the specified frame rate, dimensions and bits per pixel
    uint64_t get_video_jxsv_bit_rate(const nmos::rational& grain_rate, uint32_t frame_width, uint32_t frame_height, double bits_per_pixel);

    // See https://specs.amwa.tv/bcp-006-01/branches/v1.0-dev/docs/NMOS_With_JPEG_XS.html#flows
    // cf. nmos::make_coded_video_flow
    nmos::resource make_video_jxsv_flow(
        const nmos::id& id,
        const nmos::id& source_id,
        const nmos::id& device_id,
        const nmos::rational& grain_rate,
        unsigned int frame_width,
        unsigned int frame_height,
        const nmos::interlace_mode& interlace_mode,
        const nmos::colorspace& colorspace,
        const nmos::transfer_characteristic& transfer_characteristic,
        const sdp::sampling& color_sampling,
        unsigned int bit_depth,
        const nmos::profile& profile,
        const nmos::level& level,
        const nmos::sublevel& sublevel,
        double bits_per_pixel,
        const nmos::settings& settings);
}

#endif

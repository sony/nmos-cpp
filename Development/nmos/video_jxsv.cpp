#include "nmos/video_jxsv.h"

#include <map>
#include "nmos/capabilities.h"
#include "nmos/format.h"
#include "nmos/interlace_mode.h"
#include "nmos/json_fields.h"
#include "nmos/resource.h"

namespace sdp
{
    namespace video_jxsv
    {
        struct level_limits
        {
            uint32_t max_width;
            uint32_t max_height;
            uint64_t max_pixels;
            uint64_t max_pixel_rate;

            friend bool operator<=(const level_limits& lhs, const level_limits& rhs)
            {
                if (lhs.max_width > rhs.max_width) return false;
                if (lhs.max_height > rhs.max_height) return false;
                if (lhs.max_pixels > rhs.max_pixels) return false;
                if (lhs.max_pixel_rate > rhs.max_pixel_rate) return false;
                return true;
            }
        };

        // Calculate the lowest possible JPEG XS level from the specified frame rate and dimensions
        level get_level(const nmos::rational& frame_rate, uint32_t frame_width, uint32_t frame_height)
        {
            // See https://en.wikipedia.org/wiki/JPEG_XS#Profiles,_levels_and_sublevels
            static const std::pair<level, level_limits> levels[] = {
                { levels::Level1k_1, { 1280, 5120, 2621440, 83558400 } },
                { levels::Level2k_1, { 2048, 8192, 4194304, 133693440 } },
                { levels::Level4k_1, { 4096, 16384, 8912896, 267386880 } },
                { levels::Level4k_2, { 4096, 16384, 16777216, 534773760 } },
                { levels::Level4k_3, { 4096, 16384, 16777216, 1069547520 } },
                { levels::Level8k_1, { 8192, 32768, 35651584, 1069547520 } },
                { levels::Level8k_2, { 8192, 32768, 67108864, 2139095040 } },
                { levels::Level8k_3, { 8192, 32768, 67108864, 4278190080 } },
                { levels::Level10k_1, { 10240, 40960, 104857600, 3342336000 } }
            };

            const auto sampling_points = (int64_t)frame_width * (int64_t)frame_height;
            const auto pixels_per_second = sampling_points * frame_rate;
            const level_limits value{ frame_width, frame_height, (uint64_t)sampling_points, uint64_t(boost::rational_cast<double>(pixels_per_second) + 0.5) };

            for (const auto& level : levels)
            {
                if (value <= level.second) return level.first;
            }

            return{};
        }
    }
}

namespace nmos
{
    std::pair<sdp::video_jxsv::packetization_mode, sdp::video_jxsv::transmission_mode> make_packet_transmission_mode(const nmos::packet_transmission_mode& mode)
    {
        if (nmos::packet_transmission_modes::codestream == mode)
        {
            return{ sdp::video_jxsv::codestream, sdp::video_jxsv::sequential };
        }
        else if (nmos::packet_transmission_modes::slice_sequential == mode)
        {
            return{ sdp::video_jxsv::slice, sdp::video_jxsv::sequential };
        }
        else if (nmos::packet_transmission_modes::slice_out_of_order == mode)
        {
            return{ sdp::video_jxsv::slice, sdp::video_jxsv::out_of_order };
        }
        throw std::invalid_argument("invalid packet_transmission_mode");
    }

    nmos::packet_transmission_mode parse_packet_transmission_mode(sdp::video_jxsv::packetization_mode packetmode, sdp::video_jxsv::transmission_mode transmode)
    {
        if (sdp::video_jxsv::codestream == packetmode && sdp::video_jxsv::sequential == transmode)
        {
            return nmos::packet_transmission_modes::codestream;
        }
        else if (sdp::video_jxsv::slice == packetmode && sdp::video_jxsv::sequential == transmode)
        {
            return nmos::packet_transmission_modes::slice_sequential;
        }
        else if (sdp::video_jxsv::slice == packetmode && sdp::video_jxsv::out_of_order == transmode)
        {
            return nmos::packet_transmission_modes::slice_out_of_order;
        }
        throw std::invalid_argument("invalid packetmode/transmode");
    }

    // Construct additional "video/jxsv" parameters from the IS-04 resources
    video_jxsv_parameters make_video_jxsv_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender)
    {
        video_jxsv_parameters params;

        std::tie(params.packetmode, params.transmode) = make_packet_transmission_mode(nmos::packet_transmission_mode{ nmos::fields::packet_transmission_mode(sender) });

        params.profile = sdp::video_jxsv::profile{ nmos::fields::profile(flow) };
        params.level = sdp::video_jxsv::level{ nmos::fields::level(flow) };
        params.sublevel = sdp::video_jxsv::sublevel{ nmos::fields::sublevel(flow) };

        // cf. nmos::make_video_raw_parameters

        const auto& components = nmos::fields::components(flow);
        params.sampling = details::make_sampling(components);
        params.depth = nmos::fields::bit_depth(components.at(0));
        params.width = nmos::fields::frame_width(flow);
        params.height = nmos::fields::frame_height(flow);

        // grain_rate is optional in the flow, but if it's not there, for a video flow, it must be in the source
        const auto& grain_rate = nmos::fields::grain_rate(flow.has_field(nmos::fields::grain_rate) ? flow : source);
        params.exactframerate = nmos::parse_rational(grain_rate);

        const auto& interlace_mode = nmos::fields::interlace_mode(flow);
        params.interlace = !interlace_mode.empty() && nmos::interlace_modes::progressive.name != interlace_mode;
        params.segmented = !interlace_mode.empty() && nmos::interlace_modes::interlaced_psf.name == interlace_mode;

        // map directly
        params.tcs = sdp::transfer_characteristic_system{ nmos::fields::transfer_characteristic(flow) };
        params.colorimetry = sdp::colorimetry{ nmos::fields::colorspace(flow) };

        // hm, RANGE and PAR not currently indicated in IS-04 so omit these

        // hm, not sure how to decide between ST 2110-22:2019 and ST 2110-22:2022
        params.ssn = sdp::smpte_standard_numbers::ST2110_22_2022;

        // hm, TP is equivalent to the new sender attribute, but for now, support default value
        params.tp = sender.has_field(nmos::fields::st2110_21_sender_type)
            ? sdp::type_parameter{ nmos::fields::st2110_21_sender_type(sender) }
            : sdp::type_parameters::type_N;

        // hm, ST 2110-21 TROFF and CMAX not indicated in IS-04 so omit these
        // hm, ST 2110-10 MAXUDP, TSMODE and TSDELAY not indicated in IS-04 so omit these

        // bandwidth

        params.bit_rate = nmos::fields::bit_rate(sender);

        return params;
    }

    // Construct SDP parameters for "video/jxsv", with sensible defaults for unspecified fields
    sdp_parameters make_video_jxsv_sdp_parameters(const utility::string_t& session_name, const video_jxsv_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk)
    {
        // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
        sdp_parameters::rtpmap_t rtpmap = { payload_type, U("jxsv"), 90000 };

        // a=fmtp:<format> <format specific parameters>
        // following the order of parameters given in RFC 9134
        // See https://tools.ietf.org/html/rfc4566#section-6
        // and https://tools.ietf.org/html/rfc9134#section-7
        sdp_parameters::fmtp_t fmtp = {
            { sdp::video_jxsv::fields::packetmode, utility::ostringstreamed((uint32_t)params.packetmode) }
        };
        if (sdp::video_jxsv::sequential != params.transmode) fmtp.push_back({ sdp::video_jxsv::fields::transmode, utility::ostringstreamed((uint32_t)params.transmode) });
        if (!params.profile.empty()) fmtp.push_back({ sdp::video_jxsv::fields::profile, params.profile.name });
        if (!params.level.empty()) fmtp.push_back({ sdp::video_jxsv::fields::level, params.level.name });
        if (!params.sublevel.empty()) fmtp.push_back({ sdp::video_jxsv::fields::sublevel, params.sublevel.name });
        if (0 != params.depth) fmtp.push_back({ sdp::fields::depth, utility::ostringstreamed(params.depth) });
        if (0 != params.width) fmtp.push_back({ sdp::fields::width, utility::ostringstreamed(params.width) });
        if (0 != params.height) fmtp.push_back({ sdp::fields::height, utility::ostringstreamed(params.height) });
        if (0 != params.exactframerate) fmtp.push_back({ sdp::fields::exactframerate, nmos::details::make_exactframerate(params.exactframerate) });
        if (params.interlace) fmtp.push_back({ sdp::fields::interlace, {} });
        if (params.segmented) fmtp.push_back({ sdp::fields::segmented, {} });
        if (!params.sampling.empty()) fmtp.push_back({ sdp::fields::sampling, params.sampling.name });
        if (!params.colorimetry.empty()) fmtp.push_back({ sdp::fields::colorimetry, params.colorimetry.name });
        if (!params.tcs.empty()) fmtp.push_back({ sdp::fields::transfer_characteristic_system, params.tcs.name });
        if (!params.range.empty()) fmtp.push_back({ sdp::fields::range, params.range.name });

        // additional parameters introduced by SMPTE specs since then...
        if (!params.ssn.empty()) fmtp.push_back({ sdp::fields::smpte_standard_number, params.ssn.name });
        if (!params.tp.empty()) fmtp.push_back({ sdp::fields::type_parameter, params.tp.name });
        if (params.troff) fmtp.push_back({ sdp::fields::TROFF, utility::ostringstreamed(*params.troff) });
        if (0 != params.cmax) fmtp.push_back({ sdp::fields::CMAX, utility::ostringstreamed(params.cmax) });
        if (0 != params.maxudp) fmtp.push_back({ sdp::fields::max_udp_packet_size, utility::ostringstreamed(params.maxudp) });
        if (!params.tsmode.empty()) fmtp.push_back({ sdp::fields::timestamp_mode, params.tsmode.name });
        if (params.tsdelay) fmtp.push_back({ sdp::fields::timestamp_delay, utility::ostringstreamed(*params.tsdelay) });

        return{ session_name, sdp::media_types::video, rtpmap, fmtp, params.bit_rate, {}, {}, {}, media_stream_ids, ts_refclk };
    }

    // Get additional "video/jxsv" parameters from the SDP parameters
    video_jxsv_parameters get_video_jxsv_parameters(const sdp_parameters& sdp_params)
    {
        video_jxsv_parameters params;

        const auto packetmode = details::find_fmtp(sdp_params.fmtp, sdp::video_jxsv::fields::packetmode);
        if (sdp_params.fmtp.end() == packetmode) throw details::sdp_processing_error("missing format parameter: packetmode");
        params.packetmode = (sdp::video_jxsv::packetization_mode)utility::istringstreamed<uint32_t>(packetmode->second);

        // optional
        const auto transmode = details::find_fmtp(sdp_params.fmtp, sdp::video_jxsv::fields::transmode);
        params.transmode = sdp_params.fmtp.end() != transmode
            ? (sdp::video_jxsv::transmission_mode)utility::istringstreamed<uint32_t>(transmode->second)
            : sdp::video_jxsv::sequential;

        // optional
        const auto profile = details::find_fmtp(sdp_params.fmtp, sdp::video_jxsv::fields::profile);
        if (sdp_params.fmtp.end() != profile) params.profile = sdp::video_jxsv::profile{ profile->second };

        // optional
        const auto level = details::find_fmtp(sdp_params.fmtp, sdp::video_jxsv::fields::level);
        if (sdp_params.fmtp.end() != level) params.level = sdp::video_jxsv::level{ level->second };

        // optional
        const auto sublevel = details::find_fmtp(sdp_params.fmtp, sdp::video_jxsv::fields::sublevel);
        if (sdp_params.fmtp.end() != sublevel) params.sublevel = sdp::video_jxsv::sublevel{ sublevel->second };

        // optional
        const auto sampling = details::find_fmtp(sdp_params.fmtp, sdp::fields::sampling);
        if (sdp_params.fmtp.end() != sampling)  params.sampling = sdp::sampling{ sampling->second };

        // optional
        const auto depth = details::find_fmtp(sdp_params.fmtp, sdp::fields::depth);
        if (sdp_params.fmtp.end() != depth) params.depth = utility::istringstreamed<uint32_t>(depth->second);

        // optional
        const auto width = details::find_fmtp(sdp_params.fmtp, sdp::fields::width);
        if (sdp_params.fmtp.end() != width) params.width = utility::istringstreamed<uint32_t>(width->second);

        // optional
        const auto height = details::find_fmtp(sdp_params.fmtp, sdp::fields::height);
        if (sdp_params.fmtp.end() != height) params.height = utility::istringstreamed<uint32_t>(height->second);

        // optional
        const auto exactframerate = details::find_fmtp(sdp_params.fmtp, sdp::fields::exactframerate);
        if (sdp_params.fmtp.end() != exactframerate) params.exactframerate = nmos::details::parse_exactframerate(exactframerate->second);

        // optional
        const auto interlace = details::find_fmtp(sdp_params.fmtp, sdp::fields::interlace);
        params.interlace = sdp_params.fmtp.end() != interlace;

        // optional
        const auto segmented = details::find_fmtp(sdp_params.fmtp, sdp::fields::segmented);
        params.segmented = sdp_params.fmtp.end() != segmented;

        // optional
        const auto tcs = details::find_fmtp(sdp_params.fmtp, sdp::fields::transfer_characteristic_system);
        if (sdp_params.fmtp.end() != tcs) params.tcs = sdp::transfer_characteristic_system{ tcs->second };

        // optional
        const auto colorimetry = details::find_fmtp(sdp_params.fmtp, sdp::fields::colorimetry);
        if (sdp_params.fmtp.end() != colorimetry) params.colorimetry = sdp::colorimetry{ colorimetry->second };

        // optional
        const auto range = details::find_fmtp(sdp_params.fmtp, sdp::fields::range);
        if (sdp_params.fmtp.end() != range) params.range = sdp::range{ range->second };

        // optional
        const auto ssn = details::find_fmtp(sdp_params.fmtp, sdp::fields::smpte_standard_number);
        if (sdp_params.fmtp.end() != ssn) params.ssn = sdp::smpte_standard_number{ ssn->second };

        // optional
        const auto tp = details::find_fmtp(sdp_params.fmtp, sdp::fields::type_parameter);
        if (sdp_params.fmtp.end() != tp) params.tp = sdp::type_parameter{ tp->second };

        // optional
        const auto troff = details::find_fmtp(sdp_params.fmtp, sdp::fields::TROFF);
        if (sdp_params.fmtp.end() != troff) params.troff = utility::istringstreamed<uint32_t>(troff->second);

        // optional
        const auto cmax = details::find_fmtp(sdp_params.fmtp, sdp::fields::CMAX);
        if (sdp_params.fmtp.end() != cmax) params.cmax = utility::istringstreamed<uint32_t>(cmax->second);

        // optional
        const auto maxudp = details::find_fmtp(sdp_params.fmtp, sdp::fields::max_udp_packet_size);
        if (sdp_params.fmtp.end() != maxudp) params.maxudp = utility::istringstreamed<uint32_t>(maxudp->second);

        // optional
        const auto tsmode = details::find_fmtp(sdp_params.fmtp, sdp::fields::timestamp_mode);
        if (sdp_params.fmtp.end() != tsmode) params.tsmode = sdp::timestamp_mode{ tsmode->second };

        // optional
        const auto tsdelay = details::find_fmtp(sdp_params.fmtp, sdp::fields::timestamp_delay);
        if (sdp_params.fmtp.end() != tsdelay) params.tsdelay = utility::istringstreamed<uint32_t>(tsdelay->second);

        // optional
        if (sdp::bandwidth_types::application_specific == sdp_params.bandwidth.bandwidth_type) params.bit_rate = sdp_params.bandwidth.bandwidth;

        return params;
    }

    // Calculate the format bit rate (kilobits/second) from the specified frame rate, dimensions and bits per pixel
    uint64_t get_video_jxsv_bit_rate(const nmos::rational& grain_rate, uint32_t frame_width, uint32_t frame_height, double bits_per_pixel)
    {
        const auto sampling_points = (int64_t)frame_width * (int64_t)frame_height;
        const auto pixels_per_second = sampling_points * grain_rate;
        const auto bit_rate = boost::rational_cast<double>(pixels_per_second) * bits_per_pixel;
        return uint64_t(bit_rate / 1e3 + 0.5);
    }

    namespace details
    {
        const video_jxsv_parameters* get_jxsv(const format_parameters* format) { return get<video_jxsv_parameters>(format); }

        // NMOS Parameter Registers - Capabilities register
        // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/capabilities/
#define CAPS_ARGS const sdp_parameters& sdp, const format_parameters& format, const web::json::value& con
        static const std::map<utility::string_t, std::function<bool(CAPS_ARGS)>> jxsv_constraints
        {
            { nmos::caps::format::media_type, [](CAPS_ARGS) { return nmos::match_string_constraint(get_media_type(sdp).name, con); } },
            { nmos::caps::format::grain_rate, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (nmos::rational{} == jxsv->exactframerate || nmos::match_rational_constraint(jxsv->exactframerate, con)); } },
            { nmos::caps::format::profile, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (jxsv->profile.empty() || nmos::match_string_constraint(jxsv->profile.name, con)); } },
            { nmos::caps::format::level, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (jxsv->level.empty() || nmos::match_string_constraint(jxsv->level.name, con)); } },
            { nmos::caps::format::sublevel, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (jxsv->sublevel.empty() || nmos::match_string_constraint(jxsv->sublevel.name, con)); } },
            { nmos::caps::format::frame_height, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (0 == jxsv->height || nmos::match_integer_constraint(jxsv->height, con)); } },
            { nmos::caps::format::frame_width, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (0 == jxsv->width || nmos::match_integer_constraint(jxsv->width, con)); } },
            { nmos::caps::format::color_sampling, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (jxsv->sampling.empty() || nmos::match_string_constraint(jxsv->sampling.name, con)); } },
            { nmos::caps::format::interlace_mode, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::details::match_interlace_mode_constraint(jxsv->interlace, jxsv->segmented, con); } },
            { nmos::caps::format::colorspace, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (jxsv->colorimetry.empty() || nmos::match_string_constraint(jxsv->colorimetry.name, con)); } },
            { nmos::caps::format::transfer_characteristic, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (jxsv->tcs.empty() || nmos::match_string_constraint(jxsv->tcs.name, con)); } },
            { nmos::caps::format::component_depth, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (0 == jxsv->depth || nmos::match_integer_constraint(jxsv->depth, con)); } },
            { nmos::caps::transport::packet_transmission_mode, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_string_constraint(nmos::parse_packet_transmission_mode(jxsv->packetmode, jxsv->transmode).name, con); } },
            { nmos::caps::transport::st2110_21_sender_type, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return nmos::match_string_constraint(jxsv->tp.name, con); } },
            { nmos::caps::transport::bit_rate, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && (0 == jxsv->bit_rate || nmos::match_integer_constraint(jxsv->bit_rate, con)); } }
        };
#undef CAPS_ARGS
    }

    // Validate SDP parameters for "video/jxsv" against IS-04 receiver capabilities
    // cf. nmos::validate_sdp_parameters
    void validate_video_jxsv_sdp_parameters(const web::json::value& receiver, const nmos::sdp_parameters& sdp_params)
    {
        // this function can only be used to validate SDP data for "video/jxsv"; logic error otherwise
        const auto media_type = get_media_type(sdp_params);
        if (nmos::media_types::video_jxsv != media_type) throw std::invalid_argument("unexpected media type/encoding name");

        nmos::details::validate_sdp_parameters(details::jxsv_constraints, sdp_params, nmos::formats::video, get_video_jxsv_parameters(sdp_params), receiver);
    }

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
        const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = nmos::make_coded_video_flow(
            id, source_id, device_id,
            grain_rate,
            frame_width, frame_height, interlace_mode,
            colorspace, transfer_characteristic, color_sampling, bit_depth,
            nmos::media_types::video_jxsv,
            settings
        );
        auto& data = resource.data;

        // additional attributes required by BCP-006-01
        // see https://specs.amwa.tv/bcp-006-01/branches/v1.0-dev/docs/NMOS_With_JPEG_XS.html#flows
        if (!profile.empty()) data[nmos::fields::profile] = value(profile.name);
        if (!level.empty()) data[nmos::fields::level] = value(level.name);
        if (!sublevel.empty()) data[nmos::fields::sublevel] = value(sublevel.name);
        const auto bit_rate = nmos::get_video_jxsv_bit_rate(grain_rate, frame_width, frame_height, bits_per_pixel);
        if (0 != bit_rate) data[nmos::fields::bit_rate] = value(bit_rate);

        return resource;
    }
}

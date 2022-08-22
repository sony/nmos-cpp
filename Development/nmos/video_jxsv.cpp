#include "nmos/video_jxsv.h"

#include <map>
#include "nmos/capabilities.h"
#include "nmos/format.h"
#include "nmos/interlace_mode.h"
#include "nmos/json_fields.h"

namespace nmos
{
    // Construct additional "video/jxsv" parameters from the IS-04 resources
    video_jxsv_parameters make_video_jxsv_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender)
    {
        video_jxsv_parameters params;

        params.packet_transmission_mode = nmos::packet_transmission_mode{ nmos::fields::packet_transmission_mode(sender) };

        params.profile = nmos::profile{ nmos::fields::profile(flow) };
        params.level = nmos::level{  nmos::fields::level(flow) };
        params.sublevel = nmos::sublevel{ nmos::fields::sublevel(flow) };

        // cf. nmos::make_video_raw_parameters

        params.tp = sdp::type_parameter{ nmos::fields::st2110_21_sender_type(sender) };

        // colorimetry map directly to flow_video json "colorspace"
        params.colorimetry = sdp::colorimetry{ nmos::fields::colorspace(flow) };

        // use flow json "components" to work out sampling
        const auto& components = nmos::fields::components(flow);
        params.sampling = details::make_sampling(components);
        params.width = nmos::fields::frame_width(flow);
        params.height = nmos::fields::frame_height(flow);
        params.depth = nmos::fields::bit_depth(components.at(0));

        // also maps directly
        params.tcs = sdp::transfer_characteristic_system{ nmos::fields::transfer_characteristic(flow) };

        const auto& interlace_mode = nmos::fields::interlace_mode(flow);
        params.interlace = !interlace_mode.empty() && nmos::interlace_modes::progressive.name != interlace_mode;
        params.segmented = !interlace_mode.empty() && nmos::interlace_modes::interlaced_psf.name == interlace_mode;

        // grain_rate is optional in the flow, but if it's not there, for a video flow, it must be in the source
        const auto& grain_rate = nmos::fields::grain_rate(flow.has_field(nmos::fields::grain_rate) ? flow : source);
        params.exactframerate = nmos::rational(nmos::fields::numerator(grain_rate), nmos::fields::denominator(grain_rate));

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
        const uint32_t k = nmos::packet_transmission_modes::codestream == params.packet_transmission_mode ? 0 : 1;
        const uint32_t t = nmos::packet_transmission_modes::slice_out_of_order == params.packet_transmission_mode ? 0 : 1;
        sdp_parameters::fmtp_t fmtp = {
            { sdp::fields::packetmode, utility::ostringstreamed(k) }
        };
        if (1 != t) fmtp.push_back({ sdp::fields::transmode, utility::ostringstreamed(t) });
        if (!params.profile.empty()) fmtp.push_back({ sdp::fields::profile, params.profile.name });
        if (!params.level.empty()) fmtp.push_back({ sdp::fields::level, params.level.name });
        if (!params.sublevel.empty()) fmtp.push_back({ sdp::fields::sublevel, params.sublevel.name });

        // cf. nmos::make_video_raw_sdp_parameters

        if (0 != params.depth) fmtp.push_back({ sdp::fields::depth, utility::ostringstreamed(params.depth) });
        if (0 != params.width) fmtp.push_back({ sdp::fields::width, utility::ostringstreamed(params.width) });
        if (0 != params.height) fmtp.push_back({ sdp::fields::height, utility::ostringstreamed(params.height) });
        if (0 != params.exactframerate) fmtp.push_back({ sdp::fields::exactframerate, nmos::details::make_exactframerate(params.exactframerate) });
        if (params.interlace) fmtp.push_back({ sdp::fields::interlace, {} });
        if (params.segmented) fmtp.push_back({ sdp::fields::segmented, {} });
        if (!params.sampling.empty()) fmtp.push_back({ sdp::fields::sampling, params.sampling.name });
        if (!params.colorimetry.empty()) fmtp.push_back({ sdp::fields::colorimetry, params.colorimetry.name });
        if (!params.tcs.empty()) fmtp.push_back({ sdp::fields::transfer_characteristic_system, params.tcs.name });
        if (!params.tp.empty()) fmtp.push_back({ sdp::fields::type_parameter, params.tp.name });

        return{ session_name, sdp::media_types::video, rtpmap, fmtp, {}, {}, {}, {}, media_stream_ids, ts_refclk };
    }

    // Get additional "video/jxsv" parameters from the SDP parameters
    video_jxsv_parameters get_video_jxsv_parameters(const sdp_parameters& sdp_params)
    {
        video_jxsv_parameters params;

        if (sdp_params.fmtp.empty()) return params;

        const auto k = details::find_fmtp(sdp_params.fmtp, sdp::fields::packetmode);
        if (sdp_params.fmtp.end() != k) throw details::sdp_processing_error("missing format parameter: packetmode");

        // optional
        const auto t = details::find_fmtp(sdp_params.fmtp, sdp::fields::transmode);

        params.packet_transmission_mode = 0 == utility::istringstreamed<uint32_t>(k->second)
            ? nmos::packet_transmission_modes::codestream
            : sdp_params.fmtp.end() != t && 1 != utility::istringstreamed<uint32_t>(t->second)
                ? nmos::packet_transmission_modes::slice_sequential
                : nmos::packet_transmission_modes::slice_out_of_order;

        // optional
        const auto profile = details::find_fmtp(sdp_params.fmtp, sdp::fields::profile);
        if (sdp_params.fmtp.end() != profile) params.profile = nmos::profile{ profile->second };

        // optional
        const auto level = details::find_fmtp(sdp_params.fmtp, sdp::fields::level);
        if (sdp_params.fmtp.end() != level) params.level = nmos::level{ level->second };

        // optional
        const auto sublevel = details::find_fmtp(sdp_params.fmtp, sdp::fields::sublevel);
        if (sdp_params.fmtp.end() != sublevel) params.sublevel = nmos::sublevel{ sublevel->second };

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
        const auto sampling = details::find_fmtp(sdp_params.fmtp, sdp::fields::sampling);
        if (sdp_params.fmtp.end() != sampling)  params.sampling = sdp::sampling{ sampling->second };

        const auto colorimetry = details::find_fmtp(sdp_params.fmtp, sdp::fields::colorimetry);
        if (sdp_params.fmtp.end() != colorimetry) params.colorimetry = sdp::colorimetry{ colorimetry->second };

        // optional
        const auto tcs = details::find_fmtp(sdp_params.fmtp, sdp::fields::transfer_characteristic_system);
        if (sdp_params.fmtp.end() != tcs) params.tcs = sdp::transfer_characteristic_system{ tcs->second };

        const auto tp = details::find_fmtp(sdp_params.fmtp, sdp::fields::type_parameter);
        if (sdp_params.fmtp.end() != tp) params.tp = sdp::type_parameter{ tp->second };

        return params;
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
            { nmos::caps::format::grain_rate, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return !jxsv || nmos::rational{} == jxsv->exactframerate || nmos::match_rational_constraint(jxsv->exactframerate, con); } },
            { nmos::caps::format::profile, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_string_constraint(jxsv->profile.name, con); } },
            { nmos::caps::format::level, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_string_constraint(jxsv->level.name, con); } },
            { nmos::caps::format::sublevel, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_string_constraint(jxsv->sublevel.name, con); } },
            { nmos::caps::format::frame_height, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_integer_constraint(jxsv->height, con); } },
            { nmos::caps::format::frame_width, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_integer_constraint(jxsv->width, con); } },
            { nmos::caps::format::color_sampling, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_string_constraint(jxsv->sampling.name, con); } },
            { nmos::caps::format::interlace_mode, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::details::match_interlace_mode_constraint(jxsv->interlace, jxsv->segmented, con); } },
            { nmos::caps::format::colorspace, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_string_constraint(jxsv->colorimetry.name, con); } },
            { nmos::caps::format::transfer_characteristic, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_string_constraint(!jxsv->tcs.empty() ? jxsv->tcs.name : sdp::transfer_characteristic_systems::SDR.name, con); } },
            { nmos::caps::format::component_depth, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return jxsv && nmos::match_integer_constraint(jxsv->depth, con); } },
            { nmos::caps::transport::st2110_21_sender_type, [](CAPS_ARGS) { auto jxsv = get_jxsv(&format); return nmos::match_string_constraint(jxsv->tp.name, con); } }
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
}

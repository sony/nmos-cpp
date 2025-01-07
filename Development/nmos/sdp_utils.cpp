#include "nmos/sdp_utils.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/numeric.hpp>
#include "cpprest/basic_utils.h"
#include "nmos/capabilities.h"
#include "nmos/clock_ref_type.h"
#include "nmos/channels.h"
#include "nmos/components.h"
#include "nmos/format.h"
#include "nmos/interlace_mode.h"
#include "nmos/json_fields.h"
#include "nmos/media_type.h"
#include "nmos/transport.h"

namespace nmos
{
    namespace details
    {
        std::pair<sdp::address_type, bool> get_address_type_multicast(const utility::string_t& address)
        {
#if BOOST_VERSION >= 106600
            const auto ip_address = boost::asio::ip::make_address(utility::us2s(address));
#else
            const auto ip_address = boost::asio::ip::address::from_string(utility::us2s(address));
#endif
            return{ ip_address.is_v4() ? sdp::address_types::IP4 : sdp::address_types::IP6, ip_address.is_multicast() };
        }

        // Construct ts-refclk attributes for each leg based on the IS-04 resources
        std::vector<sdp_parameters::ts_refclk_t> make_ts_refclk(const web::json::value& node, const web::json::value& source, const web::json::value& sender, bst::optional<int> ptp_domain)
        {
            const auto& clock_name = nmos::fields::clock_name(source);
            if (clock_name.is_null()) throw sdp_creation_error("no source clock");

            const auto& clocks = nmos::fields::clocks(node);
            const auto clock = std::find_if(clocks.begin(), clocks.end(), [&](const web::json::value& clock)
            {
                return nmos::fields::name(clock) == clock_name.as_string();
            });
            if (clocks.end() == clock) throw sdp_creation_error("source clock not found");

            const auto& interface_bindings = nmos::fields::interface_bindings(sender);
            if (0 == interface_bindings.size()) throw sdp_creation_error("no sender interface_bindings");

            const nmos::clock_ref_type ref_type{ nmos::fields::ref_type(*clock) };
            if (nmos::clock_ref_types::ptp == ref_type)
            {
                const sdp::ptp_version version{ nmos::fields::ptp_version(*clock) };

                // "If the PTP timescale is in use, and the current grandmaster indicates that its timesource is traceable, devices shall signal that the PTP is traceable in the SDP."
                // See ST 2110-10:2020
                if (nmos::fields::traceable(*clock))
                {
                    return{ interface_bindings.size(), sdp_parameters::ts_refclk_t::ptp_traceable(version) };
                }
                else
                {
                    auto server = boost::algorithm::to_upper_copy(nmos::fields::gmid(*clock));
                    if (ptp_domain)
                    {
                        if (*ptp_domain < 0 || *ptp_domain > 127) throw sdp_creation_error("PTP domain out of range");

                        server += U(":") + utility::conversions::details::to_string_t(*ptp_domain);
                    }

                    return{ interface_bindings.size(), sdp_parameters::ts_refclk_t::ptp(version, server) };
                }
            }
            else if (nmos::clock_ref_types::internal == ref_type)
            {
                const auto& interfaces = nmos::fields::interfaces(node);
                return boost::copy_range<std::vector<sdp_parameters::ts_refclk_t>>(interface_bindings | boost::adaptors::transformed([&](const web::json::value& interface_binding)
                {
                    const auto& interface_name = interface_binding.as_string();
                    const auto interface = std::find_if(interfaces.begin(), interfaces.end(), [&](const web::json::value& interface)
                    {
                        return nmos::fields::name(interface) == interface_name;
                    });
                    if (interfaces.end() == interface) throw sdp_creation_error("sender interface not found");
                    return sdp_parameters::ts_refclk_t::local_mac(boost::algorithm::to_upper_copy(nmos::fields::port_id(*interface)));
                }));
            }
            else throw sdp_creation_error("unexpected clock ref_type");
        }

        // See sdp::samplings
        typedef std::pair<uint32_t, uint32_t> width_height_t;
        static const std::map<sdp::sampling, std::vector<std::pair<component_name, width_height_t>>> samplers
        {
            // Red-Green-Blue-Alpha
            { sdp::samplings::RGBA, { { component_names::R, { 1, 1 } }, { component_names::G, { 1, 1 } }, { component_names::B, { 1, 1 } }, { component_names::A, { 1, 1 } } } },
            // Red-Green-Blue
            { sdp::samplings::RGB, { { component_names::R, { 1, 1 } }, { component_names::G, { 1, 1 } }, { component_names::B, { 1, 1 } } } },
            // Non-constant luminance YCbCr
            { sdp::samplings::YCbCr_4_4_4, { { component_names::Y, { 1, 1 } }, { component_names::Cb, { 1, 1 } }, { component_names::Cr, { 1, 1 } } } },
            { sdp::samplings::YCbCr_4_2_2, { { component_names::Y, { 1, 1 } }, { component_names::Cb, { 2, 1 } }, { component_names::Cr, { 2, 1 } } } },
            { sdp::samplings::YCbCr_4_2_0, { { component_names::Y, { 1, 1 } }, { component_names::Cb, { 2, 2 } }, { component_names::Cr, { 2, 2 } } } },
            { sdp::samplings::YCbCr_4_1_1, { { component_names::Y, { 1, 1 } }, { component_names::Cb, { 4, 1 } }, { component_names::Cr, { 4, 1 } } } },
            // Constant luminance YCbCr
            { sdp::samplings::CLYCbCr_4_4_4, { { component_names::Yc, { 1, 1 } }, { component_names::Cbc, { 1, 1 } }, { component_names::Crc, { 1, 1 } } } },
            { sdp::samplings::CLYCbCr_4_2_2, { { component_names::Yc, { 1, 1 } }, { component_names::Cbc, { 2, 1 } }, { component_names::Crc, { 2, 1 } } } },
            { sdp::samplings::CLYCbCr_4_2_0, { { component_names::Yc, { 1, 1 } }, { component_names::Cbc, { 2, 2 } }, { component_names::Crc, { 2, 2 } } } },
            // Constant intensity ICtCp
            { sdp::samplings::ICtCp_4_4_4, { { component_names::I, { 1, 1 } }, { component_names::Ct, { 1, 1 } }, { component_names::Cp, { 1, 1 } } } },
            { sdp::samplings::ICtCp_4_2_2, { { component_names::I, { 1, 1 } }, { component_names::Ct, { 2, 1 } }, { component_names::Cp, { 2, 1 } } } },
            { sdp::samplings::ICtCp_4_2_0, { { component_names::I, { 1, 1 } }, { component_names::Ct, { 2, 2 } }, { component_names::Cp, { 2, 2 } } } },
            // XYZ
            { sdp::samplings::XYZ, { { component_names::X, { 1, 1 } }, { component_names::Y, { 1, 1 } }, { component_names::Z, { 1, 1 } } } },
            // Key signal represented as a single component
            { sdp::samplings::KEY, { { component_names::Key, { 1, 1 } } } },
            // Sampling signaled by the payload
            { sdp::samplings::UNSPECIFIED, {} }
        };
    }

    web::json::value make_components(const sdp::sampling& sampling, uint32_t width, uint32_t height, uint32_t depth)
    {
        using web::json::value;
        using web::json::value_from_elements;

        const auto sampler = nmos::details::samplers.find(sampling);
        if (nmos::details::samplers.end() == sampler) return value::null();

        return value_from_elements(sampler->second | boost::adaptors::transformed([&](const std::pair<component_name, nmos::details::width_height_t>& component)
        {
            return make_component(component.first, width / component.second.first, height / component.second.second, depth);
        }));
    }

    namespace details
    {
        sdp::sampling make_sampling(const web::json::array& components)
        {
            // https://tools.ietf.org/html/rfc4175#section-6.1

            // each entry of the array indicates the number of samples in horizontal and vertical direction for the specified component
            // which depends on the frame width and height, whereas the sampling values only indicate the relative (sub-)sampling frequency
            // so to account for this and handle any order of the components in the array, convert into a map from component name to
            // relative sampling period, i.e. components array of Y@1920x1080, Cb@960x540, Cr@960x540 is converted into the relative
            // component sampler Y@1x1, Cb@2x2, Cr@2x2, which is YCbCr-4:2:0

            typedef std::map<component_name, width_height_t> components_t;
            typedef std::map<components_t, sdp::sampling> samplers_t;

            static const auto samplers = boost::copy_range<samplers_t>(nmos::details::samplers | boost::adaptors::transformed([](const std::pair<sdp::sampling, std::vector<std::pair<component_name, width_height_t>>>& sampler)
            {
                return samplers_t::value_type{ boost::copy_range<components_t>(sampler.second), sampler.first };
            }));

            const auto max_samples = boost::accumulate(components | boost::adaptors::transformed([](const web::json::value& component)
            {
                return width_height_t{ nmos::fields::width(component), nmos::fields::height(component) };
            }), width_height_t{}, [](const width_height_t& lhs, const width_height_t& rhs)
            {
                return width_height_t{ (std::max)(lhs.first, rhs.first), (std::max)(lhs.second, rhs.second) };
            });
            const auto components_sampler = boost::copy_range<components_t>(components | boost::adaptors::transformed([&max_samples](const web::json::value& component)
            {
                const width_height_t samples{ nmos::fields::width(component), nmos::fields::height(component) };
                if (0 != max_samples.first % samples.first || 0 != max_samples.second % samples.second) throw sdp_creation_error("unsupported components");
                return components_t::value_type{ component_name{ nmos::fields::name(component) }, { max_samples.first / samples.first, max_samples.second / samples.second } };
            }));

            const auto sampler = samplers.find(components_sampler);
            if (samplers.end() == sampler) throw sdp_creation_error("unsupported components");

            return sampler->second;
        }

        // Exact Frame Rate
        // "Integer frame rates shall be signaled as a single decimal number (e.g. "25") whilst non-integer frame rates shall be
        // signaled as a ratio of two integer decimal numbers separated by a "forward-slash" character (e.g. "30000/1001"),
        // utilizing the numerically smallest numerator value possible."
        // See ST 2110-20:2017 Section 7.2 Required Media Type Parameters
        utility::string_t make_exactframerate(const nmos::rational& exactframerate)
        {
            return exactframerate.denominator() != 1
                ? utility::ostringstreamed(exactframerate.numerator()) + U("/") + utility::ostringstreamed(exactframerate.denominator())
                : utility::ostringstreamed(exactframerate.numerator());
        }

        nmos::rational parse_exactframerate(const utility::string_t& exactframerate)
        {
            const auto slash = exactframerate.find(U('/'));
            return utility::string_t::npos != slash
                ? nmos::rational(utility::istringstreamed<int64_t>(exactframerate.substr(0, slash)), utility::istringstreamed<int64_t>(exactframerate.substr(slash + 1)))
                : nmos::rational(utility::istringstreamed<int64_t>(exactframerate));
        }

        // Pixel Aspect Ratio
        // "PAR shall be signaled as a ratio of two integer decimal numbers separated by a "colon" character (e.g. "12:11")."
        // See ST 2110-20:2017 Section 7.3 Media Type Parameters with default values
        utility::string_t make_pixel_aspect_ratio(const nmos::rational& par)
        {
            return utility::ostringstreamed(par.numerator()) + U(":") + utility::ostringstreamed(par.denominator());
        }

        nmos::rational parse_pixel_aspect_ratio(const utility::string_t& par)
        {
            const auto slash = par.find(U(':'));
            return utility::string_t::npos != slash
                ? nmos::rational(utility::istringstreamed<int64_t>(par.substr(0, slash)), utility::istringstreamed<int64_t>(par.substr(slash + 1)))
                : nmos::rational(utility::istringstreamed<int64_t>(par));
        }

        // Construct simple media stream ids based on the sender's number of legs
        std::vector<utility::string_t> make_media_stream_ids(const web::json::value& sender)
        {
            const auto legs = nmos::fields::interface_bindings(sender).size();
            return boost::copy_range<std::vector<utility::string_t>>(boost::irange(0, (int)legs) | boost::adaptors::transformed([&](const int& index)
            {
                return utility::ostringstreamed(index);
            }));
        }
    }

    // Construct additional "video/raw" parameters from the IS-04 resources, using default values for unspecified items
    video_raw_parameters make_video_raw_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<sdp::type_parameter> tp)
    {
        video_raw_parameters params;

        // fmtp

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
        // RFC 4175 top-field-first refers to a specific payload packing option for chrominance samples in 4:2:0 video;
        // it does not correspond to nmos::interlace_modes::interlaced_tff

        // map directly
        params.tcs = sdp::transfer_characteristic_system{ nmos::fields::transfer_characteristic(flow) };
        params.colorimetry = sdp::colorimetry{ nmos::fields::colorspace(flow) };

        // hm, RANGE and PAR not currently indicated in IS-04 so omit these

        // hm, PM not indicated in IS-04 so default this
        params.pm = sdp::packing_modes::general;

        // "Senders implementing this standard shall signal the value ST2110-20:2017 unless the colorimetry value ALPHA
        // or the TCS value ST2115LOGS3 are used, in which case the value ST2110-20:2022 shall be signaled."
        params.ssn = params.colorimetry == sdp::colorimetries::ALPHA || params.tcs == sdp::transfer_characteristic_systems::ST2115LOGS3
            ? sdp::smpte_standard_numbers::ST2110_20_2022
            : sdp::smpte_standard_numbers::ST2110_20_2017;

        // hm, TP is equivalent to the new sender attribute, but for now, support optional override and default value
        params.tp = tp
            ? *tp
            : sender.has_field(nmos::fields::st2110_21_sender_type)
                ? sdp::type_parameter{ nmos::fields::st2110_21_sender_type(sender) }
                : sdp::type_parameters::type_N;

        // hm, ST 2110-21 TROFF and CMAX not indicated in IS-04 so omit these
        // hm, ST 2110-10 MAXUDP, TSMODE and TSDELAY not indicated in IS-04 so omit these

        return params;
    }

    // deprecated, use make_video_raw_parameters and then make_video_raw_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_video_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<sdp::type_parameter> tp)
    {
        auto params = make_video_raw_parameters(node, source, flow, sender, tp);
        return{ nmos::fields::label(sender), params, payload_type ? *payload_type : details::payload_type_video_default, !media_stream_ids.empty() ? media_stream_ids : details::make_media_stream_ids(sender), details::make_ts_refclk(node, source, sender, ptp_domain) };
    }

    // Construct additional "audio/L" parameters from the IS-04 resources, using default values for unspecified items
    audio_L_parameters make_audio_L_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<double> packet_time)
    {
        audio_L_parameters params;

        // rtpmap

        params.channel_count = (uint32_t)nmos::fields::channels(source).size();
        params.bit_depth = nmos::fields::bit_depth(flow);
        const auto& sample_rate(flow.at(nmos::fields::sample_rate));
        params.sample_rate = uint64_t(double(nmos::fields::numerator(sample_rate)) / double(nmos::fields::denominator(sample_rate)) + 0.5);

        // fmtp

        const auto channel_symbols = boost::copy_range<std::vector<nmos::channel_symbol>>(nmos::fields::channels(source) | boost::adaptors::transformed([](const web::json::value& channel)
        {
            return channel_symbol{ nmos::fields::symbol(channel) };
        }));
        params.channel_order = nmos::make_fmtp_channel_order(channel_symbols);

        // hm, ST 2110-10 TSMODE and TSDELAY not indicated in IS-04 so omit these

        // ptime, e.g. 1 ms or 0.125 ms
        // hm, there's a parameter constraint defined in the Capabilities register
        // but there isn't (yet?) an equivalent Sender Attributes register entry
        params.packet_time = packet_time ? *packet_time : 1;

        return params;
    }

    // deprecated, use make_audio_L_parameters and then make_audio_L_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_audio_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<double> packet_time)
    {
        auto params = make_audio_L_parameters(node, source, flow, sender, packet_time);
        return{ nmos::fields::label(sender), params, payload_type ? *payload_type : details::payload_type_audio_default, !media_stream_ids.empty() ? media_stream_ids : details::make_media_stream_ids(sender), details::make_ts_refclk(node, source, sender, ptp_domain) };
    }

    // Construct additional "video/smpte291" parameters from the IS-04 resources, using default values for unspecified items
    video_smpte291_parameters make_video_smpte291_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<nmos::vpid_code> vpid_code, bst::optional<sdp::transmission_model> tm)
    {
        video_smpte291_parameters params;

        // fmtp

        // maps directly
        params.did_sdids = boost::copy_range<std::vector<nmos::did_sdid>>(nmos::fields::DID_SDID(flow).as_array() | boost::adaptors::transformed([](const web::json::value& did_sdid)
        {
            return nmos::parse_did_sdid(did_sdid);
        }));

        // hm, VPID_Code not currently indicated in IS-04
        params.vpid_code = vpid_code ? *vpid_code : 0;

        // grain_rate is optional in the flow, but if it's not there, it should be in the source
        const auto& grain_rate = flow.has_field(nmos::fields::grain_rate)
            ? nmos::fields::grain_rate(flow)
            : source.has_field(nmos::fields::grain_rate)
                ? nmos::fields::grain_rate(source)
                : nmos::make_rational(0);
        params.exactframerate = nmos::parse_rational(grain_rate);

        // hm, TM not indicated in IS-04
        if (tm) params.tm = *tm;

        params.ssn = params.tm.empty() ? sdp::smpte_standard_numbers::ST2110_40_2018 : sdp::smpte_standard_numbers::ST2110_40_2023;

        // hm, ST 2110-21 TROFF not indicated in IS-04 so omit this
        // hm, ST 2110-10 TSMODE and TSDELAY not indicated in IS-04 so omit these

        return params;
    }

    // deprecated, use make_video_smpte291_parameters and then make_video_smpte291_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_data_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<nmos::vpid_code> vpid_code)
    {
        auto params = make_video_smpte291_parameters(node, source, flow, sender, vpid_code);
        return{ nmos::fields::label(sender), params, payload_type ? *payload_type : details::payload_type_data_default, !media_stream_ids.empty() ? media_stream_ids : details::make_media_stream_ids(sender), details::make_ts_refclk(node, source, sender, ptp_domain) };
    }

    // Construct additional "video/SMPTE2022-6" parameters from the IS-04 resources, using default values for unspecified items
    video_SMPTE2022_6_parameters make_video_SMPTE2022_6_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<sdp::type_parameter> tp)
    {
        sdp_parameters::mux_t params;

        // fmtp

        // "Senders shall comply with either the Narrow Linear Senders (Type NL) requirements, or the Wide Senders (Type W) requirements."
        // See SMPTE ST 2022-8:2019 Section 6 Network Compatibility and Transmission Traffic Shape Models
        // hm, TP is equivalent to the new sender attribute, but for now, support optional override and default value
        params.tp = tp
            ? *tp
            : sender.has_field(nmos::fields::st2110_21_sender_type)
                ? sdp::type_parameter{ nmos::fields::st2110_21_sender_type(sender) }
                : sdp::type_parameters::type_NL;

        // hm, ST 2110-21 TROFF not indicated in IS-04 so omit this

        return params;
    }

    // deprecated, use make_video_SMPTE2022_6_parameters and then make_video_SMPTE2022_6_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_mux_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, bst::optional<uint64_t> payload_type, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain, bst::optional<sdp::type_parameter> tp)
    {
        auto params = make_video_SMPTE2022_6_parameters(node, source, flow, sender, tp);
        return{ nmos::fields::label(sender), params, payload_type ? *payload_type : details::payload_type_mux_default, !media_stream_ids.empty() ? media_stream_ids : details::make_media_stream_ids(sender), details::make_ts_refclk(node, source, sender, ptp_domain) };
    }

    // Construct SDP parameters from the IS-04 resources for "video/raw", "audio/L", "video/smpte291" and "video/SMPTE2022-6"
    // using default values for unspecified items

    // deprecated, use format-specific make_<media type/subtype>_parameters and then make_<media type/subtype>_sdp_parameters or equivalent overload of make_sdp_parameters
    sdp_parameters make_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids, bst::optional<int> ptp_domain)
    {
        const auto& format = nmos::fields::format(flow);
        if (nmos::formats::video.name == format)
            return make_video_sdp_parameters(node, source, flow, sender, {}, media_stream_ids, ptp_domain, {});
        else if (nmos::formats::audio.name == format)
            return make_audio_sdp_parameters(node, source, flow, sender, {}, media_stream_ids, ptp_domain, {});
        else if (nmos::formats::data.name == format)
            return make_data_sdp_parameters(node, source, flow, sender, {}, media_stream_ids, ptp_domain, {});
        else if (nmos::formats::mux.name == format)
            return make_mux_sdp_parameters(node, source, flow, sender, {}, media_stream_ids, ptp_domain, {});
        else
            throw details::sdp_creation_error("unsupported media format");
    }

    // deprecated, provided for backwards compatibility, because it may be necessary to also specify the PTP domain to generate an RFC 7273 'ts-refclk' attribute that meets the additional constraints of ST 2110-10
    sdp_parameters make_sdp_parameters(const web::json::value& node, const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids)
    {
        return make_sdp_parameters(node, source, flow, sender, media_stream_ids, bst::nullopt);
    }

    namespace details
    {
        // "<unicast-address> is the address of the machine from which the session was created."
        // See https://tools.ietf.org/html/rfc4566#section-5.2
        const utility::string_t& get_origin_address(const web::json::value& transport_params)
        {
            // this doesn't need to be the source address of the data stream
            // e.g. a host_address which was a management interface would be OK
            // however, by convention, use the source_ip of the first leg for
            // an SDP file created at a Sender and the interface_ip of the
            // first leg for an SDP created at a Receiver
            const auto& transport_param = transport_params.at(0);
            const auto& interface_ip = nmos::fields::interface_ip(transport_param);
            // a Receiver always has an interface_ip
            if (!interface_ip.is_null()) return interface_ip.as_string();
            const auto& source_ip = nmos::fields::source_ip(transport_param);
            // a Sender always has a source_ip
            return source_ip.as_string();
        }

        // "If the session is multicast, the connection address will be an IP multicast group address.
        // If the session is not multicast, then the connection address contains the unicast IP address of the
        // expected [...] data sink."
        // See https://tools.ietf.org/html/rfc4566#section-5.7
        const utility::string_t& get_connection_address(const web::json::value& transport_param)
        {
            const auto& multicast_ip = nmos::fields::multicast_ip(transport_param);
            // a Receiver has a multicast_ip for multicast but not unicast
            if (!multicast_ip.is_null()) return multicast_ip.as_string();
            const auto& interface_ip = nmos::fields::interface_ip(transport_param);
            // a Receiver always has a (unicast) interface_ip
            if (!interface_ip.is_null()) return interface_ip.as_string();
            // a Sender always has a destination_ip which may be multicast or unicast
            const auto& destination_ip = nmos::fields::destination_ip(transport_param);
            return destination_ip.as_string();
        }
    }

    // Make a json representation of an SDP file, e.g. for sdp::make_session_description, from the specified parameters; explicitly specify whether 'source-filter' attributes are included to override the default behaviour
    static web::json::value make_session_description(const sdp_parameters& sdp_params, const web::json::value& transport_params, const web::json::value& rtpmap, const web::json::value& fmtp, bst::optional<bool> source_filters)
    {
        using web::json::value;
        using web::json::value_of;

        // note that a media description is always created for each leg in the transport_params
        // and the rtp_enabled status does not affect the leg's media description
        // see https://github.com/AMWA-TV/is-05/issues/109#issuecomment-598721418

        // check to ensure enough media_stream_ids for multi-leg transport_params
        if (transport_params.size() > 1 && transport_params.size() > sdp_params.group.media_stream_ids.size())
        {
            throw details::sdp_creation_error("not enough sdp parameters media stream ids for transport_params");
        }
        // for backward compatibility, use default ts-refclk when not (enough) specified
        const auto ts_refclk_default = sdp_parameters::ts_refclk_t::ptp_traceable();

        // so far so good, now build the session_description

        const bool keep_order = true;

        const auto& origin_address = details::get_origin_address(transport_params);

        auto session_description = value_of({
            // Protocol Version
            // See https://tools.ietf.org/html/rfc4566#section-5.1
            { sdp::fields::protocol_version, 0 },

            // Origin
            // See https://tools.ietf.org/html/rfc4566#section-5.2
            { sdp::fields::origin, value_of({
                { sdp::fields::user_name, sdp_params.origin.user_name },
                { sdp::fields::session_id, sdp_params.origin.session_id },
                { sdp::fields::session_version, sdp_params.origin.session_version },
                { sdp::fields::network_type, sdp::network_types::internet.name },
                { sdp::fields::address_type, details::get_address_type_multicast(origin_address).first.name },
                { sdp::fields::unicast_address, origin_address }
            }, keep_order) },

            // Session Name
            // See https://tools.ietf.org/html/rfc4566#section-5.3
            { sdp::fields::session_name, sdp_params.session_name },

            // Time Descriptions
            // See https://tools.ietf.org/html/rfc4566#section-5
            { sdp::fields::time_descriptions, value_of({
                value_of({
                    { sdp::fields::timing, web::json::value_of({
                        { sdp::fields::start_time, sdp_params.timing.start_time },
                        { sdp::fields::stop_time, sdp_params.timing.stop_time }
                    }) }
                }, keep_order)
            }) },

            // Attributes
            // See https://tools.ietf.org/html/rfc4566#section-5.13
            { sdp::fields::attributes, value::array() },

            // Media Descriptions
            // See https://tools.ietf.org/html/rfc4566#section-5
            { sdp::fields::media_descriptions, value::array() }
        }, keep_order);

        auto& session_attributes = session_description.at(sdp::fields::attributes);
        auto& media_descriptions = session_description.at(sdp::fields::media_descriptions);

        // group & mid attributes
        // see https://tools.ietf.org/html/rfc5888
        auto mids = value::array();

        size_t leg = 0;
        for (const auto& transport_param : transport_params.as_array())
        {
            const auto& connection_address = details::get_connection_address(transport_param);
            const auto& address_type_multicast = details::get_address_type_multicast(connection_address);

            const auto& ts_refclk = sdp_params.ts_refclk.size() > leg
                ? sdp_params.ts_refclk[leg]
                : ts_refclk_default;

            auto media_description = value_of({
                // Media
                // See https://tools.ietf.org/html/rfc4566#section-5.14
                { sdp::fields::media, value_of({
                    { sdp::fields::media_type, sdp_params.media_type.name },
                    { sdp::fields::port, transport_param.at(nmos::fields::destination_port) },
                    { sdp::fields::protocol, sdp_params.protocol.name },
                    { sdp::fields::formats, value_of({ utility::ostringstreamed(sdp_params.rtpmap.payload_type) }) }
                }, keep_order) },

                // Connection Data
                // See https://tools.ietf.org/html/rfc4566#section-5.7
                { sdp::fields::connection_data, value_of({
                    value_of({
                        { sdp::fields::network_type, sdp::network_types::internet.name },
                        { sdp::fields::address_type, address_type_multicast.first.name },
                        { sdp::fields::connection_address, sdp::address_types::IP4 == address_type_multicast.first && address_type_multicast.second
                            ? connection_address + U("/") + utility::ostringstreamed(sdp_params.connection_data.ttl)
                            : connection_address }
                    }, keep_order)
                }) },

                // Bandwidth
                // See https://tools.ietf.org/html/rfc4566#section-5.8
                { !sdp_params.bandwidth.bandwidth_type.empty() ? sdp::fields::bandwidth_information.key : U(""), value_of({
                    value_of({
                        { sdp::fields::bandwidth_type, sdp_params.bandwidth.bandwidth_type.name },
                        { sdp::fields::bandwidth, sdp_params.bandwidth.bandwidth }
                    }, keep_order)
                }) },

                // Attributes
                // See https://tools.ietf.org/html/rfc4566#section-5.13
                { sdp::fields::attributes, value::array() }

            }, keep_order);

            auto& media_attributes = media_description.at(sdp::fields::attributes);

            // insert ts-refclk if specified
            if (ts_refclk.clock_source != sdp::ts_refclk_source{})
            {
                // a=ts-refclk:ptp=<ptp version>:<ptp gmid>[:<ptp domain>]
                // a=ts-refclk:ptp=<ptp version>:traceable
                // See https://tools.ietf.org/html/rfc7273
                // a=ts-refclk:localmac=<mac-address-of-sender>
                // See SMPTE ST 2110-10:2017 Professional Media Over Managed IP Networks: System Timing and Definitions, Section 8.2 Reference Clock
                web::json::push_back(
                    media_attributes, value_of({
                        { sdp::fields::name, sdp::attributes::ts_refclk },
                        { sdp::fields::value, sdp::ts_refclk_sources::ptp == ts_refclk.clock_source ? ts_refclk.ptp_server.empty() ? value_of({
                            { sdp::fields::clock_source, sdp::ts_refclk_sources::ptp.name },
                            { sdp::fields::ptp_version, ts_refclk.ptp_version.name },
                            { sdp::fields::traceable, true }
                        }, keep_order) : value_of({
                            { sdp::fields::clock_source, sdp::ts_refclk_sources::ptp.name },
                            { sdp::fields::ptp_version, ts_refclk.ptp_version.name },
                            { sdp::fields::ptp_server, ts_refclk.ptp_server }
                        }, keep_order) : sdp::ts_refclk_sources::local_mac == ts_refclk.clock_source ? value_of({
                            { sdp::fields::clock_source, sdp::ts_refclk_sources::local_mac.name },
                            { sdp::fields::mac_address, ts_refclk.mac_address }
                        }, keep_order) : value::null() }
                    }, keep_order)
                );
            }

            // insert mediaclk if specified
            if (sdp_params.mediaclk.clock_source != sdp::mediaclk_source{})
            {
                // a=mediaclk:[id=<clock id> ]<clock source>[=<clock parameters>]
                // See https://tools.ietf.org/html/rfc7273#section-5
                web::json::push_back(
                    media_attributes, value_of({
                        { sdp::fields::name, sdp::attributes::mediaclk },
                        { sdp::fields::value, !sdp_params.mediaclk.clock_parameters.empty()
                            ? sdp_params.mediaclk.clock_source.name + U("=") + sdp_params.mediaclk.clock_parameters
                            : sdp_params.mediaclk.clock_source.name }
                    }, keep_order)
                );
            }

            // insert source-filter if source address is specified, depending on source_filters
            // for now, when source_filters does not contain an explicit value, the default is to include the source-filter attribute
            // another choice would be to do so only for source-specific multicast addresses (232.0.0.0-232.255.255.255)
            const auto& source_ip = nmos::fields::source_ip(transport_param);
            if (!source_ip.is_null() && (!source_filters || *source_filters))
            {
                // a=source-filter: <filter-mode> <nettype> <address-types> <dest-address> <src-list>
                // See https://tools.ietf.org/html/rfc4570
                web::json::push_back(
                    media_attributes, value_of({
                        { sdp::fields::name, sdp::attributes::source_filter },
                        { sdp::fields::value, value_of({
                            { sdp::fields::filter_mode, sdp::filter_modes::incl.name },
                            { sdp::fields::network_type, sdp::network_types::internet.name },
                            { sdp::fields::address_types, address_type_multicast.first.name },
                            { sdp::fields::destination_address, connection_address },
                            { sdp::fields::source_addresses, value_of({ source_ip }) }
                        }, keep_order) }
                    }, keep_order)
                );
            }

            if (0 != sdp_params.packet_time)
            {
                // a=ptime:<packet time>
                // See https://tools.ietf.org/html/rfc4566#section-6
                web::json::push_back(
                    media_attributes, value_of({
                        { sdp::fields::name, sdp::attributes::ptime },
                        { sdp::fields::value, sdp_params.packet_time }
                    }, keep_order)
                );
            }

            if (0 != sdp_params.max_packet_time)
            {
                // a=maxptime:<packet time>
                // See https://tools.ietf.org/html/rfc4566#section-6
                web::json::push_back(
                    media_attributes, value_of({
                        { sdp::fields::name, sdp::attributes::maxptime },
                        { sdp::fields::value, sdp_params.max_packet_time }
                    }, keep_order)
                );
            }

            // insert rtpmap if specified
            if (!rtpmap.is_null())
            {
                // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
                // See https://tools.ietf.org/html/rfc4566#section-6
                web::json::push_back(media_attributes, rtpmap);
            }

            // insert framerate if specified
            if (0 != sdp_params.framerate)
            {
                // a=framerate:<frame rate>
                // See https://tools.ietf.org/html/rfc4566#section-6
                web::json::push_back(
                    media_attributes, value_of({
                        { sdp::fields::name, sdp::attributes::framerate },
                        { sdp::fields::value, sdp_params.framerate }
                    }, keep_order)
                );
            }

            // insert fmtp if specified
            if (!fmtp.is_null())
            {
                // a=fmtp:<format> <format specific parameters>
                // See https://tools.ietf.org/html/rfc4566#section-6
                web::json::push_back(media_attributes, fmtp);
            }

            // insert "media stream identification" if there is more than 1 leg
            if (transport_params.size() > 1)
            {
                // a=mid:<identification-tag>
                // See https://tools.ietf.org/html/rfc5888
                const auto& mid = sdp_params.group.media_stream_ids[leg];

                // build up mids based on group::media_stream_ids
                web::json::push_back(mids, mid);

                web::json::push_back(
                    media_attributes, value_of({
                        { sdp::fields::name, sdp::attributes::mid },
                        { sdp::fields::value, mid }
                    }, keep_order)
                );
            }

            web::json::push_back(media_descriptions, std::move(media_description));

            ++leg;
        }

        // add group attribute if there is more than 1 leg
        if (mids.size() > 1)
        {
            // a=group:<semantics>[ <identification-tag>]*
            // See https://tools.ietf.org/html/rfc5888
            web::json::push_back(
                session_attributes, value_of({
                    { sdp::fields::name, sdp::attributes::group },
                    { sdp::fields::value, web::json::value_of({
                        { sdp::fields::semantics, sdp_params.group.semantics.name },
                        { sdp::fields::mids, std::move(mids) }
                    }, keep_order) },
                }, keep_order)
            );
        }

        return session_description;
    }

    static web::json::value make_rtpmap(const sdp_parameters& sdp_params)
    {
        using web::json::value_of;

        const bool keep_order = true;

        return value_of({
            { sdp::fields::name, sdp::attributes::rtpmap },
            { sdp::fields::value, value_of({
                { sdp::fields::payload_type, sdp_params.rtpmap.payload_type },
                { sdp::fields::encoding_name, sdp_params.rtpmap.encoding_name },
                { sdp::fields::clock_rate, sdp_params.rtpmap.clock_rate },
                { sdp_params.rtpmap.encoding_parameters > 0 ? sdp::fields::encoding_parameters.key : U(""), sdp_params.rtpmap.encoding_parameters }
            }, keep_order) }
        }, keep_order);
    }

    static web::json::value make_fmtp(const sdp_parameters& sdp_params)
    {
        using web::json::value;
        using web::json::value_from_elements;
        using web::json::value_of;

        const bool keep_order = true;

        return sdp_params.fmtp.empty() ? value::null() : value_of({
            { sdp::fields::name, sdp::attributes::fmtp },
            { sdp::fields::value, web::json::value_of({
                { sdp::fields::format, utility::ostringstreamed(sdp_params.rtpmap.payload_type) },
                { sdp::fields::format_specific_parameters, value_from_elements(sdp_params.fmtp | boost::adaptors::transformed([&](const sdp_parameters::fmtp_t::value_type& param)
                    {
                        return sdp::named_value(param.first, param.second, keep_order);
                    })) }
            }, keep_order) }
        }, keep_order);
    }

    // Construct SDP parameters for "video/raw", with sensible defaults for unspecified fields
    sdp_parameters make_video_raw_sdp_parameters(const utility::string_t& session_name, const video_raw_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk)
    {
        // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
        // See https://tools.ietf.org/html/rfc4566#section-6
        sdp_parameters::rtpmap_t rtpmap = { payload_type, U("raw"), 90000 };

        // a=fmtp:<format> <format specific parameters>
        // for simplicity, following the order of parameters given in VSF TR-05:2017
        // See https://tools.ietf.org/html/rfc4566#section-6
        // and http://www.videoservicesforum.org/download/technical_recommendations/VSF_TR-05_2018-06-23.pdf
        // and comments regarding the fmtp attribute parameters in get_session_description_sdp_parameters
        sdp_parameters::fmtp_t fmtp = {
            { sdp::fields::width, utility::ostringstreamed(params.width) },
            { sdp::fields::height, utility::ostringstreamed(params.height) },
            { sdp::fields::exactframerate, nmos::details::make_exactframerate(params.exactframerate) }
        };
        if (params.interlace) fmtp.push_back({ sdp::fields::interlace, {} });
        if (params.segmented) fmtp.push_back({ sdp::fields::segmented, {} });
        fmtp.push_back({ sdp::fields::sampling, params.sampling.name });
        fmtp.push_back({ sdp::fields::depth, utility::ostringstreamed(params.depth) });
        fmtp.push_back({ sdp::fields::colorimetry, params.colorimetry.name });
        if (!params.tcs.empty()) fmtp.push_back({ sdp::fields::transfer_characteristic_system, params.tcs.name });
        if (!params.pm.empty()) fmtp.push_back({ sdp::fields::packing_mode, params.pm.name });
        if (!params.ssn.empty()) fmtp.push_back({ sdp::fields::smpte_standard_number, params.ssn.name });
        if (!params.tp.empty()) fmtp.push_back({ sdp::fields::type_parameter, params.tp.name });

        // additional parameters introduced by SMPTE specs since then...
        if (!params.range.empty()) fmtp.push_back({ sdp::fields::range, params.range.name });
        if (0 != params.par) fmtp.push_back({ sdp::fields::pixel_aspect_ratio, nmos::details::make_pixel_aspect_ratio(params.par) });
        if (params.troff) fmtp.push_back({ sdp::fields::TROFF, utility::ostringstreamed(*params.troff) });
        if (0 != params.cmax) fmtp.push_back({ sdp::fields::CMAX, utility::ostringstreamed(params.cmax) });
        if (0 != params.maxudp) fmtp.push_back({ sdp::fields::max_udp_packet_size, utility::ostringstreamed(params.maxudp) });
        if (!params.tsmode.empty()) fmtp.push_back({ sdp::fields::timestamp_mode, params.tsmode.name });
        if (params.tsdelay) fmtp.push_back({ sdp::fields::timestamp_delay, utility::ostringstreamed(*params.tsdelay) });

        return{ session_name, sdp::media_types::video, rtpmap, fmtp, {}, {}, {}, {}, media_stream_ids, ts_refclk };
    }

    // Construct SDP parameters for "audio/L", with sensible defaults for unspecified fields
    sdp_parameters make_audio_L_sdp_parameters(const utility::string_t& session_name, const audio_L_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk)
    {
        // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
        // See https://tools.ietf.org/html/rfc4566#section-6
        sdp_parameters::rtpmap_t rtpmap = { payload_type, U("L") + utility::ostringstreamed(params.bit_depth), params.sample_rate, params.channel_count };

        // a=fmtp:<format> <format specific parameters>
        // See https://tools.ietf.org/html/rfc4566#section-6
        sdp_parameters::fmtp_t fmtp = {};
        if (!params.channel_order.empty()) fmtp.push_back({ sdp::fields::channel_order, params.channel_order });
        if (!params.tsmode.empty()) fmtp.push_back({ sdp::fields::timestamp_mode, params.tsmode.name });
        if (params.tsdelay) fmtp.push_back({ sdp::fields::timestamp_delay, utility::ostringstreamed(*params.tsdelay) });

        return{ session_name, sdp::media_types::audio, rtpmap, fmtp, {}, params.packet_time, {}, {}, media_stream_ids, ts_refclk };
    }

    // Construct SDP parameters for "video/smpte291", with sensible defaults for unspecified fields
    sdp_parameters make_video_smpte291_sdp_parameters(const utility::string_t& session_name, const video_smpte291_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk)
    {
        // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
        // See https://tools.ietf.org/html/rfc4566#section-6
        sdp_parameters::rtpmap_t rtpmap = { payload_type, U("smpte291"), 90000 };

        // a=fmtp:<format> <format specific parameters>
        // See https://tools.ietf.org/html/rfc4566#section-6
        sdp_parameters::fmtp_t fmtp = boost::copy_range<sdp_parameters::fmtp_t>(params.did_sdids | boost::adaptors::transformed([](const nmos::did_sdid& did_sdid)
        {
            return sdp_parameters::fmtp_t::value_type{ sdp::fields::DID_SDID, make_fmtp_did_sdid(did_sdid) };
        }));
        if (0 != params.vpid_code) fmtp.push_back({ sdp::fields::VPID_Code, utility::ostringstreamed((uint32_t)params.vpid_code) });
        if (0 != params.exactframerate) fmtp.push_back({ sdp::fields::exactframerate, nmos::details::make_exactframerate(params.exactframerate) });
        if (!params.tm.empty()) fmtp.push_back({ sdp::fields::TM, params.tm.name });
        if (!params.ssn.empty()) fmtp.push_back({ sdp::fields::smpte_standard_number, params.ssn.name });
        if (params.troff) fmtp.push_back({ sdp::fields::TROFF, utility::ostringstreamed(*params.troff) });
        if (!params.tsmode.empty()) fmtp.push_back({ sdp::fields::timestamp_mode, params.tsmode.name });
        if (params.tsdelay) fmtp.push_back({ sdp::fields::timestamp_delay, utility::ostringstreamed(*params.tsdelay) });

        return{ session_name, sdp::media_types::video, rtpmap, fmtp, {}, {}, {}, {}, media_stream_ids, ts_refclk };
    }

    // Construct SDP parameters for "video/SMPTE2022-6", with sensible defaults for unspecified fields
    sdp_parameters make_video_SMPTE2022_6_sdp_parameters(const utility::string_t& session_name, const video_SMPTE2022_6_parameters& params, uint64_t payload_type, const std::vector<utility::string_t>& media_stream_ids, const std::vector<sdp_parameters::ts_refclk_t>& ts_refclk)
    {
        // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
        // See https://tools.ietf.org/html/rfc4566#section-6
        sdp_parameters::rtpmap_t rtpmap = { payload_type, U("SMPTE2022-6"), 27000000 };

        // a=fmtp:<format> <format specific parameters>
        // See https://tools.ietf.org/html/rfc4566#section-6
        sdp_parameters::fmtp_t fmtp = {};
        if (!params.tp.empty()) fmtp.push_back({ sdp::fields::type_parameter, params.tp.name });
        if (params.troff) fmtp.push_back({ sdp::fields::TROFF, utility::ostringstreamed(*params.troff) });

        return{ session_name, sdp::media_types::video, rtpmap, fmtp, {}, {}, {}, {}, media_stream_ids, ts_refclk };
    }

    media_type get_media_type(const sdp_parameters& sdp_params)
    {
        return media_type{ sdp_params.media_type.name + U("/") + sdp_params.rtpmap.encoding_name };
    }

    web::json::value make_session_description(const sdp_parameters& sdp_params, const web::json::value& transport_params, bst::optional<bool> source_filters)
    {
        return make_session_description(sdp_params, transport_params, make_rtpmap(sdp_params), make_fmtp(sdp_params), source_filters);
    }

    namespace details
    {
        // Syntax of <connection-address> depends on <addrtype>:
        // IP4 <unicast address>
        // IP6 <unicast address>
        // IP4 <base multicast address>/<ttl>[/<number of addresses>]
        // IP6 <base multicast address>[/<number of addresses>]
        // See https://tools.ietf.org/html/rfc4566#section-5.7
        struct connection_address
        {
            utility::string_t base_address;
            uint32_t ttl;
            uint32_t number_of_addresses;

            connection_address(const utility::string_t& base_address, uint32_t ttl, uint32_t number_of_addresses)
                : base_address(base_address)
                , ttl(ttl)
                , number_of_addresses(number_of_addresses)
            {}
        };

        connection_address parse_connection_address(const sdp::address_type& address_type, const utility::string_t& connection_address)
        {
            const auto slash = connection_address.find(U('/'));

            if (utility::string_t::npos == slash) return{ connection_address, 0, 1 };

            if (sdp::address_types::IP6 == address_type)
            {
                return{
                    connection_address.substr(0, slash),
                    0,
                    utility::istringstreamed<uint32_t>(connection_address.substr(slash + 1))
                };
            }
            else // if (sdp::address_types::IP4 == address_type)
            {
                const auto slash2 = connection_address.find(U('/'), slash + 1);
                return{
                    connection_address.substr(0, slash),
                    utility::string_t::npos == slash2
                        ? utility::istringstreamed<uint32_t>(connection_address.substr(slash + 1))
                        : utility::istringstreamed<uint32_t>(connection_address.substr(slash + 1, slash2 - (slash + 1))),
                    utility::string_t::npos == slash2
                        ? 1
                        : utility::istringstreamed<uint32_t>(connection_address.substr(slash2 + 1))
                };
            }
        }

        // Set appropriate transport parameters depending on whether the specified address is multicast
        void set_multicast_ip_interface_ip(web::json::value& params, const utility::string_t& address)
        {
            using web::json::value;

            if (details::get_address_type_multicast(address).second)
            {
                params[nmos::fields::multicast_ip] = value::string(address);
                params[nmos::fields::interface_ip] = value::string(U("auto"));
            }
            else
            {
                params[nmos::fields::multicast_ip] = value::null();
                params[nmos::fields::interface_ip] = value::string(address);
            }
        }
    }

    // Get IS-05 transport parameters from the json representation of an SDP file, e.g. from sdp::parse_session_description
    web::json::value get_session_description_transport_params(const web::json::value& session_description)
    {
        using web::json::value;

        web::json::value transport_params;

        // There isn't much of a specification for interpreting SDP files and updating the
        // equivalent transport parameters, just some examples...
        // See https://specs.amwa.tv/is-05/releases/v1.0.0/docs/4.1._Behaviour_-_RTP_Transport_Type.html#interpretation-of-sdp-files

        // For now, this function should handle the following cases identified in the documentation:
        // * Unicast
        // * Source Specific Multicast
        // * Any Source Multicast
        // * Operation with SMPTE 2022-7 - Separate Source Addresses
        // * Operation with SMPTE 2022-7 - Separate Destination Addresses

        // The following cases are not yet handled:
        // * Operation with SMPTE 2022-5
        // * Operation with SMPTE 2022-7 - Temporal Redundancy
        // * Operation with RTCP

        auto& media_descriptions = sdp::fields::media_descriptions(session_description);

        for (size_t leg = 0; leg < 2; ++leg)
        {
            web::json::value params;

            // source_ip is null when there is no source-filter, indicating that "the source IP address
            // has not been configured in unicast mode, or the Receiver is in any-source multicast mode"
            // see https://specs.amwa.tv/is-05/releases/v1.1.0/APIs/schemas/with-refs/receiver_transport_params_rtp.html
            params[nmos::fields::source_ip] = value::null();

            // session connection data is the default for each media description
            auto& session_connection_data = sdp::fields::connection_data(session_description);
            if (!session_connection_data.is_null())
            {
                // hmm, how to handle multiple connection addresses?
                const auto address_type = sdp::address_type{ sdp::fields::address_type(session_connection_data) };
                const auto connection_address = details::parse_connection_address(address_type, sdp::fields::connection_address(session_connection_data));
                details::set_multicast_ip_interface_ip(params, connection_address.base_address);
            }

            // Look for the media description corresponding to this element in the transport parameters

            auto& mda = media_descriptions.as_array();
            auto source_address = leg;
            for (auto md = mda.begin(); mda.end() != md; ++md)
            {
                auto& media_description = *md;
                auto& media = sdp::fields::media(media_description);

                // check transport protocol (cf. "UDP/FEC" for Operation with SMPTE 2022-5)
                const sdp::protocol protocol{ sdp::fields::protocol(media) };
                if (sdp::protocols::RTP_AVP != protocol) continue;

                // check media type, e.g. "video"
                // (vs. IS-04 receiver's caps.media_types)
                const sdp::media_type media_type{ sdp::fields::media_type(media) };
                if (!(sdp::media_types::video == media_type || sdp::media_types::audio == media_type)) continue;

                // media connection data overrides session connection data

                auto& media_connection_data = sdp::fields::connection_data(media_description);
                if (!media_connection_data.is_null() && 0 != media_connection_data.size())
                {
                    // hmm, how to handle multiple connection addresses?
                    const auto address_type = sdp::address_type{ sdp::fields::address_type(media_connection_data.at(0)) };
                    const auto connection_address = details::parse_connection_address(address_type, sdp::fields::connection_address(media_connection_data.at(0)));
                    details::set_multicast_ip_interface_ip(params, connection_address.base_address);
                }

                // take account of the number of source addresses (cf. Operation with SMPTE 2022-7 - Separate Source Addresses)

                auto& media_attributes = sdp::fields::attributes(media_description);
                if (!media_attributes.is_null())
                {
                    auto& ma = media_attributes.as_array();

                    // hmm, this code assumes that <filter-mode> is 'incl' and ought to check that <nettype>, <address-types> and <dest-address>
                    // match the connection address, and fall back to any "session-level" source-filter values if they don't match

                    auto source_filter = sdp::find_name(ma, sdp::attributes::source_filter);
                    if (ma.end() != source_filter)
                    {
                        auto& sf = sdp::fields::value(*source_filter);
                        auto& sa = sdp::fields::source_addresses(sf);

                        if (sa.size() <= source_address)
                        {
                            source_address -= sa.size();
                            continue;
                        }

                        details::set_multicast_ip_interface_ip(params, sdp::fields::destination_address(sf));
                        params[nmos::fields::source_ip] = sdp::fields::source_addresses(sf).at(source_address);
                        source_address = 0;
                    }
                }

                if (0 != source_address)
                {
                    --source_address;
                    continue;
                }

                params[nmos::fields::destination_port] = value::number(sdp::fields::port(media));

                params[nmos::fields::rtp_enabled] = value::boolean(true);

                web::json::push_back(transport_params, params);

                break;
            }
        }

        return transport_params;
    }

    // Get the additional (non-transport) SDP parameters from the json representation of an SDP file, e.g. from sdp::parse_session_description
    sdp_parameters get_session_description_sdp_parameters(const web::json::value& sdp)
    {
        using web::json::value;

        sdp_parameters sdp_params;

        // Protocol Version
        // See https://tools.ietf.org/html/rfc4566#section-5.1
        if (0 != sdp::fields::protocol_version(sdp)) throw details::sdp_processing_error("unsupported protocol version");

        // Origin
        // See https://tools.ietf.org/html/rfc4566#section-5.2
        const auto& origin = sdp::fields::origin(sdp);
        sdp_params.origin = { sdp::fields::user_name(origin), sdp::fields::session_id(origin), sdp::fields::session_version(origin) };

        // Session Name
        // See https://tools.ietf.org/html/rfc4566#section-5.3
        sdp_params.session_name = sdp::fields::session_name(sdp);

        // Time Descriptions
        // See https://tools.ietf.org/html/rfc4566#section-5
        const auto& time_descriptions = sdp::fields::time_descriptions(sdp).as_array();
        if (time_descriptions.size())
        {
            const auto& timing = sdp::fields::timing(time_descriptions.at(0));
            sdp_params.timing = { sdp::fields::start_time(timing), sdp::fields::stop_time(timing) };
        }

        // Connection Data
        // See https://tools.ietf.org/html/rfc4566#section-5.7
        {
            const auto& session_connection_data = sdp::fields::connection_data(sdp);
            if (!session_connection_data.is_null())
            {
                // hmm, how to handle multiple connection addresses?
                const auto address_type = sdp::address_type{ sdp::fields::address_type(session_connection_data) };
                const auto connection_address = details::parse_connection_address(address_type, sdp::fields::connection_address(session_connection_data));
                sdp_params.connection_data.ttl = connection_address.ttl;
            }
        }

        // hmm, this code does not handle Synchronization Source (SSRC) level grouping or attributes
        // i.e. the 'ssrc-group' attribute or 'ssrc' used to convey e.g. 'fmtp', 'mediaclk' or 'ts-refclk'
        // see https://tools.ietf.org/html/rfc7104#section-3.2
        // and https://tools.ietf.org/html/rfc5576
        // and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xhtml#sdp-att-field

        // Group
        // a=group:<semantics>[ <identification-tag>]*
        // See https://tools.ietf.org/html/rfc5888
        auto& session_attributes = sdp::fields::attributes(sdp).as_array();
        auto group = sdp::find_name(session_attributes, sdp::attributes::group);
        if (session_attributes.end() != group)
        {
            const auto& value = sdp::fields::value(*group);

            sdp_params.group.semantics = sdp::group_semantics_type{ sdp::fields::semantics(value) };
            for (const auto& mid : sdp::fields::mids(value))
            {
                sdp_params.group.media_stream_ids.push_back(mid.as_string());
            }
        }

        // Media Descriptions
        // See https://tools.ietf.org/html/rfc4566#section-5
        const auto& media_descriptions = sdp::fields::media_descriptions(sdp);

        // ts-refclk attributes
        // See https://tools.ietf.org/html/rfc7273
        sdp_params.ts_refclk = boost::copy_range<std::vector<sdp_parameters::ts_refclk_t>>(media_descriptions.as_array() | boost::adaptors::transformed([&sdp](const value& media_description) -> sdp_parameters::ts_refclk_t
        {
            auto& media_attributes = sdp::fields::attributes(media_description).as_array();
            auto ts_refclk = sdp::find_name(media_attributes, sdp::attributes::ts_refclk);
            // default to the "session-level" value if no "media-level" value
            if (media_attributes.end() == ts_refclk)
            {
                auto& session_attributes = sdp::fields::attributes(sdp).as_array();
                ts_refclk = sdp::find_name(session_attributes, sdp::attributes::ts_refclk);
                if (session_attributes.end() == ts_refclk)
                {
                    // indicate not found by default-constructed value
                    return{};
                }
            }
            const auto& value = sdp::fields::value(*ts_refclk);
            sdp::ts_refclk_source clock_source{ sdp::fields::clock_source(value) };
            if (sdp::ts_refclk_sources::ptp == clock_source)
            {
                // no ptp-server implies traceable
                return sdp_parameters::ts_refclk_t::ptp(sdp::ptp_version{ sdp::fields::ptp_version(value) }, sdp::fields::ptp_server(value));
            }
            else if (sdp::ts_refclk_sources::local_mac == clock_source)
            {
                return sdp_parameters::ts_refclk_t::local_mac(sdp::fields::mac_address(value));
            }
            else throw details::sdp_processing_error("unsupported timestamp reference clock source");
        }));

        // hmm, for simplicity, the remainder of this code assumes that format-related information must be the same
        // in every media description, so reads it only from the first one!
        if (0 == media_descriptions.size()) throw details::sdp_processing_error("missing media descriptions");
        const auto& media_description = media_descriptions.at(0);

        // Connection Data
        // get default multicast_ip via Connection Data
        // See https://tools.ietf.org/html/rfc4566#section-5.7
        {
            const auto& media_connection_data = sdp::fields::connection_data(media_description);
            if (!media_connection_data.is_null() && 0 != media_connection_data.size())
            {
                // hmm, how to handle multiple connection addresses?
                const auto address_type = sdp::address_type{ sdp::fields::address_type(media_connection_data.at(0)) };
                const auto connection_address = details::parse_connection_address(address_type, sdp::fields::connection_address(media_connection_data.at(0)));
                sdp_params.connection_data.ttl = connection_address.ttl;
            }
        }

        // Bandwidth
        // See https://tools.ietf.org/html/rfc4566#section-5.8
        {
            const auto& media_bandwidth_information = sdp::fields::bandwidth_information(media_description);
            auto bandwidths = boost::copy_range<std::vector<sdp_parameters::bandwidth_t>>(media_bandwidth_information.as_array() | boost::adaptors::transformed([](const web::json::value& bandwidth)
            {
                return sdp_parameters::bandwidth_t{ sdp::bandwidth_type{ sdp::fields::bandwidth_type(bandwidth) }, sdp::fields::bandwidth(bandwidth) };
            }));
            if (!bandwidths.empty())
            {
                // multiple "media-level" bandwidth lines seem to be rarely used
                // e.g. RFC 3556 for RTP Control Protocol (RTCP) Bandwidth
                // see https://tools.ietf.org/html/rfc3556
                sdp_params.bandwidth = bandwidths.front();
            }
        }

        // Media
        // See https://tools.ietf.org/html/rfc4566#section-5.14
        const auto& media = sdp::fields::media(media_description);
        sdp_params.media_type = sdp::media_type{ sdp::fields::media_type(media) };
        sdp_params.protocol = sdp::protocol{ sdp::fields::protocol(media) };

        // media description attributes
        const auto& media_attributes = sdp::fields::attributes(media_description).as_array();

        // mediaclk attribute
        // See https://tools.ietf.org/html/rfc7273
        sdp_params.mediaclk = [&sdp](const value& media_description) -> sdp_parameters::mediaclk_t
        {
            auto& media_attributes = sdp::fields::attributes(media_description).as_array();
            auto mediaclk = sdp::find_name(media_attributes, sdp::attributes::mediaclk);
            // default to the "session-level" value if no "media-level" value
            if (media_attributes.end() == mediaclk)
            {
                auto& session_attributes = sdp::fields::attributes(sdp).as_array();
                mediaclk = sdp::find_name(session_attributes, sdp::attributes::mediaclk);
                if (session_attributes.end() == mediaclk)
                {
                    // indicate not found by default-constructed value
                    return{};
                }
            }
            const auto& value = sdp::fields::value(*mediaclk).as_string();
            const auto eq = value.find(U('='));
            return{ sdp::media_clock_source{ value.substr(0, eq) }, utility::string_t::npos != eq ? value.substr(eq + 1) : utility::string_t{} };
        }(media_description);

        // rtpmap attribute
        // See https://tools.ietf.org/html/rfc4566#section-6
        auto rtpmap = sdp::find_name(media_attributes, sdp::attributes::rtpmap);
        if (media_attributes.end() == rtpmap) throw details::sdp_processing_error("missing attribute: rtpmap");
        const auto& rtpmap_value = sdp::fields::value(*rtpmap);

        sdp_params.rtpmap.encoding_name = sdp::fields::encoding_name(rtpmap_value);
        sdp_params.rtpmap.payload_type = sdp::fields::payload_type(rtpmap_value);
        sdp_params.rtpmap.clock_rate = sdp::fields::clock_rate(rtpmap_value);
        sdp_params.rtpmap.encoding_parameters = sdp::fields::encoding_parameters(rtpmap_value);

        // ptime attribute
        // See https://tools.ietf.org/html/rfc4566#section-6
        {
            auto ptime = sdp::find_name(media_attributes, sdp::attributes::ptime);
            if (media_attributes.end() != ptime)
            {
                sdp_params.packet_time = sdp::fields::value(*ptime).as_double();
            }
        }

        // maxptime attribute
        // See https://tools.ietf.org/html/rfc4566#section-6
        {
            auto maxptime = sdp::find_name(media_attributes, sdp::attributes::maxptime);
            if (media_attributes.end() != maxptime)
            {
                sdp_params.max_packet_time = sdp::fields::value(*maxptime).as_double();
            }
        }

        // framerate attribute
        // See https://tools.ietf.org/html/rfc4566#section-6
        {
            auto framerate = sdp::find_name(media_attributes, sdp::attributes::framerate);
            if (media_attributes.end() != framerate)
            {
                sdp_params.framerate = sdp::fields::value(*framerate).as_double();
            }
        }

        // fmtp attribute
        // See https://tools.ietf.org/html/rfc4566#section-6
        {
            auto fmtp = sdp::find_name(media_attributes, sdp::attributes::fmtp);
            if (media_attributes.end() != fmtp)
            {
                const auto& format_specific_parameters = sdp::fields::format_specific_parameters(sdp::fields::value(*fmtp));
                sdp_params.fmtp = boost::copy_range<sdp_parameters::fmtp_t>(format_specific_parameters | boost::adaptors::transformed([&](const value& param)
                {
                    const auto& name = sdp::fields::name(param);
                    const auto& value_or_null = sdp::fields::value(param);
                    return sdp_parameters::fmtp_t::value_type{ name, !value_or_null.is_null() ? value_or_null.as_string() : U("") };
                }));
            }
        }

        return sdp_params;
    }

    // Get additional "video/raw" parameters from the SDP parameters
    template <typename MissingRequiredParameter>
    video_raw_parameters get_video_raw_parameters(const sdp_parameters& sdp_params, MissingRequiredParameter missing = MissingRequiredParameter{})
    {
        video_raw_parameters params;

        // See SMPTE ST 2110-20:2017 Section 7.2 Required Media Type Parameters
        // and Section 7.3 Media Type Parameters with default values

        const auto sampling = details::find_fmtp(sdp_params.fmtp, sdp::fields::sampling);
        if (sdp_params.fmtp.end() == sampling) missing(sdp::fields::sampling);
        else params.sampling = sdp::sampling{ sampling->second };

        const auto depth = details::find_fmtp(sdp_params.fmtp, sdp::fields::depth);
        if (sdp_params.fmtp.end() == depth) missing(sdp::fields::depth);
        else params.depth = utility::istringstreamed<uint32_t>(depth->second);

        const auto width = details::find_fmtp(sdp_params.fmtp, sdp::fields::width);
        if (sdp_params.fmtp.end() == width) missing(sdp::fields::width);
        else params.width = utility::istringstreamed<uint32_t>(width->second);

        const auto height = details::find_fmtp(sdp_params.fmtp, sdp::fields::height);
        if (sdp_params.fmtp.end() == height) missing(sdp::fields::height);
        else params.height = utility::istringstreamed<uint32_t>(height->second);

        const auto exactframerate = details::find_fmtp(sdp_params.fmtp, sdp::fields::exactframerate);
        if (sdp_params.fmtp.end() == exactframerate) missing(sdp::fields::exactframerate);
        else params.exactframerate = nmos::details::parse_exactframerate(exactframerate->second);

        // optional
        const auto interlace = details::find_fmtp(sdp_params.fmtp, sdp::fields::interlace);
        params.interlace = sdp_params.fmtp.end() != interlace;

        // optional
        const auto segmented = details::find_fmtp(sdp_params.fmtp, sdp::fields::segmented);
        params.segmented = sdp_params.fmtp.end() != segmented;

        // optional
        const auto tcs = details::find_fmtp(sdp_params.fmtp, sdp::fields::transfer_characteristic_system);
        if (sdp_params.fmtp.end() != tcs) params.tcs = sdp::transfer_characteristic_system{ tcs->second };

        const auto colorimetry = details::find_fmtp(sdp_params.fmtp, sdp::fields::colorimetry);
        if (sdp_params.fmtp.end() == colorimetry) missing(sdp::fields::colorimetry);
        else params.colorimetry = sdp::colorimetry{ colorimetry->second };

        // optional
        const auto range = details::find_fmtp(sdp_params.fmtp, sdp::fields::range);
        if (sdp_params.fmtp.end() != range) params.range = sdp::range{ range->second };

        // optional
        const auto par = details::find_fmtp(sdp_params.fmtp, sdp::fields::pixel_aspect_ratio);
        if (sdp_params.fmtp.end() != par) params.par = nmos::details::parse_pixel_aspect_ratio(par->second);

        const auto pm = details::find_fmtp(sdp_params.fmtp, sdp::fields::packing_mode);
        if (sdp_params.fmtp.end() == pm) missing(sdp::fields::packing_mode);
        else params.pm = sdp::packing_mode{ pm->second };

        const auto ssn = details::find_fmtp(sdp_params.fmtp, sdp::fields::smpte_standard_number);
        if (sdp_params.fmtp.end() == ssn) missing(sdp::fields::smpte_standard_number);
        else params.ssn = sdp::smpte_standard_number{ ssn->second };

        // "Senders and Receivers compliant to [ST 2110-20] shall comply with the provisions of SMPTE ST 2110-21."
        // See SMPTE ST 2110-20:2017 Section 6.1.1

        // See SMPTE ST 2110-21:2017 Section 8.1 Required Parameters
        // and Section 8.2 Optional Parameters

        // hmm, "TP" (type parameter) is required, but currently omitted by several vendors, so allow that for now...
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

        return params;
    }

    // Get additional "video/raw" parameters from the SDP parameters
    video_raw_parameters get_video_raw_parameters(const sdp_parameters& sdp_params)
    {
        return get_video_raw_parameters<details::throw_missing_fmtp>(sdp_params);
    }

    // Get additional "audio/L" parameters from the SDP parameters
    audio_L_parameters get_audio_L_parameters(const sdp_parameters& sdp_params)
    {
        audio_L_parameters params;

        params.channel_count = (uint32_t)sdp_params.rtpmap.encoding_parameters;
        if (0 == params.channel_count) params.channel_count = 1;

        const auto& encoding_name = sdp_params.rtpmap.encoding_name;
        params.bit_depth = !encoding_name.empty() && U('L') == encoding_name.front() ? utility::istringstreamed<uint32_t>(encoding_name.substr(1)) : 0;

        params.sample_rate = sdp_params.rtpmap.clock_rate;

        // optional
        const auto channel_order = details::find_fmtp(sdp_params.fmtp, sdp::fields::channel_order);
        if (sdp_params.fmtp.end() != channel_order) params.channel_order = channel_order->second;

        // optional
        const auto tsmode = details::find_fmtp(sdp_params.fmtp, sdp::fields::timestamp_mode);
        if (sdp_params.fmtp.end() != tsmode) params.tsmode = sdp::timestamp_mode{ tsmode->second };

        // optional
        const auto tsdelay = details::find_fmtp(sdp_params.fmtp, sdp::fields::timestamp_delay);
        if (sdp_params.fmtp.end() != tsdelay) params.tsdelay = utility::istringstreamed<uint32_t>(tsdelay->second);

        // optional
        params.packet_time = sdp_params.packet_time;

        return params;
    }

    // Get additional "video/smpte291" parameters from the SDP parameters
    video_smpte291_parameters get_video_smpte291_parameters(const sdp_parameters& sdp_params)
    {
        video_smpte291_parameters params;

        // "The SDP object shall be constructed as described in IETF RFC 8331"
        // See SMPTE ST 2110-40:2018 Section 6
        // and https://tools.ietf.org/html/rfc8331

        // optional
        params.did_sdids = boost::copy_range<std::vector<nmos::did_sdid>>(sdp_params.fmtp | boost::adaptors::filtered([](const sdp_parameters::fmtp_t::value_type& param)
        {
            return sdp::fields::DID_SDID.key == param.first;
        }) | boost::adaptors::transformed([](const sdp_parameters::fmtp_t::value_type& did_sdid)
        {
            return parse_fmtp_did_sdid(did_sdid.second);
        }));

        // optional
        const auto vpid_code = details::find_fmtp(sdp_params.fmtp, sdp::fields::VPID_Code);
        if (sdp_params.fmtp.end() != vpid_code) params.vpid_code = (nmos::vpid_code)utility::istringstreamed<uint32_t>(vpid_code->second);

        // optional
        const auto exactframerate = details::find_fmtp(sdp_params.fmtp, sdp::fields::exactframerate);
        if (sdp_params.fmtp.end() != exactframerate) params.exactframerate = nmos::details::parse_exactframerate(exactframerate->second);

        // optional
        const auto tm = details::find_fmtp(sdp_params.fmtp, sdp::fields::TM);
        if (sdp_params.fmtp.end() != tm) params.tm = sdp::transmission_model{ tm->second };

        // optional
        const auto ssn = details::find_fmtp(sdp_params.fmtp, sdp::fields::smpte_standard_number);
        if (sdp_params.fmtp.end() != ssn) params.ssn = sdp::smpte_standard_number{ ssn->second };

        // optional
        const auto troff = details::find_fmtp(sdp_params.fmtp, sdp::fields::TROFF);
        if (sdp_params.fmtp.end() != troff) params.troff = utility::istringstreamed<uint32_t>(troff->second);

        // optional
        const auto tsmode = details::find_fmtp(sdp_params.fmtp, sdp::fields::timestamp_mode);
        if (sdp_params.fmtp.end() != tsmode) params.tsmode = sdp::timestamp_mode{ tsmode->second };

        // optional
        const auto tsdelay = details::find_fmtp(sdp_params.fmtp, sdp::fields::timestamp_delay);
        if (sdp_params.fmtp.end() != tsdelay) params.tsdelay = utility::istringstreamed<uint32_t>(tsdelay->second);

        return params;
    }

    // Get additional "video/SMPTE2022-6" parameters from the SDP parameters
    video_SMPTE2022_6_parameters get_video_SMPTE2022_6_parameters(const sdp_parameters& sdp_params)
    {
        video_SMPTE2022_6_parameters params;

        // "Senders shall signal Media Type Parameters TP and TROFF as specified in ST 2110-21"
        // See SMPTE ST 2022-8:2019 Section 6

        // See SMPTE ST 2110-21:2017 Section 8.1 Required Parameters
        // and Section 8.2 Optional Parameters

        // "TP" (type parameter) is required, but allow it to be omitted for now...
        const auto tp = details::find_fmtp(sdp_params.fmtp, sdp::fields::type_parameter);
        if (sdp_params.fmtp.end() != tp) params.tp = sdp::type_parameter{ tp->second };

        // optional
        const auto troff = details::find_fmtp(sdp_params.fmtp, sdp::fields::TROFF);
        if (sdp_params.fmtp.end() != troff) params.troff = utility::istringstreamed<uint32_t>(troff->second);

        return params;
    }

    // Get SDP parameters from the json representation of an SDP file, e.g. from sdp::parse_session_description
    std::pair<sdp_parameters, web::json::value> parse_session_description(const web::json::value& session_description)
    {
        return{ get_session_description_sdp_parameters(session_description), get_session_description_transport_params(session_description) };
    }

    namespace details
    {
        nmos::format get_format(const sdp_parameters& sdp_params)
        {
            if (sdp::media_types::video == sdp_params.media_type && U("raw") == sdp_params.rtpmap.encoding_name) return nmos::formats::video;
            if (sdp::media_types::audio == sdp_params.media_type && U("L") == sdp_params.rtpmap.encoding_name.substr(0, 1)) return nmos::formats::audio;
            if (sdp::media_types::video == sdp_params.media_type && U("smpte291") == sdp_params.rtpmap.encoding_name) return nmos::formats::data;
            if (sdp::media_types::video == sdp_params.media_type && U("SMPTE2022-6") == sdp_params.rtpmap.encoding_name) return nmos::formats::mux;
            throw sdp_processing_error("unsupported media type/encoding name");
        }

        format_parameters get_format_parameters(const sdp_parameters& sdp_params)
        {
            if (sdp::media_types::video == sdp_params.media_type && U("raw") == sdp_params.rtpmap.encoding_name) return get_video_raw_parameters(sdp_params);
            if (sdp::media_types::audio == sdp_params.media_type && U("L") == sdp_params.rtpmap.encoding_name.substr(0, 1)) return get_audio_L_parameters(sdp_params);
            if (sdp::media_types::video == sdp_params.media_type && U("smpte291") == sdp_params.rtpmap.encoding_name) return get_video_smpte291_parameters(sdp_params);
            if (sdp::media_types::video == sdp_params.media_type && U("SMPTE2022-6") == sdp_params.rtpmap.encoding_name) return get_video_SMPTE2022_6_parameters(sdp_params);
            throw sdp_processing_error("unsupported media type/encoding name");
        }

        // Check the specified SDP interlace and segmented parameters against the specified interlace_mode constraint
        bool match_interlace_mode_constraint(bool interlace, bool segmented, const web::json::value& constraint)
        {
            if (!interlace) return nmos::match_string_constraint(nmos::interlace_modes::progressive.name, constraint);
            if (segmented) return nmos::match_string_constraint(nmos::interlace_modes::interlaced_psf.name, constraint);
            // hmm, don't think we can be more precise than this, see comment regarding RFC 4175 top-field-first in make_video_sdp_parameters
            return nmos::match_string_constraint(nmos::interlace_modes::interlaced_tff.name, constraint)
                || nmos::match_string_constraint(nmos::interlace_modes::interlaced_bff.name, constraint);
        }

        // for a little brevity, cf. sdp_parameters member type names
        const video_raw_parameters* get_video(const format_parameters* format) { return get<video_raw_parameters>(format); }
        const audio_L_parameters* get_audio(const format_parameters* format) { return get<audio_L_parameters>(format); }
        const video_smpte291_parameters* get_data(const format_parameters* format) { return get<video_smpte291_parameters>(format); }
        const video_SMPTE2022_6_parameters* get_mux(const format_parameters* format) { return get<video_SMPTE2022_6_parameters>(format); }

        // both video/raw and video/smpte291 may have exactframerate
        nmos::rational get_exactframerate(const format_parameters* format)
        {
            if (auto video = get_video(format)) return video->exactframerate;
            else if (auto data = get_data(format)) return data->exactframerate;
            else return{};
        }

        // NMOS Parameter Registers - Capabilities register
        // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/capabilities/
#define CAPS_ARGS const sdp_parameters& sdp, const format_parameters& format, const web::json::value& con
        static const std::map<utility::string_t, std::function<bool(CAPS_ARGS)>> format_constraints
        {
            // General Constraints

            { nmos::caps::format::media_type, [](CAPS_ARGS) { return nmos::match_string_constraint(get_media_type(sdp).name, con); } },
            // hm, how best to match (rational) nmos::caps::format::grain_rate against (double) framerate e.g. for video/SMPTE2022-6?
            // is 23.976 a match for 24000/1001? how about 23.98, or 23.9? or even 23?!
            { nmos::caps::format::grain_rate, [](CAPS_ARGS) { auto exactframerate = get_exactframerate(&format); return nmos::rational{} == exactframerate || nmos::match_rational_constraint(exactframerate, con); } },

            // Video Constraints

            { nmos::caps::format::frame_height, [](CAPS_ARGS) { auto video = get_video(&format); return video && nmos::match_integer_constraint(video->height, con); } },
            { nmos::caps::format::frame_width, [](CAPS_ARGS) { auto video = get_video(&format); return video && nmos::match_integer_constraint(video->width, con); } },
            { nmos::caps::format::color_sampling, [](CAPS_ARGS) { auto video = get_video(&format); return video && nmos::match_string_constraint(video->sampling.name, con); } },
            { nmos::caps::format::interlace_mode, [](CAPS_ARGS) { auto video = get_video(&format); return video && nmos::details::match_interlace_mode_constraint(video->interlace, video->segmented, con); } },
            { nmos::caps::format::colorspace, [](CAPS_ARGS) { auto video = get_video(&format); return video && nmos::match_string_constraint(video->colorimetry.name, con); } },
            { nmos::caps::format::transfer_characteristic, [](CAPS_ARGS) { auto video = get_video(&format); return video && nmos::match_string_constraint(!video->tcs.empty() ? video->tcs.name : sdp::transfer_characteristic_systems::SDR.name, con); } },
            { nmos::caps::format::component_depth, [](CAPS_ARGS) { auto video = get_video(&format); return video && nmos::match_integer_constraint(video->depth, con); } },

            // Audio Constraints

            { nmos::caps::format::channel_count, [](CAPS_ARGS) { auto audio = get_audio(&format); return audio && nmos::match_integer_constraint(audio->channel_count, con); } },
            { nmos::caps::format::sample_rate, [](CAPS_ARGS) { auto audio = get_audio(&format); return audio && nmos::match_rational_constraint(audio->sample_rate, con); } },
            { nmos::caps::format::sample_depth, [](CAPS_ARGS) { auto audio = get_audio(&format); return audio && nmos::match_integer_constraint(audio->bit_depth, con); } },

            // Transport Constraints

            { nmos::caps::transport::packet_time, [](CAPS_ARGS) { return 0 == sdp.packet_time || nmos::match_number_constraint(sdp.packet_time, con); } },
            { nmos::caps::transport::max_packet_time, [](CAPS_ARGS) { return 0 == sdp.max_packet_time || nmos::match_number_constraint(sdp.max_packet_time, con); } },
            { nmos::caps::transport::st2110_21_sender_type, [](CAPS_ARGS) { if (auto video = get_video(&format)) return nmos::match_string_constraint(video->tp.name, con); else if (auto mux = get_mux(&format)) return nmos::match_string_constraint(mux->tp.name, con); else return false; } }
        };
#undef CAPS_ARGS

        // Check the specified SDP parameters and format-specific parameters against the specified constraint set
        // using the specified parameter constraint functions
        bool match_sdp_parameters_constraint_set(const sdp_parameter_constraints& constraints, const sdp_parameters& sdp_params, const format_parameters& format_params, const web::json::value& constraint_set_)
        {
            using web::json::value;

            if (!nmos::caps::meta::enabled(constraint_set_)) return false;

            const auto& constraint_set = constraint_set_.as_object();
            return constraint_set.end() == std::find_if(constraint_set.begin(), constraint_set.end(), [&](const std::pair<utility::string_t, value>& constraint)
            {
                const auto found = constraints.find(constraint.first);
                return constraints.end() != found && !found->second(sdp_params, format_params, constraint.second);
            });
        }

        // Validate the specified SDP parameters and format-specific parameters against the specified receiver
        // using the specified parameter constraint functions
        void validate_sdp_parameters(const sdp_parameter_constraints& constraints, const sdp_parameters& sdp_params, const format& format, const format_parameters& format_params, const web::json::value& receiver)
        {
            const auto media_type = get_media_type(sdp_params);

            if (nmos::format{ nmos::fields::format(receiver) } != format) throw details::sdp_processing_error("unexpected media type/encoding name");

            const auto& caps = nmos::fields::caps(receiver);
            const auto& media_types_or_null = nmos::fields::media_types(caps);
            if (!media_types_or_null.is_null())
            {
                const auto& media_types = media_types_or_null.as_array();
                const auto found = std::find(media_types.begin(), media_types.end(), web::json::value::string(media_type.name));
                if (media_types.end() == found) throw details::sdp_processing_error("unsupported encoding name");
            }
            const auto& constraint_sets_or_null = nmos::fields::constraint_sets(caps);
            if (!constraint_sets_or_null.is_null())
            {
                const auto& constraint_sets = constraint_sets_or_null.as_array();
                const auto found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return details::match_sdp_parameters_constraint_set(constraints, sdp_params, format_params, constraint_set); });
                if (constraint_sets.end() == found) throw details::sdp_processing_error("unsupported transport or format-specific parameters");
            }
        }
    }

    // Validate the SDP parameters against a receiver for "video/raw", "audio/L", "video/smpte291" or "video/SMPTE2022-6"
    void validate_sdp_parameters(const web::json::value& receiver, const sdp_parameters& sdp_params)
    {
        details::validate_sdp_parameters(details::format_constraints, sdp_params, details::get_format(sdp_params), details::get_format_parameters(sdp_params), receiver);
    }
}

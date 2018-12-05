#include "nmos/sdp_utils.h"

#include <map>
#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/basic_utils.h"
#include "nmos/format.h"
#include "nmos/interlace_mode.h"
#include "nmos/json_fields.h"
#include "nmos/media_type.h"
#include "nmos/transport.h"

namespace nmos
{
    namespace details
    {
        std::pair<sdp::address_type, bool> get_address_type_multicast(const utility::string_t& address);

        nmos::sampling make_sampling(const web::json::array& components)
        {
            // https://tools.ietf.org/html/rfc4175#section-6.1

            // convert json to component name vs dimension lookup for easy access,
            // as components can be in any order inside the json
            struct dimension { int width; int height; };
            const auto dimensions = boost::copy_range<std::map<utility::string_t, dimension>>(components | boost::adaptors::transformed([](const web::json::value& component)
            {
                return std::map<utility::string_t, dimension>::value_type{ nmos::fields::name(component), dimension{ nmos::fields::width(component), nmos::fields::height(component) } };
            }));
            const auto de = dimensions.end();

            if (de != dimensions.find(U("R")) && de != dimensions.find(U("G")) && de != dimensions.find(U("B")) && de != dimensions.find(U("A")))
            {
                return nmos::samplings::RGBA;
            }
            else if (de != dimensions.find(U("R")) && de != dimensions.find(U("G")) && de != dimensions.find(U("B")))
            {
                return nmos::samplings::RGB;
            }
            else if (de != dimensions.find(U("Y")) && de != dimensions.find(U("Cb")) && de != dimensions.find(U("Cr")))
            {
                const auto& Y = dimensions.at(U("Y"));
                const auto& Cb = dimensions.at(U("Cb"));
                const auto& Cr = dimensions.at(U("Cr"));
                if (Cb.width != Cr.width || Cb.height != Cr.height) throw std::logic_error("unsupported YCbCr dimensions");
                const auto& C = Cb;
                if (Y.width == C.width)
                {
                    if (Y.height == C.height) return nmos::samplings::YCbCr_4_4_4;
                    else if (Y.height / 2 == C.height) return nmos::samplings::YCbCr_4_2_2;
                    else if (Y.height / 4 == C.height) return nmos::samplings::YCbCr_4_1_1;
                    else throw std::logic_error("unsupported YCbCr dimensions");
                }
                else if (Y.width / 2 == C.width)
                {
                    if (Y.height / 2 == C.height) return nmos::samplings::YCbCr_4_2_0;
                    else throw std::logic_error("unsupported YCbCr dimensions");
                }
                else throw std::logic_error("unsupported YCbCr dimensions");
            }
            else throw std::logic_error("unsupported components");
        };
    }

    static sdp_parameters make_video_sdp_parameters(const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids)
    {
        sdp_parameters::video_t params;
        params.tp = nmos::sender_type_parameters::type_N;

        // colorimetry map directly to flow_video json "colorspace"
        params.colorimetry = nmos::colorspace{ nmos::fields::colorspace(flow) };

        // use flow json "components" to workout sampling
        const auto& components = nmos::fields::components(flow);
        params.sampling = details::make_sampling(components);
        params.width = nmos::fields::frame_width(flow);
        params.height = nmos::fields::frame_height(flow);
        params.depth = nmos::fields::bit_depth(components.at(0));

        params.tcs = nmos::transfer_characteristic{ nmos::fields::transfer_characteristic(flow) };

        const auto& interlace_mode = nmos::fields::interlace_mode(flow);
        params.interlace = !interlace_mode.empty() && nmos::interlace_modes::progressive.name != interlace_mode;

        // grain_rate is optional in the flow, but if it's not there, for a video flow, it must be in the source
        const auto& grain_rate = nmos::fields::grain_rate(flow.has_field(nmos::fields::grain_rate) ? flow : source);
        params.exactframerate = nmos::rational(nmos::fields::numerator(grain_rate), nmos::fields::denominator(grain_rate));

        return{ sender.at(nmos::fields::label).as_string(), params, 96, media_stream_ids };
    }

    static sdp_parameters make_audio_sdp_parameters(const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids)
    {
        sdp_parameters::audio_t params;

        // rtpmap
        params.channel_count = (uint32_t)nmos::fields::channels(source).size();
        params.bit_depth = nmos::fields::bit_depth(flow);
        const auto& sample_rate(flow.at(nmos::fields::sample_rate));
        params.sample_rate = nmos::rational(nmos::fields::numerator(sample_rate), nmos::fields::denominator(sample_rate));

        // format_specific_parameters
        // hmm, should be created from source json "channels"
        params.channel_order = U("SMPTE2110.(ST)");

        // ptime
        params.packet_time = 1;

        return{ sender.at(nmos::fields::label).as_string(), params, 97, media_stream_ids };
    }

    static sdp_parameters make_data_sdp_parameters(const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids)
    {
        sdp_parameters::data_t params;

        return{ sender.at(nmos::fields::label).as_string(), params, 100, media_stream_ids };
    }

    sdp_parameters make_sdp_parameters(const web::json::value& source, const web::json::value& flow, const web::json::value& sender, const std::vector<utility::string_t>& media_stream_ids)
    {
        const auto& format = nmos::fields::format(flow);
        if (nmos::formats::video.name == format)
            return make_video_sdp_parameters(source, flow, sender, media_stream_ids);
        else if (nmos::formats::audio.name == format)
            return make_audio_sdp_parameters(source, flow, sender, media_stream_ids);
        else if (nmos::formats::data.name == format)
            return make_data_sdp_parameters(source, flow, sender, media_stream_ids);
        else
            throw std::logic_error("unsuported media format");
    }

    static web::json::value make_session_description(const sdp_parameters& sdp_params, const web::json::value& transport_params, const web::json::value& ptime, const web::json::value& rtpmap, const web::json::value& fmtp)
    {
        using web::json::value;
        using web::json::value_of;
        using web::json::array;

        // check to ensure enough media_stream_ids for transport_params
        if (transport_params.size() > sdp_params.group.media_stream_ids.size())
        {
            throw std::logic_error("not enough sdp parameters media stream ids for transport_params");
        }

        // so far so good, now build the session_description

        const bool keep_order = true;

        const auto& destination_ip = nmos::fields::destination_ip(transport_params.at(0)).as_string();
        const auto address_type_multicast = details::get_address_type_multicast(destination_ip);

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
                { sdp::fields::network_type, sdp::network_types::IN.name },
                { sdp::fields::address_type, address_type_multicast.first.name },
                { sdp::fields::unicast_address, nmos::fields::source_ip(transport_params.at(0)) }
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
            }) }

        }, keep_order);


        // group & mid attributes
        // see https://tools.ietf.org/html/rfc5888
        auto mids = value::array();

        // build media_descriptions with given media_stream_ids
        // Media Descriptions
        // See https://tools.ietf.org/html/rfc4566#section-5
        auto media_descriptions = value::array();
        int idx = 0;
        for(const auto& transport_param : transport_params.as_array())
        {
            // build up mids based on media_stream_ids
            web::json::push_back(mids, sdp_params.group.media_stream_ids[idx]);

            // setup Connection Data's connection address
            const auto& connection_address = transport_param.at(address_type_multicast.second ? nmos::fields::source_ip : nmos::fields::destination_ip);

            // build media_description
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
                        { sdp::fields::network_type, sdp::network_types::IN.name },
                        { sdp::fields::address_type, address_type_multicast.first.name },
                        { sdp::fields::connection_address, sdp::address_types::IP4 == address_type_multicast.first && address_type_multicast.second
                            ? connection_address.as_string() + U("/") + utility::ostringstreamed(sdp_params.connection_data.ttl)
                            : connection_address.as_string() }
                    }, keep_order)
                }) },

                // Attributes
                // See https://tools.ietf.org/html/rfc4566#section-5.13
                { sdp::fields::attributes, value_of({
                    // a=ts-refclk:ptp=<ptp version>:<ptp gmid>[:<ptp domain>]
                    // a=ts-refclk:ptp=<ptp version>:traceable
                    // See https://tools.ietf.org/html/rfc7273
                    // a=ts-refclk:localmac=7C-E9-D3-1B-9A-AF
                    // See SMPTE ST 2110-10:2017 Professional Media Over Managed IP Networks: System Timing and Definitions, Section 8.2 Reference Clock
                    value_of({
                        { sdp::fields::name, sdp::attributes::ts_refclk },
                        { sdp::fields::value, value_of({
                            { sdp::fields::clock_source, sdp_params.ts_refclk.clock_source.name },
                            { sdp::fields::ptp_version, sdp_params.ts_refclk.ptp_version.name },
                            { sdp::fields::ptp_server, !sdp_params.ts_refclk.ptp_server.empty() ? sdp_params.ts_refclk.ptp_server : U("traceable") }
                        }, keep_order) }
                    }, keep_order),

                    // a=mediaclk:[id=<clock id> ]<clock source>[=<clock parameters>]
                    // See https://tools.ietf.org/html/rfc7273#section-5
                    value_of({
                        { sdp::fields::name, sdp::attributes::mediaclk },
                        { sdp::fields::value, sdp_params.mediaclk.clock_source.name + U("=") + sdp_params.mediaclk.clock_parameters }
                    }, keep_order)

                }) } //attribues

            }, keep_order); //media_description

            // insert source-filter if multicast
            if (address_type_multicast.second)
            {
                auto& attributes = media_description.at(sdp::fields::attributes);
                // a=source-filter: <filter-mode> <nettype> <address-types> <dest-address> <src-list>
                // See https://tools.ietf.org/html/rfc4570
                web::json::push_back(
                    attributes, value_of({
                        { sdp::fields::name, sdp::attributes::source_filter },
                        { sdp::fields::value, value_of({
                            { sdp::fields::filter_mode, sdp::filter_modes::incl.name },
                            { sdp::fields::network_type, sdp::network_types::IN.name },
                            { sdp::fields::address_types, address_type_multicast.first.name },
                            { sdp::fields::destination_address, transport_param.at(nmos::fields::destination_ip) },
                            { sdp::fields::source_addresses, value_of({ transport_param.at(nmos::fields::source_ip) }) }
                        }, keep_order) }
                    }, keep_order)
                );
            }

            // insert ptime if set
            // a=ptime:<packet time>
            // See https://tools.ietf.org/html/rfc4566#section-6
            if (!ptime.is_null())
            {
                auto& attributes = media_description.at(sdp::fields::attributes);
                web::json::push_back(attributes, ptime);
            }

            // insert rtpmap if set
            // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
            // See https://tools.ietf.org/html/rfc4566#section-6
            if (!rtpmap.is_null())
            {
                auto& attributes = media_description.at(sdp::fields::attributes);
                web::json::push_back(attributes, rtpmap);
            }

            // insert fmtp if set
            // a=fmtp:<format> <format specific parameters>
            // See https://tools.ietf.org/html/rfc4566#section-6
            if (!fmtp.is_null())
            {
                auto& attributes = media_description.at(sdp::fields::attributes);
                web::json::push_back(attributes, fmtp);
            }

            // insert "media stream identification" if there are more than 1 leg
            // a=mid:<identification-tag>
            // See https://tools.ietf.org/html/rfc5888
            if((transport_params.size() > 1) && (sdp_params.group.media_stream_ids.size() > idx))
            {
                auto& attributes = media_description.at(sdp::fields::attributes);
                web::json::push_back(
                    attributes, value_of({
                        { sdp::fields::name, sdp::attributes::mid },
                        { sdp::fields::value, sdp_params.group.media_stream_ids[idx] }
                    }, keep_order)
                );
            }

            web::json::push_back(media_descriptions, media_description);

            ++idx;
        }

        // add group attribute if there are more than 1 leg
        // a=group:<semantics>[ <identification-tag>]*
        // See https://tools.ietf.org/html/rfc5888
        if (mids.size() > 1)
        {
            session_description[sdp::fields::attributes] = value_of({
                web::json::value_of({
                    { sdp::fields::name, sdp::attributes::group },
                    { sdp::fields::value, web::json::value_of({
                        { sdp::fields::semantics, sdp_params.group.semantics },
                        { sdp::fields::mids, mids }
                    }, keep_order) },
                }, keep_order)
            });
        }

        session_description[sdp::fields::media_descriptions] = media_descriptions;

        return session_description;
    }

    static web::json::value make_video_session_description(const sdp_parameters& sdp_params, const web::json::value& transport_params)
    {
        using web::json::value_of;

        const bool keep_order = true;

        // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
        // See https://tools.ietf.org/html/rfc4566#section-6
        const auto& rtpmap = value_of({
            { sdp::fields::name, sdp::attributes::rtpmap },
            { sdp::fields::value, web::json::value_of({
                { sdp::fields::payload_type, sdp_params.rtpmap.payload_type },
                { sdp::fields::encoding_name, sdp_params.rtpmap.encoding_name },
                { sdp::fields::clock_rate, sdp_params.rtpmap.clock_rate }
            }, keep_order) }
        }, keep_order);

        // a=fmtp:<format> <format specific parameters>
        // for simplicity, following the order of parameters given in VSF TR-05:2017
        // See https://tools.ietf.org/html/rfc4566#section-6
        // and http://www.videoservicesforum.org/download/technical_recommendations/VSF_TR-05_2018-06-23.pdf
        auto format_specific_parameters = value_of({
            sdp::named_value(sdp::fields::width, utility::ostringstreamed(sdp_params.video.width)),
            sdp::named_value(sdp::fields::height, utility::ostringstreamed(sdp_params.video.height)),
            sdp::named_value(sdp::fields::exactframerate, sdp_params.video.exactframerate.denominator() != 1
                ? utility::ostringstreamed(sdp_params.video.exactframerate.numerator()) + U("/") + utility::ostringstreamed(sdp_params.video.exactframerate.denominator())
                : utility::ostringstreamed(sdp_params.video.exactframerate.numerator()))
        });
        if (sdp_params.video.interlace) web::json::push_back(format_specific_parameters, sdp::named_value(sdp::fields::interlace));
        web::json::push_back(format_specific_parameters, sdp::named_value(sdp::fields::sampling, sdp_params.video.sampling.name));
        web::json::push_back(format_specific_parameters, sdp::named_value(sdp::fields::depth, utility::ostringstreamed(sdp_params.video.depth)));
        web::json::push_back(format_specific_parameters, sdp::named_value(sdp::fields::colorimetry, sdp_params.video.colorimetry.name));
        if (!sdp_params.video.tcs.name.empty()) web::json::push_back(format_specific_parameters, sdp::named_value(sdp::fields::TCS, sdp_params.video.tcs.name));
        web::json::push_back(format_specific_parameters, sdp::named_value(sdp::fields::PM, U("2110GPM"))); // or "2110BPM"
        web::json::push_back(format_specific_parameters, sdp::named_value(sdp::fields::SSN, U("ST2110-20:2017"))); // or soon, "ST2110-20:2019"?
        web::json::push_back(format_specific_parameters, sdp::named_value(sdp::fields::TP, sdp_params.video.tp.name));

        const auto& fmtp = web::json::value_of({
            { sdp::fields::name, sdp::attributes::fmtp },
            { sdp::fields::value, web::json::value_of({
                { sdp::fields::format, utility::ostringstreamed(sdp_params.rtpmap.payload_type) },
                { sdp::fields::format_specific_parameters, format_specific_parameters }
            }, keep_order) }
        }, keep_order);

        return make_session_description(sdp_params, transport_params, {}, rtpmap, fmtp);
    }

    static web::json::value make_audio_session_description(const sdp_parameters& sdp_params, const web::json::value& transport_params)
    {
        using web::json::value_of;

        const bool keep_order = true;

        // a=ptime:<packet time>
        // See https://tools.ietf.org/html/rfc4566#section-6
        const auto& ptime = value_of({
            { sdp::fields::name, sdp::attributes::ptime },
            { sdp::fields::value, sdp_params.audio.packet_time }
        }, keep_order);

        // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
        // See https://tools.ietf.org/html/rfc4566#section-6
        const auto& rtpmap = value_of({
            { sdp::fields::name, sdp::attributes::rtpmap },
            { sdp::fields::value, web::json::value_of({
                { sdp::fields::payload_type, sdp_params.rtpmap.payload_type },
                { sdp::fields::encoding_name, sdp_params.rtpmap.encoding_name },
                { sdp::fields::clock_rate, sdp_params.rtpmap.clock_rate },
                { sdp::fields::encoding_parameters, sdp_params.audio.channel_count }
            }, keep_order) }
        }, keep_order);

        // a=fmtp:<format> <format specific parameters>
        // See https://tools.ietf.org/html/rfc4566#section-6
        const auto& format_specific_parameters = value_of({
            sdp::named_value(sdp::fields::channel_order, sdp_params.audio.channel_order)
        });
        const auto& fmtp = web::json::value_of({
            { sdp::fields::name, sdp::attributes::fmtp },
            { sdp::fields::value, web::json::value_of({
                { sdp::fields::format, utility::ostringstreamed(sdp_params.rtpmap.payload_type) },
                { sdp::fields::format_specific_parameters, format_specific_parameters }
            }, keep_order) }
        }, keep_order);

        return make_session_description(sdp_params, transport_params, ptime, rtpmap, fmtp);
    }

    static web::json::value make_data_session_description(const sdp_parameters& sdp_params, const web::json::value& transport_params)
    {
        using web::json::value_of;

        const bool keep_order = true;

        // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
        // See https://tools.ietf.org/html/rfc4566#section-6
        const auto& rtpmap = value_of({
            { sdp::fields::name, sdp::attributes::rtpmap },
            { sdp::fields::value, web::json::value_of({
                { sdp::fields::payload_type, sdp_params.rtpmap.payload_type },
                { sdp::fields::encoding_name, sdp_params.rtpmap.encoding_name },
                { sdp::fields::clock_rate, sdp_params.rtpmap.clock_rate }
            }, keep_order) }
        }, keep_order);

        return make_session_description(sdp_params, transport_params, {}, rtpmap, {});
    }

    namespace details
    {
        nmos::format get_format(const sdp_parameters& sdp_params)
        {
            if (sdp::media_types::video == sdp_params.media_type && U("raw") == sdp_params.rtpmap.encoding_name) return nmos::formats::video;
            if (sdp::media_types::audio == sdp_params.media_type) return nmos::formats::audio;
            if (sdp::media_types::video == sdp_params.media_type && U("smpte291") == sdp_params.rtpmap.encoding_name) return nmos::formats::data;
            return{};
        }

        static nmos::media_type get_media_type(const sdp_parameters& sdp_params)
        {
            return nmos::media_type{ sdp_params.media_type.name + U("/") + sdp_params.rtpmap.encoding_name };
        }
    }

    web::json::value make_session_description(const sdp_parameters& sdp_params, const web::json::value& transport_params)
    {
        const auto format = details::get_format(sdp_params);
        if (nmos::formats::video == format) return make_video_session_description(sdp_params, transport_params);
        if (nmos::formats::audio == format) return make_audio_session_description(sdp_params, transport_params);
        if (nmos::formats::data == format)  return make_data_session_description(sdp_params, transport_params);
        throw std::logic_error("unsupported ST2110 media");
    }

    namespace details
    {
        // Remove any suffixed <ttl> and/or <number of addresses> value
        // See https://tools.ietf.org/html/rfc4566#section-5.7
        inline utility::string_t connection_base_address(const utility::string_t& connection_address)
        {
            return connection_address.substr(0, connection_address.find(U('/')));
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

    // Get transport parameters from the parsed SDP file
    web::json::value get_session_description_transport_params(const web::json::value& session_description, bool smpte2022_7)
    {
        using web::json::value;
        using web::json::value_of;

        auto transport_params = smpte2022_7 ? value_of({ value::object(), value::object() }) : value_of({ value::object() });

        // There isn't much of a specification for interpreting SDP files and updating the
        // equivalent transport parameters, just some examples...
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#interpretation-of-sdp-files

        // For now, this function should handle the following cases identified in the documentation:
        // * Unicast
        // * Source Specific Multicast
        // * Operation with SMPTE 2022-7 - Separate Source Addresses
        // * Operation with SMPTE 2022-7 - Separate Destination Addresses

        // The following cases are not yet handled:
        // * Operation with SMPTE 2022-5
        // * Operation with SMPTE 2022-7 - Temporal Redundancy
        // * Operation with RTCP

        auto& media_descriptions = sdp::fields::media_descriptions(session_description);

        for (size_t leg = 0; leg < transport_params.size(); ++leg)
        {
            auto& params = transport_params[leg];

            params[nmos::fields::rtp_enabled] = value::boolean(false);

            params[nmos::fields::source_ip] = value::string(sdp::fields::unicast_address(sdp::fields::origin(session_description)));

            // session connection data is the default for each media description
            auto& session_connection_data = sdp::fields::connection_data(session_description);
            if (!session_connection_data.is_null())
            {
                details::set_multicast_ip_interface_ip(params, details::connection_base_address(sdp::fields::connection_address(session_connection_data)));
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

                // take account of the number of source addresses (cf. Operation with SMPTE 2022-7 - Separate Source Addresses)

                auto& media_attributes = sdp::fields::attributes(media_description);
                if (!media_attributes.is_null())
                {
                    auto& ma = media_attributes.as_array();

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

                // media connection data overrides session connection data (if no source filter)
                if (params[nmos::fields::source_ip].is_null())
                {
                    auto& mcda = sdp::fields::connection_data(media_description);
                    if (0 != mcda.size())
                    {
                        // hmm, how to handle multiple connection data?
                        auto& media_connection_data = mcda.at(0);
                        details::set_multicast_ip_interface_ip(params, details::connection_base_address(sdp::fields::connection_address(media_connection_data)));
                    }
                }

                params[nmos::fields::rtp_enabled] = value::boolean(true);

                break;
            }
        }

        return transport_params;
    }

    namespace details
    {
        std::runtime_error sdp_processing_error(const std::string& message)
        {
            return std::runtime_error{ "sdp processing error - " + message };
        }
    }

    sdp_parameters get_session_description_sdp_parameters(const web::json::value& sdp)
    {
        using web::json::value;
        using web::json::value_of;

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
        auto parse_connection_address = [](const utility::string_t& address)
        {
            const auto slash = address.find(U('/'));
            return std::make_pair(address.substr(0, slash), utility::string_t::npos != slash ? utility::istringstreamed<uint32_t>(address.substr(slash + 1)) : 0);
        };
        {
            const auto& connection_data = sdp::fields::connection_data(sdp);
            if (!connection_data.is_null() && 0 != connection_data.size())
            {
                auto connection_address_ttl = parse_connection_address(sdp::fields::connection_address(connection_data.at(0)));
                sdp_params.connection_data.ttl = connection_address_ttl.second;
            }
        }

        // Group
        // a=group:<semantics>[ <identification-tag>]*
        // See https://tools.ietf.org/html/rfc5888
        auto sdp_attributes = sdp::fields::attributes(sdp).as_array();
        if (sdp_attributes.size())
        {
            auto group = sdp::find_name(sdp_attributes, sdp::attributes::group);
            if (sdp_attributes.end() != group)
            {
                const auto& value = sdp::fields::value(*group);

                sdp_params.group.semantics = sdp::fields::semantics(value);
                for (const auto& mid : sdp::fields::mids(value))
                {
                    sdp_params.group.media_stream_ids.push_back(mid.as_string());
                }
            }
        }

        // Media Descriptions
        // See https://tools.ietf.org/html/rfc4566#section-5
        const auto& media_descriptions = sdp::fields::media_descriptions(sdp);
        if (0 == media_descriptions.size()) throw details::sdp_processing_error("missing media descriptions");
        const auto& media_description = media_descriptions.at(0);

        // Connection Data
        // get default multicast_ip via Connection Data
        // see https://tools.ietf.org/html/rfc4566#section-5.7
        {
            const auto& connection_data = sdp::fields::connection_data(media_description);
            if (!connection_data.is_null() && 0 != connection_data.size())
            {
                auto connection_address_ttl = parse_connection_address(sdp::fields::connection_address(connection_data.at(0)));
                sdp_params.connection_data.ttl = connection_address_ttl.second;
            }
        }

        // Media
        // See https://tools.ietf.org/html/rfc4566#section-5.14
        const auto& media = sdp::fields::media(media_description);
        sdp_params.media_type = sdp::media_type{ sdp::fields::media_type(media) };
        sdp_params.protocol = sdp::protocol{ sdp::fields::protocol(media) };

        // media description attributes
        auto& attributes = sdp::fields::attributes(media_description).as_array();

        // ts_refclk attribute
        // See https://tools.ietf.org/html/rfc7273
        auto ts_refclk = sdp::find_name(attributes, sdp::attributes::ts_refclk);
        if (attributes.end() != ts_refclk)
        {
            const auto& value = sdp::fields::value(*ts_refclk);

            sdp_params.ts_refclk.clock_source = sdp::ts_refclk_source{ sdp::fields::clock_source(value) };
            sdp_params.ts_refclk.ptp_version = sdp::ptp_version{ sdp::fields::ptp_version(value) };
            sdp_params.ts_refclk.ptp_server = sdp::fields::ptp_server(value);
        }

        // mediaclk attribute
        // See https://tools.ietf.org/html/rfc7273
        auto mediaclk = sdp::find_name(attributes, sdp::attributes::mediaclk);
        if (attributes.end() != mediaclk)
        {
            const auto& value = sdp::fields::value(*mediaclk).as_string();
            const auto eq = value.find(U('='));
            sdp_params.mediaclk = { sdp::media_clock_source{ value.substr(0, eq) }, utility::string_t::npos != eq ? value.substr(eq + 1) : utility::string_t{} };
        }

        // rtmap attribute
        // See https://tools.ietf.org/html/rfc4566#section-6
        auto rtpmap = sdp::find_name(attributes, sdp::attributes::rtpmap);
        if (attributes.end() == rtpmap) throw details::sdp_processing_error("missing attribute: rtpmap");
        const auto& rtpmap_value = sdp::fields::value(*rtpmap);

        sdp_params.rtpmap.encoding_name = sdp::fields::encoding_name(rtpmap_value);
        sdp_params.rtpmap.payload_type = sdp::fields::payload_type(rtpmap_value);
        sdp_params.rtpmap.clock_rate = sdp::fields::clock_rate(rtpmap_value);

        const auto format = details::get_format(sdp_params);
        const auto is_video_sdp = nmos::formats::video == format;
        const auto is_audio_sdp = nmos::formats::audio == format;

        if (is_audio_sdp)
        {
            const auto& encoding_name = sdp::fields::encoding_name(rtpmap_value);
            sdp_params.audio.bit_depth = !encoding_name.empty() && U('L') == encoding_name.front() ? utility::istringstreamed<uint32_t>(encoding_name.substr(1)) : 0;

            sdp_params.audio.sample_rate = nmos::rational{ sdp::fields::clock_rate(rtpmap_value) };
            sdp_params.audio.channel_count = (uint32_t)sdp::fields::encoding_parameters(rtpmap_value);
        }

        // ptime attribute
        // See https://tools.ietf.org/html/rfc4566#section-6
        {
            auto ptime = sdp::find_name(attributes, sdp::attributes::ptime);
            if (is_audio_sdp)
            {
                if (attributes.end() == ptime) throw details::sdp_processing_error("missing attribute: ptime");
                sdp_params.audio.packet_time = sdp::fields::value(*ptime).as_integer();
            }
        }

        // fmtp attribute
        // See https://tools.ietf.org/html/rfc4566#section-6
        auto fmtp = sdp::find_name(attributes, sdp::attributes::fmtp);
        if (is_video_sdp || is_audio_sdp)
        {
            if (attributes.end() == fmtp) throw details::sdp_processing_error("missing attribute: fmtp");
            const auto& fmtp_value = sdp::fields::value(*fmtp);
            const auto& format_specific_parameters = sdp::fields::format_specific_parameters(fmtp_value);

            if (is_video_sdp)
            {
                const auto width = sdp::find_name(format_specific_parameters, sdp::fields::width);
                if (format_specific_parameters.end() == width) throw details::sdp_processing_error("missing format parameter: width");
                sdp_params.video.width = utility::istringstreamed<uint32_t>(sdp::fields::value(*width).as_string());

                const auto height = sdp::find_name(format_specific_parameters, sdp::fields::height);
                if (format_specific_parameters.end() == height) throw details::sdp_processing_error("missing format parameter: height");
                sdp_params.video.height = utility::istringstreamed<uint32_t>(sdp::fields::value(*height).as_string());

                auto parse_rational = [](const utility::string_t& rational_string)
                {
                    const auto slash = rational_string.find(U('/'));
                    return nmos::rational(utility::istringstreamed<uint64_t>(rational_string.substr(0, slash)), utility::string_t::npos != slash ? utility::istringstreamed<uint64_t>(rational_string.substr(slash + 1)) : 1);
                };
                const auto exactframerate = sdp::find_name(format_specific_parameters, sdp::fields::exactframerate);
                if (format_specific_parameters.end() == exactframerate) throw details::sdp_processing_error("missing format parameter: exactframerate");
                sdp_params.video.exactframerate = parse_rational(sdp::fields::value(*exactframerate).as_string());

                // optional
                const auto interlace = sdp::find_name(format_specific_parameters, sdp::fields::interlace);
                sdp_params.video.interlace = format_specific_parameters.end() != interlace;

                const auto sampling = sdp::find_name(format_specific_parameters, sdp::fields::sampling);
                if (format_specific_parameters.end() == sampling) throw details::sdp_processing_error("missing format parameter: sampling");
                sdp_params.video.sampling = nmos::sampling{ sdp::fields::value(*sampling).as_string() };

                const auto depth = sdp::find_name(format_specific_parameters, sdp::fields::depth);
                if (format_specific_parameters.end() == depth) throw details::sdp_processing_error("missing format parameter: depth");
                auto test = sdp::fields::value(*depth);
                sdp_params.video.depth = utility::istringstreamed<uint32_t>(sdp::fields::value(*depth).as_string());

                // optional
                const auto tcs = sdp::find_name(format_specific_parameters, sdp::fields::TCS);
                if (format_specific_parameters.end() != tcs)
                {
                    sdp_params.video.tcs = nmos::transfer_characteristic{ sdp::fields::value(*tcs).as_string() };
                }

                const auto colorimetry = sdp::find_name(format_specific_parameters, sdp::fields::colorimetry);
                if (format_specific_parameters.end() == colorimetry) throw details::sdp_processing_error("missing format parameter: colorimetry");
                sdp_params.video.colorimetry = nmos::colorspace{ sdp::fields::value(*colorimetry).as_string() };

                // don't check "PM" (packing mode)

                // don't check "SSN" (SMPTE Standard Number)

                const auto tp = sdp::find_name(format_specific_parameters, sdp::fields::TP);
                if (format_specific_parameters.end() == tp) throw details::sdp_processing_error("missing format parameter: TP");
                sdp_params.video.tp = nmos::sender_type_parameter{ sdp::fields::value(*tp).as_string() };
            }
            else
            {
                const auto channel_order = sdp::find_name(format_specific_parameters, sdp::fields::channel_order);
                if (format_specific_parameters.end() == channel_order) throw details::sdp_processing_error("missing format parameter: channel_order");
                sdp_params.audio.channel_order = sdp::fields::value(*channel_order).as_string();
            }
        }

        return sdp_params;
    }

    std::pair<sdp_parameters, web::json::value> parse_session_description(const web::json::value& session_description)
    {
        const auto& media_descriptions = sdp::fields::media_descriptions(session_description).as_array();
        const bool smpte2022_7 = 2 == media_descriptions.size();

        return{ get_session_description_sdp_parameters(session_description), nmos::get_session_description_transport_params(session_description, smpte2022_7) };
    }
}

#include <boost/asio/ip/address.hpp> // included late to avoid trouble from the IN preprocessor definition on Windows

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
    }
}

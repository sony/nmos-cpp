#include "nmos/sdp_utils.h"

#include "cpprest/basic_utils.h"
#include "nmos/json_fields.h"
#include "sdp/json.h"

#include <boost/asio/ip/address.hpp> // included late to avoid trouble from the IN preprocessor definition on Windows

namespace nmos
{
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

#if BOOST_VERSION >= 106600
            const auto ip_address = boost::asio::ip::make_address(utility::us2s(address));
#else
            const auto ip_address = boost::asio::ip::address::from_string(utility::us2s(ip));
#endif

            if (ip_address.is_multicast())
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

        auto transport_params = value::array({ smpte2022_7 ? 2 : 1, value::object() });

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

                    // should check rtpmap attribute's encoding name, e.g. "raw"
                    // (vs. IS-04 receiver's caps.media_types)

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
                    }
                }

                params[nmos::fields::destination_port] = value::number(sdp::fields::port(media));

                // media connection data overrides session connection data
                auto& mcda = sdp::fields::connection_data(media_description);
                if (0 != mcda.size())
                {
                    // hmm, how to handle multiple connection data?
                    auto& media_connection_data = mcda.at(0);
                    details::set_multicast_ip_interface_ip(params, details::connection_base_address(sdp::fields::connection_address(media_connection_data)));
                }

                params[nmos::fields::rtp_enabled] = value::boolean(true);
            }
        }

        return transport_params;
    }
}

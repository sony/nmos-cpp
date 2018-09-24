#ifndef SDP_JSON_H
#define SDP_JSON_H

#include <algorithm>
#include "cpprest/json_utils.h"
#include "nmos/string_enum.h" // for now, accept dependency on nmos module

// SDP: Session Description Protocol
// See https://tools.ietf.org/html/rfc4566
namespace sdp
{
    // Field accessors simplify access to fields in the SDP json representation
    namespace fields
    {
        // Protocol Version
        // See https://tools.ietf.org/html/rfc4566#section-5.1
        const web::json::field_with_default<uint32_t> protocol_version{ U("protocol_version"), 0 };

        // Origin
        // See https://tools.ietf.org/html/rfc4566#section-5.2
        const web::json::field_as_value origin{ U("origin") };
        const web::json::field_as_string user_name{ U("user_name") };
        const web::json::field<uint64_t> session_id{ U("session_id") };
        const web::json::field<uint64_t> session_version{ U("session_version") };
        const web::json::field_as_string network_type{ U("network_type") }; // see sdp::network_types
        const web::json::field_as_string address_type{ U("address_type") }; // see sdp::address_types
        const web::json::field_as_string unicast_address{ U("unicast_address") };

        // Session Name
        // See https://tools.ietf.org/html/rfc4566#section-5.3
        const web::json::field_as_string session_name{ U("session_name") };

        // Session/Media Information
        // See https://tools.ietf.org/html/rfc4566#section-5.4
        const web::json::field_as_string_or information{ U("information"), {} };

        // URI
        // See https://tools.ietf.org/html/rfc4566#section-5.5
        const web::json::field_as_string_or uri{ U("uri"), {} };

        // Email Addresses
        // See https://tools.ietf.org/html/rfc4566#section-5.6
        const web::json::field_as_value_or email_addresses{ U("email_addresses"), web::json::value::array() };

        // Phone Numbers
        // See https://tools.ietf.org/html/rfc4566#section-5.6
        const web::json::field_as_value_or phone_numbers{ U("phone_numbers"), web::json::value::array() };

        // Connection Data
        // See https://tools.ietf.org/html/rfc4566#section-5.7
        const web::json::field_as_value_or connection_data{ U("connection_data"), {} };
        //const web::json::field_as_string network_type{ U("network_type") }; // see sdp::network_types
        //const web::json::field_as_string address_type{ U("address_type") }; // see sdp::address_types
        const web::json::field_as_string connection_address{ U("connection_address") };

        // Bandwidth Information
        // See https://tools.ietf.org/html/rfc4566#section-5.8
        const web::json::field_as_value_or bandwidth_information{ U("bandwidth_information"), web::json::value::array() };
        const web::json::field_as_string bandwidth_type{ U("bandwidth_type") }; // see sdp::bandwidth_types
        const web::json::field<uint64_t> bandwidth{ U("bandwidth") };

        // Time Descriptions
        // See https://tools.ietf.org/html/rfc4566#section-5
        const web::json::field_as_value time_descriptions{ U("time_descriptions") }; // array

        // Timing
        // See https://tools.ietf.org/html/rfc4566#section-5.9
        const web::json::field_as_value timing{ U("timing") };
        const web::json::field<uint64_t> start_time{ U("start_time") };
        const web::json::field<uint64_t> stop_time{ U("stop_time") };

        // Repeat Times
        // See https://tools.ietf.org/html/rfc4566#section-5.10
        const web::json::field_as_value_or repeat_times{ U("repeat_times"), web::json::value::array() };
        const web::json::field_as_value repeat_interval{ U("repeat_interval") };
        const web::json::field_as_value active_duration{ U("active_duration") };
        const web::json::field_as_array offsets{ U("offsets") };
        const web::json::field<int64_t> time_value{ U("time_value") }; // signed for time zone adjustment values
        const web::json::field_as_string_or time_unit{ U("time_unit"), {} }; // see sdp::time_units

        // Zone Adjustments (Time Zones)
        // See https://tools.ietf.org/html/rfc4566#section-5.11
        const web::json::field_as_value_or zone_adjustments{ U("zone_adjustments"), web::json::value::array() };
        const web::json::field<uint64_t> time{ U("time") };
        const web::json::field_as_value adjustment{ U("adjustment") };
        //const web::json::field<int64_t> time_value{ U("time_value") }; // signed for time zone adjustment values
        //const web::json::field_as_string_or time_unit{ U("time_unit"), {} }; // see sdp::time_units

        // Encryption Keys (deprecated)
        // See https://tools.ietf.org/html/rfc4566#section-5.12
        const web::json::field_as_string_or encryption_key{ U("encryption_key"), {} };

        // Attributes
        // See https://tools.ietf.org/html/rfc4566#section-5.13
        const web::json::field_as_value_or attributes{ U("attributes"), web::json::value::array() };
        const web::json::field_as_string name{ U("name") };
        const web::json::field_as_value_or value{ U("value"), web::json::value::null() };

        // Media Descriptions
        // See https://tools.ietf.org/html/rfc4566#section-5
        const web::json::field_as_value_or media_descriptions{ U("media_descriptions"), web::json::value::array() };

        // Media
        // See https://tools.ietf.org/html/rfc4566#section-5.14
        const web::json::field_as_value media{ U("media") };
        const web::json::field_as_string media_type{ U("media_type") }; // see sdp::media_types
        const web::json::field<uint64_t> port{ U("port") };
        const web::json::field_with_default<uint64_t> port_count{ U("port_count"), 1 };
        const web::json::field_as_string protocol{ U("protocol") }; // see sdp::protocols
        const web::json::field_as_value_or formats{ U("formats"), web::json::value::array() };
    }

    // Attribute names
    namespace attributes
    {
        // See https://tools.ietf.org/html/rfc4566#section-6
        const utility::string_t cat{ U("cat") };
        const utility::string_t keywds{ U("keywds") };
        const utility::string_t tool{ U("tool") };
        const utility::string_t ptime{ U("ptime") };
        const utility::string_t maxptime{ U("maxptime") };
        const utility::string_t rtpmap{ U("rtpmap") };
        const utility::string_t recvonly{ U("recvonly") };
        const utility::string_t sendrecv{ U("sendrecv") };
        const utility::string_t sendonly{ U("sendonly") };
        const utility::string_t inactive{ U("inactive") };
        const utility::string_t orient{ U("orient") };
        const utility::string_t type{ U("type") };
        const utility::string_t charset{ U("charset") };
        const utility::string_t sdplang{ U("sdplang") };
        const utility::string_t lang{ U("lang") };
        const utility::string_t framerate{ U("framerate") };
        const utility::string_t quality{ U("quality") };
        const utility::string_t fmtp{ U("fmtp") };

        // See https://tools.ietf.org/html/rfc4570
        const utility::string_t source_filter{ U("source-filter") };

        // See https://tools.ietf.org/html/rfc5888
        const utility::string_t group{ U("group") };
        const utility::string_t mid{ U("mid") };

        // See https://tools.ietf.org/html/rfc7273
        const utility::string_t ts_refclk{ U("ts-refclk") };
        const utility::string_t mediaclk{ U("mediaclk") };
    }

    namespace fields
    {
        // Attribute Values

        // a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
        // See https://tools.ietf.org/html/rfc4566#section-6
        const web::json::field<uint64_t> payload_type{ U("payload_type") };
        const web::json::field_as_string encoding_name{ U("encoding_name") };
        const web::json::field<uint64_t> clock_rate{ U("clock_rate") };
        const web::json::field_with_default<uint64_t> encoding_parameters{ U("encoding_parameters"), 1 };

        // a=fmtp:<format> <format specific parameters>
        // See https://tools.ietf.org/html/rfc4566#section-6
        const web::json::field_as_string format{ U("format") };
        const web::json::field_as_array format_specific_parameters{ U("format_specific_parameters") };

        // a=group:<semantics>[ <identification-tag>]*
        // See https://tools.ietf.org/html/rfc5888
        const web::json::field_as_string semantics{ U("semantics") }; // "DUP", etc.
        const web::json::field_as_array mids{ U("mids") };

        // a=source-filter: <filter-mode> <nettype> <address-types> <dest-address> <src-list>
        // See https://tools.ietf.org/html/rfc4570
        const web::json::field_as_string filter_mode{ U("filter_mode") }; // see sdp::filter_modes
        //const web::json::field_as_string network_type{ U("network_type") }; // see sdp::network_types
        const web::json::field_as_string address_types{ U("address_types") }; // see sdp::address_types
        const web::json::field_as_string destination_address{ U("destination_address") }; // address or "*"
        const web::json::field_as_array source_addresses{ U("source_addresses") }; // array of addresses

        // a=ts-refclk:ntp=<ntp server addr>
        // a=ts-refclk:ntp=/traceable/
        // a=ts-refclk:ptp=<ptp version>:<ptp gmid>[:<ptp domain>]
        // a=ts-refclk:ptp=<ptp version>:traceable
        // a=ts-refclk:gps
        // a=ts-refclk:gal
        // a=ts-refclk:glonass
        // a=ts-refclk:local
        // a=ts-refclk:private[:traceable]
        // See https://tools.ietf.org/html/rfc7273
        // a=ts-refclk:localmac=7C-E9-D3-1B-9A-AF
        // See SMPTE ST 2110-10:2017 Professional Media Over Managed IP Networks: System Timing and Definitions, Section 8.2 Reference Clock
        const web::json::field_as_string clock_source{ U("clock_source") }; // see sdp::timestamp_reference_clock_sources
        const web::json::field_with_default<bool> traceable{ U("traceable"), false };
        const web::json::field_as_string_or ntp_server{ U("ntp_server"), {} };
        const web::json::field_as_string ptp_version{ U("ptp_version") };
        const web::json::field_as_string_or ptp_server{ U("ptp_server"), {} };

        // a=mediaclk:[id=<clock id> ]<clock source>[=<clock parameters>]
        // See https://tools.ietf.org/html/rfc7273#section-5
    }

    // find an array element with the specified name (useful with attributes and format specific parameters)
    inline web::json::array::const_iterator find_name(const web::json::array& name_value_array, const utility::string_t& name)
    {
        return std::find_if(name_value_array.begin(), name_value_array.end(), [&](const web::json::value& nv)
        {
            return sdp::fields::name(nv) == name;
        });
    }

    // Time Units
    // See https://tools.ietf.org/html/rfc4566#section-5.10
    DEFINE_STRING_ENUM(time_unit)
    namespace time_units
    {
        const time_unit days{ U("d") };
        const time_unit hours{ U("h") };
        const time_unit minutes{ U("m") };
        const time_unit seconds{ U("s") };
        const time_unit none{};
    }

    // Media Types
    // See https://tools.ietf.org/html/rfc4566#section-8.2.1
    DEFINE_STRING_ENUM(media_type)
    namespace media_types
    {
        const media_type audio{ U("audio") };
        const media_type video{ U("video") };
        const media_type text{ U("text") };
        const media_type application{ U("application") };
        const media_type message{ U("message") };
    }

    // Transport Protocols
    // See https://tools.ietf.org/html/rfc4566#section-8.2.2
    DEFINE_STRING_ENUM(protocol)
    namespace protocols
    {
        // RTP Profile for Audio and Video Conferences with Minimal Control running over UDP
        const protocol RTP_AVP{ U("RTP/AVP") };
        // Secure Real-time Transport Protocol running over UDP
        const protocol RTP_SAVP{ U("RTP/SAVP") };
        // unspecified protocol running over UDP
        const protocol udp{ U("udp") };
    }

    // Bandwidth Specifiers
    // See https://tools.ietf.org/html/rfc4566#section-8.2.5
    DEFINE_STRING_ENUM(bandwidth_type)
    namespace bandwidth_types
    {
        // "conference total" bandwidth
        const bandwidth_type CT{ U("CT") };
        // application specific (maximum bandwidth)
        const bandwidth_type AS{ U("AS") };
    }

    // Network Types
    // See https://tools.ietf.org/html/rfc4566#section-8.2.6
    DEFINE_STRING_ENUM(network_type)
    namespace network_types
    {
        const network_type IN{ U("IN") };
    }

    // Address Types
    // See https://tools.ietf.org/html/rfc4566#section-8.2.8
    DEFINE_STRING_ENUM(address_type)
    namespace address_types
    {
        const address_type IP4{ U("IP4") };
        const address_type IP6{ U("IP4") };
    }
}

// Session Description Protocol (SDP) Source Filters
// See https://tools.ietf.org/html/rfc4570
namespace sdp
{
    // Filter Mode
    DEFINE_STRING_ENUM(filter_mode)
    namespace filter_modes
    {
        const filter_mode incl{ U("incl") };
        const filter_mode excl{ U("excl") };
    }

    // Address Types
    namespace address_types
    {
        const address_type wildcard{ U("*") };
    }
}

// RTP Clock Source Signalling
// See https://tools.ietf.org/html/rfc7273
namespace sdp
{
    // Timestamp Reference Clock Source
    // See https://tools.ietf.org/html/rfc7273#section-8.3
    // and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xhtml#timestamp-ref-clock-source
    DEFINE_STRING_ENUM(timestamp_reference_clock_source)
    namespace timestamp_reference_clock_sources
    {
        // Network Time Protocol
        const timestamp_reference_clock_source NTP{ U("ntp") };
        // Precision Time Protocol
        const timestamp_reference_clock_source PTP{ U("ptp") };
        // Global Positioning System
        const timestamp_reference_clock_source GPS{ U("gps") };
        // Galileo
        const timestamp_reference_clock_source GAL{ U("gal") };
        // Global Navigation Satellite System
        const timestamp_reference_clock_source GLONASS{ U("glonass") };
        // Local Clock
        const timestamp_reference_clock_source LOCAL{ U("local") };
        // Private Clock
        const timestamp_reference_clock_source PRIVATE{ U("private") };

        // Local MAC
        // See SMPTE ST 2110-10:2017 Professional Media Over Managed IP Networks: System Timing and Definitions, Section 8.2 Reference Clock
        const timestamp_reference_clock_source LOCAL_MAC{ U("localmac") };
    }
    typedef timestamp_reference_clock_source ts_refclk_source;
    namespace ts_refclk_sources = timestamp_reference_clock_sources;

    // PTP Version
    // See https://tools.ietf.org/html/rfc7273#section-4.3
    // and https://tools.ietf.org/html/rfc7273#section-4.8
    DEFINE_STRING_ENUM(ptp_version)
    namespace ptp_versions
    {
        const ptp_version IEEE1588_2002{ U("IEEE1588-2002") };
        const ptp_version IEEE1588_2008{ U("IEEE1588-2008") };
        const ptp_version IEEE802_1AS_2011{ U("IEEE802.1AS-2011") };
    }

    // Media Clock Source
    // See https://tools.ietf.org/html/rfc7273#section-8.4
    // and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xhtml#media-clock-source
    DEFINE_STRING_ENUM(media_clock_source)
    namespace media_clock_sources
    {
        // Asynchronously Generated Media Clock
        const media_clock_source sender{ U("sender") };
        // Direct-Referenced Media Clock
        const media_clock_source direct{ U("direct") };
        // IEEE1722 Media Stream Identifier
        const media_clock_source IEEE1722{ U("IEEE1722") };
    }
    typedef media_clock_source mediaclk_source;
    namespace mediaclk_sources = media_clock_sources;
}

#endif

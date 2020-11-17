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
        const web::json::field_as_string semantics{ U("semantics") }; // see sdp::group_semantics
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
        // a=ts-refclk:localmac=<mac-address-of-sender>
        // See SMPTE ST 2110-10:2017 Professional Media Over Managed IP Networks: System Timing and Definitions, Section 8.2 Reference Clock
        const web::json::field_as_string clock_source{ U("clock_source") }; // see sdp::timestamp_reference_clock_sources
        const web::json::field_with_default<bool> traceable{ U("traceable"), false };
        const web::json::field_as_string_or ntp_server{ U("ntp_server"), {} };
        const web::json::field_as_string ptp_version{ U("ptp_version") }; // see sdp::ptp_versions
        const web::json::field_as_string_or ptp_server{ U("ptp_server"), {} }; // <ptp gmid>[:<ptp domain>]
        const web::json::field_as_string mac_address{ U("mac_address") };

        // a=mediaclk:[id=<clock id> ]<clock source>[=<clock parameters>]
        // See https://tools.ietf.org/html/rfc7273#section-5
    }

    // make a named value (useful for attributes)
    inline web::json::value named_value(const utility::string_t& name, const web::json::value& value = {}, bool keep_order = true)
    {
        return !value.is_null()
            ? web::json::value_of({ { sdp::fields::name, web::json::value::string(name) }, { sdp::fields::value, value } }, keep_order)
            : web::json::value_of({ { sdp::fields::name, web::json::value::string(name) } }, keep_order);
    }

    // make a named value (useful for format specific parameters)
    inline web::json::value named_value(const utility::string_t& name, const utility::string_t& value)
    {
        return named_value(name, web::json::value::string(value));
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
    // and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xml#sdp-parameters-1
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
    // and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xml#sdp-parameters-2
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
    // and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xml#sdp-parameters-3
    DEFINE_STRING_ENUM(bandwidth_type)
    namespace bandwidth_types
    {
        // "conference total" bandwidth
        const bandwidth_type conference_total{ U("CT") };
        // application specific (maximum bandwidth)
        const bandwidth_type application_specific{ U("AS") };
    }

    // Network Types
    // See https://tools.ietf.org/html/rfc4566#section-8.2.6
    // and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xml#sdp-parameters-4
    DEFINE_STRING_ENUM(network_type)
    namespace network_types
    {
        // internet
        const network_type internet{ U("IN") };
    }

    // Address Types
    // See https://tools.ietf.org/html/rfc4566#section-8.2.8
    // and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xml#sdp-parameters-5
    DEFINE_STRING_ENUM(address_type)
    namespace address_types
    {
        // IPv4
        const address_type IP4{ U("IP4") };
        // IPv6
        const address_type IP6{ U("IP6") };
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
        // inclusive
        const filter_mode incl{ U("incl") };
        // exclusive
        const filter_mode excl{ U("excl") };
    }

    // Address Types
    namespace address_types
    {
        const address_type wildcard{ U("*") };
    }
}

// Session Description Protocol (SDP) Grouping Framework
// See https://tools.ietf.org/html/rfc5888
// and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xml#sdp-parameters-13
// and https://tools.ietf.org/html/rfc5576
// and https://www.iana.org/assignments/sdp-parameters/sdp-parameters.xml#sdp-parameters-17
namespace sdp
{
    // Group semantics
    DEFINE_STRING_ENUM(group_semantics_type)
    namespace group_semantics
    {
        // See https://tools.ietf.org/html/rfc7104
        const group_semantics_type duplication{ U("DUP") };
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
        const timestamp_reference_clock_source ntp{ U("ntp") };
        // Precision Time Protocol
        const timestamp_reference_clock_source ptp{ U("ptp") };
        // Global Positioning System
        const timestamp_reference_clock_source gps{ U("gps") };
        // Galileo
        const timestamp_reference_clock_source galileo{ U("gal") };
        // Global Navigation Satellite System
        const timestamp_reference_clock_source glonass{ U("glonass") };
        // Local Clock
        const timestamp_reference_clock_source local_clock{ U("local") };
        // Private Clock
        const timestamp_reference_clock_source private_clock{ U("private") };

        // Local MAC
        // See SMPTE ST 2110-10:2017 Professional Media Over Managed IP Networks: System Timing and Definitions, Section 8.2 Reference Clock
        const timestamp_reference_clock_source local_mac{ U("localmac") };
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

// SDP Format Specific Parameters
// See https://tools.ietf.org/html/rfc4566#section-6
namespace sdp
{
    namespace fields
    {
        // See https://tools.ietf.org/html/rfc4175
        // and SMPTE ST 2110-20:2017 Section 7 Session Description Protocol (SDP) Considerations
        // and VSF TR-05:2018
        const web::json::field<uint32_t> width{ U("width") };
        const web::json::field<uint32_t> height{ U("height") };
        const web::json::field_as_string exactframerate{ U("exactframerate") };
        const web::json::field_as_bool_or interlace{ U("interlace"), false };
        const web::json::field_as_bool_or segmented{ U("segmented"), false };
        const web::json::field_as_bool_or top_field_first{ U("top-field-first"), false };
        const web::json::field_as_string sampling{ U("sampling") };
        const web::json::field<uint32_t> depth{ U("depth") };
        const web::json::field_as_string transfer_characteristic_system{ U("TCS") }; // "if unspecified, receivers shall assume the value SDR"
        const web::json::field_as_string colorimetry{ U("colorimetry") };
        const web::json::field_as_string packing_mode{ U("PM") };
        const web::json::field_as_string smpte_standard_number{ U("SSN") };

        // See SMPTE ST 2110-21:2017 Section 8 Session Description Considerations
        const web::json::field_as_string type_parameter{ U("TP") };

        // See SMPTE ST 2110-30:2017
        // and https://tools.ietf.org/html/rfc3190
        const web::json::field_as_string channel_order{ U("channel-order") }; // "<convention>.<order>", e.g. "SMPTE2110.(ST)"

        // See SMPTE ST 2110-40:2018
        // and https://tools.ietf.org/html/rfc8331
        const web::json::field_as_string DID_SDID{ U("DID_SDID") }; // e.g. "{0x41,0x01}"
        const web::json::field<uint32_t> VPID_Code{ U("VPID_Code") }; // 1..255
    }
}

namespace sdp
{
    // Colour (sub-)sampling mode of the video stream, e.g. "YCbCr-4:2:2"
    // See https://tools.ietf.org/html/rfc4175
    // and SMPTE ST 2110-20:2107 Section 7.4.1 Samping
    // and VSF TR-05:2018, etc.
    DEFINE_STRING_ENUM(sampling)
    namespace samplings
    {
        const sampling RGBA{ U("RGBA") };
        const sampling RGB{ U("RGB") };
        // Non-constant luminance YCbCr
        const sampling YCbCr_4_4_4{ U("YCbCr-4:4:4") };
        const sampling YCbCr_4_2_2{ U("YCbCr-4:2:2") };
        const sampling YCbCr_4_2_0{ U("YCbCr-4:2:0") };
        const sampling YCbCr_4_1_1{ U("YCbCr-4:1:1") };
        // Constant intensity ICtCp
        const sampling ICtCp_4_4_4{ U("ICtCp-4:4:4") };
        const sampling ICtCp_4_2_2{ U("ICtCp-4:2:2") };
        const sampling ICtCp_4_2_0{ U("ICtCp-4:2:0") };
        // XYZ
        const sampling XYZ{ U("XYZ") };
    }

    // Colorimetry
    // See https://tools.ietf.org/html/rfc4175
    // and SMPTE ST 2110-20:2017 Section 7.5 Permitted values of Colorimetry
    // and AMWA IS-04 v1.2 "colorspace"
    DEFINE_STRING_ENUM(colorimetry)
    namespace colorimetries
    {
        // ITU-R BT.601-7
        // hmm, RFC 4175 defines "BT601-5" for ITU Recommendation BT.601-5?
        const colorimetry BT601{ U("BT601") };
        // ITU-R BT.709-6
        // hmm, RFC 5175 defines "BT709-2" for ITU Recommendation BT.709-2 and then uses "BT.709-2" in an example!!
        const colorimetry BT709{ U("BT709") };
        // ITU-R BT.2020-2
        const colorimetry BT2020{ U("BT2020") };
        // ITU-R BT.2100 Table 2
        const colorimetry BT2100{ U("BT2100") };
    }

    // Packing Mode
    // See SMPTE ST 2110-20:2017 Section 6.3.1 Packing Modes, etc.
    DEFINE_STRING_ENUM(packing_mode)
    namespace packing_modes
    {
        // General Packing Mode
        const packing_mode general{ U("2110GPM") };
        // Block Packing Mode
        const packing_mode block{ U("2110BPM") };
    }

    // SMPTE Standard Number
    // See SMPTE ST 2110-20:2017 Section 7.2 Required Media Type Parameters
    DEFINE_STRING_ENUM(smpte_standard_number)
    namespace smpte_standard_numbers
    {
        const smpte_standard_number ST2110_20_2017{ U("ST2110-20:2017") };
    }

    // TP (Media Type Parameter)
    // See SMPTE ST 2110-21:2017 Section 7.1 Compliance Definitions - Senders
    DEFINE_STRING_ENUM(type_parameter)
    namespace type_parameters
    {
        // Narrow Senders (Type N)
        const type_parameter type_N{ U("2110TPN") };
        // Narrow Linear Senders (Type NL)
        const type_parameter type_NL{ U("2110TPNL") };
        // Wide Senders (Type W)
        const type_parameter type_W{ U("2110TPW") };
    }

    // TCS (Transfer Characteristic System)
    // See SMPTE ST 2110-21:2017 Section 7.6 Permitted values of TCS
    // and AMWA IS-04 v1.2 "transfer_characteristic"
    DEFINE_STRING_ENUM(transfer_characteristic_system)
    namespace transfer_characteristic_systems
    {
        // Standard Dynamic Range
        const transfer_characteristic_system SDR{ U("SDR") };
        // Perceptual Quantization
        const transfer_characteristic_system PQ{ U("PQ") };
        // Hybrid Log Gamma
        const transfer_characteristic_system HLG{ U("HLG") };
    }
}

#endif

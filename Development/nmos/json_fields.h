#ifndef NMOS_JSON_FIELDS_H
#define NMOS_JSON_FIELDS_H

#include "cpprest/json_utils.h"
#include "nmos/api_version.h" // for nmos::api_version and parse_api_version
#include "nmos/version.h" // for nmos::tai and parse_version

// json field accessor helpers
namespace web
{
    namespace json
    {
        namespace details
        {
            template <> struct value_as<nmos::api_version>
            {
                nmos::api_version operator()(const web::json::value& value) const { return nmos::parse_api_version(value.as_string()); }
            };

            template <> struct value_as<nmos::tai>
            {
                nmos::tai operator()(const web::json::value& value) const { return nmos::parse_version(value.as_string()); }
            };
        }
    }
}

namespace nmos
{
    // Field accessors simplify access to fields in json request bodies
    namespace fields
    {
        const web::json::field_as_string id{ U("id") };
        const web::json::field<tai> version{ U("version") };

        // IS-04 Discovery and Registration

        // (mostly) for node_api and registration_api
        const web::json::field_as_string type{ U("type") };
        const web::json::field_as_value data{ U("data") };
        // resource_core
        const web::json::field_as_string label{ U("label") };
        const web::json::field_as_string description{ U("description") };
        const web::json::field_as_object tags{ U("tags") };
        // node
        const web::json::field_as_value_or caps{ U("caps"), web::json::value::object() }; // required in node, receiver, source, optional in sender (and up to v1.3 only used in receiver)
        const web::json::field_as_array interfaces{ U("interfaces") };
        const web::json::field_as_string name{ U("name") }; // used in both interfaces and clocks
        const web::json::field_as_string chassis_id{ U("chassis_id") };
        const web::json::field_as_string port_id{ U("port_id") };
        const web::json::field_as_value_or attached_network_device{ U("attached_network_device"), {} }; // object
        const web::json::field_as_array clocks{ U("clocks") };
        const web::json::field_as_string ref_type{ U("ref_type") };
        const web::json::field_as_string ptp_version{ U("version") };
        const web::json::field_as_string gmid{ U("gmid") };
        const web::json::field_as_bool traceable{ U("traceable") };
        const web::json::field_as_bool locked{ U("locked") };
        // device
        const web::json::field_as_string node_id{ U("node_id") };
        const web::json::field_as_array senders{ U("senders") }; // deprecated
        const web::json::field_as_array receivers{ U("receivers") }; // deprecated
        // source_core
        const web::json::field_as_string device_id{ U("device_id") }; // also used in also used in sender, receiver, and flow from v1.1
        const web::json::field_as_array parents{ U("parents") }; // also used in flow
        const web::json::field_as_value clock_name{ U("clock_name") }; // string or null
        const web::json::field_as_string format{ U("format") }; // also used in flow
        // source_audio
        const web::json::field_as_array channels{ U("channels") };
        const web::json::field_as_string_or symbol{ U("symbol"), U("") }; // or nmos::channel_symbol?
        // flow_core
        const web::json::field_as_string source_id{ U("source_id") };
        const web::json::field_as_value grain_rate{ U("grain_rate") }; // or field<nmos::rational> with a bit of work!
        const web::json::field_as_integer numerator{ U("numerator") };
        const web::json::field_as_integer_or denominator{ U("denominator"), 1 };
        // flow_video
        const web::json::field_as_integer frame_width{ U("frame_width") };
        const web::json::field_as_integer frame_height{ U("frame_height") };
        const web::json::field_as_string colorspace{ U("colorspace") };
        // flow_video_raw
        const web::json::field_as_array components{ U("components") };
        const web::json::field_as_string_or transfer_characteristic{ U("transfer_characteristic"), U("") }; // or "SDR"?
        const web::json::field_as_string_or interlace_mode{ U("interlace_mode"), U("") }; // or "progressive"?
        const web::json::field_as_integer width{ U("width") };
        const web::json::field_as_integer height{ U("height") };
        const web::json::field_as_integer bit_depth{ U("bit_depth") }; // also used in flow_audio_raw
        // flow_audio
        const web::json::field_as_value sample_rate{ U("sample_rate") };
        // flow_sdianc_data
        const web::json::field_as_value_or DID_SDID{ U("DID_SDID"), web::json::value::array() };
        const web::json::field_as_string_or DID{ U("DID"), U("0x00") }; // probably an oversight this isn't required
        const web::json::field_as_string_or SDID{ U("SDID"), U("0x00") };
        // sender
        const web::json::field_as_array interface_bindings{ U("interface_bindings") }; // also used in receiver
        const web::json::field_as_string transport{ U("transport") }; // also used in receiver
        const web::json::field_as_value flow_id{ U("flow_id") };
        const web::json::field_as_value_or manifest_href{ U("manifest_href"), {} }; // string, or null from v1.3
        const web::json::field_as_value subscription{ U("subscription") }; // from v1.2; also used in receiver from v1.0
        const web::json::field_as_value receiver_id{ U("receiver_id") }; // used in sender subscription
        const web::json::field_as_bool active{ U("active") }; // used in sender subscription; also used in receiver subscription from v1.2
        // receiver_core
        const web::json::field_as_value sender_id{ U("sender_id") }; // used in receiver subscription
        // receiver_audio, receiver_data, receiver_mux, receiver_video
        const web::json::field_as_value_or media_types{ U("media_types"), {} }; // array of string; used in receiver caps
        // receiver_data
        const web::json::field_as_value_or event_types{ U("event_types"), {} }; // array of string; used in receiver caps

        // (mostly) for query_api
        const web::json::field_as_bool persist{ U("persist") };
        const web::json::field_as_bool secure{ U("secure") };
        const web::json::field_as_string resource_path{ U("resource_path") };
        const web::json::field_as_value params{ U("params") }; // object
        const web::json::field_as_integer max_update_rate_ms{ U("max_update_rate_ms") };
        const web::json::field_as_string ws_href{ U("ws_href") };
        const web::json::field_as_string subscription_id{ U("subscription_id") };

        // (mostly) for query_ws_api
        const web::json::field<tai> creation_timestamp{ U("creation_timestamp") };
        const web::json::field<tai> origin_timestamp{ U("origin_timestamp") };
        const web::json::field<tai> sync_timestamp{ U("sync_timestamp") };
        const web::json::field_as_string topic{ U("topic") };

        // IS-05 Device Connection Management

        // for connection_api
        const web::json::field_as_value endpoint_constraints{ U("constraints") }; // array
        const web::json::field<double> constraint_maximum{ U("maximum") }; // integer, number
        const web::json::field<double> constraint_minimum{ U("minimum") }; // integer, number
        const web::json::field_as_value constraint_enum{ U("enum") }; // array
        const web::json::field_as_string constraint_pattern{ U("pattern") }; // regex
        const web::json::field_as_string_or constraint_description{ U("description"), {} }; // string
        const web::json::field_as_value endpoint_staged{ U("staged") }; // object
        const web::json::field_as_value endpoint_active{ U("active") }; // object
        const web::json::field_as_value_or endpoint_transportfile{ U("transportfile"), {} }; // object
        const web::json::field_as_value_or transportfile_data{ U("data"), {} }; // string
        const web::json::field_as_string transportfile_type{ U("type") };
        const web::json::field_as_string transportfile_href{ U("href") };
        const web::json::field_as_bool master_enable{ U("master_enable") };
        const web::json::field_as_value_or activation{ U("activation"), {} }; // object
        const web::json::field_as_value_or mode{ U("mode"), {} }; // string or null
        const web::json::field_as_value_or requested_time{ U("requested_time"), {} }; // string or null
        const web::json::field_as_value_or activation_time{ U("activation_time"), {} }; // string or null
        const web::json::field_as_value_or transport_file{ U("transport_file"), {} }; // object
        const web::json::field_as_value transport_params{ U("transport_params") }; // array

        // for urn:x-nmos:transport:rtp
        const web::json::field_as_value_or multicast_ip{ U("multicast_ip"), {} }; // string or null
        const web::json::field_as_value_or interface_ip{ U("interface_ip"), {} }; // string or null
        const web::json::field_as_value_or source_ip{ U("source_ip"), {} }; // string or null
        const web::json::field_as_value_or destination_port{ U("destination_port"), {} }; // string or integer
        const web::json::field_as_value_or destination_ip{ U("destination_ip"), {} }; // string
        const web::json::field_as_value_or source_port{ U("source_port"), {} }; // string or integer
        const web::json::field_as_bool_or rtp_enabled{ U("rtp_enabled"), false };
        const web::json::field_as_value_or fec_destination_ip{ U("fec_destination_ip"), {} }; // string
        const web::json::field_as_value_or fec1D_destination_port{ U("fec1D_destination_port"), {} }; // string or integer
        const web::json::field_as_value_or fec2D_destination_port{ U("fec2D_destination_port"), {} }; // string or integer
        const web::json::field_as_value_or fec1D_source_port{ U("fec1D_source_port"), {} }; // string or integer
        const web::json::field_as_value_or fec2D_source_port{ U("fec2D_source_port"), {} }; // string or integer
        const web::json::field_as_value_or rtcp_destination_ip{ U("rtcp_destination_ip"), {} }; // string
        const web::json::field_as_value_or rtcp_destination_port{ U("rtcp_destination_port"), {} }; // string or integer
        const web::json::field_as_value_or rtcp_source_port{ U("rtcp_source_port"), {} }; // string or integer
        const web::json::field_as_value_or fec_mode{ U("fec_mode"), {} }; // string
        // for urn:x-nmos:transport:websocket
        const web::json::field_as_value_or connection_uri{ U("connection_uri"), {} }; // string or null
        const web::json::field_as_value_or connection_authorization{ U("connection_authorization"), {} }; // string or bool
        // for urn:x-nmos:transport:mqtt
        const web::json::field_as_value_or source_host{ U("source_host"), {} }; // string or null
        //const web::json::field_as_value_or source_port{ U("source_port"), {} }; // string or integer
        const web::json::field_as_value_or destination_host{ U("destination_host"), {} }; // string or null
        //const web::json::field_as_value_or destination_port{ U("destination_port"), {} }; // string or integer
        const web::json::field_as_value_or broker_protocol{ U("broker_protocol"), {} }; // string
        const web::json::field_as_value_or broker_authorization{ U("broker_authorization"), {} }; // string or bool
        const web::json::field_as_value_or broker_topic{ U("broker_topic"), {} }; // string or null
        const web::json::field_as_value_or connection_status_broker_topic{ U("connection_status_broker_topic"), {} }; // string or null

        // IS-07 Event & Tally

        // for events_api
        const web::json::field_as_value endpoint_type{ U("type") }; // object
        const web::json::field_as_value endpoint_state{ U("state") }; // object
        const web::json::field_as_value identity{ U("identity") }; // object
        const web::json::field_as_value timing{ U("timing") }; // object

        // for events_ws_api commands
        const web::json::field_as_string command{ U("command") };
        // for the "subscription" command
        const web::json::field_as_array sources{ U("sources") };
        // for the "health" command
        const web::json::field<tai> timestamp{ U("timestamp") };

        // for events_ws_api messages
        const web::json::field_as_string message_type{ U("message_type") };
        // for "state" messages
        const web::json::field_as_string state_event_type{ U("event_type") };
        const web::json::field_as_value state_payload{ U("payload") };
        const web::json::field_as_bool payload_boolean_value{ U("value") };
        const web::json::field_as_number payload_number_value{ U("value") };
        const web::json::field_with_default<int64_t> payload_number_scale{ U("scale"), 1 };
        const web::json::field_as_string payload_string_value{ U("value") };

        // for connection_api
        const web::json::field_as_value_or ext_is_07_source_id{ U("ext_is_07_source_id"), {} }; // string or null
        const web::json::field_as_value_or ext_is_07_rest_api_url{ U("ext_is_07_rest_api_url"), {} }; // string or null

        // IS-08 Audio Channel Mapping

        // for channelmapping_api
        const web::json::field_as_string channelmapping_id{ U("channelmapping_id") };
        const web::json::field_as_value endpoint_io{ U("io") }; // object
        // also uses endpoint_active and endpoint_staged
        const web::json::field_as_string activation_id{ U("activation_id") };
        const web::json::field_as_value map{ U("map") }; // object
        const web::json::field_as_value action{ U("action") }; // object
        const web::json::field_as_value routable_inputs{ U("routable_inputs") }; // array or null
        const web::json::field_as_value input{ U("input") }; // string or null
        const web::json::field_as_value channel_index{ U("channel_index") }; // integer or null
        const web::json::field_as_integer block_size{ U("block_size") };
        const web::json::field_as_bool reordering{ U("reordering") };

        // IS-09 System Parameters

        // for system_api
        const web::json::field_as_value is04{ U("is04") };
        const web::json::field_as_integer heartbeat_interval{ U("heartbeat_interval") }; // 1..1000, typically 5
        const web::json::field_as_value ptp{ U("ptp") };
        const web::json::field_as_integer announce_receipt_timeout{ U("announce_receipt_timeout") }; // 2..10, typically 3
        const web::json::field_as_integer domain_number{ U("domain_number") }; // 0..127
    }

    // Fields for experimental extensions
    namespace experimental
    {
        // Field accessors simplify access to fields in json messages
        namespace fields
        {
            const web::json::field<nmos::api_version> api_version{ U("api_version") };
        }
    }
}

#endif

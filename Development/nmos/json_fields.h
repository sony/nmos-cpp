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
        const web::json::field_as_string id{ U("id") }; // see nmos::id
        const web::json::field<tai> version{ U("version") };

        // IS-04 Discovery and Registration

        // (mostly) for node_api and registration_api
        const web::json::field_as_string type{ U("type") }; // see nmos::type
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
        const web::json::field_as_string ref_type{ U("ref_type") }; // see nmos::clock_ref_type
        const web::json::field_as_string ptp_version{ U("version") };
        const web::json::field_as_string gmid{ U("gmid") };
        const web::json::field_as_bool traceable{ U("traceable") };
        const web::json::field_as_bool locked{ U("locked") };
        // device
        const web::json::field_as_string node_id{ U("node_id") }; // see nmos::id
        const web::json::field_as_array senders{ U("senders") }; // deprecated, see nmos::id
        const web::json::field_as_array receivers{ U("receivers") }; // deprecated, see nmos::id
        // source_core
        const web::json::field_as_string device_id{ U("device_id") }; // also used in also used in sender, receiver, and flow from v1.1, see nmos::id
        const web::json::field_as_array parents{ U("parents") }; // also used in flow, see nmos::id
        const web::json::field_as_value clock_name{ U("clock_name") }; // string or null, see nmos::clock_name
        const web::json::field_as_string format{ U("format") }; // also used in flow, see nmos::format
        // source_audio
        const web::json::field_as_array channels{ U("channels") };
        const web::json::field_as_string_or symbol{ U("symbol"), U("") }; // see nmos::channel_symbol
        // flow_core
        const web::json::field_as_string source_id{ U("source_id") }; // see nmos::id
        const web::json::field_as_value grain_rate{ U("grain_rate") }; // see nmos::make_rational, etc.
        const web::json::field_as_integer numerator{ U("numerator") };
        const web::json::field_as_integer_or denominator{ U("denominator"), 1 };
        const web::json::field_as_string media_type{ U("media_type") }; // strictly speaking, defined in flow_video_raw, flow_video_coded, etc., see nmos::media_type
        // flow_video
        const web::json::field_as_integer frame_width{ U("frame_width") };
        const web::json::field_as_integer frame_height{ U("frame_height") };
        const web::json::field_as_string colorspace{ U("colorspace") }; // see nmos::colorspace
        // flow_video_raw
        const web::json::field_as_array components{ U("components") }; // see nmos::make_components, etc.
        const web::json::field_as_string_or transfer_characteristic{ U("transfer_characteristic"), U("") }; // if omitted (empty), assume "SDR", see nmos::transfer_characteristic
        const web::json::field_as_string_or interlace_mode{ U("interlace_mode"), U("") }; // if omitted (empty), assume "progressive", see nmos::interlace_mode
        const web::json::field_as_integer width{ U("width") };
        const web::json::field_as_integer height{ U("height") };
        const web::json::field_as_integer bit_depth{ U("bit_depth") }; // also used in flow_audio_raw
        // flow_audio
        const web::json::field_as_value sample_rate{ U("sample_rate") }; // see nmos::rational
        // flow_sdianc_data
        const web::json::field_as_value_or DID_SDID{ U("DID_SDID"), web::json::value::array() }; // see nmos::did_sdid
        const web::json::field_as_string_or DID{ U("DID"), U("0x00") }; // probably an oversight this isn't required
        const web::json::field_as_string_or SDID{ U("SDID"), U("0x00") };
        // sender
        const web::json::field_as_array interface_bindings{ U("interface_bindings") }; // also used in receiver
        const web::json::field_as_string transport{ U("transport") }; // also used in receiver
        const web::json::field_as_value flow_id{ U("flow_id") }; // see nmos::id
        const web::json::field_as_value_or manifest_href{ U("manifest_href"), {} }; // string, or null from v1.3
        const web::json::field_as_value subscription{ U("subscription") }; // from v1.2; also used in receiver from v1.0
        const web::json::field_as_value receiver_id{ U("receiver_id") }; // used in sender subscription, see nmos::id
        const web::json::field_as_bool active{ U("active") }; // used in sender subscription; also used in receiver subscription from v1.2
        // receiver_core
        const web::json::field_as_value sender_id{ U("sender_id") }; // used in receiver subscription, see nmos::id
        // receiver_audio, receiver_data, receiver_mux, receiver_video
        const web::json::field_as_value_or media_types{ U("media_types"), {} }; // array of string; used in receiver caps, see nmos::media_type
        // receiver_data
        const web::json::field_as_value_or event_types{ U("event_types"), {} }; // array of string; used in receiver caps, see nmos::event_type

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
        const web::json::field_as_value constraint_maximum{ U("maximum") }; // integer, number or rational
        const web::json::field_as_value constraint_minimum{ U("minimum") }; // integer, number or rational
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
        const web::json::field_as_value_or mode{ U("mode"), {} }; // string or null, see nmos::activation_mode
        const web::json::field_as_value_or requested_time{ U("requested_time"), {} }; // string or null, see nmos::tai
        const web::json::field_as_value_or activation_time{ U("activation_time"), {} }; // string or null, see nmos::tai
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
        const web::json::field_as_string state_event_type{ U("event_type") }; // see nmos::event_type
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
        const web::json::field_as_value_or syslog{ U("syslog"), {} };
        const web::json::field_as_value_or syslogv2{ U("syslogv2"), {} };
        const web::json::field_as_string hostname{ U("hostname") }; // hostname, ipv4 or ipv6
        const web::json::field_as_integer port{ U("port") }; // 1..65535

        // IS-12 Control Protocol and MS-05 model definitions
        namespace nc
        {
            // for control_protocol_ws_api
            const web::json::field_as_integer message_type{ U("messageType") };

            // for control_protocol_ws_api commands
            const web::json::field_as_array commands{ U("commands") };
            const web::json::field_as_array subscriptions{ U("subscriptions") };
            const web::json::field_as_integer oid{ U("oid") };
            const web::json::field_as_value method_id{ U("methodId") };
            const web::json::field_as_value_or arguments{ U("arguments"), {} };
            const web::json::field_as_value id{ U("id") };
            const web::json::field_as_integer level{ U("level") };
            const web::json::field_as_integer index{ U("index") };

            // for control_protocol_ws_api responses & errors
            const web::json::field_as_value responses{ U("responses") };
            const web::json::field_as_value result{ U("result") };
            const web::json::field_as_integer status{ U("status") };
            const web::json::field_as_value value{ U("value") };
            const web::json::field_as_string error_message{ U("errorMessage") };

            // for control_protocol_ws_api commands & responses
            const web::json::field_as_integer handle{ U("handle") };

            // for cntrol_protocol_ws_api notifications
            const web::json::field_as_array notifications{ U("notifications") };
            const web::json::field_as_value event_data{ U("eventData") };
            const web::json::field_as_value event_id{ U("eventId") };

            const web::json::field_as_array class_id{ U("classId") };
            const web::json::field_as_bool constant_oid{ U("constantOid") };
            const web::json::field_as_integer owner{ U("owner") };
            const web::json::field_as_string role{ U("role") };
            const web::json::field_as_string user_label{ U("userLabel") };
            const web::json::field_as_array touchpoints{ U("touchpoints") };
            const web::json::field_as_array runtime_property_constraints{ U("runtimePropertyConstraints") };
            const web::json::field_as_bool recurse{ U("recurse") };
            const web::json::field_as_bool enabled{ U("enabled") };
            const web::json::field_as_array members{ U("members") };
            const web::json::field_as_string description{ U("description") };
            const web::json::field_as_string nc_version{ U("ncVersion") }; // NcVersionCode
            const web::json::field_as_value manufacturer{ U("manufacturer") }; // NcManufacturer
            const web::json::field_as_value product{ U("product") }; // NcProduct
            const web::json::field_as_string serial_number{ U("serialNumber") };
            const web::json::field_as_string user_inventory_code{ U("userInventoryCode") };
            const web::json::field_as_string device_name{ U("deviceName") };
            const web::json::field_as_string device_role{ U("deviceRole") };
            const web::json::field_as_value operational_state{ U("operationalState") }; // NcDeviceOperationalState
            const web::json::field_as_integer reset_cause{ U("resetCause") }; // NcResetCause
            const web::json::field_as_string message{ U("message") };
            const web::json::field_as_array control_classes{ U("controlClasses") }; // sequence<NcClassDescriptor>
            const web::json::field_as_array datatypes{ U("datatypes") }; // sequence<NcDatatypeDescriptor>
            const web::json::field_as_string name{ U("name")};
            const web::json::field_as_string fixed_role{ U("fixedRole") };
            const web::json::field_as_array properties{ U("properties") }; // sequence<NcPropertyDescriptor>
            const web::json::field_as_array methods{ U("methods") }; // sequence<NcMethodDescriptor>
            const web::json::field_as_array events{ U("events") }; // sequence<NcEventDescriptor>
            const web::json::field_as_integer type{ U("type") }; // NcDatatypeType
            const web::json::field_as_value constraints{ U("constraints") }; // NcParameterConstraints
            const web::json::field_as_integer organization_id{ U("organizationId") };
            const web::json::field_as_string website{ U("website") };
            const web::json::field_as_string key{ U("key") };
            const web::json::field_as_string revision_level{ U("revisionLevel") };
            const web::json::field_as_string brand_name{ U("brandName") };
            const web::json::field_as_string uuid{ U("uuid") };
            const web::json::field_as_string type_name{ U("typeName") };
            const web::json::field_as_bool is_read_only{ U("isReadOnly") };
            const web::json::field_as_bool is_persistent{ U("isPersistent") };
            const web::json::field_as_bool is_nullable{ U("isNullable") };
            const web::json::field_as_bool is_sequence{ U("isSequence") };
            const web::json::field_as_bool is_deprecated{ U("isDeprecated") };
            const web::json::field_as_bool is_constant{ U("isConstant") };
            const web::json::field_as_string parent_type{ U("parentType") };
            const web::json::field_as_string event_datatype{ U("eventDatatype") };
            const web::json::field_as_string result_datatype{ U("resultDatatype") };
            const web::json::field_as_array parameters{ U("parameters") };
            const web::json::field_as_array items{ U("items") }; // sequence<NcEnumItemDescriptor>
            const web::json::field_as_array fields{ U("fields") }; // sequence<NcFieldDescriptor>
            const web::json::field_as_integer generic_state{ U("generic") }; // NcDeviceGenericState
            const web::json::field_as_string device_specific_details{ U("deviceSpecificDetails") };
            const web::json::field_as_array path{ U("path") }; // NcRolePath
            const web::json::field_as_bool case_sensitive{ U("caseSensitive") };
            const web::json::field_as_bool match_whole_string{ U("matchWholeString") };
            const web::json::field_as_bool include_derived{ U("includeDerived") };
            const web::json::field_as_bool include_inherited{ U("includeInherited") };
            const web::json::field_as_string context_namespace{ U("contextNamespace") };
            const web::json::field_as_value default_value{ U("defaultValue") };
            const web::json::field_as_integer change_type{ U("changeType") }; // NcPropertyChangeType
            const web::json::field_as_integer sequence_item_index{ U("sequenceItemIndex") }; // NcId
            const web::json::field_as_value property_id{ U("propertyId") };
            const web::json::field_as_value maximum{ U("maximum") };
            const web::json::field_as_value minimum{ U("minimum") };
            const web::json::field_as_value step{ U("step") };
            const web::json::field_as_integer max_characters{ U("maxCharacters") };
            const web::json::field_as_string pattern{ U("pattern") };
            const web::json::field_as_value resource{ U("resource") };
            const web::json::field_as_string resource_type{ U("resourceType") };
            const web::json::field_as_string io_id{ U("ioId") };
            const web::json::field_as_integer connection_status{ U("connectionStatus") }; // NcConnectionStatus
            const web::json::field_as_string connection_status_message{ U("connectionStatusMessage") };
            const web::json::field_as_integer payload_status{ U("payloadStatus") }; // NcPayloadStatus
            const web::json::field_as_string payload_status_message{ U("payloadStatusMessage") };
            const web::json::field_as_bool signal_protection_status{ U("signalProtectionStatus") };
            const web::json::field_as_bool active{ U("active") };
            const web::json::field_as_array values{ U("values") };
            const web::json::field_as_string validation_fingerprint{ U("validationFingerprint") };
            const web::json::field_as_string status_message{ U("statusMessage") };
            const web::json::field_as_value data_set{ U("dataSet") }; // NcBulkValuesHolder
            const web::json::field_as_value traits{ U("traits") };
            const web::json::field_as_array included_property_traits{ U("includedPropertyTraits") };
        }

        // NMOS Parameter Registers

        // Sender Attributes Register

        // See https://specs.amwa.tv/nmos-parameter-registers/branches/main/sender-attributes/#st-2110-21-sender-type
        const web::json::field_as_string st2110_21_sender_type{ U("st2110_21_sender_type") }; // see nmos::st2110_21_sender_type
    }

    // IS-10 Authorization
    namespace experimental
    {
        namespace fields
        {
            // Authorization Server Metadata
            const web::json::field_as_value authorization_server_metadata{ U("authorization_server_metadata") };
            // see https://tools.ietf.org/html/rfc8414#section-2
            const web::json::field_as_string_or issuer{ U("issuer"),{} };
            const web::json::field_as_string_or authorization_endpoint{ U("authorization_endpoint"),{} };
            const web::json::field_as_string_or token_endpoint{ U("token_endpoint"),{} };
            const web::json::field_as_string_or registration_endpoint{ U("registration_endpoint"),{} };
            const web::json::field_as_array scopes_supported{ U("scopes_supported") };  // OPTIONAL
            const web::json::field_as_array response_types_supported{ U("response_types_supported") };
            const web::json::field_as_array response_modes_supported{ U("response_modes_supported") }; // OPTIONAL
            const web::json::field_as_array grant_types_supported{ U("grant_types_supported") }; // OPTIONAL
            const web::json::field_as_array token_endpoint_auth_methods_supported{ U("token_endpoint_auth_methods_supported") }; // OPTIONAL
            const web::json::field_as_array token_endpoint_auth_signing_alg_values_supported{ U("token_endpoint_auth_signing_alg_values_supported") }; // OPTIONAL
            const web::json::field_as_string service_documentation{ U("service_documentation") }; // OPTIONAL
            const web::json::field_as_array ui_locales_supported{ U("ui_locales_supported") }; // OPTIONAL
            const web::json::field_as_string op_policy_uri{ U("op_policy_uri") }; // OPTIONAL
            const web::json::field_as_string op_tos_uri{ U("op_tos_uri") }; // OPTIONAL
            const web::json::field_as_string revocation_endpoint{ U("revocation_endpoint") }; // OPTIONAL
            const web::json::field_as_array revocation_endpoint_auth_methods_supported{ U("revocation_endpoint_auth_methods_supported") }; // OPTIONAL
            const web::json::field_as_array revocation_endpoint_auth_signing_alg_values_supported{ U("revocation_endpoint_auth_signing_alg_values_supported") }; // OPTIONAL
            const web::json::field_as_string introspection_endpoint{ U("introspection_endpoint") }; // OPTIONAL
            const web::json::field_as_array introspection_endpoint_auth_methods_supported{ U("introspection_endpoint_auth_methods_supported") }; // OPTIONAL
            const web::json::field_as_array introspection_endpoint_auth_signing_alg_values_supported{ U("introspection_endpoint_auth_signing_alg_values_supported") }; // OPTIONAL
            const web::json::field_as_array code_challenge_methods_supported{ U("code_challenge_methods_supported") };

            // Client Metadata
            const web::json::field_as_value client_metadata{ U("client_metadata") };
            // see https://tools.ietf.org/html/rfc7591#section-2
            // see https://tools.ietf.org/html/rfc7591#section-3.1
            // see https://tools.ietf.org/html/rfc7591#section-3.2
            const web::json::field_as_array redirect_uris{ U("redirect_uris") };
            //const web::json::field_as_string token_endpoint_auth_method{ U("token_endpoint_auth_method") }; // OPTIONAL already defined in settings
            const web::json::field_as_array grant_types{ U("grant_types") }; // OPTIONAL
            const web::json::field_as_array response_types{ U("response_types") }; // OPTIONAL
            const web::json::field_as_string client_name{ U("client_name") }; // OPTIONAL
            const web::json::field_as_string client_uri{ U("client_uri") }; // OPTIONAL
            const web::json::field_as_string logo_uri{ U("logo_uri") }; // OPTIONAL
            const web::json::field_as_string scope{ U("scope") }; // OPTIONAL
            const web::json::field_as_array contacts{ U("contacts") }; // OPTIONAL
            const web::json::field_as_string tos_uri{ U("tos_uri") }; // OPTIONAL
            const web::json::field_as_string policy_uri{ U("policy_uri") }; // OPTIONAL
            const web::json::field_as_value jwks{ U("jwks") }; // OPTIONAL
            const web::json::field_as_array keys{ U("keys") }; // use inside jwks
            const web::json::field_as_string software_id{ U("software_id") }; // OPTIONAL
            const web::json::field_as_string software_version{ U("software_version") }; // OPTIONAL
            const web::json::field_as_string_or client_id{ U("client_id"), {} };
            const web::json::field_as_string client_secret{ U("client_secret") }; // OPTIONAL
            const web::json::field_as_integer client_id_issued_at{ U("client_id_issued_at") }; // OPTIONAL
            const web::json::field_as_integer_or client_secret_expires_at{ U("client_secret_expires_at"),0 };
            const web::json::field_as_string azp{ U("azp") }; // OPTIONAL
            // OpenID Connect extension
            const web::json::field_as_string registration_client_uri{ U("registration_client_uri") }; // OPTIONAL
            const web::json::field_as_string registration_access_token{ U("registration_access_token") }; // OPTIONAL

            // use for Authorization Server Metadata & Client Metadata
            const web::json::field_as_string_or jwks_uri{ U("jwks_uri"),{} };
        }
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

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

        // (mostly) for registration_api
        const web::json::field_as_string type{ U("type") };
        const web::json::field_as_value data{ U("data") };
        const web::json::field_as_string node_id{ U("node_id") };
        const web::json::field_as_string device_id{ U("device_id") };
        const web::json::field_as_string source_id{ U("source_id") };
        const web::json::field_as_string flow_id{ U("flow_id") };
        const web::json::field_as_value subscription{ U("subscription") };
        const web::json::field_as_value sender_id{ U("sender_id") };
        const web::json::field_as_value receiver_id{ U("receiver_id") };
        const web::json::field_as_array senders{ U("senders") };
        const web::json::field_as_array receivers{ U("receivers") };
        const web::json::field_as_array parents{ U("parents") };
        // (mostly) for query_api
        const web::json::field_as_bool persist{ U("persist") };
        const web::json::field_as_bool secure{ U("secure") };
        const web::json::field_as_string resource_path{ U("resource_path") };
        const web::json::field_as_object params{ U("params") };
        const web::json::field_as_integer max_update_rate_ms{ U("max_update_rate_ms") };
        const web::json::field_as_string ws_href{ U("ws_href") };
        const web::json::field_as_string subscription_id{ U("subscription_id") };
        // (mostly) for query_ws_api
        const web::json::field<tai> creation_timestamp{ U("creation_timestamp") };
        const web::json::field<tai> origin_timestamp{ U("origin_timestamp") };
        const web::json::field<tai> sync_timestamp{ U("sync_timestamp") };
        const web::json::field_as_string topic{ U("topic") };

        // IS-05 Connection Management

        // for connection_api
        const web::json::field_as_bool master_enable{ U("master_enable") };
        const web::json::field_as_value activation{ U("activation") };
        const web::json::field_as_string mode{ U("mode") };
        const web::json::field<tai> requested_time{ U("requested_time") };
        const web::json::field_as_array transport_params{ U("transport_params") };
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

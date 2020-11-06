#ifndef NMOS_CAPABILITIES_H
#define NMOS_CAPABILITIES_H

#include "cpprest/json_utils.h"
#include "nmos/rational.h"

namespace nmos
{
    // BCP-xxx-01 Receiver Capabilities
    // See https://github.com/AMWA-TV/nmos-receiver-capabilities/blob/v1.0-dev/docs/1.0.%20Receiver%20Capabilities.md
    namespace fields
    {
        const web::json::field_as_value_or constraint_sets{ U("constraint_sets"), web::json::value::array() };
    }

    // See https://github.com/AMWA-TV/nmos-receiver-capabilities/blob/v1.0-dev/docs/1.0.%20Receiver%20Capabilities.md#string-constraint-keywords
    web::json::value make_caps_string_constraint(const std::vector<utility::string_t>& enum_values = {});

    // See https://github.com/AMWA-TV/nmos-receiver-capabilities/blob/v1.0-dev/docs/1.0.%20Receiver%20Capabilities.md#integer-and-number-constraint-keywords
    web::json::value make_caps_integer_constraint(const std::vector<int64_t>& enum_values = {}, int64_t minimum = (std::numeric_limits<int64_t>::max)(), int64_t maximum = (std::numeric_limits<int64_t>::min)());

    // See https://github.com/AMWA-TV/nmos-receiver-capabilities/blob/v1.0-dev/docs/1.0.%20Receiver%20Capabilities.md#integer-and-number-constraint-keywords
    web::json::value make_caps_number_constraint(const std::vector<double>& enum_values = {}, double minimum = (std::numeric_limits<double>::max)(), double maximum = (std::numeric_limits<double>::min)());

    // See https://github.com/AMWA-TV/nmos-receiver-capabilities/blob/v1.0-dev/docs/1.0.%20Receiver%20Capabilities.md#boolean-constraint-keywords
    web::json::value make_caps_boolean_constraint(const std::vector<bool>& enum_values = {});

    // See https://github.com/AMWA-TV/nmos-receiver-capabilities/blob/v1.0-dev/docs/1.0.%20Receiver%20Capabilities.md#rational-constraint-keywords
    web::json::value make_caps_rational_constraint(const std::vector<nmos::rational>& enum_values = {}, const nmos::rational& minimum = (std::numeric_limits<int64_t>::max)(), const nmos::rational& maximum = 0);

    // NMOS Parameter Registers - Capabilities register
    // See https://github.com/AMWA-TV/nmos-parameter-registers/blob/capabilities/capabilities/README.md
    namespace caps
    {
        namespace meta
        {
            const web::json::field_as_string_or label{ U("urn:x-nmos:cap:meta:label"), U("") };
            const web::json::field_as_integer_or preference{ U("urn:x-nmos:cap:meta:preference"), 0 };
            const web::json::field_as_bool_or enabled{ U("urn:x-nmos:cap:meta:enabled"), true };
        }
        namespace format
        {
            const web::json::field_as_value_or media_type{ U("urn:x-nmos:cap:format:media_type"), {} }; // string
            const web::json::field_as_value_or grain_rate{ U("urn:x-nmos:cap:format:grain_rate"), {} }; // rational
            const web::json::field_as_value_or frame_width{ U("urn:x-nmos:cap:format:frame_width"), {} }; // integer
            const web::json::field_as_value_or frame_height{ U("urn:x-nmos:cap:format:frame_height"), {} }; // integer
            const web::json::field_as_value_or interlace_mode{ U("urn:x-nmos:cap:format:interlace_mode"), {} }; // string
            const web::json::field_as_value_or colorspace{ U("urn:x-nmos:cap:format:colorspace"), {} }; // string
            const web::json::field_as_value_or transfer_characteristic{ U("urn:x-nmos:cap:format:transfer_characteristic"), {} }; // string
            const web::json::field_as_value_or color_sampling{ U("urn:x-nmos:cap:format:color_sampling"), {} }; // string
            const web::json::field_as_value_or component_depth{ U("urn:x-nmos:cap:format:component_depth"), {} }; // integer
            const web::json::field_as_value_or channel_count{ U("urn:x-nmos:cap:format:channel_count"), {} }; // integer
            const web::json::field_as_value_or sample_rate{ U("urn:x-nmos:cap:format:sample_rate"), {} }; // rational
            const web::json::field_as_value_or sample_depth{ U("urn:x-nmos:cap:format:sample_depth"), {} }; // integer
            const web::json::field_as_value_or event_type{ U("urn:x-nmos:cap:format:event_type"), {} }; // string
        }
        namespace transport
        {
            const web::json::field_as_value_or packet_time{ U("urn:x-nmos:cap:transport:packet_time"), {} }; // number
            const web::json::field_as_value_or max_packet_time{ U("urn:x-nmos:cap:transport:max_packet_time"), {} }; // number
        }
    }
}

#endif

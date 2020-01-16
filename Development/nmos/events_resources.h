#ifndef NMOS_EVENTS_RESOURCES_H
#define NMOS_EVENTS_RESOURCES_H

#include "cpprest/json.h"
#include "nmos/event_type.h"
#include "nmos/id.h"
#include "nmos/tai.h"

namespace nmos
{
    struct resource;

    // IS-07 Events API resources
    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/6.0.%20Event%20and%20tally%20rest%20api.md#1-introduction
    // Each IS-07 source's data is a json object with an "id" field
    // and a field for the Events API endpoints of that logical single resource
    // i.e.
    // a "type" field, which must have a value conforming to the type schema,
    // and a "state" field, which must have a value conforming to the event schema
    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type.json
    // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event.json
    // For WebSocket connections, subscription and grain resources will also be added

    nmos::resource make_events_source(const nmos::id& id, const web::json::value& state, const web::json::value& type);

    // Events API source state
    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/6.0.%20Event%20and%20tally%20rest%20api.md#3-usage

    namespace details
    {
        // just allows source_id to be passed to make_events_state, etc. alone, or with a flow_id
        struct events_state_identity
        {
            events_state_identity(nmos::id source_id, nmos::id flow_id = {})
                : source_id(std::move(source_id))
                , flow_id(std::move(flow_id))
            {}
            nmos::id source_id;
            nmos::id flow_id;
        };

        // just allows creation_timestamp to be passed to make_events_state, etc. alone or with origin_timestamp and action_timestamp
        struct events_state_timing
        {
            events_state_timing(nmos::tai creation_timestamp = tai_now(), nmos::tai origin_timestamp = {}, nmos::tai action_timestamp = {})
                : creation_timestamp(std::move(creation_timestamp))
                , origin_timestamp(std::move(origin_timestamp))
                , action_timestamp(std::move(action_timestamp))
            {}
            nmos::tai creation_timestamp;
            nmos::tai origin_timestamp;
            nmos::tai action_timestamp;
        };

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_core.json
        // "The flow_id will NOT be included in the response to a [REST API query for the] state because the state is held by
        // the source which has no dependency on a flow. It will, however, appear when being sent through one of the two
        // specified transports because it will pass from the source through a flow and out on the network through the sender."
        // Therefore, since the stored data in the event resources is also used to generate the messages on the transport, it
        // *should* include the flow id. It will be removed to generate the Events API /state response.
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/2.0.%20Message%20types.md#11-the-state-message-type
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/4.0.%20Core%20models.md#1-introduction
        web::json::value make_events_state_identity(const nmos::details::events_state_identity& identity);

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_core.json
        web::json::value make_events_state_timing(const nmos::details::events_state_timing& timing);
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/number.json
    struct events_number
    {
        events_number() : value(), scale() {}
        events_number(double value) : value(value), scale() {}
        events_number(double value, int64_t scale) : value(value), scale(scale) {}

        bool is_scaled() const { return 0 != scale; }
        double scaled_value() const { return is_scaled() ? value / (double)scale : value; }

        // hmm, web::json::number rather than double?
        double value;
        int64_t scale;
    };

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_boolean.json
    web::json::value make_events_boolean_state(const nmos::details::events_state_identity& identity, bool payload_value, const nmos::event_type& type = nmos::event_types::boolean, const nmos::details::events_state_timing& timing = {});
    inline web::json::value make_events_boolean_state(const nmos::details::events_state_identity& identity, bool payload_value, const nmos::details::events_state_timing& timing)
    {
        return make_events_boolean_state(identity, payload_value, nmos::event_types::boolean, timing);
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_number.json
    web::json::value make_events_number_state(const nmos::details::events_state_identity& identity, const events_number& payload, const nmos::event_type& type = nmos::event_types::number, const nmos::details::events_state_timing& timing = {});
    inline web::json::value make_events_number_state(const nmos::details::events_state_identity& identity, const events_number& payload, const nmos::details::events_state_timing& timing)
    {
        return make_events_number_state(identity, payload, nmos::event_types::number, timing);
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_string.json
    web::json::value make_events_string_state(const nmos::details::events_state_identity& identity, const utility::string_t& payload_value, const nmos::event_type& type = nmos::event_types::string, const nmos::details::events_state_timing& timing = {});
    inline web::json::value make_events_string_state(const nmos::details::events_state_identity& identity, const utility::string_t& payload_value, const nmos::details::events_state_timing& timing)
    {
        return make_events_string_state(identity, payload_value, nmos::event_types::string, timing);
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_object.json
    // (out of scope for version 1.0 of this specification)
    web::json::value make_events_object_state(const nmos::details::events_state_identity& identity, const web::json::value& payload, const nmos::event_type& type = nmos::event_types::object, const nmos::details::events_state_timing& timing = {});
    inline web::json::value make_events_object_state(const nmos::details::events_state_identity& identity, const web::json::value& payload, const nmos::details::events_state_timing& timing)
    {
        return make_events_object_state(identity, payload, nmos::event_types::object, timing);
    }

    // Events API source type
    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/6.0.%20Event%20and%20tally%20rest%20api.md#3-usage

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type_boolean.json
    web::json::value make_events_boolean_type();

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type_number.json
    web::json::value make_events_number_type(const events_number& min, const events_number& max, const events_number& step = {}, const utility::string_t& unit = {}, int64_t scale = {});

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type_string.json
    web::json::value make_events_string_type(int64_t min_length = {}, int64_t max_length = {}, const utility::string_t& pattern = {});

    struct events_enum_element_details
    {
        events_enum_element_details(utility::string_t label, utility::string_t description)
            : label(std::move(label))
            , description(std::move(description))
        {}
        utility::string_t label;
        utility::string_t description;
    };

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type_boolean_enum.json
    // hmm, map or vector-of-pair?
    web::json::value make_events_boolean_enum_type(const std::vector<std::pair<bool, events_enum_element_details>>& values);

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type_number_enum.json
    web::json::value make_events_number_enum_type(const std::vector<std::pair<double, events_enum_element_details>>& values);

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type_string_enum.json
    web::json::value make_events_string_enum_type(const std::vector<std::pair<utility::string_t, events_enum_element_details>>& values);

    // (out of scope for version 1.0 of this specification)
    web::json::value make_events_object_type();

    // Events commands
    // These are not resources, so maybe these belong in their own file, e.g. nmos/events_commands.h?
    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.1/docs/5.2.%20Transport%20-%20Websocket.md#4-subscriptions-strategy

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.1/APIs/schemas/command_subscription.json
    web::json::value make_events_subscription_command(const std::vector<nmos::id>& sources);

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/command_health.json
    web::json::value make_events_health_command(const nmos::tai& timestamp = tai_now());
}

#endif

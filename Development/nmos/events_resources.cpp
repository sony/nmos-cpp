#include "nmos/events_resources.h"

#include "nmos/resource.h"
#include "nmos/is07_versions.h"

namespace nmos
{
    nmos::resource make_events_source(const nmos::id& id, const web::json::value& state, const web::json::value& type)
    {
        using web::json::value_of;

        auto data = value_of({
            { nmos::fields::id, id },
            { nmos::fields::device_id, U("nobody expects the spanish inquisition") },
            { nmos::fields::endpoint_state, state },
            { nmos::fields::endpoint_type, type }
        });

        return{ is07_versions::v1_0, types::source, data, true };
    }

    namespace details
    {
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_core.json
        // "The flow_id will NOT be included in the response to a [REST API query for the] state because the state is held by
        // the source which has no dependency on a flow. It will, however, appear when being sent through one of the two
        // specified transports because it will pass from the source through a flow and out on the network through the sender."
        // Therefore, since the stored data in the event resources is also used to generate the messages on the transport, it
        // *should* include the flow id. It will be removed to generate the Events API /state response.
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/2.0.%20Message%20types.md#11-the-state-message-type
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/4.0.%20Core%20models.md#1-introduction
        web::json::value make_events_state_identity(const nmos::details::events_state_identity& identity)
        {
            using web::json::value_of;

            return value_of({
                { U("source_id"), identity.source_id },
                { !identity.flow_id.empty() ? U("flow_id") : U(""), identity.flow_id }
            }, true);
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_core.json
        web::json::value make_events_state_timing(const nmos::details::events_state_timing& timing)
        {
            using web::json::value_of;

            return value_of({
                { U("creation_timestamp"), nmos::make_version(timing.creation_timestamp) },
                { nmos::tai{} != timing.origin_timestamp ? U("origin_timestamp") : U(""), nmos::make_version(timing.origin_timestamp) },
                { nmos::tai{} != timing.action_timestamp ? U("action_timestamp") : U(""), nmos::make_version(timing.action_timestamp) }
            }, true);
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_core.json
        web::json::value make_events_state(const nmos::details::events_state_identity& identity, web::json::value payload, const nmos::event_type& type, const nmos::details::events_state_timing& timing)
        {
            using web::json::value_of;

            return value_of({
                { U("identity"), make_events_state_identity(identity) },
                { U("event_type"), type.name },
                { U("timing"), make_events_state_timing(timing) },
                { U("payload"), std::move(payload) },
                { U("message_type"), U("state") }
            });
        }

        web::json::value make_payload(bool payload_value)
        {
            using web::json::value_of;

            return value_of({ { U("value"), payload_value } });
        }

        web::json::value make_payload(const events_number& payload)
        {
            using web::json::value_of;

            return value_of({
                { U("value"), payload.value },
                { payload.is_scaled() ? U("scale") : U(""), payload.scale }
            });
        }

        web::json::value make_payload(const utility::string_t& payload_value)
        {
            using web::json::value_of;

            return value_of({ { U("value"), payload_value } });
        }
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_boolean.json
    web::json::value make_events_boolean_state(const nmos::details::events_state_identity& identity, bool payload_value, const nmos::event_type& type, const nmos::details::events_state_timing& timing)
    {
        // should check type is nmos::event_types::boolean or a derived type
        return details::make_events_state(identity, details::make_payload(payload_value), type, timing);
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_number.json
    web::json::value make_events_number_state(const nmos::details::events_state_identity& identity, const events_number& payload, const nmos::event_type& type, const nmos::details::events_state_timing& timing)
    {
        // should check type is nmos::event_types::number or a derived type
        return details::make_events_state(identity, details::make_payload(payload), type, timing);
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_string.json
    web::json::value make_events_string_state(const nmos::details::events_state_identity& identity, const utility::string_t& payload_value, const nmos::event_type& type, const nmos::details::events_state_timing& timing)
    {
        // should check type is nmos::event_types::string or a derived type
        return details::make_events_state(identity, details::make_payload(payload_value), type, timing);
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/event_object.json
    // (out of scope for version 1.0 of this specification)
    web::json::value make_events_object_state(const nmos::details::events_state_identity& identity, const web::json::value& payload, const nmos::event_type& type, const nmos::details::events_state_timing& timing)
    {
        // should check type is nmos::event_types::object or a derived type
        return details::make_events_state(identity, payload, type, timing);
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type_boolean.json
    web::json::value make_events_boolean_type()
    {
        using web::json::value_of;

        return value_of({ { U("type"), nmos::event_types::boolean.name } });
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type_number.json
    web::json::value make_events_number_type(const events_number& min, const events_number& max, const events_number& step, const utility::string_t& unit, int64_t scale)
    {
        using web::json::value_of;

        return value_of({
            { U("type"), nmos::event_types::number.name },
            { 0 != scale ? U("scale") : U(""), scale },
            { U("min"), details::make_payload(min) },
            { U("max"), details::make_payload(max) },
            { 0 != step.value ? U("step") : U(""), details::make_payload(step) },
            { !unit.empty() ? U("unit") : U(""), unit }
        });
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/type_string.json
    web::json::value make_events_string_type(int64_t min_length, int64_t max_length, const utility::string_t& pattern)
    {
        using web::json::value_of;

        // hmm, explicit "min_length": 0 is actually valid, but omitting it has the same effect
        return value_of({
            { U("type"), nmos::event_types::string.name },
            { 0 != min_length ? U("min_length") : U(""), min_length },
            { 0 != max_length ? U("max_length") : U(""), max_length },
            { !pattern.empty() ? U("pattern") : U(""), pattern }
        });
    }

    // (out of scope for version 1.0 of this specification)
    web::json::value make_events_object_type()
    {
        using web::json::value_of;

        return value_of({ { U("type"), nmos::event_types::object.name } });
    }
}

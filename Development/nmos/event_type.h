#ifndef NMOS_EVENT_TYPE_H
#define NMOS_EVENT_TYPE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // IS-07 Event & Tally event types
    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md
    DEFINE_STRING_ENUM(event_type)
    namespace event_types
    {
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#21-boolean
        const event_type boolean{ U("boolean") };
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#22-string
        const event_type string{ U("string") };
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#23-number
        const event_type number{ U("number") };
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#4-object
        // "The usage of the object event type is out of scope of this specification for version 1.0"
        const event_type object{ U("object") };

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#231-measurements
        inline const event_type measurement(const event_type& base_type, const utility::string_t& name, const utility::string_t& unit = {})
        {
            // specific measurement types are always "{baseType}/{Name}/{Unit}"
            // partial types use the wildcard "*" and may be "{baseType}/{Name}/*" or "{baseType}/*" (but not e.g. "{baseType}/*/{Unit}")
            return !unit.empty()
                ? event_type{ base_type.name + U('/') + name + U('/') + unit }
                : event_type{ base_type.name + U('/') + name };
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#3-enum
        inline const event_type named_enum(const event_type& base_type, const utility::string_t& name)
        {
            return event_type{ base_type.name + U("/enum/") + name };
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#event-types-capability-management
        // "A wildcard (*) must replace a whole word and can only be used at the end of an event_type definition."
        const utility::string_t wildcard{ U("*") };
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.x/docs/3.0.%20Event%20types.md#event-types-capability-management
    inline bool is_matching_event_type(const event_type& capability, const event_type& type)
    {
        // "Comparisons between event_type values must be case sensitive."
        auto& c = capability.name;
        auto& t = type.name;
        if (!c.empty() && U('*') == c.back())
            return c.size() <= t.size() && std::equal(c.begin(), c.end() - 1, t.begin() + c.size() - 1);
        else
            return c == t;
    }
}

#endif

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
        inline const event_type measurement(const utility::string_t& name, const utility::string_t& unit)
        {
            // specific measurement types are always "number/{Name}/{Unit}"
            // names and units should not be empty or contain the '/' character
            return event_type{ number.name + U('/') + name + U('/') + unit };
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#3-enum
        inline const event_type named_enum(const event_type& base_type, const utility::string_t& name)
        {
            return event_type{ base_type.name + U("/enum/") + name };
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#event-types-capability-management
        // "A wildcard (*) must replace a whole word and can only be used at the end of an event_type definition."
        struct wildcard_type
        {
            const event_type operator()(const event_type& base_type) const
            {
                return event_type{ base_type.name + U("/*") };
            }
        };
        const wildcard_type wildcard;

        // "any measurement unit" partial event type
        inline const event_type measurement(const utility::string_t& name, const wildcard_type&)
        {
            return wildcard(event_type{ number.name + U('/') + name });
        }

        // "any named enumeration" partial event type
        inline const event_type named_enum(const event_type& base_type, const wildcard_type&)
        {
            return wildcard(event_type{ base_type.name + U("/enum") });
        }

        // deprecated, provided for backwards compatibility, use measurement(name, unit) since a measurement must be a number
        inline const event_type measurement(const event_type& base_type, const utility::string_t& name, const utility::string_t& unit)
        {
            return event_type{ base_type.name + U('/') + name + U('/') + unit };
        }

        // deprecated, provided for backwards compatibility, use measurement(name, wildcard) since a measurement must be a number
        inline const event_type measurement(const event_type& base_type, const utility::string_t& name, const wildcard_type&)
        {
            return wildcard(event_type{ base_type.name + U('/') + name });
        }

        // deprecated, provided for backwards compatibility, use wildcard(base_type) since these wildcards do not only match measurements
        inline const event_type measurement(const event_type& base_type, const wildcard_type&)
        {
            return wildcard(base_type);
        }
    }

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/3.0.%20Event%20types.md#event-types-capability-management
    inline bool is_matching_event_type(const event_type& capability, const event_type& type)
    {
        // "Comparisons between event_type values must be case sensitive."
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.1/docs/3.0.%20Event%20types.md#1-introduction
        auto& c = capability.name;
        auto& t = type.name;
        // The wildcard in a partial event type matches zero or more 'levels', e.g. "number/*" matches both "number" and "number/temperature/C".
        // A wildcard cannot be used at the top 'level', i.e. "*" is not a partial event type that matches any base type.
        if (2 < c.size() && c[c.size() - 2] == U('/') && c[c.size() - 1] == U('*'))
            return c.size() - 2 <= t.size() && std::equal(c.begin(), c.end() - 2, t.begin())
            && (c.size() - 2 == t.size() || t[c.size() - 2] == U('/'));
        else
            return c == t;
    }
}

#endif

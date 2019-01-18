#ifndef NMOS_ID_H
#define NMOS_ID_H

#include <memory>
#include "cpprest/details/basic_types.h"

namespace nmos
{
    // "Each logical entity is identified by a UUID"
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/5.1.%20Data%20Model%20-%20Identifier%20Mapping.md

    // Since identifiers are passed as strings in the APIs, and the formatting of identifiers has been a little
    // inconsistent between implementations in the past, they are currently stored simply as strings...
    typedef utility::string_t id;

    // a random number-based UUID (v4) generator
    // non-copyable, not thread-safe
    class id_generator
    {
    public:
        id_generator();
        ~id_generator();
        id operator()();

    private:
        struct impl_t;
        std::unique_ptr<impl_t> impl;
    };

    // generate a random number-based UUID (v4)
    // note, when creating multiple UUIDs, using an id_generator can be more efficient depending on platform and dependencies
    id make_id();

    // generate a name-based UUID (v5)
    id make_repeatable_id(id namespace_id, const utility::string_t& name);
}

#endif

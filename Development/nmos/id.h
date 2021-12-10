#ifndef NMOS_ID_H
#define NMOS_ID_H

#include <memory>
#include "cpprest/details/basic_types.h"

namespace nmos
{
    // "Each logical entity is identified by a UUID"
    // See https://specs.amwa.tv/is-04/releases/v1.2.0/docs/5.1._Data_Model_-_Identifier_Mapping.html

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

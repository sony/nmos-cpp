#ifndef NMOS_ID_H
#define NMOS_ID_H

#include "cpprest/details/basic_types.h"

namespace nmos
{
    // "Each logical entity is identified by a UUID"
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/5.1.%20Data%20Model%20-%20Identifier%20Mapping.md

    // Since identifiers are passed as strings in the APIs, and the formatting of identifiers has been a little
    // inconsistent between implementations in the past, they are currently stored simply as strings...
    typedef utility::string_t id;

    // generate a random number-based UUID (v4)
    id make_id();

    // generate a name-based UUID (v5)
    id make_repeatable_id(id namespace_id, const utility::string_t& name);
}

#endif

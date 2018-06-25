#ifndef NMOS_ID_H
#define NMOS_ID_H

#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "cpprest/basic_utils.h"

namespace nmos
{
    // "Each logical entity is identified by a UUID"
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/5.1.%20Data%20Model%20-%20Identifier%20Mapping.md

    // Since identifiers are passed as strings in the APIs, and the formatting of identifiers has been a little
    // inconsistent between implementations in the past, they are currently stored simply as strings...
    typedef utility::string_t id;

    // generate a random number-based UUID (v4)
    inline id make_id()
    {
        return utility::s2us(boost::uuids::to_string(boost::uuids::random_generator()()));
    }

    // generate a name-based UUID (v5)
    inline id make_repeatable_id(id namespace_id, const utility::string_t& name)
    {
        return utility::s2us(boost::uuids::to_string(boost::uuids::name_generator(boost::uuids::string_generator()(namespace_id))(utility::us2s(name))));
    }
}

#endif

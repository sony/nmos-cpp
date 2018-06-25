#include "nmos/id.h"

#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "cpprest/basic_utils.h"

namespace nmos
{
    // generate a random number-based UUID (v4)
    id make_id()
    {
        return utility::s2us(boost::uuids::to_string(boost::uuids::random_generator()()));
    }

    // generate a name-based UUID (v5)
    id make_repeatable_id(id namespace_id, const utility::string_t& name)
    {
        return utility::s2us(boost::uuids::to_string(boost::uuids::name_generator(boost::uuids::string_generator()(namespace_id))(utility::us2s(name))));
    }
}

#include "nmos/id.h"

#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "cpprest/basic_utils.h"

namespace nmos
{
    struct id_generator::impl_t
    {
        boost::uuids::random_generator gen;
    };

    id_generator::id_generator()
        : impl(new impl_t)
    {
    }

    id_generator::~id_generator()
    {
        // explicitly defined so that impl_t is a complete type for the unique_ptr destructor
    }

    id id_generator::operator()()
    {
        return utility::s2us(boost::uuids::to_string(impl->gen()));
    };

    // generate a random number-based UUID (v4)
    // note, when creating multiple UUIDs, using a generator can be more efficient depending on platform and dependencies
    id make_id()
    {
        return id_generator()();
    }

    // generate a name-based UUID (v5)
    id make_repeatable_id(id namespace_id, const utility::string_t& name)
    {
        return utility::s2us(boost::uuids::to_string(boost::uuids::name_generator(boost::uuids::string_generator()(namespace_id))(utility::us2s(name))));
    }
}

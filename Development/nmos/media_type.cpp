#include "nmos/media_type.h"

#include <boost/algorithm/string/predicate.hpp>

namespace nmos
{
    bool equals_media_type(const media_type& lhs, const media_type& rhs)
    {
        return boost::algorithm::iequals(lhs.name, rhs.name);
    }
}

#include "nmos/group_hint.h"

#include <boost/algorithm/string/split.hpp>

namespace nmos
{
    utility::string_t make_group_hint(const group_hint& group_hint)
    {
        return group_hint.optional_group_scope.name.empty()
            ? group_hint.group_name + U(':') + group_hint.role_in_group
            : group_hint.group_name + U(':') + group_hint.role_in_group + U(':') + group_hint.optional_group_scope.name;
    }

    group_hint parse_group_hint(const utility::string_t& group_hint)
    {
        std::vector<utility::string_t> split;
        boost::algorithm::split(split, group_hint, [](utility::char_t c) { return U(':') == c; });
        split.resize(3);
        return{ split[0], split[1], group_scope{ split[2] } };
    }
}

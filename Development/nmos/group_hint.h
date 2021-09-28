#ifndef NMOS_GROUP_HINT_H
#define NMOS_GROUP_HINT_H

#include "cpprest/json_utils.h"
#include "nmos/string_enum.h"

// Group Hint
// See https://github.com/AMWA-TV/nmos-parameter-registers/blob/main/tags/grouphint.md
namespace nmos
{
    namespace fields
    {
        const web::json::field_as_value_or group_hint{ U("urn:x-nmos:tag:grouphint/v1.0"), web::json::value::array() };
    }

    DEFINE_STRING_ENUM(group_scope)
    namespace group_scopes
    {
        const group_scope device{ U("device") };
        const group_scope node{ U("node") };
    }

    struct group_hint
    {
        utility::string_t group_name;
        utility::string_t role_in_group;
        group_scope optional_group_scope;

        group_hint(const utility::string_t& group_name, const utility::string_t& role_in_group, const group_scope& optional_group_scope = {})
            : group_name(group_name)
            , role_in_group(role_in_group)
            , optional_group_scope(optional_group_scope)
        {}
    };

    utility::string_t make_group_hint(const group_hint& group_hint);

    group_hint parse_group_hint(const utility::string_t& group_hint);
}

#endif

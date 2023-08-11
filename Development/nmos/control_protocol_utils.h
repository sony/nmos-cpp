#ifndef NMOS_CONTROL_PROTOCOL_UTILS_H
#define NMOS_CONTROL_PROTOCOL_UTILS_H

#include "cpprest/basic_utils.h"
#include "nmos/control_protocol_resource.h" // for nc_class_id definition
#include "nmos/resources.h"

namespace nmos
{
    void get_member_descriptors(const resources& resources, resources::iterator resource, bool recurse, web::json::array& descriptors);

    void find_members_by_role(const resources& resources, resources::iterator resource, const utility::string_t& role, bool match_whole_string, bool case_sensitive, bool recurse, web::json::array& nc_block_member_descriptors);

    void find_members_by_class_id(const resources& resources, resources::iterator resource, const details::nc_class_id& class_id, bool include_derived, bool recurse, web::json::array& descriptors);
}

#endif

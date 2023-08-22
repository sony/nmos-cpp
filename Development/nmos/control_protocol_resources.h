#ifndef NMOS_CONTROL_PROTOCOL_RESOURCES_H
#define NMOS_CONTROL_PROTOCOL_RESOURCES_H

#include <map>
#include "nmos/control_protocol_resource.h" // for details::nc_oid definition
#include "nmos/settings.h"

namespace nmos
{
    namespace experimental
    {
        struct control_protocol_state;
    }

    struct resource;

    // create device manager resource
    nmos::resource make_device_manager(details::nc_oid oid, nmos::resource& root_block, const nmos::settings& settings);

    // create class manager resource
    nmos::resource make_class_manager(details::nc_oid oid, nmos::resource& root_block, const nmos::experimental::control_protocol_state& control_protocol_state);

    // create block resource
    nmos::resource make_block(details::nc_oid oid, const web::json::value& owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints = web::json::value::null(), const web::json::value& runtime_property_constraints = web::json::value::null(), const web::json::value& members = web::json::value::array());
    nmos::resource make_block(details::nc_oid oid, details::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints = web::json::value::null(), const web::json::value& runtime_property_constraints = web::json::value::null(), const web::json::value& members = web::json::value::array());

    // help function to add member to block
    bool add_member_to_block(const utility::string_t& description, const web::json::value& nc_block, web::json::value& parent);

    // create Root block resource
    nmos::resource make_root_block();
}

#endif

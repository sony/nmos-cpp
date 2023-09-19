#ifndef NMOS_CONTROL_PROTOCOL_RESOURCES_H
#define NMOS_CONTROL_PROTOCOL_RESOURCES_H

#include "nmos/control_protocol_typedefs.h" // for details::nc_oid definition
#include "nmos/settings.h"

namespace nmos
{
    namespace experimental
    {
        struct control_protocol_state;
    }

    struct resource;

    // create block resource
    resource make_block(nc_oid oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints = web::json::value::null(), const web::json::value& runtime_property_constraints = web::json::value::null(), const web::json::value& members = web::json::value::array());

    // create Root block resource
    resource make_root_block();

    // create Device manager resource
    resource make_device_manager(nc_oid oid, const nmos::settings& settings);

    // create Class manager resource
    resource make_class_manager(nc_oid oid, const nmos::experimental::control_protocol_state& control_protocol_state);
}

#endif

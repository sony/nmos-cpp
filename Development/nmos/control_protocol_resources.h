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

    struct control_protocol_resource;

    // create block resource
    control_protocol_resource make_block(nc_oid oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints = web::json::value::null(), const web::json::value& runtime_property_constraints = web::json::value::null(), const web::json::value& members = web::json::value::array());

    // create Root block resource
    control_protocol_resource make_root_block();

    // create Device manager resource
    control_protocol_resource make_device_manager(nc_oid oid, const nmos::settings& settings);

    // create Class manager resource
    control_protocol_resource make_class_manager(nc_oid oid, const nmos::experimental::control_protocol_state& control_protocol_state);

    // create Receiver Monitor resource
    control_protocol_resource make_receiver_monitor(nc_oid oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints = web::json::value::null(), const web::json::value& runtime_property_constraints = web::json::value::null(),
        nc_connection_status::status connection_status = nc_connection_status::status::undefined,
        const utility::string_t& connection_status_message = U(""),
        nc_payload_status::status payload_status = nc_payload_status::status::undefined,
        const utility::string_t& payload_status_message = U("")
    );
}

#endif

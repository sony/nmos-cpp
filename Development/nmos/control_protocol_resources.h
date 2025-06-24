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

    // Monitoring feature set control classes
    //
    // create Receiver Monitor resource
    control_protocol_resource make_receiver_monitor(nc_oid oid, bool constant_oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints = web::json::value::null(), const web::json::value& runtime_property_constraints = web::json::value::null(), bool enabled = true,
        nc_overall_status::status overall_status = nc_overall_status::status::inactive,
        const utility::string_t& overall_status_message = U(""),
        nc_link_status::status link_status = nc_link_status::all_up,
        const utility::string_t& link_status_message = U(""),
        nc_connection_status::status connection_status = nc_connection_status::status::inactive,
        const utility::string_t& connection_status_message = U(""),
        nc_synchronization_status::status synchronization_status = nc_synchronization_status::not_used,
        const utility::string_t& synchronization_status_message = U(""),
        const web::json::value& synchronization_source_id = web::json::value::null(),
        nc_stream_status::status stream_status = nc_stream_status::inactive,
        const utility::string_t& stream_status_message = U(""),
        uint32_t status_reporting_delay = 3,
        bool auto_reset_monitor = true
    );

    // create Sender Monitor resource
    control_protocol_resource make_sender_monitor(nc_oid oid, bool constant_oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints = web::json::value::null(), const web::json::value& runtime_property_constraints = web::json::value::null(), bool enabled = true,
        nc_overall_status::status overall_status = nc_overall_status::status::inactive,
        const utility::string_t& overall_status_message = U(""),
        nc_link_status::status link_status = nc_link_status::all_up,
        const utility::string_t& link_status_message = U(""),
        nc_transmission_status::status transmission_status = nc_transmission_status::status::inactive,
        const utility::string_t& transmission_status_message = U(""),
        nc_synchronization_status::status synchronization_status = nc_synchronization_status::not_used,
        const utility::string_t& synchronization_status_message = U(""),
        const web::json::value& synchronization_source_id = web::json::value::null(),
        nc_essence_status::status stream_status = nc_essence_status::inactive,
        const utility::string_t& essence_status_message = U(""),
        uint32_t status_reporting_delay = 3,
        bool auto_reset_counters = true
    );

    // Identification feature set control classes
    //
    // create Ident Beacon resource
    control_protocol_resource make_ident_beacon(nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints = web::json::value::null(), const web::json::value& runtime_property_constraints = web::json::value::null(), bool enabled = true,
        bool active = false
    );
}

#endif

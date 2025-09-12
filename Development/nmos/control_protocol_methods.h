#ifndef NMOS_CONTROL_PROTOCOL_METHODS_H
#define NMOS_CONTROL_PROTOCOL_METHODS_H

#include "nmos/control_protocol_handlers.h"
#include "nmos/resources.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    namespace nc
    {
        // NcObject methods implementation
        // Get property value
        web::json::value get(const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set property value
        web::json::value set(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, control_protocol_property_changed_handler property_changed, slog::base_gate& gate);
        // Get sequence item
        web::json::value get_sequence_item(const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Set sequence item
        web::json::value set_sequence_item(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler, control_protocol_property_changed_handler property_changed, slog::base_gate& gate);
        // Add item to sequence
        web::json::value add_sequence_item(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler, control_protocol_property_changed_handler property_changed, slog::base_gate& gate);
        // Delete sequence item
        web::json::value remove_sequence_item(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, control_protocol_property_changed_handler property_changed, slog::base_gate& gate);
        // Get sequence length
        web::json::value get_sequence_length(const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

        // NcBlock methods implementation
        // Get descriptors of members of the block
        web::json::value get_member_descriptors(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate);
        // Finds member(s) by path
        web::json::value find_members_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate);
        // Finds members with given role name or fragment
        web::json::value find_members_by_role(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate);
        // Finds members with given class id
        web::json::value find_members_by_class_id(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate);

        // NcClassManager methods implementation
        // Get a single class descriptor
        web::json::value get_control_class(const web::json::value& arguments, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);
        // Get a single datatype descriptor
        web::json::value get_datatype(const web::json::value& arguments, bool is_deprecated, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype, slog::base_gate& gate);

        // NcReceiverMonitor methods implementation
    // Gets the lost packet counters
        web::json::value get_lost_packet_counters(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_packet_counters_handler get_lost_packet_counters, slog::base_gate& gate);
        // Gets the last packet counters
        web::json::value get_late_packet_counters(nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, get_packet_counters_handler get_last_packet_counters, slog::base_gate& gate);
        // Resets the packet counters and messages
        web::json::value reset_monitor(nmos::resources& resources, const nmos::resource& resource, const web::json::value&, bool is_deprecated, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, control_protocol_property_changed_handler property_changed, reset_monitor_handler reset_monitor, slog::base_gate& gate);
    }
}
#endif

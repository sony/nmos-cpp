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
    namespace details
    {
        // NcObject methods implementation
        // Get property value
        web::json::value get(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Set property value
        web::json::value set(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Get sequence item
        web::json::value get_sequence_item(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Set sequence item
        web::json::value set_sequence_item(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Add item to sequence
        web::json::value add_sequence_item(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Delete sequence item
        web::json::value remove_sequence_item(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Get sequence length
        web::json::value get_sequence_length(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler, slog::base_gate& gate);

        // NcBlock methods implementation
        // Get descriptors of members of the block
        web::json::value get_member_descriptors(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Finds member(s) by path
        web::json::value find_members_by_path(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Finds members with given role name or fragment
        web::json::value find_members_by_role(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Finds members with given class id
        web::json::value find_members_by_class_id(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler, get_control_protocol_datatype_handler, slog::base_gate& gate);

        // NcClassManager methods implementation
        // Get a single class descriptor
        web::json::value get_control_class(nmos::resources&, nmos::resources::iterator, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler get_control_protocol_class, get_control_protocol_datatype_handler, slog::base_gate& gate);
        // Get a single datatype descriptor
        web::json::value get_datatype(nmos::resources&, nmos::resources::iterator, int32_t handle, const web::json::value& arguments, get_control_protocol_class_handler, get_control_protocol_datatype_handler get_control_protocol_datatype, slog::base_gate& gate);
    }
}

#endif

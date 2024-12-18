#ifndef NMOS_CONFIGURATION_METHODS_H
#define NMOS_CONFIGURATION_METHODS_H

#include "nmos/configuration_handlers.h"
#include "nmos/control_protocol_handlers.h"
#include "nmos/resources.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct control_protocol_resource;

    // Implementation of IS-14 function for creating backup dataset from a Device Model
    web::json::value get_properties_by_path(const nmos::resources& resources, nmos::experimental::control_protocol_state& control_protocol_state, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, const nmos::resource& resource, bool recurse);

    // Check to see if role_path is sub path of parent_role_path
    bool is_role_path_root(const web::json::value& role_path_, const web::json::value& parent_role_path);

    bool is_block_modified(const nmos::resource& resource, const web::json::value& object_properties_holder);

    // Get role path of resource given the Device Model resources
    web::json::value get_role_path(const nmos::resources& resources, const nmos::resource& resource);

    web::json::value apply_backup_data_set(nmos::resources& resources, nmos::experimental::control_protocol_state& control_protocol_state, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const nmos::resource& resource, const web::json::array& object_properties_holders, bool recurse, const web::json::value& restore_mode, bool validate, nmos::modify_read_only_config_properties_handler modify_read_only_config_properties, nmos::modify_rebuildable_block_handler modify_rebuildable_block);

    web::json::value validate_set_properties_by_path(nmos::resources& resources, nmos::experimental::control_protocol_state& control_protocol_state, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::modify_read_only_config_properties_handler modify_read_only_config_properties, nmos::modify_rebuildable_block_handler modify_rebuildable_block);

    web::json::value set_properties_by_path(nmos::resources& resources, nmos::experimental::control_protocol_state& control_protocol_state, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::modify_read_only_config_properties_handler modify_read_only_config_properties, nmos::modify_rebuildable_block_handler modify_rebuildable_block);
}

#endif
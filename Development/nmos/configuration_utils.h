#ifndef NMOS_CONFIGURATION_UTILS_H
#define NMOS_CONFIGURATION_UTILS_H

#include "nmos/configuration_handlers.h"
#include "nmos/control_protocol_handlers.h"
#include "nmos/resources.h"

namespace nmos
{
    struct control_protocol_resource;

    // Check to see if role_path is sub path of parent_role_path
    bool is_role_path_root(const web::json::value& role_path_, const web::json::value& parent_role_path);

    bool is_block_modified(const nmos::resource& resource, const web::json::value& object_properties_holder);

    // Get role path of resource given the Device Model resources
    web::json::value get_role_path(const nmos::resources& resources, const nmos::resource& resource);

    web::json::value apply_backup_data_set(nmos::resources& resources, const nmos::resource& resource, const web::json::array& object_properties_holders, bool recurse, const web::json::value& restore_mode, bool validate, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_value_holders_handler filter_property_value_holders, nmos::modify_rebuildable_block_handler modify_rebuildable_block);
}

#endif
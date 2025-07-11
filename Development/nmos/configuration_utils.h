#ifndef NMOS_CONFIGURATION_UTILS_H
#define NMOS_CONFIGURATION_UTILS_H

#include "nmos/configuration_handlers.h"
#include "nmos/control_protocol_handlers.h"
#include "nmos/resources.h"

namespace nmos
{
    typedef std::map<web::json::array, web::json::value> object_properties_map;

    struct control_protocol_resource;

    namespace details
    {
        web::json::value modify_rebuildable_block(nmos::resources& resources, object_properties_map& object_properties_holder_map, const nmos::resource& resource, const web::json::array& target_role_path, const web::json::value& block_object_properties_holder, bool validate, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::remove_device_model_object_handler remove_device_model_object, nmos::add_device_model_object_handler add_device_model_object);
    }

    // Check to see if role_path is sub path of parent_role_path
    bool is_role_path_root(const web::json::array& role_path_, const web::json::array& parent_role_path);

    bool is_block_modified(const nmos::resource& resource, const web::json::value& object_properties_holder);

    // Get role path of resource given the Device Model resources
    web::json::array get_role_path(const nmos::resources& resources, const nmos::resource& resource);

    // Get object_properties_holder for specified target_role_path
    web::json::array get_object_properties_holder(const web::json::array& object_properties_holders, const web::json::array& target_role_path);

    web::json::value apply_backup_data_set(nmos::resources& resources, const nmos::resource& resource, const web::json::array& object_properties_holders, bool recurse, const web::json::value& restore_mode, bool validate, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_holders_handler filter_property_holders, nmos::remove_device_model_object_handler remove_device_model_object, nmos::add_device_model_object_handler add_device_model_object);

    web::json::value get_property_holder(const web::json::value& object_properties_holder, const nmos::nc_property_id& property_id);
}

#endif

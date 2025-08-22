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
    web::json::value get_properties_by_path(const nmos::resources& resources, const nmos::resource& resource, bool recurse, bool include_descriptors, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, nmos::create_validation_fingerprint_handler create_validation_fingerprint);

    web::json::value validate_set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, nmos::nc_restore_mode::restore_mode restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, nmos::validate_validation_fingerprint_handler validate_validation_fingerprint, nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list, nmos::remove_device_model_object_handler remove_device_model_object, nmos::create_device_model_object_handler create_device_model_object);

    web::json::value set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, nmos::nc_restore_mode::restore_mode restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, nmos::validate_validation_fingerprint_handler validate_validation_fingerprint, nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list, nmos::remove_device_model_object_handler remove_device_model_object, nmos::create_device_model_object_handler create_device_model_object);
}

#endif

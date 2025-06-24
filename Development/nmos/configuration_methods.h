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
    web::json::value get_properties_by_path(const nmos::resources& resources, const nmos::resource& resource, bool recurse, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor);

    web::json::value validate_set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_holders_handler filter_property_holders, nmos::remove_device_model_object_handler remove_device_model_object, nmos::add_device_model_object_handler add_device_model_object);

    web::json::value set_properties_by_path(nmos::resources& resources, const nmos::resource& resource, const web::json::value& backup_data_set, bool recurse, const web::json::value& restore_mode, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::filter_property_holders_handler filter_property_holders, nmos::remove_device_model_object_handler remove_device_model_object, nmos::add_device_model_object_handler add_device_model_object);
}

#endif

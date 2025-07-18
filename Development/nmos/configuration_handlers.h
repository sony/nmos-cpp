#ifndef NMOS_CONFIGURATION_HANDLERS_H
#define NMOS_CONFIGURATION_HANDLERS_H

#include <functional>
#include "nmos/control_protocol_handlers.h"
#include "nmos/resources.h"
#include "nmos/control_protocol_resource.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    namespace experimental
    {
        struct control_protocol_state;
    }
    // This callback is invoked if attempting to modify read only properties when restoring a configuration.
    // This function should modify the Device Model object directly and return a corresponding NcObjectPropertiesSetValidation object
    typedef std::function<std::vector<nmos::nc_property_id>(const nmos::resource& resource, const std::vector<utility::string_t>& role_path, const std::vector<nmos::nc_property_id>& property_ids)> get_read_only_modification_allow_list_handler;

    // This callback is invoked if attempting to remove a device model object when restoring a configuration.
    // This function should handle the modification of the Device Model and any corresponding NMOS resources
    // and return true if successful and false otherwise
    typedef std::function<bool(const nmos::resource& resource, const std::vector<utility::string_t>& role_path, bool validate)> remove_device_model_object_handler;

    // This callback is invoked if attempting to add a device model object to a rebuildable block when restoring a configuration.
    // This function should handle the modification of the Device Model and any corresponding NMOS resources
    // and return corresponding NcObjectPropertiesSetValidation objects for the object added
    typedef std::function<nmos::control_protocol_resource(const nmos::nc_class_id& class_id, nmos::nc_oid oid, bool constant_oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, bool validate, const std::map<nmos::nc_property_id, web::json::value>& property_values)> create_device_model_object_handler;
}

#endif

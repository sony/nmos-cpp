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
    // This callback is invoked when getting a backup dataset to generate a validation fingerprint
    // This function should generate a fingerprint that can be used for subsequent validation
    typedef std::function<utility::string_t(const nmos::resources& resources, const nmos::resource& resource)> create_validation_fingerprint_handler;

    // This callback is invoked when validating or restoring a backup dataset
    // This function should validate the validation fingerprint
    typedef std::function<bool(const nmos::resources& resources, const nmos::resource& resource, const utility::string_t& validation_fingerprint)> validate_validation_fingerprint_handler;

    // This callback is invoked if attempting to modify read only properties when restoring a configuration.
    // This function return a vector of property ids of the read only properties allowed to be modified
    typedef std::function<std::vector<nmos::nc_property_id>(const nmos::resource& resource, const std::vector<utility::string_t>& role_path, const std::vector<nmos::nc_property_id>& property_ids)> get_read_only_modification_allow_list_handler;

    // This callback is invoked if attempting to remove a device model object when restoring a configuration.
    // This function calls back before an object is removed from the device model.
    typedef std::function<bool(const nmos::resource& resource, const std::vector<utility::string_t>& role_path, bool validate)> remove_device_model_object_handler;

    // This callback is invoked if attempting to add a device model object to a rebuildable block when restoring a configuration.
    // This function should return the newly created device model object.
    typedef std::function<nmos::control_protocol_resource(const nmos::nc_class_id& class_id, nmos::nc_oid oid, bool constant_oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, bool validate, const std::map<nmos::nc_property_id, web::json::value>& property_values)> create_device_model_object_handler;
}

#endif

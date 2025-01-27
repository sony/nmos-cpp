#ifndef NMOS_CONFIGURATION_RESOURCES_H
#define NMOS_CONFIGURATION_RESOURCES_H

#include "nmos/control_protocol_typedefs.h"
#include "nmos/resources.h"

namespace nmos
{
    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const web::json::value& notices, const utility::string_t& status_message);

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const web::json::value& notices);

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status);

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const utility::string_t& status_message);
}

#endif

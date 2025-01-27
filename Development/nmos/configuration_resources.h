#ifndef NMOS_CONFIGURATION_RESOURCES_H
#define NMOS_CONFIGURATION_RESOURCES_H

#include "nmos/configuration_handlers.h"
#include "nmos/control_protocol_handlers.h"
#include "nmos/resources.h"

namespace nmos
{
    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const web::json::array& notices, const utility::string_t& statusMessage);
}

#endif

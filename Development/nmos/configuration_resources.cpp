#include "nmos/configuration_resources.h"

#include "cpprest/json_utils.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/slog.h"

namespace nmos
{
    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const web::json::array& notices, const utility::string_t& status_message)
    {
        return nc::details::make_nc_object_properties_set_validation(role_path, status, notices, web::json::value::string(status_message));
    }

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const web::json::array& notices)
    {
        return nc::details::make_nc_object_properties_set_validation(role_path, status, notices, web::json::value::null());
    }

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status)
    {
        return nc::details::make_nc_object_properties_set_validation(role_path, status, web::json::value::array().as_array(), web::json::value::null());
    }

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const utility::string_t& status_message)
    {
        return nc::details::make_nc_object_properties_set_validation(role_path, status, web::json::value::array().as_array(), web::json::value::string(status_message));
    }
}

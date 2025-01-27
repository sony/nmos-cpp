#include "nmos/configuration_resources.h"

#include "cpprest/json_utils.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace details
    {
        web::json::value make_nc_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const web::json::value& notices, const web::json::value& status_message)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::nc::path, web::json::value_from_elements(role_path)},
                { nmos::fields::nc::status, status },
                { nmos::fields::nc::notices, notices },
                { nmos::fields::nc::status_message, status_message }
                });
        }
    }

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const web::json::value& notices, const utility::string_t& status_message)
    {
        return details::make_nc_object_properties_set_validation(role_path, status, notices, web::json::value::string(status_message));
    }

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const web::json::value& notices)
    {
        return details::make_nc_object_properties_set_validation(role_path, status, notices, web::json::value::null());
    }

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status)
    {
        return details::make_nc_object_properties_set_validation(role_path, status, web::json::value::array(), web::json::value::null());
    }

    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const utility::string_t& status_message)
    {
        return details::make_nc_object_properties_set_validation(role_path, status, web::json::value::array(), web::json::value::string(status_message));
    }
}

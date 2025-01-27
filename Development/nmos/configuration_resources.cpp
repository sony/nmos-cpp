#include "nmos/configuration_methods.h"

#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/slog.h"

namespace nmos
{
    web::json::value make_object_properties_set_validation(const web::json::array& role_path, const nmos::nc_restore_validation_status::status status, const web::json::array& notices, const utility::string_t& statusMessage)
    {
		return web::json::value();
    }
}

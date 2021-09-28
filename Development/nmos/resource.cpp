#include "nmos/resource.h"

#include "nmos/json_fields.h"
#include "nmos/version.h"

namespace nmos
{
    namespace details
    {
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/resource_core.json
        web::json::value make_resource_core(const nmos::id& id, const utility::string_t& label, const utility::string_t& description, const web::json::value& tags)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::id, id },
                { nmos::fields::version, nmos::make_version() },
                { nmos::fields::label, label },
                { nmos::fields::description, description },
                { nmos::fields::tags, tags }
            });
        }

        web::json::value make_resource_core(const nmos::id& id, const nmos::settings& settings)
        {
            const auto& label = nmos::experimental::fields::label(settings);
            const auto& description = nmos::experimental::fields::description(settings);

            return make_resource_core(id, label, description);
        }
    }
}

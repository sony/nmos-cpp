#include "nmos/resource.h"

#include "nmos/version.h"

namespace nmos
{
    namespace details
    {
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/resource_core.json
        web::json::value make_resource_core(const nmos::id& id, const nmos::settings& settings)
        {
            using web::json::value;

            const auto label = value::string(nmos::experimental::fields::label(settings));

            value data;

            data[U("id")] = value::string(id);
            data[U("version")] = value::string(nmos::make_version());
            data[U("label")] = label;
            data[U("description")] = label;
            data[U("tags")] = value::object();

            return data;
        }
    }
}

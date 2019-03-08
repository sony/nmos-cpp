#include "nmos/system_resources.h"

#include "nmos/json_fields.h"
#include "nmos/resource.h"

namespace nmos
{
    nmos::resource make_system_global(const nmos::id& id, const nmos::settings& settings)
    {
        using web::json::value_of;

        auto data = details::make_resource_core(id, settings);

        data[U("is04")] = value_of({
                { U("heartbeat_interval"), nmos::fields::registration_heartbeat_interval(settings) }
        });
        data[U("ptp")] = value_of({
            { U("announce_receipt_timeout"), 2 },
            { U("domain_number"), 57 }
        });

        return{ { 1, 0 }, types::global, data, true };
    }

    namespace experimental
    {
        // assign a system global configuration resource according to the settings
        void assign_system_global_resource(nmos::resource& resource, const nmos::settings& settings)
        {
            resource = nmos::make_system_global(nmos::make_repeatable_id(nmos::experimental::fields::seed_id(settings), U("/x-nmos/system/global")), settings);
        }
    }
}

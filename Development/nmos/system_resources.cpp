#include "nmos/system_resources.h"

#include "nmos/is09_versions.h"
#include "nmos/json_fields.h"
#include "nmos/resource.h"

namespace nmos
{
    nmos::resource make_system_global(const nmos::id& id, const nmos::settings& settings)
    {
        return{ is09_versions::v1_0, types::global, make_system_global_data(id, settings), true };
    }

    web::json::value make_system_global_data(const nmos::id& id, const nmos::settings& settings)
    {
        using web::json::value_of;

        auto data = details::make_resource_core(id, settings);

        data[nmos::fields::is04] = value_of({
            { nmos::fields::heartbeat_interval, nmos::fields::registration_heartbeat_interval(settings) }
        });
        data[nmos::fields::ptp] = value_of({
            { nmos::fields::announce_receipt_timeout, nmos::fields::ptp_announce_receipt_timeout(settings) },
            { nmos::fields::domain_number, nmos::fields::ptp_domain_number(settings) }
        });

        return data;
    }

    std::pair<nmos::id, nmos::settings> parse_system_global_data(const web::json::value& data)
    {
        using web::json::value_of;

        const auto& is04 = nmos::fields::is04(data);
        const auto& ptp = nmos::fields::ptp(data);

        return{
            nmos::fields::id(data),
            value_of({
                { nmos::fields::registration_heartbeat_interval, nmos::fields::heartbeat_interval(is04) },
                { nmos::fields::ptp_announce_receipt_timeout, nmos::fields::announce_receipt_timeout(ptp) },
                { nmos::fields::ptp_domain_number, nmos::fields::domain_number(ptp) }
            })
        };
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

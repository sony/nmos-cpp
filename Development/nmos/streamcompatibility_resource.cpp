#include "nmos/streamcompatibility_resource.h"

#include <unordered_set>
#include "nmos/capabilities.h" // for nmos::fields::constraint_sets
#include "nmos/resource.h"
#include "nmos/json_fields.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            web::json::value make_streamcompatibility_edid_endpoint(const web::uri& edid_uri, const utility::string_t& edid_binary, bool locked)
            {
                using web::json::value;

                value data;
                if (!edid_uri.is_empty())
                {
                    data[nmos::fields::edid_href] = value::string(edid_uri.to_string());
                }
                if (!edid_binary.empty())
                {
                    data[nmos::fields::edid_binary] = value::string(edid_binary);
                }
                data[nmos::fields::temporarily_locked] = value::boolean(locked);

                return data;
            }
        }

        web::json::value make_streamcompatibility_active_constraints_endpoint(const web::json::value& constraint_sets, bool locked)
        {
            using web::json::value_of;

            auto active_constraint_sets = value_of({
                { nmos::fields::constraint_sets, constraint_sets }
            });

            return value_of({
                { nmos::fields::active_constraint_sets, active_constraint_sets },
                { nmos::fields::temporarily_locked, locked }
            });
        }

        web::json::value make_streamcompatibility_edid_endpoint(bool locked)
        {
            return details::make_streamcompatibility_edid_endpoint({}, {}, locked);
        }

        web::json::value make_streamcompatibility_edid_endpoint(const web::uri& edid_uri, bool locked)
        {
            return details::make_streamcompatibility_edid_endpoint(edid_uri, {}, locked);
        }

        web::json::value make_streamcompatibility_edid_endpoint(const utility::string_t& edid_binary, bool locked)
        {
            return details::make_streamcompatibility_edid_endpoint({}, edid_binary, locked);
        }

        web::json::value make_streamcompatibility_input_output_base(const nmos::id& id, const nmos::id& device_id, bool connected, bool edid_support, const nmos::settings& settings)
        {
            using web::json::value;

            auto data = nmos::details::make_resource_core(id, settings);

            data[nmos::fields::connected] = value::boolean(connected);
            data[nmos::fields::device_id] = value::string(device_id);
            data[nmos::fields::edid_support] = value::boolean(edid_support);

            return data;
        }
    }
}

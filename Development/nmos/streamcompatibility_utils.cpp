#include "nmos/streamcompatibility_utils.h"

namespace nmos
{
    namespace experimental
    {
        // it's expected that write lock is already catched for the model
        void update_version(nmos::resources& resources, const nmos::id& resource_id, const utility::string_t& new_version)
        {
            modify_resource(resources, resource_id, [&new_version](nmos::resource& resource)
            {
                resource.data[nmos::fields::version] = web::json::value::string(new_version);
            });
        }

        // it's expected that write lock is already catched for the model
        void update_version(nmos::resources& resources, const web::json::array& resource_ids, const utility::string_t& new_version)
        {
            for (const auto& resource_id : resource_ids)
            {
                update_version(resources, resource_id.as_string(), new_version);
            }
        }
    }
}

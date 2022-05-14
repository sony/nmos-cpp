#include "nmos/id.h"
#include "nmos/resources.h"

namespace nmos
{
    namespace experimental
    {
        void update_version(nmos::resources& resources, const nmos::id& resource_id, const utility::string_t& new_version);
        void update_version(nmos::resources& resources, const web::json::array& resource_ids, const utility::string_t& new_version);
    }
}

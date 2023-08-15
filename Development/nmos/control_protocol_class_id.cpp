#include "nmos/control_protocol_class_id.h"

namespace nmos
{
    namespace details
    {
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassid
        web::json::value make_nc_class_id(const nc_class_id& class_id)
        {
            using web::json::value;

            auto nc_class_id = value::array();
            for (const auto class_id_item : class_id) { web::json::push_back(nc_class_id, class_id_item); }
            return nc_class_id;
        }

        nc_class_id parse_nc_class_id(const web::json::array& class_id_)
        {
            nc_class_id class_id;
            for (auto& element : class_id_)
            {
                class_id.push_back(element.as_integer());
            }
            return class_id;
        }
    }
}

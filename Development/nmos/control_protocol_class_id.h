#ifndef NMOS_CONTROL_PROTOCOL_CLASS_ID_H
#define NMOS_CONTROL_PROTOCOL_CLASS_ID_H

#include "cpprest/json_utils.h"

namespace nmos
{
    namespace details
    {
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassid
        typedef std::vector<int32_t> nc_class_id;

        // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassid
        web::json::value make_nc_class_id(const nc_class_id& class_id);
        nc_class_id parse_nc_class_id(const web::json::array& class_id);
    }
}

#endif

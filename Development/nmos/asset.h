#ifndef NMOS_ASSET_H
#define NMOS_ASSET_H

#include "cpprest/json_utils.h"

// Asset Distinguishing Information
// See https://specs.amwa.tv/bcp-002-02/
// and https://specs.amwa.tv/nmos-parameter-registers/branches/main/tags/
namespace nmos
{
    namespace fields
    {
        const web::json::field_as_value_or asset_manufacturer{ U("urn:x-nmos:tag:asset:manufacturer/v1.0"), web::json::value::array() };
        const web::json::field_as_value_or asset_product_name{ U("urn:x-nmos:tag:asset:product/v1.0"), web::json::value::array() };
        const web::json::field_as_value_or asset_instance_id{ U("urn:x-nmos:tag:asset:instance-id/v1.0"), web::json::value::array() };
        const web::json::field_as_value_or asset_function{ U("urn:x-nmos:tag:asset:function/v1.0"), web::json::value::array() };
    }
}

#endif

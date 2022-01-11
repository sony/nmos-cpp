#ifndef NMOS_FLOWCOMPATIBILITY_API_H
#define NMOS_FLOWCOMPATIBILITY_API_H

#include "nmos/api_utils.h"
#include "nmos/slog.h"

// Flow Compatibility Management API implementation
// See https://github.com/AMWA-TV/is-11/blob/v1.0-dev/APIs/FlowCompatibilityManagementAPI.raml
namespace nmos
{
    struct node_model;

    namespace experimental
    {
        web::http::experimental::listener::api_router make_flowcompatibility_api(nmos::node_model& model, slog::base_gate& gate);
    }
}

#endif

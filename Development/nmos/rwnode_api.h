#ifndef NMOS_RWNODE_API_H
#define NMOS_RWNODE_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// Read/Write Node API implementation
// See https://specs.amwa.tv/is-rwnode/branches/v1.0-dev/APIs/ReadWriteNodeAPI.html
namespace nmos
{
    struct model;

    web::http::experimental::listener::api_router make_rwnode_api(nmos::model& model, slog::base_gate& gate);
}

#endif

#ifndef NMOS_MANIFEST_API_H
#define NMOS_MANIFEST_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// Experimental Manifest API implementation, to provide endpoints for Node API "manifest_href"
// using implementation details shared with the Connection API
namespace nmos
{
    struct node_model;

    namespace experimental
    {
        web::http::experimental::listener::api_router make_manifest_api(const nmos::node_model& model, slog::base_gate& gate);
    }
}

#endif

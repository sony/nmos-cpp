#ifndef NMOS_CONNECTION_API_H
#define NMOS_CONNECTION_API_H

#include "cpprest/api_router.h"
#include "nmos/id.h"

namespace slog
{
    class base_gate;
}

// Connection API implementation
// See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml
namespace nmos
{
    struct node_model;
    struct type;

    web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, slog::base_gate& gate);

    namespace details
    {
        void handle_connection_resource_patch(web::http::http_response res, nmos::node_model& model, const std::pair<nmos::id, nmos::type>& id_type, const web::json::value& patch, slog::base_gate& gate);
    }
}

#endif

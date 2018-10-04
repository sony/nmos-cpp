#ifndef NMOS_CONNECTION_API_H
#define NMOS_CONNECTION_API_H

#include "cpprest/api_router.h"
#include "nmos/resources.h" // for nmos::resources, id, tai

namespace slog
{
    class base_gate;
}

// Connection API implementation
// See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml
namespace nmos
{
    struct node_model;

    web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, slog::base_gate& gate);

    namespace details
    {
        void set_staged_activation_time(nmos::resources& connection_resources, const nmos::id& id, const nmos::tai& tai = tai_now());
        void set_staged_activation_not_pending(nmos::resources& connection_resources, const nmos::id& id);
    }
}

#endif

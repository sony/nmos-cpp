#ifndef NMOS_QUERY_API_H
#define NMOS_QUERY_API_H

#include <mutex>
#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// Query API implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2-dev/APIs/QueryAPI.raml
namespace nmos
{
    struct model;

    web::http::experimental::listener::api_router make_query_api(nmos::model& model, std::mutex& mutex, slog::base_gate& gate);
}

#endif

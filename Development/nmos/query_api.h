#ifndef NMOS_QUERY_API_H
#define NMOS_QUERY_API_H

#include "cpprest/api_router.h"
#include "nmos/mutex.h"
#include "nmos/settings.h"

namespace slog
{
    class base_gate;
}

// Query API implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/QueryAPI.raml
namespace nmos
{
    struct model;

    web::http::experimental::listener::api_router make_query_api(nmos::model& model, nmos::mutex& mutex, slog::base_gate& gate);

    struct resource_paging;

    namespace details
    {
        web::uri make_query_uri_with_no_paging(const web::http::http_request& req, const nmos::settings& settings);
        void add_paging_headers(web::http::http_headers& headers, const nmos::resource_paging& paging, const web::uri& base_link);
    }
}

#endif

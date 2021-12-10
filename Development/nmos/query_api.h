#ifndef NMOS_QUERY_API_H
#define NMOS_QUERY_API_H

#include "cpprest/api_router.h"
#include "nmos/settings.h"

namespace slog
{
    class base_gate;
}

// Query API implementation
// See https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/QueryAPI.html
namespace nmos
{
    struct registry_model;

    web::http::experimental::listener::api_router make_query_api(nmos::registry_model& model, slog::base_gate& gate);

    struct resource_paging;

    namespace details
    {
        web::uri make_query_uri_with_no_paging(const web::http::http_request& req, const nmos::settings& settings);
        void add_paging_headers(web::http::http_headers& headers, const nmos::resource_paging& paging, const web::uri& base_link);
    }
}

#endif

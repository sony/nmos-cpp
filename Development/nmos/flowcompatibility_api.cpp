#include "nmos/flowcompatibility_api.h"

#include <boost/range/adaptor/transformed.hpp>
#include "nmos/is11_versions.h"
#include "nmos/model.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_unmounted_flowcompatibility_api(nmos::node_model& model, slog::base_gate& gate);

        web::http::experimental::listener::api_router make_flowcompatibility_api(nmos::node_model& model, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router flowcompatibility_api;

            flowcompatibility_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
                return pplx::task_from_result(true);
            });

            flowcompatibility_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("flowcompatibility/") }, req, res));
                return pplx::task_from_result(true);
            });

            const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is11_versions::from_settings(model.settings); });
            flowcompatibility_api.support(U("/x-nmos/") + nmos::patterns::flowcompatibility_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
                return pplx::task_from_result(true);
            });

            flowcompatibility_api.mount(U("/x-nmos/") + nmos::patterns::flowcompatibility_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_flowcompatibility_api(model, gate));

            return flowcompatibility_api;
        }

        web::http::experimental::listener::api_router make_unmounted_flowcompatibility_api(nmos::node_model& model, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router flowcompatibility_api;

            // check for supported API version
            const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is11_versions::from_settings(model.settings); });

            flowcompatibility_api.support(U(".*"), nmos::details::make_api_version_handler(versions, gate_));

            flowcompatibility_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("senders/"), U("receivers/"), U("inputs/"), U("outputs/") }, req, res));
                return pplx::task_from_result(true);
            });

            flowcompatibility_api.support(U("/") + nmos::patterns::flowCompatibilityResourceType.pattern + U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, web::json::value::array());
                return pplx::task_from_result(true);
            });

            return flowcompatibility_api;
        }
    }
}

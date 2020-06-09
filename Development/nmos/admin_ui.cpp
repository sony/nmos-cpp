#include "nmos/admin_ui.h"

#include "nmos/api_utils.h"
#include "nmos/filesystem_route.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_api_sub_route(const utility::string_t& sub_route, web::http::experimental::listener::api_router sub_router, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router router;

            const route_pattern api = make_route_pattern(U("api"), sub_route);

            router.support(U("/?"), methods::GET, [sub_route](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ sub_route + U('/') }, req, res));
                return pplx::task_from_result(true);
            });

            router.support(U("/") + api.pattern, methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::TemporaryRedirect); // or status_codes::MovedPermanently?
                res.headers().add(web::http::header_names::location, req.request_uri().path() + U('/'));
                return pplx::task_from_result(true);
            });

            router.mount(U("/") + api.pattern + U("(?=.)"), web::http::methods::GET, std::move(sub_router));

            return router;
        }

        web::http::experimental::listener::api_router make_admin_ui(const utility::string_t& filesystem_root, slog::base_gate& gate)
        {
            // To serve the admin UI, only a few HTML, JavaScript and CSS files are necessary
            const nmos::experimental::content_types valid_extensions =
            {
                { U("ico"), U("image/x-icon") },
                { U("html"), U("text/html") },
                { U("js"), U("application/javascript") },
                { U("map"), U("application/json") },
                { U("json"), U("application/json") },
                { U("css"), U("text/css") },
                { U("png"), U("image/png") }
            };

            return nmos::experimental::make_api_sub_route(U("admin"), nmos::experimental::make_filesystem_route(filesystem_root, nmos::experimental::make_relative_path_content_type_handler(valid_extensions), gate), gate);
        }
    }
}

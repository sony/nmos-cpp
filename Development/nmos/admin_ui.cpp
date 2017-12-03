#include "nmos/admin_ui.h"

#include "nmos/api_utils.h"
#include "nmos/filesystem_route.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        namespace patterns
        {
            const route_pattern admin_ui = make_route_pattern(U("api"), U("admin"));
        }

        web::http::experimental::listener::api_router make_admin_ui(const utility::string_t& filesystem_root, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router admin_ui;

            admin_ui.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ JU("admin/") }));
                return pplx::task_from_result(true);
            });

            admin_ui.support(U("/") + nmos::experimental::patterns::admin_ui.pattern + U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                // temporarily hard-coded hack to redirect admin root to the index.html
                set_reply(res, status_codes::TemporaryRedirect);
                res.headers().add(web::http::header_names::location, U("/admin/index.html"));
                return pplx::task_from_result(true);
            });

            // To serve the admin UI, only a few HTML, JavaScript and CSS files are necessary
            const nmos::experimental::content_types valid_extensions =
            {
                { U("ico"), U("image/x-icon") },
                { U("html"), U("text/html") },
                { U("js"), U("application/javascript") },
                { U("css"), U("text/css") },
                { U("png"), U("image/png") }
            };

            admin_ui.mount(U("/") + nmos::experimental::patterns::admin_ui.pattern, web::http::methods::GET, nmos::experimental::make_filesystem_route(filesystem_root, nmos::experimental::make_relative_path_content_type_validator(valid_extensions), gate));

            nmos::add_api_finally_handler(admin_ui, gate);

            return admin_ui;
        }
    }
}

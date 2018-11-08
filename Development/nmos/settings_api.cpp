#include "nmos/settings_api.h"

#include "nmos/api_utils.h"
#include "nmos/model.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_settings_api(nmos::base_model& model, std::atomic<slog::severity>& logging_level, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router settings_api;

            settings_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("settings/") }, res));
                return pplx::task_from_result(true);
            });

            settings_api.support(U("/settings/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("all/") }, res));
                return pplx::task_from_result(true);
            });

            settings_api.support(U("/settings/all/?"), methods::GET, [&model](http_request, http_response res, const string_t&, const route_parameters&)
            {
                auto lock = model.read_lock();
                set_reply(res, status_codes::OK, model.settings);
                return pplx::task_from_result(true);
            });

            settings_api.support(U("/settings/all/?"), methods::POST, [&model, &logging_level, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                return details::extract_json(req, parameters, gate).then([&, req, res](value body) mutable
                {
                    auto lock = model.write_lock();

                    // Validate request?

                    model.settings = body;

                    // for the moment, logging_level is a special case because we want to turn it into an atomic value
                    // that can be read by logging statements without locking the mutex protecting the settings
                    logging_level = nmos::fields::logging_level(model.settings);

                    // notify anyone who cares...
                    model.notify();

                    set_reply(res, status_codes::OK, model.settings);

                    return true;
                });
            });

            return settings_api;
        }
    }
}

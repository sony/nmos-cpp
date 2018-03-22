#include "nmos/settings_api.h"

#include "nmos/api_utils.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_settings_api(nmos::settings& settings, std::atomic<slog::severity>& logging_level, nmos::mutex& mutex, nmos::condition_variable& condition, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router settings_api;

            settings_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ JU("settings/") }));
                return pplx::task_from_result(true);
            });

            settings_api.support(U("/settings/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ JU("all/") }));
                return pplx::task_from_result(true);
            });

            settings_api.support(U("/settings/all/?"), methods::GET, [&settings, &mutex](http_request, http_response res, const string_t&, const route_parameters&)
            {
                nmos::read_lock lock(mutex);
                set_reply(res, status_codes::OK, settings);
                return pplx::task_from_result(true);
            });

            settings_api.support(U("/settings/all/?"), methods::POST, [&settings, &logging_level, &mutex, &condition](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                return req.extract_json().then([&, req, res](value body) mutable
                {
                    nmos::write_lock lock(mutex);

                    // Validate request?

                    settings = body;

                    // for the moment, logging_level is a special case because we want to turn it into an atomic value
                    // that can be read by logging statements without locking the mutex protecting the settings
                    logging_level = nmos::fields::logging_level(settings);

                    // notify anyone who cares...
                    condition.notify_all();

                    set_reply(res, status_codes::OK, settings);

                    return true;
                });
            });

            nmos::add_api_finally_handler(settings_api, gate);

            return settings_api;
        }
    }
}

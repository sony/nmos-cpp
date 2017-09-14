#include "nmos/settings_api.h"

#include "nmos/api_utils.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_settings_api(nmos::settings& settings, std::mutex& mutex, std::atomic<slog::severity>& logging_level, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router settings_api;

            settings_api.support(U("/?"), methods::GET, [](const http_request&, http_response& res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ JU("settings/") }));
                return true;
            });

            settings_api.support(U("/settings/?"), methods::GET, [](const http_request&, http_response& res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ JU("all/") }));
                return true;
            });

            settings_api.support(U("/settings/all/?"), methods::GET, [&settings, &mutex](const http_request&, http_response& res, const string_t&, const route_parameters&)
            {
                std::lock_guard<std::mutex> lock(mutex);
                set_reply(res, status_codes::OK, settings);
                return true;
            });

            settings_api.support(U("/settings/all/?"), methods::POST, [&settings, &mutex, &logging_level](const http_request& req, http_response& res, const string_t&, const route_parameters&)
            {
                std::lock_guard<std::mutex> lock(mutex);

                // should probably use a .then() continuation, bound to settings and the mutex, but pah
                settings = req.extract_json().get();

                // for the moment, logging_level is a special case because we want to turn it into an atomic value
                // that can be read by logging statements without locking the mutex protecting the settings
                logging_level = nmos::fields::logging_level(settings);

                set_reply(res, status_codes::OK, settings);

                return true;
            });

            nmos::add_api_finally_handler(settings_api, gate);

            return settings_api;
        }
    }
}

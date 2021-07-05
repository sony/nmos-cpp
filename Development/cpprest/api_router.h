#ifndef CPPREST_API_ROUTER_H
#define CPPREST_API_ROUTER_H

#include <functional>
#include <list>
#include <unordered_map>
#include "cpprest/http_utils.h"
#include "cpprest/json_ops.h" // hmm, only for names used in using declarations
#include "cpprest/regex_utils.h" // hmm, only for types used in details functions

// api_router is an extension to http_listener that uses regexes to define route patterns
namespace web
{
    namespace http
    {
        namespace experimental
        {
            namespace listener
            {
                // a using-directive with the following namespace makes defining routers less verbose
                // using namespace api_router_using_declarations;
                namespace api_router_using_declarations {}

                typedef std::unordered_map<utility::string_t, utility::string_t> route_parameters;

                // route handlers have access to the request, and a mutable response object, the route path and parameters extracted from it by the matched route pattern;
                // a handler may e.g. reply to the request or initiate asynchronous processing, and returns a flag indicating whether to continue matching routes or not
                typedef std::function<pplx::task<bool>(web::http::http_request req, web::http::http_response res, const utility::string_t& route_path, const route_parameters& parameters)> route_handler;

                // api router implementation
                namespace details
                {
                    class api_router_impl;

                    enum match_flag_type { match_entire = 0, match_prefix = 1 };

                    utility::string_t get_route_relative_path(const web::http::http_request& req, const utility::string_t& route_path);
                    route_parameters get_parameters(const utility::named_sub_matches_t& parameter_sub_matches, const utility::smatch_t& route_match);
                    bool route_regex_match(const utility::string_t& path, utility::smatch_t& route_match, const utility::regex_t& route_regex, match_flag_type flags);
                }

                class api_router
                {
                public:
                    api_router();

                    // allow use as a handler with http_listener
                    void operator()(web::http::http_request req);
                    // allow use as a mounted handler in another api_router
                    pplx::task<bool> operator()(web::http::http_request req, web::http::http_response res, const utility::string_t& route_path, const route_parameters& parameters);

                    // add a method-specific handler to support requests for this route
                    void support(const utility::string_t& route_pattern, const web::http::method& method, route_handler handler);
                    // add a handler to support all other requests for this route (must be added after any method-specific handlers)
                    void support(const utility::string_t& route_pattern, route_handler all_handler);

                    // add a method-specific handler to support requests for this route and sub-routes
                    void mount(const utility::string_t& route_pattern, const web::http::method& method, route_handler handler);
                    // add a handler to support all other requests for this route and sub-routes (must be added after any method-specific handlers)
                    void mount(const utility::string_t& route_pattern, route_handler all_handler);

                    // provide an exception handler for this route and sub-routes (using std::current_exception, etc.)
                    void set_exception_handler(route_handler handler);

                private:
                    std::shared_ptr<details::api_router_impl> impl;
                };

                // convenient using declarations to make defining routers less verbose
                namespace api_router_using_declarations
                {
                    using utility::string_t;
                    using web::http::experimental::listener::api_router;
                    using web::http::experimental::listener::route_parameters;
                    using web::http::http_request;
                    using web::http::http_response;
                    using web::http::methods;
                    using web::http::set_reply;
                    using web::http::status_codes;
                    using web::json::value;
                    using web::json::value_of;
                }
            }
        }
    }
}

#endif

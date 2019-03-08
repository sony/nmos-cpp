#ifndef CPPREST_API_ROUTER_H
#define CPPREST_API_ROUTER_H

#include <functional>
#include <list>
#include <unordered_map>
#include "pplx/pplx_utils.h"
#include "cpprest/http_listener.h"
#include "cpprest/http_utils.h" // hmm, only for names used in using declarations
#include "cpprest/json_utils.h" // hmm, only for names used in using declarations
#include "cpprest/regex_utils.h" // hmm, only for types used in private static functions
#include "detail/private_access.h"

// api_router is an extension to http_listener that uses regexes to define route patterns
namespace web
{
    namespace http
    {
        namespace experimental
        {
            namespace listener
            {
                // RAII helper for http_listener sessions (could be extracted to another header)
                typedef pplx::open_close_guard<http_listener> http_listener_guard;

                // a using-directive with the following namespace makes defining routers less verbose
                // using namespace api_router_using_declarations;
                namespace api_router_using_declarations {}

                // platform-specific wildcard address to accept connections for any address
#if defined(_WIN32) && !defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
                const utility::string_t host_wildcard{ _XPLATSTR("*") }; // "weak wildcard"
#else
                const utility::string_t host_wildcard{ _XPLATSTR("0.0.0.0") };
#endif

                // make an http address to be used to accept connections for the specified address and port
                inline web::uri make_listener_uri(const utility::string_t& host_address, int port)
                {
                    return web::uri_builder().set_scheme(_XPLATSTR("http")).set_host(host_address).set_port(port).to_uri();
                }

                // make an http address to be used to accept connections for the specified port for any address
                inline web::uri make_listener_uri(int port)
                {
                    return make_listener_uri(host_wildcard, port);
                }

                typedef std::unordered_map<utility::string_t, utility::string_t> route_parameters;

                // route handlers have access to the request, and a mutable response object, the route path and parameters extracted from it by the matched route pattern;
                // a handler may e.g. reply to the request or initiate asynchronous processing, and returns a flag indicating whether to continue matching routes or not
                typedef std::function<pplx::task<bool>(web::http::http_request, web::http::http_response, const utility::string_t&, const route_parameters&)> route_handler;

                class api_router
                {
                    DETAIL_PRIVATE_ACCESS_DECLARATION
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
                    enum match_flag_type { match_entire = 0, match_prefix = 1 };
                    typedef std::pair<utility::regex_t, utility::named_sub_matches_t> regex_named_sub_matches_type;
                    struct route { match_flag_type flags; regex_named_sub_matches_type route_pattern; web::http::method method; route_handler handler; };
                    typedef std::list<route> route_handlers;
                    typedef route_handlers::iterator iterator;

                    static utility::string_t get_route_relative_path(const web::http::http_request& req, const utility::string_t& route_path);
                    static pplx::task<bool> call(const route_handler& handler, const route_handler& exception_handler, web::http::http_request req, web::http::http_response res, const utility::string_t& route_path, const route_parameters& parameters);
                    static void handle_method_not_allowed(const route& route, web::http::http_response& res, const utility::string_t& route_path, const route_parameters& parameters);
                    static route_parameters get_parameters(const utility::named_sub_matches_t& parameter_sub_matches, const utility::smatch_t& route_match);
                    static route_parameters insert(route_parameters&& into, const route_parameters& range);
                    static bool route_regex_match(const utility::string_t& path, utility::smatch_t& route_match, const utility::regex_t& route_regex, match_flag_type flags);

                    pplx::task<bool> operator()(web::http::http_request req, web::http::http_response res, const utility::string_t& route_path, const route_parameters& parameters, iterator route);

                    // to allow routes to be added out-of-order, support() and mount() could easily be given overloads that accept and return an iterator (const_iterator in C++11)
                    iterator insert(iterator where, match_flag_type flags, const utility::string_t& route_pattern, const web::http::method& method, route_handler handler);

                    route_handlers routes;
                    route_handler exception_handler;
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

#ifndef NMOS_API_UTILS_H
#define NMOS_API_UTILS_H

#include <map>
#include <set>
#include "cpprest/api_router.h"
#include "cpprest/http_listener.h" // for web::http::experimental::listener::http_listener_config
#include "cpprest/regex_utils.h"
#include "cpprest/ws_listener.h" // for web::websockets::experimental::listener::websocket_listener_config
#include "nmos/settings.h" // just a forward declaration of nmos::settings

namespace slog
{
    class base_gate;
}

// Utility types, constants and functions for implementing NMOS REST APIs
namespace nmos
{
    struct api_version;
    struct type;

    // Patterns are used to form parameterised route paths
    // (could be moved to cpprest/api_router.h or cpprest/route_pattern.h?)

    struct route_pattern { utility::string_t name; utility::string_t pattern; };

    inline route_pattern make_route_pattern(const utility::string_t& name, const utility::string_t& base_pattern)
    {
        return{ name, utility::make_named_sub_match(name, base_pattern) };
    }

    // Patterns allow access to variable parts of the route path, both to allow common implementation
    // for similar endpoints, and to simplify consistent logging of values from URLs such as
    // http://example.com/x-nmos/{api}/{version}/{resourceType}/{resourceId}/exampleendpoint
    namespace patterns
    {
        // IS-04 Node API
        const route_pattern node_api = make_route_pattern(U("api"), U("node"));
        // IS-04 Query API
        const route_pattern query_api = make_route_pattern(U("api"), U("query"));
        // IS-04 Registration API
        const route_pattern registration_api = make_route_pattern(U("api"), U("registration"));
        // IS-05 Connection API
        const route_pattern connection_api = make_route_pattern(U("api"), U("connection"));
        // IS-07 Events API
        const route_pattern events_api = make_route_pattern(U("api"), U("events"));
		// IS-08 Channel Mapping API
		const route_pattern channelmapping_api = make_route_pattern(U("api"), U("channelmapping"));
        // IS-09 System API (originally specified in JT-NM TR-1001-1:2018 Annex A)
        const route_pattern system_api = make_route_pattern(U("api"), U("system"));

        // API version pattern
        const route_pattern version = make_route_pattern(U("version"), U("v[0-9]+\\.[0-9]+"));

        // Registration API supports registering all resource types
        const route_pattern resourceType = make_route_pattern(U("resourceType"), U("nodes|devices|sources|flows|senders|receivers"));
        // Query API supports all the resource types that can be registered, plus subscriptions
        const route_pattern queryType = make_route_pattern(U("resourceType"), U("nodes|devices|sources|flows|senders|receivers|subscriptions"));
        // Node API supports all the subresource types, plus self (which is a node)
        const route_pattern subresourceType = make_route_pattern(U("resourceType"), U("devices|sources|flows|senders|receivers"));

        // Connection API just manipulates senders and receivers (and only senders have the /transportfile endpoint)
        const route_pattern connectorType = make_route_pattern(U("resourceType"), U("senders|receivers"));
        const route_pattern senderType = make_route_pattern(U("resourceType"), U("senders"));
        const route_pattern stagingType = make_route_pattern(U("stagingType"), U("active|staged"));

        // Events API only allows access to the type and state of Event & Tally-compatible sources
        const route_pattern sourceType = make_route_pattern(U("resourceType"), U("sources"));
        // Each Events API source has a /type and /state endpoint
        const route_pattern eventTypeState = make_route_pattern(U("eventTypeState"), U("type|state"));

        // Channel Mapping API
        const route_pattern inputOutputType = make_route_pattern(U("resourceType"), U("inputs|outputs"));
        const route_pattern inputType = make_route_pattern(U("resourceType"), U("inputs"));
        const route_pattern outputType = make_route_pattern(U("resourceType"), U("outputs"));
        const route_pattern inputOutputId = make_route_pattern(U("inputOutputId"), U("[a-zA-Z0-9\\-_]+"));
        const route_pattern inputSubroute = make_route_pattern(U("inputSubroute"), U("properties|parent|channels|caps"));
        const route_pattern outputSubroute = make_route_pattern(U("outputSubroute"), U("properties|sourceid|channels|caps"));
        const route_pattern activationId = make_route_pattern(U("activationId"), U("[a-zA-Z0-9\\-_]+"));

        // Common patterns
        const route_pattern resourceId = make_route_pattern(U("resourceId"), U("[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}"));
    }

    // Note that a resourceType does not have the same value as a "proper" type, use this mapping instead!
    // Also note that a subscription for a single type has resource_path == U('/') + resourceType
    // and the websocket messages have topic == resource_path + U('/')

    // Map from a resourceType, i.e. the plural string used in the API endpoint routes, to a "proper" type
    nmos::type type_from_resourceType(const utility::string_t& resourceType);

    // Map from a "proper" type to a resourceType, i.e. the plural string used in the API endpoint routes
    utility::string_t resourceType_from_type(const nmos::type& type);

    // Other utility functions for generating NMOS response headers and body

    // experimental extension, to support human-readable HTML rendering of NMOS responses
    namespace experimental
    {
        namespace details
        {
            bool is_html_response_preferred(const web::http::http_request& req, const utility::string_t& mime_type);
            web::json::value make_html_response_a_tag(const web::uri& href, const web::json::value& value);
            web::json::value make_html_response_a_tag(const utility::string_t& sub_route, const web::http::http_request& req);
        }
    }

    // construct a standard NMOS "child resources" response, from the specified sub-routes
    // merging with ones from an existing response
    // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.0.%20APIs.md#api-paths
    web::json::value make_sub_routes_body(std::set<utility::string_t> sub_routes, const web::http::http_request& req, web::http::http_response res);

    // construct sub-routes for the specified API versions
    std::set<utility::string_t> make_api_version_sub_routes(const std::set<nmos::api_version>& versions);

    // construct a standard NMOS error response, using the default reason phrase if no user error information is specified
    web::json::value make_error_response_body(web::http::status_code code, const utility::string_t& error = {}, const utility::string_t& debug = {});

    // set up a standard NMOS error response, using the default reason phrase if no user error information is specified
    void set_error_reply(web::http::http_response& res, web::http::status_code code, const utility::string_t& error = {}, const utility::string_t& debug = {});

    // set up a standard NMOS error response, using the default reason phrase and the specified debug information
    void set_error_reply(web::http::http_response& res, web::http::status_code code, const std::exception& debug);

    // add handler to set appropriate response headers, and error response body if indicated - call this only after adding all others!
    void add_api_finally_handler(web::http::experimental::listener::api_router& api, slog::base_gate& gate);

    // modify the specified API to handle all requests (including CORS preflight requests via "OPTIONS") and attach it to the specified listener
    void support_api(web::http::experimental::listener::http_listener& listener, web::http::experimental::listener::api_router api, slog::base_gate& gate);

    // construct an http_listener on the specified address and port, modifying the specified API to handle all requests
    // (including CORS preflight requests via "OPTIONS")
    web::http::experimental::listener::http_listener make_api_listener(bool secure, const utility::string_t& host_address, int port, web::http::experimental::listener::api_router api, web::http::experimental::listener::http_listener_config config, slog::base_gate& gate);

    // construct an http_listener on the specified port, modifying the specified API to handle all requests
    // (including CORS preflight requests via "OPTIONS")
    inline web::http::experimental::listener::http_listener make_api_listener(int port, web::http::experimental::listener::api_router api, web::http::experimental::listener::http_listener_config config, slog::base_gate& gate)
    {
        return make_api_listener(false, web::http::experimental::listener::host_wildcard, port, api, config, gate);
    }

    // construct a websocket_listener on the specified address and port - captures handlers by reference!
    web::websockets::experimental::listener::websocket_listener make_ws_api_listener(bool secure, const utility::string_t& host_address, int port, const web::websockets::experimental::listener::websocket_listener_handlers& handlers, web::websockets::experimental::listener::websocket_listener_config config, slog::base_gate& gate);

    // returns "http" or "https" depending on settings
    utility::string_t http_scheme(const nmos::settings& settings);

    // returns "ws" or "wss" depending on settings
    utility::string_t ws_scheme(const nmos::settings& settings);

    // returns "mqtt" or "secure-mqtt"
    utility::string_t mqtt_scheme(bool secure);

    // returns "mqtt" or "secure-mqtt" depending on settings
    utility::string_t mqtt_scheme(const nmos::settings& settings);

    namespace details
    {
        // exception to skip other route handlers and then send the response (see add_api_finally_handler)
        struct to_api_finally_handler {};

        // decode URI-encoded string value elements in a JSON object
        void decode_elements(web::json::value& value);

        // URI-encode string value elements in a JSON object
        void encode_elements(web::json::value& value);

        // extract JSON after checking the Content-Type header
        pplx::task<web::json::value> extract_json(const web::http::http_request& req, slog::base_gate& gate);

        // extract JSON after checking the Content-Type header
        pplx::task<web::json::value> extract_json(const web::http::http_response& res, slog::base_gate& gate);

        // add the NMOS-specified CORS response headers
        web::http::http_response& add_cors_preflight_headers(const web::http::http_request& req, web::http::http_response& res);
        web::http::http_response& add_cors_headers(web::http::http_response& res);

        // make user error information (to be used with status_codes::NotFound)
        utility::string_t make_erased_resource_error();

        // make handler to check supported API version, and set error response otherwise
        web::http::experimental::listener::route_handler make_api_version_handler(const std::set<api_version>& versions, slog::base_gate& gate);

        // make handler to set appropriate response headers, and error response body if indicated
        web::http::experimental::listener::route_handler make_api_finally_handler(slog::base_gate& gate);
    }
}

#endif

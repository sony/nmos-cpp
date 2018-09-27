#ifndef NMOS_API_UTILS_H
#define NMOS_API_UTILS_H

#include <map>
#include "cpprest/api_router.h"
#include "cpprest/regex_utils.h"
#include "nmos/type.h"

namespace slog
{
    class base_gate;
}

// Utility types, constants and functions for implementing NMOS REST APIs
namespace nmos
{
    // Patterns are used to form parameterised route paths
    // (could be moved to cpprest/api_router.h or cpprest/route_pattern.h?)

    struct route_pattern { utility::string_t name; utility::string_t pattern; };

    inline route_pattern make_route_pattern(const utility::string_t& name, const utility::string_t& base_pattern)
    {
        return{ name, utility::make_named_sub_match(name, base_pattern) };
    }

    namespace patterns
    {
        const route_pattern node_api = make_route_pattern(U("api"), U("node"));
        const route_pattern query_api = make_route_pattern(U("api"), U("query"));
        const route_pattern registration_api = make_route_pattern(U("api"), U("registration"));
        const route_pattern connection_api = make_route_pattern(U("api"), U("connection"));

        // AMWA IS-04 Discovery and Registration specifies the Node, Query and Registration APIs
        const route_pattern is04_version = make_route_pattern(U("version"), U("v1\\.[0-2]")); // for now, just v1.0, v1.1, v1.2

        // AMWA IS-05 Connection Management specifies the Connection APIs
        const route_pattern is05_version = make_route_pattern(U("version"), U("v1\\.0")); // for now, just v1.0

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

    // construct a standard NMOS error response, using the default reason phrase if no user error information is specified
    web::json::value make_error_response_body(web::http::status_code code, const utility::string_t& error = {}, const utility::string_t& debug = {});

    // add handler to set appropriate response headers, and error response body if indicated - call this only after adding all others!
    void add_api_finally_handler(web::http::experimental::listener::api_router& api, slog::base_gate& gate);

    // use an API to handle all requests (including CORS preflight requests via "OPTIONS") - captures api by reference!
    void support_api(web::http::experimental::listener::http_listener& listener, web::http::experimental::listener::api_router& api);

    namespace details
    {
        // decode URI-encoded string value elements in a JSON object
        void decode_elements(web::json::value& value);

        // URI-encode string value elements in a JSON object
        void encode_elements(web::json::value& value);

        // extract JSON after checking the Content-Type header
        pplx::task<web::json::value> extract_json(const web::http::http_request& req, const web::http::experimental::listener::route_parameters& parameters, slog::base_gate& gate);

        // add the NMOS-specified CORS response headers
        web::http::http_response& add_cors_preflight_headers(const web::http::http_request& req, web::http::http_response& res);
        web::http::http_response& add_cors_headers(web::http::http_response& res);

        // make handler to set appropriate response headers, and error response body if indicated
        web::http::experimental::listener::route_handler make_api_finally_handler(slog::base_gate& gate);
    }
}

#endif

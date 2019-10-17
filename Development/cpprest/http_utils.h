#ifndef CPPREST_HTTP_UTILS_H
#define CPPREST_HTTP_UTILS_H

#include "pplx/pplx_utils.h"
#include "cpprest/http_msg.h"
#include "cpprest/uri_schemes.h"

// Utility types, constants and functions for HTTP
namespace web
{
    namespace http
    {
        std::pair<utility::string_t, int> get_host_port(const web::http::http_request& req);

        namespace details
        {
            // Extract the basic 'type/subtype' media type from a Content-Type value
            utility::string_t get_mime_type(const utility::string_t& content_type);

            // Check if a media type is JSON
            bool is_mime_type_json(const utility::string_t& mime_type);
        }

        // Determine if a value is found in a header that represents a set of values, like "Allow", "Accept" or "Link"
        bool has_header_value(const http_headers& headers, const utility::string_t& name, const utility::string_t& value);
        template <typename T>
        bool has_header_value(const http_headers& headers, const utility::string_t& name, const T& value)
        {
            return has_header_value(headers, name, utility::conversions::details::print_string(value));
        }

        // Add a value to a header that represents a set of values, like "Allow", "Accept" or "Link"
        bool add_header_value(http_headers& headers, const utility::string_t& name, utility::string_t value);
        template <typename T>
        bool add_header_value(http_headers& headers, const utility::string_t& name, const T& value)
        {
            return add_header_value(headers, name, utility::conversions::details::print_string(value));
        }

        // Set response fields like the equivalent http_request::reply() functions
        void set_reply(web::http::http_response& res, web::http::status_code code);
        void set_reply(web::http::http_response& res, web::http::status_code code, const concurrency::streams::istream& body, const utility::string_t& content_type = _XPLATSTR("application/octet-stream"));
        void set_reply(web::http::http_response& res, web::http::status_code code, const concurrency::streams::istream& body, utility::size64_t content_length, const utility::string_t& content_type = _XPLATSTR("application/octet-stream"));
        void set_reply(web::http::http_response& res, web::http::status_code code, const utility::string_t& body_text, const utility::string_t& content_type = _XPLATSTR("text/plain"));
        void set_reply(web::http::http_response& res, web::http::status_code code, const web::json::value& body_data);

        // "The first digit of the status-code defines the class of response.
        // The last two digits do not have any categorization role .There are
        // five values for the first digit"
        // See https://tools.ietf.org/html/rfc7231#section-6
        // and https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml

        // "1xx (Informational): The request was received, continuing process"
        inline bool is_informational_status_code(web::http::status_code code)
        {
            return 100 <= code && code <= 199;
        }

        // "2xx (Successful): The request was successfully received, understood, and accepted"
        inline bool is_success_status_code(web::http::status_code code)
        {
            return 200 <= code && code <= 299;
        }

        // "3xx (Redirection): Further action needs to be taken in order to complete the request"
        inline bool is_redirection_status_code(web::http::status_code code)
        {
            return 300 <= code && code <= 399;
        }

        // "4xx (Client Error): The request contains bad syntax or cannot be fulfilled"
        inline bool is_client_error_status_code(web::http::status_code code)
        {
            return 400 <= code && code <= 499;
        }

        // "5xx (Server Error): The server failed to fulfill an apparently valid request"
        inline bool is_server_error_status_code(web::http::status_code code)
        {
            return 500 <= code && code <= 599;
        }

        inline bool is_error_status_code(web::http::status_code code)
        {
            return 400 <= code && code <= 599;
        }

        namespace cors
        {
            // Functions related to Cross-Origin Resource Sharing (CORS) headers
            // See https://fetch.spec.whatwg.org/
            bool is_cors_response_header(const web::http::http_headers::key_type& header);
            bool is_cors_safelisted_response_header(const web::http::http_headers::key_type& header);

            namespace header_names
            {
                // CORS request headers
                const web::http::http_headers::key_type request_method{ _XPLATSTR("Access-Control-Request-Method") };
                const web::http::http_headers::key_type request_headers{ _XPLATSTR("Access-Control-Request-Headers") };
                // CORS response headers
                const web::http::http_headers::key_type allow_origin{ _XPLATSTR("Access-Control-Allow-Origin") };
                const web::http::http_headers::key_type allow_credentials{ _XPLATSTR("Access-Control-Allow-Credentials") };
                const web::http::http_headers::key_type allow_methods{ _XPLATSTR("Access-Control-Allow-Methods") };
                const web::http::http_headers::key_type allow_headers{ _XPLATSTR("Access-Control-Allow-Headers") };
                const web::http::http_headers::key_type max_age{ _XPLATSTR("Access-Control-Max-Age") };
                const web::http::http_headers::key_type expose_headers{ _XPLATSTR("Access-Control-Expose-Headers") };
            }
        }

        // empty_status_code is the default value for http_msg::status_code()
        const web::http::status_code empty_status_code = (std::numeric_limits<uint16_t>::max)();

        // Get the standard reason phrase corresponding to the status code, same as used by http_response
        utility::string_t get_default_reason_phrase(web::http::status_code code);

        namespace experimental
        {
            // Parameterised-Tokens = #ptoken
            // ptoken               = ptoken-name *( OWS ";" OWS ptoken-param )
            // ptoken-name          = token
            // ptoken-param         = ptoken-param-name OWS "=" OWS ptoken-param-value
            // ptoken-param-name    = token
            // ptoken-param-value   = token / quoted-string
            // E.g. Transfer-Encoding uses this format
            // See https://tools.ietf.org/html/rfc7230#section-4

            typedef utility::string_t token;
            typedef std::pair<token, utility::string_t> ptoken_param;
            typedef std::vector<ptoken_param> ptoken_params;
            typedef std::pair<token, ptoken_params> ptoken;
            typedef std::vector<ptoken> ptokens;

            utility::string_t make_ptokens_header(const ptokens& values);
            ptokens parse_ptokens_header(const utility::string_t& value);


            namespace header_names
            {
                // Server Timing
                // See https://w3c.github.io/server-timing/#the-server-timing-header-field
                const web::http::http_headers::key_type server_timing{ _XPLATSTR("Server-Timing") };

                // Resource Timing 2
                // See https://www.w3.org/TR/resource-timing-2/#sec-timing-allow-origin
                const web::http::http_headers::key_type timing_allow_origin{ _XPLATSTR("Timing-Allow-Origin") };
            }

            struct timing_metric
            {
                utility::string_t name;
                double duration; // milliseconds
                utility::string_t description;
                timing_metric(utility::string_t name, double duration = 0.0, utility::string_t description = {}) : name(name), duration(duration), description(description) {}
                timing_metric(utility::string_t name, utility::string_t description) : name(name), duration(0.0), description(description) {}

                auto tied() const -> decltype(std::tie(name, duration, description)) { return std::tie(name, duration, description); }
                friend bool operator==(const timing_metric& lhs, const timing_metric& rhs) { return lhs.tied() == rhs.tied(); }
                friend bool operator!=(const timing_metric& lhs, const timing_metric& rhs) { return !(lhs == rhs); }
            };

            typedef std::vector<timing_metric> timing_metrics;

            utility::string_t make_timing_header(const timing_metrics& values);
            timing_metrics parse_timing_header(const utility::string_t& value);
        }

        // Determine whether http_request::reply() has been called already
        bool has_initiated_response(const web::http::http_request& req);

        namespace experimental
        {
            namespace listener
            {
                class http_listener;

                // RAII helper for http_listener sessions
                typedef pplx::open_close_guard<http_listener> http_listener_guard;

                // platform-specific wildcard address to accept connections for any address
#if defined(_WIN32) && !defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
                const utility::string_t host_wildcard{ _XPLATSTR("*") }; // "weak wildcard"
#else
                const utility::string_t host_wildcard{ _XPLATSTR("0.0.0.0") };
#endif

                // make an address to be used to accept HTTP or HTTPS connections for the specified address and port
                inline web::uri make_listener_uri(bool secure, const utility::string_t& host_address, int port)
                {
                    return web::uri_builder().set_scheme(web::http_scheme(secure)).set_host(host_address).set_port(port).to_uri();
                }

                // make an address to be used to accept HTTP connections for the specified address and port
                inline web::uri make_listener_uri(const utility::string_t& host_address, int port)
                {
                    return make_listener_uri(false, host_address, port);
                }

                // make an address to be used to accept HTTP connections for the specified port for any address
                inline web::uri make_listener_uri(int port)
                {
                    return make_listener_uri(host_wildcard, port);
                }
            }
        }
    }
}

#endif

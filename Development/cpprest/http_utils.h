#ifndef CPPREST_HTTP_UTILS_H
#define CPPREST_HTTP_UTILS_H

#include "cpprest/http_msg.h"

// Extensions to the HTTP request and reply message interfaces
namespace web
{
    namespace http
    {
        std::pair<utility::string_t, int> get_host_port(const web::http::http_request& req);

        // Set response fields like the equivalent http_request::reply() functions
        void set_reply(web::http::http_response& res, web::http::status_code code);
        void set_reply(web::http::http_response& res, web::http::status_code code, const concurrency::streams::istream& body, const utility::string_t& content_type = U("application/octet-stream"));
        void set_reply(web::http::http_response& res, web::http::status_code code, const concurrency::streams::istream& body, utility::size64_t content_length, const utility::string_t& content_type = U("application/octet-stream"));
        void set_reply(web::http::http_response& res, web::http::status_code code, const utility::string_t& body_text, const utility::string_t& content_type = U("text/plain"));
        void set_reply(web::http::http_response& res, web::http::status_code code, const web::json::value& body_data);

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
                const web::http::http_headers::key_type request_method{ U("Access-Control-Request-Method") };
                const web::http::http_headers::key_type request_headers{ U("Access-Control-Request-Headers") };
                // CORS response headers
                const web::http::http_headers::key_type allow_origin{ U("Access-Control-Allow-Origin") };
                const web::http::http_headers::key_type allow_credentials{ U("Access-Control-Allow-Credentials") };
                const web::http::http_headers::key_type allow_methods{ U("Access-Control-Allow-Methods") };
                const web::http::http_headers::key_type allow_headers{ U("Access-Control-Allow-Headers") };
                const web::http::http_headers::key_type max_age{ U("Access-Control-Max-Age") };
                const web::http::http_headers::key_type expose_headers{ U("Access-Control-Expose-Headers") };
            }
        }

        // empty_status_code is the default value for http_msg::status_code()
        const web::http::status_code empty_status_code = (std::numeric_limits<uint16_t>::max)();

        // Get the standard reason phrase corresponding to the status code, same as used by http_response
        utility::string_t get_default_reason_phrase(web::http::status_code code);

        // Determine whether http_request::reply() has been called already
        bool has_initiated_response(const web::http::http_request& req);
    }
}

#endif

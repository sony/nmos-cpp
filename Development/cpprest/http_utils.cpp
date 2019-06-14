#include "cpprest/http_utils.h"

#include <algorithm>
#include <map>
#include <set>
#include "cpprest/basic_utils.h" // for utility::conversions
#include "detail/private_access.h"

namespace web
{
    namespace http
    {
        std::pair<utility::string_t, int> get_host_port(const web::http::http_request& req)
        {
            //return{ req.absolute_uri().host(), req.absolute_uri().port() };
            // That naive approach doesn't work at least on Windows.
            // See https://github.com/Microsoft/cpprestsdk/issues/401
            // Instead, try to use the 'Host' header.
            // In order to support deployment behind a reverse proxy, also look at the 'X-Forwarded-Host' header.
            // The RFC 7239 equivalent is the 'host=' directive in the 'Forwarded' header, but Apache and Lighttpd
            // seem to add 'X-Forwarded-Host' out of the box.
            // See https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/X-Forwarded-Host
            auto header = req.headers().find(U("X-Forwarded-Host"));
            if (req.headers().end() == header) header = req.headers().find(web::http::header_names::host);
            if (req.headers().end() != header)
            {
                auto first = header->second.substr(0, header->second.find(','));
                auto colon = first.find(':');
                if (utility::string_t::npos == colon) return{ std::move(first), 0 };
                return{ first.substr(0, colon), utility::conversions::details::scan_string(first.substr(colon + 1), 0) };
            }
            else
            {
                return{};
            }
        }

        namespace details
        {
            // Extract the basic 'type/subtype' from a Content-Type value
            utility::string_t get_mime_type(const utility::string_t& content_type)
            {
                auto first = std::find_if_not(content_type.begin(), content_type.end(), [](utility::char_t c) { return U(' ') == c || U('\t') == c; });
                // media-type = type "/" subtype *( OWS ";" OWS parameter )
                // OWS        = *( SP / HTAB )
                auto last = std::find_if(first, content_type.end(), [](utility::char_t c) { return U(';') == c || U(' ') == c || U('\t') == c; });
                return{ first, last };
            }
        }

        bool has_header_value(const http_headers& headers, const utility::string_t& name, const utility::string_t& value)
        {
            const auto header = headers.find(name);
            if (headers.end() == header || header->second.empty())
            {
                return false;
            }
            else
            {
                // this provides protection against substring matches but relies on a comma being followed by single space
                // consistently, and doesn't handle quoted string values that may contain this delimiter
                const auto comma = _XPLATSTR(", ");
                const auto searchable = comma + header->second + comma;
                return utility::string_t::npos != searchable.find(comma + value + comma);
            }
        }

        bool add_header_value(http_headers& headers, const utility::string_t& name, utility::string_t value)
        {
            if (has_header_value(headers, name, value))
            {
                return false;
            }
            else
            {
                headers.add(name, value);
                return true;
            }
        }

        void set_reply(web::http::http_response& res, web::http::status_code code)
        {
            res.set_status_code(code);
        }

        void set_reply(web::http::http_response& res, web::http::status_code code, const concurrency::streams::istream& body, const utility::string_t& content_type)
        {
            res.set_status_code(code);
            res.set_body(body, content_type);
        }

        void set_reply(web::http::http_response& res, web::http::status_code code, const concurrency::streams::istream& body, utility::size64_t content_length, const utility::string_t& content_type)
        {
            res.set_status_code(code);
            res.set_body(body, content_length, content_type);
        }

        void set_reply(web::http::http_response& res, web::http::status_code code, const utility::string_t& body_text, const utility::string_t& content_type)
        {
            res.set_status_code(code);
            res.set_body(body_text, content_type);
        }

        void set_reply(web::http::http_response& res, web::http::status_code code, const web::json::value& body_data)
        {
            res.set_status_code(code);
            res.set_body(body_data);
        }

        namespace cors
        {
            bool is_cors_response_header(const web::http::http_headers::key_type& header)
            {
                // See https://fetch.spec.whatwg.org/
                static const std::set<web::http::http_headers::key_type> cors_response_headers
                {
                    web::http::cors::header_names::allow_origin,
                    web::http::cors::header_names::allow_credentials,
                    web::http::cors::header_names::allow_methods,
                    web::http::cors::header_names::allow_headers,
                    web::http::cors::header_names::max_age,
                    web::http::cors::header_names::expose_headers
                };
                return cors_response_headers.end() != cors_response_headers.find(header);
            }

            bool is_cors_safelisted_response_header(const web::http::http_headers::key_type& header)
            {
                // See https://fetch.spec.whatwg.org/#cors-safelisted-response-header-name
                static const std::set<web::http::http_headers::key_type> cors_safelisted_response_headers
                {
                    // don't need to include these simple headers in the Expose-Headers header
                    web::http::header_names::cache_control,
                    web::http::header_names::content_language,
                    web::http::header_names::content_type, // unless the value is not one of the CORS-safelisted types?
                    web::http::header_names::expires,
                    web::http::header_names::last_modified,
                    web::http::header_names::pragma
                };
                return cors_safelisted_response_headers.end() != cors_safelisted_response_headers.find(header);
            }
        }

        // based on existing function from cpprestsdk's internal_http_helpers.h
        utility::string_t get_default_reason_phrase(web::http::status_code code)
        {
            static const std::map<web::http::status_code, const utility::char_t*> default_reason_phrases
            {
#define _PHRASES
#define DAT(a,b,c) {status_codes::a, c},
#include "cpprest/details/http_constants.dat"
#undef _PHRASES
#undef DAT
            };

            auto found = default_reason_phrases.find(code);
            return default_reason_phrases.end() != found ? found->second : _XPLATSTR("");
        }

        namespace details
        {
            struct http_request_impl { typedef std::shared_ptr<web::http::details::_http_request>(web::http::http_request::*type); };
            struct http_request_initiated_response { typedef pplx::details::atomic_long(web::http::details::_http_request::*type); };
        }

        bool has_initiated_response(const web::http::http_request& req)
        {
            return 0 < *(req.*detail::stowed<details::http_request_impl>::value).*detail::stowed<details::http_request_initiated_response>::value;
        }
    }
}

// Sigh. "An explicit instantiation shall appear in an enclosing namespace of its template."
template struct detail::stow_private<web::http::details::http_request_impl, &web::http::http_request::_m_impl>;
template struct detail::stow_private<web::http::details::http_request_initiated_response, &web::http::details::_http_request::m_initiated_response>;

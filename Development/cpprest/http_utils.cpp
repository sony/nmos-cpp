#include "cpprest/http_utils.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <boost/algorithm/string/predicate.hpp>
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
            // Extract the basic 'type/subtype' media type from a Content-Type value
            utility::string_t get_mime_type(const utility::string_t& content_type)
            {
                auto first = std::find_if_not(content_type.begin(), content_type.end(), [](utility::char_t c) { return U(' ') == c || U('\t') == c; });
                // media-type = type "/" subtype *( OWS ";" OWS parameter )
                // OWS        = *( SP / HTAB )
                auto last = std::find_if(first, content_type.end(), [](utility::char_t c) { return U(';') == c || U(' ') == c || U('\t') == c; });
                return{ first, last };
            }

            // Check if a media type is JSON
            bool is_mime_type_json(const utility::string_t& mime_type)
            {
                // as well as application/json, also check for the +json structured syntax suffix
                // see https://tools.ietf.org/html/rfc6839#section-3.1
                return web::http::details::mime_types::application_json == mime_type || boost::algorithm::iends_with(mime_type, U("+json"));
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
                // (http_headers::add does the former, and doesn't explicitly support the latter)
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
            // this http_response::set_body overload blindly adds "; charset=utf-8" (because it converts body_text to UTF-8)
            // which for "application/json" isn't necessary, or strictly valid
            // see https://www.iana.org/assignments/media-types/application/json
            // same goes for "application/sdp"
            // see https://www.iana.org/assignments/media-types/application/sdp
            res.set_body(body_text, content_type);
            if (web::http::details::mime_types::application_json == content_type || U("application/sdp") == content_type)
            {
                res.headers().set_content_type(content_type);
            }
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

        namespace experimental
        {
            namespace details
            {
                // token = 1*tchar
                // tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*"
                //       / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
                //       / DIGIT / ALPHA
                // see https://tools.ietf.org/html/rfc7230#section-3.2.6
                inline bool is_tchar(utility::char_t c)
                {
                    static const utility::string_t tchar_punct{ U("!#$%&'*+-.^_`|~") };
                    return std::isalnum(c) || std::string::npos != tchar_punct.find(c);
                }

                // "A sender SHOULD NOT generate a quoted-pair in a quoted-string except where necessary to quote DQUOTE and backslash"
                // see https://tools.ietf.org/html/rfc7230#section-3.2.6
                inline bool is_backslash_required(utility::char_t c)
                {
                    return U('"') == c || U('\\') == c;
                }
            }

            utility::string_t make_ptokens_header(const ptokens& values)
            {
                utility::string_t result;
                for (const auto& value : values)
                {
                    // comma is followed by a single space (as in http_headers::add)
                    if (!result.empty()) { result.push_back(U(',')); result.push_back(U(' ')); }
                    result += value.first;
                    for (const auto& param : value.second)
                    {
                        result.push_back(U(';'));
                        result.append(param.first);
                        result.push_back(U('='));
                        if (param.second.empty() || param.second.end() != std::find_if(param.second.begin(), param.second.end(), [](utility::char_t c) { return !details::is_tchar(c); }))
                        {
                            result.push_back(U('"'));
                            for (auto c : param.second)
                            {
                                if (details::is_backslash_required(c))
                                {
                                    result.push_back(U('\\'));
                                }
                                result.push_back(c);
                            }
                            result.push_back(U('"'));
                        }
                        else
                        {
                            result.append(param.second);
                        }
                    }
                }
                return result;
            }

            ptokens parse_ptokens_header(const utility::string_t& value)
            {
                enum {
                    pre_value,
                    value_name,
                    pre_param,
                    pre_param_name,
                    param_name,
                    pre_param_value,
                    param_value,
                    param_value_token,
                    param_value_quoted_string,
                    param_value_quoted_string_escape
                } state = pre_value;

                ptokens result;
                utility::string_t name;
                for (auto c : value)
                {
                    switch (state)
                    {
                    case pre_value:
                        // surprising handling of multiple commas is due to the ABNF List Extension: #rule
                        // #element => [ ( "," / element ) *( OWS "," [ OWS element ] ) ]
                        // see https://tools.ietf.org/html/rfc7230#section-7
                        if (details::is_tchar(c)) { name.push_back(c); state = value_name; break; }
                        if (U(',') == c) { break; }
                        if (U(' ') == c || U('\t') == c) { break; }
                        throw std::invalid_argument("invalid value name, expected tchar");
                    case value_name:
                        if (details::is_tchar(c)) { name.push_back(c); break; }
                        result.push_back({ name, {} }); name.clear();
                        if (U(',') == c) { state = pre_value;  break; }
                        if (U(';') == c) { state = pre_param_name; break; }
                        if (U(' ') == c || U('\t') == c) { state = pre_param; break; }
                        throw std::invalid_argument("invalid value name, expected tchar");
                    case pre_param:
                        if (U(',') == c) { state = pre_value; break; }
                        if (U(';') == c) { state = pre_param_name; break; }
                        if (U(' ') == c || U('\t') == c) { break; }
                        throw std::invalid_argument("invalid value, expected ',' or ';'");
                    case pre_param_name:
                        if (details::is_tchar(c)) { name.push_back(c); state = param_name; break; }
                        if (U(' ') == c || U('\t') == c) { break; }
                        throw std::invalid_argument("invalid parameter name, expected tchar");
                    case param_name:
                        if (details::is_tchar(c)) { name.push_back(c); break; }
                        result.back().second.push_back({ name, {} }); name.clear();
                        if (U('=') == c) { state = param_value; break; }
                        if (U(' ') == c || U('\t') == c) { state = pre_param_value; break; }
                        throw std::invalid_argument("invalid parameter name, expected tchar");
                    case pre_param_value:
                        if (U('=') == c) { state = param_value; break; }
                        if (U(' ') == c || U('\t') == c) { break; }
                        throw std::invalid_argument("invalid parameter, expected '='");
                    case param_value:
                        if (details::is_tchar(c)) { result.back().second.back().second.push_back(c); state = param_value_token; break; }
                        if (U('"') == c) { state = param_value_quoted_string; break; }
                        if (U(' ') == c || U('\t') == c) { break; }
                        throw std::invalid_argument("invalid parameter value, expected tchar or '\"'");
                    case param_value_token:
                        if (details::is_tchar(c)) { result.back().second.back().second.push_back(c); break; }
                        if (U(',') == c) { state = pre_value; break; }
                        if (U(';') == c) { state = pre_param_name; break; }
                        if (U(' ') == c || U('\t') == c) { state = pre_param; break; }
                        throw std::invalid_argument("invalid parameter value, expected tchar");
                    case param_value_quoted_string:
                        if (U('"') == c) { state = pre_param; break; }
                        if (U('\\') == c) { state = param_value_quoted_string_escape; break; }
                        result.back().second.back().second.push_back(c);
                        break;
                    case param_value_quoted_string_escape:
                        result.back().second.back().second.push_back(c);
                        state = param_value_quoted_string;
                        break;
                    default:
                        throw std::logic_error("unreachable code");
                    }
                }

                if (!name.empty())
                {
                    switch (state)
                    {
                    case value_name:
                        result.push_back({ name, {} }); name.clear(); break;
                    case param_name:
                        throw std::invalid_argument("invalid parameter, expected '='");
                    default:
                        throw std::logic_error("unreachable code");
                    }
                }
                return result;
            }

            namespace details
            {
                template <typename TimePoint>
                inline double milliseconds_since_epoch(const TimePoint& tp)
                {
                    return std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()) / 1000.0;
                }
            }

            utility::string_t make_timing_header(const timing_metrics& values)
            {
                ptokens results;
                for (auto& value : values)
                {
                    ptoken token{ value.name, {} };
                    if (0.0 != value.duration) token.second.push_back({ U("dur"), utility::ostringstreamed(value.duration) });
                    if (!value.description.empty()) token.second.push_back({ U("desc"), value.description });
                    results.push_back(std::move(token));
                }
                return make_ptokens_header(results);
            }

            timing_metrics parse_timing_header(const utility::string_t& value)
            {
                // See https://w3c.github.io/server-timing/#server-timing-header-parsing-algorithm
                timing_metrics results;
                auto ptokens = parse_ptokens_header(value);
                for (auto& ptoken : ptokens)
                {
                    timing_metric metric{ ptoken.first };
                    // "Set duration to the server-timing-param-value for the server-timing-param where server-timing-param-name
                    // is case-insensitively equal to "dur", or value 0 if omitted or not representable as a double"
                    const auto dur = std::find_if(ptoken.second.begin(), ptoken.second.end(), [](const ptoken_param& param) { return boost::algorithm::iequals(param.first, U("dur")); });
                    if (ptoken.second.end() != dur) metric.duration = utility::istringstreamed(dur->second, 0.0);
                    const auto desc = std::find_if(ptoken.second.begin(), ptoken.second.end(), [](const ptoken_param& param) { return boost::algorithm::iequals(param.first, U("desc")); });
                    if (ptoken.second.end() != desc) metric.description = desc->second;
                    results.push_back(std::move(metric));
                }
                return results;
            }
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

#include "nmos/api_utils.h"

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/json_visit.h"
#include "cpprest/uri_schemes.h"
#include "cpprest/ws_utils.h"
#include "nmos/api_version.h"
#include "nmos/slog.h"
#include "nmos/type.h"
#include "nmos/version.h"

namespace web
{
    // because web::uri::encode_uri(de, web::uri::components::query) doesn't encode '&' (or ';', or '=')
    utility::string_t uri_encode_query_value(const utility::string_t& value);
}

namespace nmos
{
    namespace details
    {
        // decode URI-encoded string value elements in a JSON object
        void decode_elements(web::json::value& value)
        {
            for (auto& element : value.as_object())
            {
                if (element.second.is_string())
                {
                    element.second = web::json::value::string(web::uri::decode(element.second.as_string()));
                }
            }
        }

        // URI-encode string value elements in a JSON object
        void encode_elements(web::json::value& value)
        {
            for (auto& element : value.as_object())
            {
                if (element.second.is_string())
                {
                    auto de = element.second.as_string();
                    auto en = web::uri_encode_query_value(de);
                    element.second = web::json::value::string(en);
                }
            }
        }

        // extract JSON after checking the Content-Type header
        template <typename HttpMessage>
        inline pplx::task<web::json::value> extract_json(const HttpMessage& msg, slog::base_gate& gate)
        {
            auto mime_type = web::http::details::get_mime_type(msg.headers().content_type());

            if (web::http::details::is_mime_type_json(mime_type))
            {
                // No "charset" parameter is defined for the application/json media type
                // but it's quite common so don't even bother to log a warning...
                // See https://www.iana.org/assignments/media-types/application/json

                // by default, extract_json only allows application/json itself - the above check is better!
                return msg.extract_json(true);
            }
            else if (mime_type.empty())
            {
                // "If a Content-Type header field is not present, the recipient MAY
                // [...] examine the data to determine its type."
                // See https://tools.ietf.org/html/rfc7231#section-3.1.1.5

                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing Content-Type: should be application/json";

                return msg.extract_json(true);
            }
            else
            {
                // more helpful message than from web::http::details::http_msg_base::parse_and_check_content_type for unacceptable content-type
                return pplx::task_from_exception<web::json::value>(web::http::http_exception(U("Incorrect Content-Type: ") + msg.headers().content_type() + U(", should be application/json")));
            }
        }

        pplx::task<web::json::value> extract_json(const web::http::http_request& req, slog::base_gate& gate)
        {
            return extract_json<>(req, gate);
        }

        pplx::task<web::json::value> extract_json(const web::http::http_response& res, slog::base_gate& gate)
        {
            return extract_json<>(res, gate);
        }

        // add the NMOS-specified CORS response headers
        web::http::http_response& add_cors_preflight_headers(const web::http::http_request& req, web::http::http_response& res)
        {
            // NMOS specification says "all NMOS APIs MUST implement valid CORS HTTP headers in responses"
            // See also e.g. https://fetch.spec.whatwg.org/#cors-safelisted-request-header

            // Convert any "Allow" response header which has been prepared into the equivalent CORS preflight response header
            const auto methods = res.headers().find(web::http::header_names::allow);
            if (res.headers().end() != methods)
            {
                res.headers().add(web::http::cors::header_names::allow_methods, methods->second);
                res.headers().remove(web::http::header_names::allow);
            }
            // otherwise, don't add this response header?
            // or include the request method?
            // or a default set? e.g. ("GET, PUT, POST, HEAD, OPTIONS, DELETE")

            // Indicate that all the requested headers are allowed
            const auto headers = req.headers().find(web::http::cors::header_names::request_headers);
            if (req.headers().end() != headers)
            {
                res.headers().add(web::http::cors::header_names::allow_headers, headers->second);
                // only "application/x-www-form-urlencoded", "multipart/form-data" and "text/plain" are CORS-safelisted
                // but all POST requests to NMOS APIs will have JSON bodies, so the preflight request had better mention
                // "Content-Type" in this case
            }

            // Indicate preflight response may be cached for 24 hours (since the answer isn't likely to be dynamic)
            res.headers().add(web::http::cors::header_names::max_age, 24 * 60 * 60);

            return res;
        }

        // add the NMOS-specified CORS response headers
        web::http::http_response& add_cors_headers(web::http::http_response& res)
        {
            // Indicate that any Origin is allowed
            res.headers().add(web::http::cors::header_names::allow_origin, U("*"));

            // some browsers seem not to support the latest spec which allows the wildcard "*"
            for (const auto& header : res.headers())
            {
                if (!web::http::cors::is_cors_response_header(header.first) && !web::http::cors::is_cors_safelisted_response_header(header.first))
                {
                    web::http::add_header_value(res.headers(), web::http::cors::header_names::expose_headers, header.first);
                }
            }
            return res;
        }
    }

    // Map from a resourceType, i.e. the plural string used in the API endpoint routes, to a "proper" type
    nmos::type type_from_resourceType(const utility::string_t& resourceType)
    {
        static const std::map<utility::string_t, nmos::type> types_from_resourceType
        {
            { U("self"), nmos::types::node }, // for the Node API
            { U("nodes"), nmos::types::node },
            { U("devices"), nmos::types::device },
            { U("sources"), nmos::types::source },
            { U("flows"), nmos::types::flow },
            { U("senders"), nmos::types::sender },
            { U("receivers"), nmos::types::receiver },
            { U("subscriptions"), nmos::types::subscription },
            { U("inputs"), nmos::types::input },
            { U("outputs"), nmos::types::output }
        };
        return types_from_resourceType.at(resourceType);
    }

    // Map from a "proper" type to a resourceType, i.e. the plural string used in the API endpoint routes
    utility::string_t resourceType_from_type(const nmos::type& type)
    {
        static const std::map<nmos::type, utility::string_t> resourceTypes_from_type
        {
            { nmos::types::node, U("nodes") },
            { nmos::types::device, U("devices") },
            { nmos::types::source, U("sources") },
            { nmos::types::flow, U("flows") },
            { nmos::types::sender, U("senders") },
            { nmos::types::receiver, U("receivers") },
            { nmos::types::subscription, U("subscriptions") },
            { nmos::types::grain, {} }, // subscription websocket grains aren't exposed via the Query API
            { nmos::types::input, U("inputs") },
            { nmos::types::output, U("outputs") }
        };
        return resourceTypes_from_type.at(type);
    }

    // experimental extension, to support human-readable HTML rendering of NMOS responses
    namespace experimental
    {
        namespace details
        {
            bool is_html_response_preferred(const web::http::http_request& req, const utility::string_t& mime_type)
            {
                // hmm, parsing of the Accept header could be much better and should take account of quality values
                const auto accept = req.headers().find(web::http::header_names::accept);
                return req.headers().end() != accept
                    && !boost::algorithm::contains(accept->second, mime_type)
                    && boost::algorithm::contains(accept->second, U("text/html"));
            }

            web::json::value make_html_response_a_tag(const web::uri& href, const web::json::value& value)
            {
                using web::json::value_of;

                return value_of({
                    { U("$href"), href.to_string() },
                    { U("$_"), value }
                });
            }

            web::json::value make_html_response_a_tag(const utility::string_t& sub_route, const web::http::http_request& req)
            {
                using web::json::value;
                return make_html_response_a_tag(web::uri_builder(req.request_uri()).append_path(sub_route).to_uri(), value::string(sub_route));
            }
        }
    }

    // construct a standard NMOS "child resources" response, from the specified sub-routes
    // merging with ones from an existing response
    // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.0.%20APIs.md#api-paths
    web::json::value make_sub_routes_body(std::set<utility::string_t> sub_routes, const web::http::http_request& req, web::http::http_response res)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        std::set<value> results;

        if (res.body())
        {
            auto body = res.extract_json(false).get();
            results.insert(body.as_array().begin(), body.as_array().end());
        }

        // experimental extension, to support human-readable HTML rendering of NMOS responses
        if (experimental::details::is_html_response_preferred(req, web::http::details::mime_types::application_json))
        {
            for (auto& sub_route : sub_routes)
            {
                // build a full path, using request_uri, rather than just the simple relative href
                // in order to make working links when the current request didn't have a trailing slash
                results.insert(experimental::details::make_html_response_a_tag(sub_route, req));
            }
        }
        else
        {
            for (auto& sub_route : sub_routes)
            {
                results.insert(value::string(sub_route));
            }
        }

        return web::json::value_from_elements(results);
    }

    // construct sub-routes for the specified API versions
    std::set<utility::string_t> make_api_version_sub_routes(const std::set<nmos::api_version>& versions)
    {
        return boost::copy_range<std::set<utility::string_t>>(versions | boost::adaptors::transformed([](const nmos::api_version& v) { return make_api_version(v) + U("/"); }));
    }

    // construct a standard NMOS error response, using the default reason phrase if no user error information is specified
    web::json::value make_error_response_body(web::http::status_code code, const utility::string_t& error, const utility::string_t& debug)
    {
        return web::json::value_of({
            { U("code"), code }, // must be 400..599
            { U("error"), !error.empty() ? error : web::http::get_default_reason_phrase(code) },
            { U("debug"), !debug.empty() ? web::json::value::string(debug) : web::json::value::null() }
        }, true);
    }

    // experimental extension, to support human-readable HTML rendering of NMOS responses
    namespace experimental
    {
        const char* headers_stylesheet = R"-stylesheet-(
.headers
{
  font-family: monospace;
  color: grey;
  border-bottom: 1px solid lightgrey
}
.headers ol
{
  list-style: none;
  padding: 0
}
)-stylesheet-";

        // objects with the keywords $href and $_ are rendered as HTML anchor (a) tags
        // in order that the elements in NMOS "child resources" responses can be made into links
        // and id values in resources can also be made into links to the appropriate resource
        template <typename CharType>
        struct basic_html_visitor : web::json::experimental::basic_html_visitor<CharType>
        {
            typedef web::json::experimental::basic_html_visitor<CharType> base;
            typedef typename base::char_type char_type;
            typedef typename base::literals literals;
            typedef typename base::html_entities html_entities;

            basic_html_visitor(std::basic_ostream<char_type>& os)
                : base(os)
            {}

            // visit callbacks
            using base::operator();
            void operator()(const web::json::value& value, web::json::string_tag tag)
            {
                const bool href = !name && is_href(value.as_string());
                const auto str = escape(escape_characters(value.as_string()));
                start_span(name ? "name" : "string");
                os << html_entities::quot;
                if (href) start_a(str);
                os << str;
                if (href) end_a();
                os << html_entities::quot;
                end_span();
            }
            void operator()(const web::json::value& value, web::json::object_tag)
            {
                if (value.has_field(U("$href")) && value.has_field(U("$_")))
                {
                    const auto href = escape(escape_characters(value.at(U("$href")).as_string()));

                    const auto& v = value.at(U("$_"));
                    if (v.is_string())
                    {
                        // cute rendering of simple string links with the surrounding quotes outside the link
                        start_span("string");
                        os << html_entities::quot;
                        start_a(href);
                        // hmm, special handling for empty strings?
                        os << escape(escape_characters(v.as_string()));
                        end_a();
                        os << html_entities::quot;
                        end_span();
                    }
                    else
                    {
                        // for other value types, the whole value is rendered inside the link
                        start_a(href);
                        web::json::visit(*this, v);
                        end_a();
                    }
                }
                else
                {
                    web::json::visit_object(*this, value);
                }
            }
            void operator()(const web::json::value& value, web::json::array_tag)
            {
                web::json::visit_array(*this, value);
            }

            static bool is_href(const utility::string_t& str)
            {
                static const auto schemes = { U("http://"), U("https://"), U("ws://"), U("wss://") };
                return boost::algorithm::any_of(schemes, [&str](const utility::string_t& scheme)
                {
                    return boost::algorithm::istarts_with(str, scheme);
                });
            }

            using base::escape_characters;
            using base::escape;

        protected:
            using base::os;
            using base::name;

            using base::start_span;
            using base::end_span;
            void start_a(const std::basic_string<char_type>& href) { this->os << "<a href=\"" << href << "\">"; }
            void end_a() { this->os << "</a>"; }
        };

        // construct an HTML rendering of an NMOS response
        std::string make_html_response_body(const web::http::http_response& res, slog::base_gate& gate)
        {
            typedef basic_html_visitor<char> html_visitor;

            std::ostringstream html;
            html << "<html><head>";
            html << "<style>" << headers_stylesheet << web::json::experimental::html_stylesheet << "</style>";
            html << "<script>" << web::json::experimental::html_script << "</script>";
            html << "</head><body>";
            html << "<div class=\"headers\"><ol>";
            for (const auto& header : res.headers())
            {
                html << "<li>";
                html << "<span class=\"name\">";
                html << html_visitor::escape(utility::us2s(header.first));
                html << "</span>";
                html << ": ";
                html << "<span class=\"value\">";
                if (header.first == web::http::header_names::location)
                {
                    const auto html_value = html_visitor::escape(utility::us2s(header.second));
                    html << "<a href=\"" << html_value << "\">" << html_value << "</a>";
                }
                else if (header.first == U("Link"))
                {
                    // this regex pattern matches the usual whitespace precisely, but that's good enough
                    static const utility::regex_t link(U(R"-regex-(<([^>]*)>; rel="([^"]*)"(, )?)-regex-"));
                    auto first = header.second.begin();
                    utility::smatch_t match;
                    while (first != header.second.end())
                    {
                        if (bst::regex_search(first, header.second.end(), match, link, bst::regex_constants::match_continuous))
                        {
                            const auto html_link = html_visitor::escape(utility::us2s(match[1].str()));
                            const auto html_rel = html_visitor::escape(utility::us2s(match[2].str()));
                            const auto html_comma = html_visitor::escape(utility::us2s(match[3].str()));

                            html << html_visitor::html_entities::lt;
                            html << "<a href=\"" << html_link << "\">" << html_link << "</a>";
                            html << html_visitor::html_entities::gt;
                            html << "; ";
                            html << "rel=" << html_visitor::html_entities::quot << html_rel << html_visitor::html_entities::quot;
                            html << html_comma;

                            first = match[0].second;
                        }
                        else
                        {
                            // match failed, so just output the remainder of the value
                            html << html_visitor::escape(utility::us2s({ first, header.second.end() }));
                            first = header.second.end();
                        }
                    }
                }
                else
                {
                    html << html_visitor::escape(utility::us2s(header.second));
                }
                html << "</span>";
                html << "</li>";
            }
            html << "</ol></div><br/>";
            html << "<div class=\"json gutter\">";
            web::json::visit(html_visitor(html), nmos::details::extract_json(res, gate).get());
            html << "</div>";
            html << "</body></html>";
            return html.str();
        }
    }

    namespace details
    {
        // make user error information (to be used with status_codes::NotFound)
        utility::string_t make_erased_resource_error()
        {
            return U("resource has recently expired or been deleted");
        }

        // make handler to check supported API version, and set error response otherwise
        web::http::experimental::listener::route_handler make_api_version_handler(const std::set<api_version>& versions, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            return [versions, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                const auto version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
                if (versions.end() == versions.find(version))
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Unsupported API version";
                    set_error_reply(res, status_codes::NotFound, U("Not Found; unsupported API version"));
                    throw details::to_api_finally_handler{}; // in order to skip other route handlers and then send the response
                }
                return pplx::task_from_result(true);
            };
        }

        static const utility::string_t received_time{ U("X-Received-Time") };
        static const utility::string_t actual_method{ U("X-Actual-Method") };

        // make handler to set appropriate response headers, and error response body if indicated
        web::http::experimental::listener::route_handler make_api_finally_handler(slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            return [&gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);

                const auto received_time = req.headers().find(details::received_time);
                const auto processing_dur = req.headers().end() != received_time
                    ? std::chrono::duration_cast<std::chrono::microseconds>(nmos::tai_clock::now() - nmos::time_point_from_tai(nmos::parse_version(received_time->second))).count() / 1000.0
                    : 0.0;

                // experimental extension, to add Server-Timing response header
                if (req.headers().end() != received_time)
                {
                    req.headers().remove(details::received_time);
                    res.headers().add(web::http::experimental::header_names::server_timing, web::http::experimental::make_timing_header({ { U("proc"), processing_dur } }));
                    res.headers().add(web::http::experimental::header_names::timing_allow_origin, U("*"));
                }

                // if it was a HEAD request, restore that and discard any response body
                // since RFC 7231 says "the server MUST NOT send a message body in the response"
                // see https://tools.ietf.org/html/rfc7231#section-4.3.2
                if (web::http::has_header_value(req.headers(), details::actual_method, methods::HEAD))
                {
                    req.set_method(methods::HEAD);
                    req.headers().remove(details::actual_method);
                    if (res.body()) res.body() = concurrency::streams::istream();
                }

                if (web::http::empty_status_code == res.status_code())
                {
                    res.set_status_code(status_codes::NotFound);
                }

                if (status_codes::MethodNotAllowed == res.status_code())
                {
                    // obviously, OPTIONS requests are allowed in addition to any methods that have been identified already
                    web::http::add_header_value(res.headers(), web::http::header_names::allow, methods::OPTIONS);

                    // and HEAD requests are allowed if GET requests are
                    if (web::http::has_header_value(res.headers(), web::http::header_names::allow, web::http::methods::GET))
                    {
                        web::http::add_header_value(res.headers(), web::http::header_names::allow, web::http::methods::HEAD);
                    }

                    if (methods::OPTIONS == req.method())
                    {
                        res.set_status_code(status_codes::OK);

                        // distinguish a vanilla OPTIONS request from a CORS preflight request
                        if (req.headers().has(web::http::cors::header_names::request_method))
                        {
                            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "CORS preflight request";
                            nmos::details::add_cors_preflight_headers(req, res);
                        }
                        else
                        {
                            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "OPTIONS request";
                        }
                    }
                    else
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Method not allowed for this route";
                    }
                }
                else if (status_codes::NotFound == res.status_code())
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Route not found";
                }

                if (web::http::is_error_status_code(res.status_code()))
                {
                    // don't replace an existing response body which might contain richer error information
                    if (!res.body())
                    {
                        res.set_body(nmos::make_error_response_body(res.status_code()));
                    }
                }

                // since the Accept request header may affect the response, indicate that it should be taken into account
                // when deciding whether or not a cached response can be used
                res.headers().add(web::http::header_names::vary, web::http::header_names::accept);

                nmos::details::add_cors_headers(res);

                // experimental extension, to support human-readable HTML rendering of NMOS responses

                const auto mime_type = web::http::details::get_mime_type(res.headers().content_type());
                if (web::http::details::is_mime_type_json(mime_type) && experimental::details::is_html_response_preferred(req, mime_type))
                {
                    res.set_body(nmos::experimental::make_html_response_body(res, gate));
                    res.headers().set_content_type(U("text/html; charset=utf-8"));
                }

                slog::detail::logw<slog::log_statement, slog::base_gate>(gate, slog::severities::more_info, SLOG_FLF) << nmos::stash_categories({ nmos::categories::access }) << nmos::common_log_stash(req, res) << "Sending response after " << processing_dur << "ms";

                req.reply(res);
                return pplx::task_from_result(false); // don't continue matching routes
            };
        }
    }

    // set up a standard NMOS error response, using the default reason phrase if no user error information is specified
    void set_error_reply(web::http::http_response& res, web::http::status_code code, const utility::string_t& error, const utility::string_t& debug)
    {
        set_reply(res, code, nmos::make_error_response_body(code, error, debug));
        // https://stackoverflow.com/questions/38654336/is-it-good-practice-to-modify-the-reason-phrase-of-an-http-response/38655533#38655533
        //res.set_reason_phrase(error);
    }

    // set up a standard NMOS error response, using the default reason phrase and the specified debug information
    void set_error_reply(web::http::http_response& res, web::http::status_code code, const std::exception& debug)
    {
        set_error_reply(res, code, {}, utility::s2us(debug.what()));
    }

    // add handler to set appropriate response headers, and error response body if indicated - call this only after adding all others!
    void add_api_finally_handler(web::http::experimental::listener::api_router& api, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api.support(U(".*"), details::make_api_finally_handler(gate_));

        api.set_exception_handler([&gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            try
            {
                std::rethrow_exception(std::current_exception());
            }
            // assume a JSON error indicates a bad request
            catch (const web::json::json_exception& e)
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "JSON error: " << e.what();
                set_error_reply(res, status_codes::BadRequest, e);
            }
            // likewise an HTTP error, e.g. from http_request::extract_json
            catch (const web::http::http_exception& e)
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "HTTP error: " << e.what() << " [" << e.error_code() << "]";
                set_error_reply(res, status_codes::BadRequest, e);
            }
            // while a runtime_error (often) indicates an unimplemented feature
            catch (const std::runtime_error& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error: " << e.what();
                set_error_reply(res, status_codes::NotImplemented, e);
            }
            // and a logic_error (probably) indicates some other implementation error
            catch (const std::logic_error& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error: " << e.what();
                set_error_reply(res, status_codes::InternalError, e);
            }
            // and this one asks to skip other route handlers and then send the response
            catch (const details::to_api_finally_handler&)
            {
                if (web::http::empty_status_code == res.status_code())
                {
                    slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected to_api_finally_handler exception";
                }
            }
            // and other exception types are unexpected errors
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what();
                set_error_reply(res, status_codes::InternalError, e);
            }
            catch (...)
            {
                slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception";
                set_error_reply(res, status_codes::InternalError);
            }

            return pplx::task_from_result(true); // continue matching routes, e.g. the 'finally' handler
        });
    }

    // modify the specified API to handle all requests (including CORS preflight requests via "OPTIONS") and attach it to the specified listener - captures api by reference!
    void support_api(web::http::experimental::listener::http_listener& listener, web::http::experimental::listener::api_router& api_, slog::base_gate& gate)
    {
        add_api_finally_handler(api_, gate);
        auto api = [&api_, &gate](web::http::http_request req)
        {
            req.headers().add(details::received_time, nmos::make_version());
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF)
                << stash_remote_address(req.remote_address())
                << stash_http_method(req.method())
                << stash_request_uri(req.request_uri())
                << stash_http_version(req.http_version())
                << "Received request";
            api_(req);
        };
        listener.support(api);
        listener.support(web::http::methods::OPTIONS, api); // to handle CORS preflight requests
        listener.support(web::http::methods::HEAD, [api](web::http::http_request req) // to handle HEAD requests
        {
            // this naive approach means that the API may well generate a response body
            req.headers().add(details::actual_method, web::http::methods::HEAD);
            req.set_method(web::http::methods::GET);
            api(req);
        });
    }

    // construct an http_listener on the specified address and port, modifying the specified API to handle all requests
    // (including CORS preflight requests via "OPTIONS") - captures api by reference!
    web::http::experimental::listener::http_listener make_api_listener(bool secure, const utility::string_t& host_address, int port, web::http::experimental::listener::api_router& api, web::http::experimental::listener::http_listener_config config, slog::base_gate& gate)
    {
        web::http::experimental::listener::http_listener api_listener(web::http::experimental::listener::make_listener_uri(secure, host_address, port), std::move(config));
        nmos::support_api(api_listener, api, gate);
        return api_listener;
    }

    // construct a websocket_listener on the specified address and port - captures handlers by reference!
    web::websockets::experimental::listener::websocket_listener make_ws_api_listener(bool secure, const utility::string_t& host_address, int port, const web::websockets::experimental::listener::websocket_listener_handlers& handlers, web::websockets::experimental::listener::websocket_listener_config config, slog::base_gate& gate)
    {
        web::websockets::experimental::listener::websocket_listener ws_listener(web::websockets::experimental::listener::make_listener_uri(secure, host_address, port), std::move(config));
        ws_listener.set_handlers({
            std::ref(handlers.validate_handler),
            std::ref(handlers.open_handler),
            std::ref(handlers.close_handler),
            std::ref(handlers.message_handler)
        });
        return ws_listener;
    }

    // returns "http" or "https" depending on settings
    utility::string_t http_scheme(const nmos::settings& settings)
    {
        return web::http_scheme(nmos::experimental::fields::client_secure(settings));
    }

    // returns "ws" or "wss" depending on settings
    utility::string_t ws_scheme(const nmos::settings& settings)
    {
        return web::ws_scheme(nmos::experimental::fields::client_secure(settings));
    }

    // returns "mqtt" or "secure-mqtt"
    utility::string_t mqtt_scheme(bool secure)
    {
        return secure ? U("secure-mqtt") : U("mqtt");
    }

    // returns "mqtt" or "secure-mqtt" depending on settings
    utility::string_t mqtt_scheme(const nmos::settings& settings)
    {
        return mqtt_scheme(nmos::experimental::fields::client_secure(settings));
    }
}

#if 0
#include "detail/private_access.h"

namespace web
{
    namespace details
    {
        struct uri_encode_query_impl { typedef utility::string_t(*type)(const utf8string&); };
    }

    // because web::uri::encode_uri(de, web::uri::components::query) doesn't encode '&' (or ';', or '=')
    utility::string_t uri_encode_query_value(const utility::string_t& value)
    {
        return detail::stowed<details::uri_encode_query_impl>::value(utility::conversions::details::print_utf8string(value));
    }
}

template struct detail::stow_private<web::details::uri_encode_query_impl, &web::uri::encode_query_impl>;
#else
// unfortunately the private access trick doesn't work on Visual Studio 2015
namespace web
{
    namespace details
    {
        // return true if c should be encoded in a query parameter value
        inline bool is_query_value_unsafe(int c)
        {
            static const utf8string safe
            {
                // unreserved characters
                "-._~"
                // sub-delimiters - except '&' most importantly, the alternative separator ';'
                // and '=' and '+' for clarity
                "!$'()*,"
                // path - except '%'
                "/:@"
                // query
                "?"
            };
            return !utility::details::is_alnum(c) && utf8string::npos == safe.find((char)c);
        }

// Following function lifted from cpprestsdk/Release/src/uri/uri.cpp
// Encodes all characters not in given set determined by given function.
template<class F>
utility::string_t encode_impl(const utf8string& raw, F should_encode)
{
    const utility::char_t* const hex = _XPLATSTR("0123456789ABCDEF");
    utility::string_t encoded;
    for (auto iter = raw.begin(); iter != raw.end(); ++iter)
    {
        // for utf8 encoded string, char ASCII can be greater than 127.
        int ch = static_cast<unsigned char>(*iter);
        // ch should be same under both utf8 and utf16.
        if (should_encode(ch))
        {
            encoded.push_back(_XPLATSTR('%'));
            encoded.push_back(hex[(ch >> 4) & 0xF]);
            encoded.push_back(hex[ch & 0xF]);
        }
        else
        {
            // ASCII don't need to be encoded, which should be same on both utf8 and utf16.
            encoded.push_back((utility::char_t)ch);
        }
    }
    return encoded;
}

        utility::string_t uri_encode_query_impl(const utf8string& raw)
        {
            return details::encode_impl(raw, details::is_query_value_unsafe);
        }
    }

    // because web::uri::encode_uri(de, web::uri::components::query) doesn't encode '&' (or ';', or '=')
    utility::string_t uri_encode_query_value(const utility::string_t& value)
    {
        return details::uri_encode_query_impl(utility::conversions::details::print_utf8string(value));
    }
}
#endif

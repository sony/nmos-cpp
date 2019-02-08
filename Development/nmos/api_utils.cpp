#include "nmos/api_utils.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/api_version.h"
#include "nmos/slog.h"
#include "nmos/type.h"

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
                    auto en = web::uri::encode_uri(de, web::uri::components::query);
                    element.second = web::json::value::string(en);
                }
            }
        }

        // extract JSON after checking the Content-Type header
        pplx::task<web::json::value> extract_json(const web::http::http_request& req, slog::base_gate& gate)
        {
            auto content_type = req.headers().content_type();
            auto semicolon = content_type.find(U(';'));
            if (utility::string_t::npos != semicolon) content_type.erase(semicolon);
            boost::algorithm::trim(content_type);

            if (web::http::details::mime_types::application_json == content_type)
            {
                // No "charset" parameter is defined for the application/json media-type
                // but it's quite common so don't even bother to log a warning...
                // See https://www.iana.org/assignments/media-types/application/json

                return req.extract_json(false);
            }
            else if (content_type.empty())
            {
                // "If a Content-Type header field is not present, the recipient MAY
                // [...] examine the data to determine its type."
                // See https://tools.ietf.org/html/rfc7231#section-3.1.1.5

                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing Content-Type: should be application/json";

                return req.extract_json(true);
            }
            else
            {
                // more helpful message than from web::http::details::http_msg_base::parse_and_check_content_type for unacceptable content-type
                return pplx::task_from_exception<web::json::value>(web::http::http_exception(U("Incorrect Content-Type: ") + req.headers().content_type() + U(", should be application/json")));
            }
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
            { U("subscriptions"), nmos::types::subscription }
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
            { nmos::types::grain, {} } // subscription websocket grains aren't exposed via the Query API
        };
        return resourceTypes_from_type.at(type);
    }

    // construct a standard NMOS "child resources" response, from the specified sub-routes
    // merging with ones from an existing response
    // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.0.%20APIs.md#api-paths
    web::json::value make_sub_routes_body(std::set<utility::string_t> sub_routes, web::http::http_response res)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        if (res.body())
        {
            auto body = res.extract_json().get();

            for (auto& element : body.as_array())
            {
                sub_routes.insert(element.as_string());
            }
        }

        return web::json::value_from_elements(sub_routes);
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

        static const utility::string_t actual_method{ U("X-Actual-Method") };

        // make handler to set appropriate response headers, and error response body if indicated
        web::http::experimental::listener::route_handler make_api_finally_handler(slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            return [&gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);

                // if it was a HEAD request, restore that and discard any response body
                // since RFC 7231 says "the server MUST NOT send a message body in the response"
                // see https://tools.ietf.org/html/rfc7231#section-4.3.2
                if (web::http::has_header_value(req.headers(), actual_method, methods::HEAD))
                {
                    req.set_method(methods::HEAD);
                    req.headers().remove(actual_method);
                    if (res.body()) res.body() = concurrency::streams::bytestream::open_istream(std::vector<unsigned char>{});
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

                nmos::details::add_cors_headers(res);

                slog::detail::logw<slog::log_statement, slog::base_gate>(gate, slog::severities::more_info, SLOG_FLF) << nmos::stash_categories({ nmos::categories::access }) << nmos::common_log_stash(req, res) << "Sending response";

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
    void support_api(web::http::experimental::listener::http_listener& listener, web::http::experimental::listener::api_router& api, slog::base_gate& gate)
    {
        add_api_finally_handler(api, gate);
        listener.support(std::ref(api));
        listener.support(web::http::methods::OPTIONS, std::ref(api)); // to handle CORS preflight requests
        listener.support(web::http::methods::HEAD, [&api](web::http::http_request req) // to handle HEAD requests
        {
            // this naive approach means that the API may well generate a response body
            req.headers().add(details::actual_method, web::http::methods::HEAD);
            req.set_method(web::http::methods::GET);
            api(req);
        });
    }

    // construct an http_listener on the specified port, using the specified API to handle all requests
    web::http::experimental::listener::http_listener make_api_listener(int port, web::http::experimental::listener::api_router& api, web::http::experimental::listener::http_listener_config config, slog::base_gate& gate)
    {
        web::http::experimental::listener::http_listener api_listener(web::http::experimental::listener::make_listener_uri(port), std::move(config));
        nmos::support_api(api_listener, api, gate);
        return api_listener;
    }
}

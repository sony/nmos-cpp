#include "nmos/api_utils.h"

#include <boost/algorithm/string/trim.hpp>
#include "nmos/slog.h"

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
        pplx::task<web::json::value> extract_json(const web::http::http_request& req, const web::http::experimental::listener::route_parameters& parameters, slog::base_gate& gate)
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

                slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Missing Content-Type: should be application/json";

                return req.extract_json(true);
            }
            else
            {
                // more helpful message than from web::http::details::http_msg_base::parse_and_check_content_type for unacceptable content-type
                return pplx::task_from_exception<web::json::value>(web::http::http_exception(U("Incorrect Content-Type: ") + req.headers().content_type() + U(", should be application/json")));
            }
        }

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

        web::http::http_response& add_cors_headers(web::http::http_response& res)
        {
            // Indicate that any Origin is allowed
            res.headers().add(web::http::cors::header_names::allow_origin, U("*"));

            // some browsers seem not to support the latest spec which allows the wildcard "*"
            for (const auto& header : res.headers())
            {
                if (!web::http::cors::is_cors_response_header(header.first) && !web::http::cors::is_cors_safelisted_response_header(header.first))
                {
                    res.headers().add(web::http::cors::header_names::expose_headers, header.first);
                }
            }
            return res;
        }
    }

    namespace details
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
    }

    // Map from a resourceType, i.e. the plural string used in the API endpoint routes, to a "proper" type
    nmos::type type_from_resourceType(const utility::string_t& resourceType)
    {
        return details::types_from_resourceType.at(resourceType);
    }

    // Map from a "proper" type to a resourceType, i.e. the plural string used in the API endpoint routes
    utility::string_t resourceType_from_type(const nmos::type& type)
    {
        return details::resourceTypes_from_type.at(type);
    }

    web::json::value make_error_response_body(web::http::status_code code, const utility::string_t& error, const utility::string_t& debug)
    {
        web::json::value result = web::json::value::object(true);
        result[U("code")] = code; // must be 400..599
        result[U("error")] = !error.empty() ? web::json::value::string(error) : web::json::value::string(web::http::get_default_reason_phrase(code));
        result[U("debug")] = !debug.empty() ? web::json::value::string(debug) : web::json::value::null();
        return result;
    }

    namespace details
    {
        web::http::experimental::listener::route_handler make_api_finally_handler(slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            return [&gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                if (web::http::empty_status_code == res.status_code())
                {
                    res.set_status_code(status_codes::NotFound);
                }

                if (methods::OPTIONS == req.method() && status_codes::MethodNotAllowed == res.status_code())
                {
                    res.set_status_code(status_codes::OK);

                    // obviously, OPTIONS requests are allowed in addition to any methods that have been identified already
                    res.headers().add(web::http::header_names::allow, methods::OPTIONS);

                    // distinguish a vanilla OPTIONS request from a CORS preflight request
                    if (req.headers().has(web::http::cors::header_names::request_method))
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "CORS preflight request";
                        nmos::details::add_cors_preflight_headers(req, res);
                    }
                    else
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "OPTIONS request";
                    }
                }
                else if (status_codes::MethodNotAllowed == res.status_code())
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Method not allowed for this route";
                }
                else if (status_codes::NotFound == res.status_code())
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Route not found";
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

                slog::detail::logw<slog::log_statement, slog::base_gate>(gate, slog::severities::more_info, SLOG_FLF) << nmos::stash_category(nmos::categories::access) << nmos::common_log_stash(req, res) << "Sending response";

                req.reply(res);
                return pplx::task_from_result(false); // don't continue matching routes
            };
        }

        void set_error_reply(web::http::http_response& res, web::http::status_code code, const utility::string_t& debug)
        {
            set_reply(res, code, nmos::make_error_response_body(code, {}, debug));
        }
    }

    void add_api_finally_handler(web::http::experimental::listener::api_router& api, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api.support(U(".*"), details::make_api_finally_handler(gate));

        api.set_exception_handler([&gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            try
            {
                std::rethrow_exception(std::current_exception());
            }
            // assume a JSON error indicates a bad request
            catch (const web::json::json_exception& e)
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "JSON error: " << e.what();
                details::set_error_reply(res, status_codes::BadRequest, utility::s2us(e.what()));
            }
            // likewise an HTTP error, e.g. from http_request::extract_json
            catch (const web::http::http_exception& e)
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "HTTP error: " << e.what() << " [" << e.error_code() << "]";
                details::set_error_reply(res, status_codes::BadRequest, utility::s2us(e.what()));
            }
            // while a runtime_error (often) indicates an unimplemented feature
            catch (const std::runtime_error& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Implementation error: " << e.what();
                details::set_error_reply(res, status_codes::NotImplemented, utility::s2us(e.what()));
            }
            // and other exception types are unexpected errors
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Unexpected exception: " << e.what();
                details::set_error_reply(res, status_codes::InternalError, utility::s2us(e.what()));
            }
            catch (...)
            {
                slog::log<slog::severities::severe>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Unexpected unknown exception";
                details::set_error_reply(res, status_codes::InternalError);
            }

            return pplx::task_from_result(true); // continue matching routes, e.g. the 'finally' handler
        });
    }

    void support_api(web::http::experimental::listener::http_listener& listener, web::http::experimental::listener::api_router& api)
    {
        listener.support(std::ref(api));
        listener.support(web::http::methods::OPTIONS, std::ref(api)); // to handle CORS preflight requests
    }
}

#include "cpprest/api_router.h"
#include "cpprest/http_utils.h"

namespace web
{
    namespace http
    {
        namespace experimental
        {
            namespace listener
            {
                utility::string_t api_router::get_route_relative_path(const web::http::http_request& req, const utility::string_t& route_path)
                {
                    // If the route path is empty, then just return the listener-relative URI.
                    if (route_path.empty() || route_path == U("/"))
                    {
                        return req.relative_uri().path();
                    }

                    const utility::string_t prefix = web::uri::decode(route_path);
                    const utility::string_t path = web::uri::decode(req.relative_uri().path());

                    if (path.size() >= prefix.size() && 0 == path.compare(0, prefix.size(), prefix))
                    {
                        utility::string_t result = web::uri::encode_uri(path.substr(prefix.size()), web::uri::components::path);
                        return result;
                    }
                    else
                    {
                        throw web::http::http_exception(U("Error: request was not prefixed with route path"));
                    }
                }

                api_router::api_router()
                {}

                void api_router::operator()(web::http::http_request req)
                {
                    web::http::http_response res;

                    (*this)(req, res, {}, {}).then([req, res](bool continue_matching)
                    {
                        if (continue_matching)
                        {
                            // no route matched, or one or more routes matched, but not for the requested method, or all handlers returned true, "continue matching routes"

                            // if one or more routes matched, but not for the requested method, the status code and "Allow" header have been set appropriately;
                            // otherwise, if no route matched, or no handler set the response status code, indicate route not found
                            if (empty_status_code == res.status_code())
                            {
                                res.set_status_code(status_codes::NotFound);
                            }

                            req.reply(res);
                        }
                    });
                }

                static const web::http::method any_method{};

                pplx::task<bool> api_router::operator()(web::http::http_request req, web::http::http_response res, const utility::string_t& route_path, const route_parameters& parameters)
                {
                    return (*this)(req, res, route_path, parameters, routes.begin());
                }

                pplx::task<bool> api_router::operator()(web::http::http_request req, web::http::http_response res, const utility::string_t& route_path, const route_parameters& parameters, iterator route)
                {
                    const utility::string_t path = get_route_relative_path(req, route_path); // required, as must live longer than the match results
                    for (; routes.end() != route; ++route)
                    {
                        utility::smatch_t route_match;
                        if (route_regex_match(path, route_match, route->route_pattern.first, route->flags))
                        {
                            // route_path for this route handler is constructed by appending the entire matching expression
                            const auto merged_path = route_path + route_match.str();
                            // existing parameters are inserted into the new parameters rather than vice-versa so that new parameters replace existing ones with the same name
                            const auto merged_parameters = insert(get_parameters(route->route_pattern.second, route_match), parameters);

                            if (route->method == req.method() || any_method == route->method)
                            {
                                return call(route->handler, exception_handler, req, res, merged_path, merged_parameters).then([=](bool continue_matching)
                                {
                                    if (!continue_matching)
                                    {
                                        // short-circuit other routes, e.g. if the hander actually sent a reply rather than just modifying the response object
                                        return pplx::task_from_result(false);
                                    }

                                    return (*this)(req, res, route_path, parameters, std::next(route));
                                });
                            }
                            else
                            {
                                handle_method_not_allowed(*route, res, merged_path, merged_parameters);
                            }
                        }
                    }
                    // no route matched; indicate "continue matching routes"
                    return pplx::task_from_result(true);
                }

                pplx::task<bool> api_router::call(const route_handler& handler, const route_handler& exception_handler, web::http::http_request req, web::http::http_response res, const utility::string_t& route_path, const route_parameters& parameters)
                {
                    if (!exception_handler)
                    {
                        return handler(req, res, route_path, parameters);
                    }

                    // this handler may:
                    //   return a task that (immediately or asynchronously) indicates "continue matching routes" or not,
                    //   return a task that (immediately or asynchronously) contains an exception,
                    //   throw an exception itself
                    // the exception handler should be applied in either of the latter cases
                    try
                    {
                        return handler(req, res, route_path, parameters).then([=](pplx::task<bool> continue_matching_or_exception)
                        {
                            try
                            {
                                return pplx::task_from_result(continue_matching_or_exception.get());
                            }
                            catch (...)
                            {
                                return exception_handler(req, res, route_path, parameters);
                            }
                        });
                    }
                    catch (...)
                    {
                        return exception_handler(req, res, route_path, parameters);
                    }
                }

                void api_router::handle_method_not_allowed(const route& route, web::http::http_response& res, const utility::string_t& route_path, const route_parameters& parameters)
                {
                    // a preceding route handler may have already set a status code, but if not, this is worth reporting as a "near miss"
                    if (empty_status_code == res.status_code())
                    {
                        res.set_status_code(status_codes::MethodNotAllowed);
                        // a subsequent route handler may yet overwrite this
                    }
                    // hmm, it's not a guarantee that the route.handler for this route.method would in fact result in a response being sent,
                    // but it seems nice to add the "Allow" header even so?
                    res.headers().add(header_names::allow, route.method);

                    // there doesn't seem to be an elegant way to save the route_path or parameters for diagnostics
                }

                void api_router::support(const utility::string_t& route_pattern, const web::http::method& method, route_handler handler)
                {
                    insert(routes.end(), match_entire, route_pattern, method, handler);
                }

                void api_router::support(const utility::string_t& route_pattern, route_handler all_handler)
                {
                    insert(routes.end(), match_entire, route_pattern, any_method, all_handler);
                }

                void api_router::mount(const utility::string_t& route_pattern, const web::http::method& method, route_handler handler)
                {
                    insert(routes.end(), match_prefix, route_pattern, method, handler);
                }

                void api_router::mount(const utility::string_t& route_pattern, route_handler all_handler)
                {
                    insert(routes.end(), match_prefix, route_pattern, any_method, all_handler);
                }

                void api_router::set_exception_handler(route_handler handler)
                {
                    exception_handler = handler;
                }

                api_router::iterator api_router::insert(iterator where, match_flag_type flags, const utility::string_t& route_pattern, const web::http::method& method, route_handler handler)
                {
                    auto parsed = utility::parse_regex_named_sub_matches(route_pattern);
                    return routes.insert(where, { flags, { utility::regex_t(parsed.first), parsed.second }, method, handler });
                }

                route_parameters api_router::get_parameters(const utility::named_sub_matches_t& parameter_sub_matches, const utility::smatch_t& route_match)
                {
                    route_parameters parameters;
                    for (auto& named_sub_match : parameter_sub_matches)
                    {
                        auto& sub_match = route_match[named_sub_match.second];
                        if (sub_match.matched)
                        {
                            parameters[named_sub_match.first] = sub_match.str();
                        }
                    }
                    return parameters;
                }

                route_parameters api_router::insert(route_parameters&& into, const route_parameters& range)
                {
                    // unorderd_map::insert only inserts elements if the container doesn't already contain an element with an equivalent key
                    into.insert(range.begin(), range.end());
                    return std::move(into);
                }

                bool api_router::route_regex_match(const utility::string_t& path, utility::smatch_t& route_match, const utility::regex_t& route_regex, match_flag_type flags)
                {
                    return match_prefix == flags
                        ? bst::regex_search(path, route_match, route_regex, bst::regex_constants::match_continuous)
                        : bst::regex_match(path, route_match, route_regex);
                }
            }
        }
    }
}

#include "nmos/logging_api.h"

#include <boost/range/adaptor/transformed.hpp>
#include "nmos/api_utils.h"
#include "nmos/query_utils.h"
#include "rql/rql.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_unmounted_logging_api(nmos::experimental::log_model& model, slog::base_gate& gate);

        web::http::experimental::listener::api_router make_logging_api(nmos::experimental::log_model& model, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router logging_api;

            logging_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("log/") }, res));
                return pplx::task_from_result(true);
            });

            logging_api.support(U("/log/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("v1.0/") }, res));
                return pplx::task_from_result(true);
            });

            logging_api.mount(U("/log/v1.0"), make_unmounted_logging_api(model, gate));

            return logging_api;
        }

        namespace fields
        {
            const web::json::field<tai> cursor{ U("cursor") };
        }

        bool match_logging_rql(const web::json::value& value, const web::json::value& query)
        {
            return query.is_null() || rql::evaluator
            {
                [&value](web::json::value& results, const web::json::value& key)
                {
                    return web::json::extract(value.as_object(), results, key.as_string());
                },
                rql::default_any_operators()
            }(query) == rql::value_true;
        }

        // Predicate to match events against a query
        struct event_query
        {
            typedef const nmos::experimental::event& argument_type;
            typedef bool result_type;

            event_query(const web::json::value& flat_query_params);

            result_type operator()(argument_type event) const;

            // the query/exemplar object for a Basic Query
            web::json::value basic_query;

            // a representation of the RQL abstract syntax tree for an Advanced Query
            web::json::value rql_query;
        };

        event_query::event_query(const web::json::value& flat_query_params)
            : basic_query(web::json::unflatten(flat_query_params))
        {
            if (basic_query.has_field(U("paging")))
            {
                basic_query.erase(U("paging"));
            }
            if (basic_query.has_field(U("query")))
            {
                auto& advanced = basic_query.at(U("query"));
                if (advanced.has_field(U("rql")))
                {
                    rql_query = rql::parse_query(web::json::field_as_string{ U("rql") }(advanced));
                }
                basic_query.erase(U("query"));
            }
        }

        event_query::result_type event_query::operator()(argument_type event) const
        {
            return web::json::match_query(event.data, basic_query, web::json::match_icase | web::json::match_substr)
                && match_logging_rql(event.data, rql_query);
        }

        // Cursor-based paging parameters
        struct event_paging
        {
            explicit event_paging(const web::json::value& flat_query_params, const nmos::tai& max_until = nmos::tai_max(), size_t default_limit = (std::numeric_limits<size_t>::max)(), size_t max_limit = (std::numeric_limits<size_t>::max)());

            // determine if the range [until, since) and limit are valid
            bool valid() const;

            // return only the results up until the time specified (inclusive)
            nmos::tai until;

            // return only the results since the time specified (non-inclusive)
            nmos::tai since;

            // restrict the response to the specified number of results
            size_t limit;

            // where both 'since' and 'until' parameters are specified, the 'since' value takes precedence
            // where a resulting data set is constrained by the server's value of 'limit'
            bool since_specified;

            template <typename Predicate>
            boost::any_range<const nmos::experimental::event, boost::bidirectional_traversal_tag, const nmos::experimental::event&, std::ptrdiff_t> page(const nmos::experimental::events& events, Predicate match)
            {
                return paging::cursor_based_page(events, match, until, since, limit, !since_specified);
            }
        };

        event_paging::event_paging(const web::json::value& flat_query_params, const nmos::tai& max_until, size_t default_limit, size_t max_limit)
            : until(max_until)
            , since(nmos::tai_min())
            , limit(default_limit)
            , since_specified(false)
        {
            auto query_params = web::json::unflatten(flat_query_params);

            // extract the supported paging options
            if (query_params.has_field(U("paging")))
            {
                auto& paging = query_params.at(U("paging")).as_object();
                for (auto& field : paging)
                {
                    if (field.first == U("until"))
                    {
                        until = nmos::parse_version(field.second.as_string());
                        // "These parameters may be used to construct a URL which would return the same set of bounded data on consecutive requests"
                        // therefore the response to a request with 'until' in the future should be fixed up...
                        if (until > max_until) until = max_until;
                    }
                    else if (field.first == U("since"))
                    {
                        since = nmos::parse_version(field.second.as_string());
                        since_specified = true;
                        // how should a request with 'since' in the future be fixed up?
                        if (since > until && until == max_until) until = since;
                    }
                    else if (field.first == U("limit"))
                    {
                        // "If the client had requested a page size which the server is unable to honour, the actual page size would be returned"
                        limit = utility::istringstreamed<size_t>(field.second.as_string());
                        if (limit > max_limit) limit = max_limit;
                    }
                    // as for resource_query, an error is reported for unimplemented parameters
                    else
                    {
                        throw std::runtime_error("unimplemented parameter - paging." + utility::us2s(field.first));
                    }
                }
            }
        }

        bool event_paging::valid() const
        {
            return since <= until;
        }

        namespace details
        {
            // make user error information (to be used with status_codes::BadRequest)
            utility::string_t make_valid_paging_error(const nmos::experimental::event_paging& paging)
            {
                return U("the value of the 'paging.since' parameter must be less than or equal to the value of the 'paging.until' parameter");
            }
        }

        // Cursor-based paging customisation points

        inline nmos::tai extract_cursor(const nmos::experimental::events&, nmos::experimental::events::const_iterator it)
        {
            return fields::cursor(it->data);
        }

        inline nmos::experimental::events::const_iterator lower_bound(const nmos::experimental::events& index, const nmos::tai& cursor)
        {
            return std::lower_bound(index.begin(), index.end(), cursor, [](const nmos::experimental::event& event, const nmos::tai& cursor)
            {
                return fields::cursor(event.data) > cursor;
            });
        }

        namespace details
        {
            utility::string_t make_query_parameters(web::json::value flat_query_params)
            {
                // any non-string query parameters need serializing before encoding
                // but web::json::query_from_value does this...

                // special case, RQL is kept as the URI-encoded string
                const auto encoded_rql = nmos::fields::query_rql(flat_query_params);

                // all other string values need encoding
                nmos::details::encode_elements(flat_query_params);

                if (flat_query_params.has_field(nmos::fields::query_rql))
                {
                    flat_query_params[nmos::fields::query_rql] = web::json::value::string(encoded_rql);
                }

                return web::json::query_from_value(flat_query_params);
            }

            web::json::value parse_query_parameters(const utility::string_t& query)
            {
                auto flat_query_params = web::json::value_from_query(query);

                // special case, RQL is kept as the URI-encoded string
                const auto encoded_rql = nmos::fields::query_rql(flat_query_params);

                // all other string values need decoding
                nmos::details::decode_elements(flat_query_params);

                if (flat_query_params.has_field(nmos::fields::query_rql))
                {
                    flat_query_params[nmos::fields::query_rql] = web::json::value::string(encoded_rql);
                }

                // any non-string query parameters need parsing after decoding...
                if (flat_query_params.has_field(nmos::fields::paging_limit))
                {
                    flat_query_params[nmos::fields::paging_limit] = web::json::value::parse(nmos::fields::paging_limit(flat_query_params));
                }

                return flat_query_params;
            }

            web::uri make_query_uri_with_no_paging(const web::http::http_request& req)
            {
                // could rebuild the query parameters from the decoded and parsed query string, rather than individually deleting the paging parameters from the request?
                auto query_params = parse_query_parameters(req.request_uri().query());
                if (query_params.has_field(U("paging.since")))
                {
                    query_params.erase(U("paging.since"));
                }
                if (query_params.has_field(U("paging.until")))
                {
                    query_params.erase(U("paging.until"));
                }
                if (query_params.has_field(U("paging.limit")))
                {
                    query_params.erase(U("paging.limit"));
                }

                // RFC 5988 allows relative URLs, but NMOS specification examples are all absolute URLs
                // See https://tools.ietf.org/html/rfc5988#section-5
                // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.5.%20APIs%20-%20Query%20Parameters.md

                // get the request host and port
                auto req_host_port = web::http::get_host_port(req);

                return web::uri_builder()
                    .set_scheme(U("http"))
                    .set_host(req_host_port.first)
                    .set_port(req_host_port.second)
                    .set_path(req.request_uri().path())
                    .set_query(make_query_parameters(query_params))
                    .to_uri();
            }

            void add_paging_headers(web::http::http_headers& headers, const nmos::experimental::event_paging& paging, const web::uri& base_link)
            {
                // X-Paging-Limit "identifies the current limit being used for paging. This may not match the requested value if the requested value was too high for the implementation"
                headers.add(U("X-Paging-Limit"), paging.limit);

                // X-Paging-Since "identifies the current value of the query parameter 'paging.since' in use, or if not specified identifies what value it would have had to return this data set.
                // This value may be re-used as the paging.until value of a query to return the previous page of results."
                headers.add(U("X-Paging-Since"), make_version(paging.since));

                // X-Paging-Until "identifies the current value of the query parameter 'paging.until' in use, or if not specified identifies what value it would have had to return this data set.
                // This value may be re-used as the paging.since value of a query to return the next page of results."
                headers.add(U("X-Paging-Until"), make_version(paging.until));

                // Link header "provides references to cursors for paging. The 'rel' attribute may be one of 'next', 'prev', 'first' or 'last'"

                auto link = web::uri_builder(base_link)
                    .append_query(U("paging.limit=") + utility::ostringstreamed(paging.limit))
                    .to_string();

                // "The Link header identifies the 'next' and 'prev' cursors which an application may use to make its next requests."
                // (though note, both the 'next' and 'prev' cursors may return empty responses)

                headers.add(U("Link"), U("<") + link + U("&paging.until=") + make_version(paging.since) + U(">; rel=\"prev\""));

                headers.add(U("Link"), U("<") + link + U("&paging.since=") + make_version(paging.until) + U(">; rel=\"next\""));

                // "An implementation may also provide 'first' and 'last' cursors to identify the beginning and end of a collection of this can be addressed via a consistent URL."

                headers.add(U("Link"), U("<") + link + U("&paging.since=") + make_version(nmos::tai_min()) + U(">; rel=\"first\""));

                headers.add(U("Link"), U("<") + link + U(">; rel=\"last\""));
            }
        }

        web::http::experimental::listener::api_router make_unmounted_logging_api(nmos::experimental::log_model& model, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router logging_api;

            logging_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("events/") }, res));
                return pplx::task_from_result(true);
            });

            logging_api.support(U("/events/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();

                // Extract and decode the query string

                auto flat_query_params = details::parse_query_parameters(req.request_uri().query());

                // Configure the query predicate

                event_query match(flat_query_params);

                // Configure the paging parameters

                // default limit ought to be part of log/settings
                event_paging paging(flat_query_params, tai_now(), 100);

                if (paging.valid())
                {
                    // Get the payload and update the paging parameters
                    struct default_constructible_event_query_wrapper { const event_query* impl; bool operator()(const event& e) const { return (*impl)(e); } };
                    auto page = paging.page(model.events, default_constructible_event_query_wrapper{ &match });

                    size_t count = 0;

                    set_reply(res, status_codes::OK,
                        web::json::serialize(page,
                            [&count](const event& event){ ++count; return event.data; }),
                        U("application/json"));

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Returning " << count << " matching log events";

                    details::add_paging_headers(res.headers(), paging, details::make_query_uri_with_no_paging(req));
                }
                else
                {
                    set_error_reply(res, status_codes::BadRequest, U("Bad Request; ") + details::make_valid_paging_error(paging));
                }

                return pplx::task_from_result(true);
            });

            logging_api.support(U("/events/?"), methods::DEL, [&model](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                auto lock = model.write_lock();

                if (req.request_uri().query().empty())
                {
                    model.events.clear();
                    set_reply(res, status_codes::NoContent);
                }
                else
                {
                    set_reply(res, status_codes::NotImplemented);
                }

                return pplx::task_from_result(true);
            });

            logging_api.support(U("/events/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model](http_request, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();

                const string_t eventId = parameters.at(nmos::patterns::resourceId.name);

                typedef events::index<tags::id>::type by_id;
                by_id& ids = model.events.get<tags::id>();

                auto event = ids.find(eventId);
                if (ids.end() != event)
                {
                    set_reply(res, status_codes::OK, event->data);
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            return logging_api;
        }

        namespace details
        {
            template <typename T>
            inline utility::string_t ostringstreamed(const T& value)
            {
                std::ostringstream os; os << value; return utility::s2us(os.str());
            }

            inline web::json::value json_from_message(const slog::async_log_message& message, const id& id, const tai& cursor)
            {
                auto json_message = web::json::value_of({
                    { U("timestamp"), ostringstreamed(slog::put_timestamp(message.timestamp(), "%Y-%m-%dT%H:%M:%06.3SZ")) },
                    { U("level"), message.level() },
                    { U("level_name"), ostringstreamed(slog::put_severity_name(message.level())) },
                    { U("thread_id"), ostringstreamed(message.thread_id()) },
                    { U("source_location"), web::json::value_of({
                        { U("file"), utility::s2us(message.file()) },
                        { U("line"), message.line() },
                        { U("function"), utility::s2us(message.function()) }
                    }, true) },
                    { U("message"), utility::s2us(message.str()) },
                    // adding a unique id, and unique cursor, just to allow the API to provide access to events in the standard REST manner
                    { U("id"), id },
                    { U("cursor"), nmos::make_version(cursor) }
                }, true);

                // a few useful optional properties are only stashed in some log messages
                const auto http_method = nmos::get_http_method_stash(message.stream());
                if (!http_method.empty()) json_message[U("http_method")] = web::json::value::string(http_method);
                const auto request_uri = nmos::get_request_uri_stash(message.stream());
                if (!request_uri.is_empty()) json_message[U("request_uri")] = web::json::value::string(request_uri.to_string());
                const auto route_parameters = nmos::get_route_parameters_stash(message.stream());
                if (!route_parameters.empty()) json_message[U("route_parameters")] = web::json::value_from_fields(route_parameters);
                const auto categories = nmos::get_categories_stash(message.stream());
                if (!categories.empty()) json_message[U("tags")][U("category")] = web::json::value_from_elements(categories | boost::adaptors::transformed(utility::s2us));

                return json_message;
            }
        }

        // logically necessary, practically not!
        inline tai strictly_increasing_cursor(const events& events, tai cursor = tai_now())
        {
            const auto most_recent = events.empty() ? tai{} : fields::cursor(events.front().data);
            return cursor > most_recent ? cursor : tai_from_time_point(time_point_from_tai(most_recent) + tai_clock::duration(1));
        }

        void insert_log_event(events& events, const slog::async_log_message& message, const id& id)
        {
            // capacity ought to be part of log/settings
            const std::size_t capacity = 1234;
            while (events.size() > capacity)
            {
                events.pop_back();
            }
            events.push_front({ details::json_from_message(message, id, strictly_increasing_cursor(events)) });
        }
    }
}

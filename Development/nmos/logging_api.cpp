#include "nmos/logging_api.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/api_utils.h"
#include "nmos/query_utils.h"
#include "nmos/slog.h"
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

            logging_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("log/") }, req, res));
                return pplx::task_from_result(true);
            });

            logging_api.support(U("/log/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("v1.0/") }, req, res));
                return pplx::task_from_result(true);
            });

            logging_api.mount(U("/log/v1.0"), make_unmounted_logging_api(model, gate));

            return logging_api;
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
        struct log_event_query
        {
            typedef const nmos::experimental::log_event& argument_type;
            typedef bool result_type;

            log_event_query(const web::json::value& flat_query_params);

            result_type operator()(argument_type event) const;

            // the query/exemplar object for a Basic Query
            web::json::value basic_query;

            // a representation of the RQL abstract syntax tree for an Advanced Query
            web::json::value rql_query;
        };

        log_event_query::log_event_query(const web::json::value& flat_query_params)
            : basic_query(web::json::unflatten(flat_query_params))
        {
            if (basic_query.has_field(U("paging")))
            {
                basic_query.erase(U("paging"));
            }
            if (basic_query.has_field(U("query")))
            {
                auto& advanced = basic_query.at(U("query")).as_object();
                for (auto& field : advanced)
                {
                    if (field.first == U("rql"))
                    {
                        rql_query = rql::parse_query(field.second.as_string());
                    }
                    // an error is reported for unimplemented parameters
                    else
                    {
                        throw std::runtime_error("unimplemented parameter - query." + utility::us2s(field.first));
                    }
                }
                basic_query.erase(U("query"));
            }
        }

        log_event_query::result_type log_event_query::operator()(argument_type event) const
        {
            return web::json::match_query(event.data, basic_query, web::json::match_icase | web::json::match_substr)
                && match_logging_rql(event.data, rql_query);
        }

        // Cursor-based paging parameters
        struct log_event_paging
        {
            explicit log_event_paging(const web::json::value& flat_query_params, const nmos::tai& max_until = nmos::tai_max(), size_t default_limit = (std::numeric_limits<size_t>::max)(), size_t max_limit = (std::numeric_limits<size_t>::max)());

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
            boost::any_range<const nmos::experimental::log_event, boost::bidirectional_traversal_tag, const nmos::experimental::log_event&, std::ptrdiff_t> page(const nmos::experimental::log_events& events, Predicate match)
            {
                return paging::cursor_based_page(events, match, until, since, limit, !since_specified);
            }
        };

        log_event_paging::log_event_paging(const web::json::value& flat_query_params, const nmos::tai& max_until, size_t default_limit, size_t max_limit)
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
                        limit = (size_t)field.second.as_integer();
                        if (limit > max_limit) limit = max_limit;
                    }
                    // an error is reported for unimplemented parameters
                    else
                    {
                        throw std::runtime_error("unimplemented parameter - paging." + utility::us2s(field.first));
                    }
                }
            }
        }

        bool log_event_paging::valid() const
        {
            return since <= until;
        }

        namespace details
        {
            // make user error information (to be used with status_codes::BadRequest)
            utility::string_t make_valid_paging_error(const nmos::experimental::log_event_paging& paging)
            {
                return U("the value of the 'paging.since' parameter must be less than or equal to the value of the 'paging.until' parameter");
            }
        }

        // Cursor-based paging customisation points

        inline nmos::tai extract_cursor(const nmos::experimental::log_events&, nmos::experimental::log_events::const_iterator it)
        {
            return it->cursor;
        }

        inline nmos::experimental::log_events::const_iterator lower_bound(const nmos::experimental::log_events& index, const nmos::tai& cursor)
        {
            return std::lower_bound(index.begin(), index.end(), cursor, [](const nmos::experimental::log_event& event, const nmos::tai& cursor)
            {
                return event.cursor > cursor;
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

            web::uri make_query_uri_with_no_paging(const web::http::http_request& req, const nmos::settings& settings)
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

                // get the request host and port (or use the primary host address, and port, from settings)
                auto req_host_port = web::http::get_host_port(req);
                if (req_host_port.first.empty())
                {
                    req_host_port.first = nmos::get_host(settings);
                }
                if (0 == req_host_port.second)
                {
                    req_host_port.second = nmos::experimental::fields::logging_port(settings);
                }

                return web::uri_builder()
                    .set_scheme(nmos::http_scheme(settings)) // no means to detect the API protocol from the request unless reverse proxy added X-Forwarded-Proto?
                    .set_host(req_host_port.first)
                    .set_port(req_host_port.second)
                    .set_path(req.request_uri().path())
                    .set_query(make_query_parameters(query_params))
                    .to_uri();
            }

            void add_paging_headers(web::http::http_headers& headers, const nmos::experimental::log_event_paging& paging, const web::uri& base_link)
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

            logging_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("events/") }, req, res));
                return pplx::task_from_result(true);
            });

            logging_api.support(U("/events/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();

                // Extract and decode the query string

                auto flat_query_params = details::parse_query_parameters(req.request_uri().query());

                // Configure the query predicate

                log_event_query match(flat_query_params);

                // Configure the paging parameters

                // Use the paging limit (default and max) from the setings
                log_event_paging paging(flat_query_params, tai_now(), (size_t)nmos::experimental::fields::logging_paging_default(model.settings), (size_t)nmos::experimental::fields::logging_paging_limit(model.settings));

                if (paging.valid())
                {
                    // Get the payload and update the paging parameters
                    struct default_constructible_event_query_wrapper { const log_event_query* impl; bool operator()(const log_event& e) const { return (*impl)(e); } };
                    auto page = paging.page(model.events, default_constructible_event_query_wrapper{ &match });

                    size_t count = 0;

                    set_reply(res, status_codes::OK,
                        web::json::serialize_array(page
                            | boost::adaptors::transformed(
                                [&count](const log_event& event){ ++count; return event.data; }
                            )),
                        web::http::details::mime_types::application_json);

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Returning " << count << " matching log events";

                    details::add_paging_headers(res.headers(), paging, details::make_query_uri_with_no_paging(req, model.settings));
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

                auto& by_id = model.events.get<tags::id>();
                auto event = by_id.find(eventId);
                if (by_id.end() != event)
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
    }
}

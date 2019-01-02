#include "nmos/query_api.h"

#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/json_validator.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/slog.h"
#include "nmos/version.h"

namespace nmos
{
    inline web::http::experimental::listener::api_router make_unmounted_query_api(nmos::registry_model& model, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_query_api(nmos::registry_model& model, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router query_api;

        query_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, res));
            return pplx::task_from_result(true);
        });

        query_api.support(U("/x-nmos/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("query/") }, res));
            return pplx::task_from_result(true);
        });

        query_api.support(U("/x-nmos/") + nmos::patterns::query_api.pattern + U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("v1.0/"), U("v1.1/"), U("v1.2/") }, res));
            return pplx::task_from_result(true);
        });

        query_api.mount(U("/x-nmos/") + nmos::patterns::query_api.pattern + U("/") + nmos::patterns::is04_version.pattern, make_unmounted_query_api(model, gate));

        return query_api;
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
            if (flat_query_params.has_field(nmos::experimental::fields::query_strip))
            {
                flat_query_params[nmos::experimental::fields::query_strip] = web::json::value::parse(nmos::experimental::fields::query_strip(flat_query_params));
            }

            return flat_query_params;
        }

        web::uri make_query_uri_with_no_paging(const web::http::http_request& req, const nmos::settings& settings)
        {
            // could rebuild the query parameters from the decoded and parsed query string, rather than individually deleting the paging parameters from the request?
            auto query_params = parse_query_parameters(req.request_uri().query());
            if (query_params.has_field(U("paging.order")))
            {
                query_params.erase(U("paging.order"));
            }
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
                req_host_port.first = nmos::fields::host_address(settings);
            }
            if (0 == req_host_port.second)
            {
                req_host_port.second = nmos::fields::query_port(settings);
            }

            return web::uri_builder()
                .set_scheme(U("http")) // for now, no means to detect the API protocol?
                .set_host(req_host_port.first)
                .set_port(req_host_port.second)
                .set_path(req.request_uri().path()) // could also build from the route parameters, version and resourceType?
                .set_query(make_query_parameters(query_params))
                .to_uri();
        }

        void add_paging_headers(web::http::http_headers& headers, const nmos::resource_paging& paging, const web::uri& base_link)
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
                .append_query(U("paging.order=") + (paging.order_by_created ? US("create") : US("update")))
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

    inline web::http::experimental::listener::api_router make_unmounted_query_api(nmos::registry_model& model, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router query_api;

        query_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("nodes/"), U("devices/"), U("sources/"), U("flows/"), U("senders/"), U("receivers/"), U("subscriptions/") }, res));
            return pplx::task_from_result(true);
        });

        query_api.support(U("/") + nmos::patterns::queryType.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.registry_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceType = parameters.at(nmos::patterns::queryType.name);

            // Extract and decode the query string

            const auto flat_query_params = details::parse_query_parameters(req.request_uri().query());

            // Configure the query predicate

            const resource_query match(version, U('/') + resourceType, flat_query_params);

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Querying " << resourceType;

            // Configure the paging parameters

            // Limit queries to the current resources (although tai_now() would also be an option?) and use the paging limit (default and max) from the setings
            resource_paging paging(flat_query_params, most_recent_update(resources), (size_t)nmos::fields::query_paging_default(model.settings), (size_t)nmos::fields::query_paging_limit(model.settings));

            if (paging.valid())
            {
                // Get the payload and update the paging parameters
                struct default_constructible_resource_query_wrapper { const resource_query* impl; bool operator()(const nmos::resource& r) const { return (*impl)(r); } };
                auto page = paging.page(resources, default_constructible_resource_query_wrapper{ &match }); // std::cref(match) is OK from Boost.Range 1.56.0

                size_t count = 0;

                set_reply(res, status_codes::OK,
                    web::json::serialize(page,
                        [&count, &match](const nmos::resources::value_type& resource) { ++count; return match.downgrade(resource); }),
                    U("application/json"));

                slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning " << count << " matching " << resourceType;

                details::add_paging_headers(res.headers(), paging, details::make_query_uri_with_no_paging(req, model.settings));
            }
            else
            {
                set_reply(res, status_codes::BadRequest);
            }

            return pplx::task_from_result(true);
        });

        query_api.support(U("/") + nmos::patterns::queryType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.registry_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceType = parameters.at(nmos::patterns::queryType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            // Extract and decode the query string

            const auto flat_query_params = details::parse_query_parameters(req.request_uri().query());

            // Configure a query predicate, though only downgrade queries are supported on this endpoint, no basic or advanced (RQL) query parameters
            const resource_query match(version, U('/') + resourceType, flat_query_params);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource && nmos::is_permitted_downgrade(*resource, match.version, match.downgrade_version))
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning resource: " << resourceId;
                set_reply(res, status_codes::OK, match.downgrade(*resource));

                // experimental extension, see also nmos::make_resource_events for equivalent WebSockets extension
                if (!match.strip || resource->version < match.version)
                {
                    res.headers().add(U("X-API-Version"), make_api_version(resource->version));
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        const web::json::experimental::json_validator validator
        {
            nmos::experimental::load_json_schema,
            boost::copy_range<std::vector<web::uri>>(is04_versions::all | boost::adaptors::transformed(experimental::make_queryapi_subscriptions_post_request_schema_uri))
        };

        query_api.support(U("/subscriptions/?"), methods::POST, [&model, validator, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            return details::extract_json(req, parameters, gate).then([&, req, res, parameters](value data) mutable
            {
                // could start out as a shared/read lock, only upgraded to an exclusive/write lock when the subscription is actually inserted into resources
                auto lock = model.write_lock();
                auto& resources = model.registry_resources;

                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));

                // Validate JSON syntax according to the schema

                const bool allow_invalid_resources = nmos::fields::allow_invalid_resources(model.settings);
                if (!allow_invalid_resources)
                {
                    validator.validate(data, experimental::make_queryapi_subscriptions_post_request_schema_uri(version));
                }
                else
                {
                    try
                    {
                        validator.validate(data, experimental::make_queryapi_subscriptions_post_request_schema_uri(version));
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "JSON error: " << e.what();
                    }
                }

                // Validate request semantics

                bool valid = true;

                // all of these fields are required - should validate their types too, really
                const bool valid_required_fields = data.has_field(nmos::fields::max_update_rate_ms) && data.has_field(nmos::fields::persist) && data.has_field(nmos::fields::resource_path) && data.has_field(nmos::fields::params);
                valid = valid && valid_required_fields;

                // clients aren't allowed to modify an existing subscription, so check for the fields forbidden in a request
                const bool valid_forbidden_fields = !data.has_field(nmos::fields::id) && !data.has_field(nmos::fields::ws_href);
                valid = valid && valid_forbidden_fields;

                // v1.1 introduced support for secure websockets
                if (nmos::is04_versions::v1_0 != version)
                {
                    // "NB: Default should be 'false' if the API is being presented via HTTP, and 'true' for HTTPS"
                    if (!data.has_field(nmos::fields::secure))
                    {
                        data[nmos::fields::secure] = value::boolean(false); // for now, no means to detect the API protocol?
                    }

                    // for now, only support HTTP
                    const bool valid_secure_field = false == nmos::fields::secure(data);
                    valid = valid && valid_secure_field;
                }

                if (valid)
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Subscription requested on " << nmos::fields::resource_path(data) << ", to be " << (nmos::fields::persist(data) ? "persistent" : "non-persistent");

                    // get the request host
                    auto req_host = web::http::get_host_port(req).first;
                    if (req_host.empty())
                    {
                        req_host = nmos::fields::host_address(model.settings);
                    }

                    // search for a matching existing subscription
                    auto& by_type = resources.get<tags::type>();
                    const auto subscriptions = by_type.equal_range(details::has_data(nmos::types::subscription));
                    auto resource = std::find_if(subscriptions.first, subscriptions.second, [&req_host, &version, &data](const resources::value_type& resource)
                    {
                        return version == resource.version
                            && nmos::fields::max_update_rate_ms(data) == nmos::fields::max_update_rate_ms(resource.data)
                            && nmos::fields::persist(data) == nmos::fields::persist(resource.data)
                            && (nmos::is04_versions::v1_0 == version || nmos::fields::secure(data) == nmos::fields::secure(resource.data))
                            && nmos::fields::resource_path(data) == nmos::fields::resource_path(resource.data)
                            && nmos::fields::params(data) == nmos::fields::params(resource.data)
                            // and finally, a matching subscription must be being served via the same interface as this request
                            // (which, let's approximate by checking the host matches)
                            && req_host == web::uri(nmos::fields::ws_href(resource.data)).host();
                    });
                    const bool creating = subscriptions.second == resource;

                    if (creating)
                    {
                        // unlike registration, it's the server's responsibility to create the subscription id
                        nmos::id id = nmos::make_id();

                        data[nmos::fields::id] = value::string(id);

                        // generate the websocket url
                        data[nmos::fields::ws_href] = value::string(web::uri_builder()
                            .set_scheme(U("ws"))
                            .set_host(req_host)
                            .set_port(nmos::fields::query_ws_port(model.settings))
                            .set_path(U("/x-nmos/query/") + parameters.at(U("version")) + U("/subscriptions/") + id)
                            .to_string());

                        // never expire persistent subscriptions, they are only deleted when explicitly requested
                        nmos::resource subscription{ version, nmos::types::subscription, data, nmos::fields::persist(data) };

                        insert_resource(resources, std::move(subscription));
                    }
                    else
                    {
                        // just return the existing subscription
                        // downgrade doesn't apply to subscriptions; at this point, version must be equal to resource->version
                        data = resource->data;
                    }

                    set_reply(res, creating ? status_codes::Created : status_codes::OK, data);
                    const string_t location(U("/x-nmos/query/") + parameters.at(U("version")) + U("/subscriptions/") + nmos::fields::id(data));
                    res.headers().add(web::http::header_names::location, location);
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Subscription requested is invalid";

                    set_reply(res, status_codes::BadRequest);
                }

                return true;
            });
        });

        query_api.support(U("/subscriptions/") + nmos::patterns::resourceId.pattern + U("/?"), methods::DEL, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            // could start out as a shared/read lock, only upgraded to an exclusive/write lock when the subscription is actually deleted from resources
            auto lock = model.write_lock();
            auto& resources = model.registry_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t subscriptionId = parameters.at(nmos::patterns::resourceId.name);

            auto subscription = find_resource(resources, { subscriptionId, nmos::types::subscription });
            // downgrade doesn't apply to subscriptions; at this point, version must be equal to subscription->version
            if (resources.end() != subscription && subscription->version == version)
            {
                if (nmos::fields::persist(subscription->data))
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Deleting subscription: " << subscriptionId;
                    erase_resource(resources, subscription->id, false);

                    set_reply(res, status_codes::NoContent);
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Not deleting non-persistent subscription: " << subscriptionId;
                    set_error_reply(res, status_codes::Forbidden, U("Forbidden; a non-persistent subscription is managed by the Query API and cannot be deleted"));
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        return query_api;
    }
}

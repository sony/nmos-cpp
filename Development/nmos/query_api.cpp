#include "nmos/query_api.h"

#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/slog.h"
#include "nmos/version.h"

namespace nmos
{
    inline web::http::experimental::listener::api_router make_unmounted_query_api(nmos::model& model, nmos::mutex& mutex, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_query_api(nmos::model& model, nmos::mutex& mutex, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router query_api;

        query_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("x-nmos/") }));
            return pplx::task_from_result(true);
        });

        query_api.support(U("/x-nmos/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("query/") }));
            return pplx::task_from_result(true);
        });

        query_api.support(U("/x-nmos/") + nmos::patterns::query_api.pattern + U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("v1.0/"), JU("v1.1/"), JU("v1.2/") }));
            return pplx::task_from_result(true);
        });

        query_api.mount(U("/x-nmos/") + nmos::patterns::query_api.pattern + U("/") + nmos::patterns::is04_version.pattern, make_unmounted_query_api(model, mutex, gate));

        nmos::add_api_finally_handler(query_api, gate);

        return query_api;
    }

    namespace details
    {
        web::uri make_query_uri_with_no_paging(const web::http::http_request& req, const nmos::settings& settings)
        {
            // could rebuild the query parameters from the decoded and parsed resource query, rather than individually deleting the paging parameters from the request?
            auto query_params = web::json::value_from_query(req.request_uri().query());
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

            // get the request host
            auto req_host = web::http::get_host_port(req).first;
            if (req_host.empty())
            {
                req_host = nmos::fields::host_address(settings);
            }

            return web::uri_builder()
                .set_scheme(U("http")) // for now, no means to detect the API protocol?
                .set_host(req_host)
                .set_port(nmos::fields::query_port(settings)) // could also get from the request?
                .set_path(req.request_uri().path()) // could also build from the route parameters, version and resourceType?
                .set_query(web::json::query_from_value(query_params))
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

    inline web::http::experimental::listener::api_router make_unmounted_query_api(nmos::model& model, nmos::mutex& mutex, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router query_api;

        query_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("nodes/"), JU("devices/"), JU("sources/"), JU("flows/"), JU("senders/"), JU("receivers/"), JU("subscriptions/") }));
            return pplx::task_from_result(true);
        });

        query_api.support(U("/") + nmos::patterns::queryType.pattern + U("/?"), methods::GET, [&model, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::read_lock lock(mutex);

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceType = parameters.at(nmos::patterns::queryType.name);

            auto flat_query_params = web::json::value_from_query(req.request_uri().query());
            for (auto& param : flat_query_params.as_object())
            {
                // special case, RQL needs the URI-encoded string
                if (U("query.rql") == param.first) continue;
                // everything else needs the decoded string
                param.second = web::json::value::string(web::uri::decode(param.second.as_string()));
            }

            // Configure the query predicate

            const resource_query match(version, U('/') + resourceType, flat_query_params);

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Querying " << resourceType;

            // Configure the paging parameters

            // Limit queries to the current resources (although tai_now() would also be an option?) and use the paging limit (default and max) from the setings
            resource_paging paging(flat_query_params, most_recent_update(model.resources), (size_t)nmos::fields::query_paging_default(model.settings), (size_t)nmos::fields::query_paging_limit(model.settings));

            if (paging.valid())
            {
                // Get the payload and update the paging parameters
                struct default_constructible_resource_query_wrapper { const resource_query* impl; bool operator()(const nmos::resource& r) const { return (*impl)(r); } };
                auto page = paging.page(model.resources, default_constructible_resource_query_wrapper{ &match }); // std::cref(match) is OK from Boost.Range 1.56.0

                size_t count = 0;

                set_reply(res, status_codes::OK,
                    web::json::serialize(page,
                        [&count, &version](const nmos::resources::value_type& resource) { ++count; return nmos::downgrade(resource, version); }),
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

        query_api.support(U("/") + nmos::patterns::queryType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::read_lock lock(mutex);

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceType = parameters.at(nmos::patterns::queryType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(model.resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (model.resources.end() != resource && nmos::is_permitted_downgrade(*resource, version))
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning resource: " << resourceId;
                set_reply(res, status_codes::OK, nmos::downgrade(*resource, version));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        query_api.support(U("/subscriptions/?"), methods::POST, [&model, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            return req.extract_json().then([&, req, res, parameters](value data) mutable
            {
                // could start out as a shared/read lock, only upgraded to an exclusive/write lock when the subscription is actually inserted into resources
                nmos::write_lock lock(mutex);

                slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Subscription requested";

                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));

                // Validate request

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
                        data[nmos::fields::secure] = false; // for now, no means to detect the API protocol?
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
                    auto& by_type = model.resources.get<tags::type>();
                    const auto subscriptions = by_type.equal_range(nmos::types::subscription);
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

                        insert_resource(model.resources, std::move(subscription));
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
                    set_reply(res, status_codes::BadRequest);
                }

                return true;
            });
        });

        query_api.support(U("/subscriptions/") + nmos::patterns::resourceId.pattern + U("/?"), methods::DEL, [&model, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            // could start out as a shared/read lock, only upgraded to an exclusive/write lock when the subscription is actually deleted from resources
            nmos::write_lock lock(mutex);

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t subscriptionId = parameters.at(nmos::patterns::resourceId.name);

            auto subscription = find_resource(model.resources, { subscriptionId, nmos::types::subscription });
            // downgrade doesn't apply to subscriptions; at this point, version must be equal to subscription->version
            if (model.resources.end() != subscription && subscription->version == version)
            {
                if (nmos::fields::persist(subscription->data))
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Deleting subscription: " << subscriptionId;
                    erase_resource(model.resources, subscription->id);

                    set_reply(res, status_codes::NoContent);
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Not deleting non-persistent subscription: " << subscriptionId;
                    set_reply(res, status_codes::Forbidden, nmos::make_error_response_body(status_codes::Forbidden, U("Forbidden; a non-persistent subscription is managed by the Query API and cannot be deleted")));
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

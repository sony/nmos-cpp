#include "nmos/query_api.h"

#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/slog.h"

namespace nmos
{
    inline web::http::experimental::listener::api_router make_unmounted_query_api(nmos::model& model, std::mutex& mutex, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_query_api(nmos::model& model, std::mutex& mutex, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router query_api;

        query_api.support(U("/?"), methods::GET, [](const http_request&, http_response& res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("x-nmos/") }));
            return true;
        });

        query_api.support(U("/x-nmos/?"), methods::GET, [](const http_request&, http_response& res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("query/") }));
            return true;
        });

        query_api.support(U("/x-nmos/") + nmos::patterns::query_api.pattern + U("/?"), methods::GET, [](const http_request&, http_response& res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("v1.0/"), JU("v1.1/"), JU("v1.2/") }));
            return true;
        });

        query_api.mount(U("/x-nmos/") + nmos::patterns::query_api.pattern + U("/") + nmos::patterns::is04_version.pattern, make_unmounted_query_api(model, mutex, gate));

        nmos::add_api_finally_handler(query_api, gate);

        return query_api;
    }

    inline web::http::experimental::listener::api_router make_unmounted_query_api(nmos::model& model, std::mutex& mutex, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router query_api;

        query_api.support(U("/?"), methods::GET, [](const http_request&, http_response& res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("nodes/"), JU("devices/"), JU("sources/"), JU("flows/"), JU("senders/"), JU("receivers/"), JU("subscriptions/") }));
            return true;
        });

        query_api.support(U("/") + nmos::patterns::queryType.pattern + U("/?"), methods::GET, [&model, &mutex, &gate](const http_request& req, http_response& res, const string_t&, const route_parameters& parameters)
        {
            std::lock_guard<std::mutex> lock(mutex);

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

            const resource_query match(version, U('/') + resourceType, flat_query_params);

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Querying " << resourceType;

            size_t count = 0;

            set_reply(res, status_codes::OK,
                web::json::serialize_if(model.resources,
                    match,
                    [&count, &version](const nmos::resources::value_type& resource) { ++count; return nmos::downgrade(resource, version); }),
                U("application/json"));

            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning " << count << " matching " << resourceType;

            return true;
        });

        query_api.support(U("/") + nmos::patterns::queryType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &mutex, &gate](const http_request& req, http_response& res, const string_t&, const route_parameters& parameters)
        {
            std::lock_guard<std::mutex> lock(mutex);

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceType = parameters.at(nmos::patterns::queryType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = model.resources.find(resourceId);
            if (model.resources.end() != resource)
            {
                if (resource->type == nmos::type_from_resourceType(resourceType) && nmos::is_permitted_downgrade(*resource, version))
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning resource: " << resourceId;
                    set_reply(res, status_codes::OK, nmos::downgrade(*resource, version));
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return true;
        });

        query_api.support(U("/subscriptions/?"), methods::POST, [&model, &mutex, &gate](const http_request& req, http_response& res, const string_t&, const route_parameters& parameters)
        {
            std::lock_guard<std::mutex> lock(mutex);

            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Subscription requested";

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));

            // Extract request body

            value data = req.extract_json().get();

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
                resources::iterator resource = std::find_if(model.resources.begin(), model.resources.end(), [&req_host, &version, &data](const resources::value_type& resource)
                {
                    return version == resource.version
                        && nmos::types::subscription == resource.type
                        && nmos::fields::max_update_rate_ms(data) == nmos::fields::max_update_rate_ms(resource.data)
                        && nmos::fields::persist(data) == nmos::fields::persist(resource.data)
                        && (nmos::is04_versions::v1_0 == version || nmos::fields::secure(data) == nmos::fields::secure(resource.data))
                        && nmos::fields::resource_path(data) == nmos::fields::resource_path(resource.data)
                        && nmos::fields::params(data) == nmos::fields::params(resource.data)
                        // and finally, a matching subscription must be being served via the same interface as this request
                        // (which, let's approximate by checking the host matches)
                        && req_host == web::uri(nmos::fields::ws_href(resource.data)).host();
                });
                const bool creating = model.resources.end() == resource;

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

        query_api.support(U("/subscriptions/") + nmos::patterns::resourceId.pattern + U("/?"), methods::DEL, [&model, &mutex, &gate](const http_request& req, http_response& res, const string_t&, const route_parameters& parameters)
        {
            std::lock_guard<std::mutex> lock(mutex);

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t subscriptionId = parameters.at(nmos::patterns::resourceId.name);

            auto subscription = model.resources.find(subscriptionId);
            if (model.resources.end() != subscription)
            {
                // downgrade doesn't apply to subscriptions; at this point, version must be equal to subscription->version
                if (subscription->type == nmos::types::subscription && subscription->version == version)
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
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return true;
        });

        return query_api;
    }
}

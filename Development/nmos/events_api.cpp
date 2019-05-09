#include "nmos/events_api.h"

#include "nmos/api_utils.h"
#include "nmos/is07_versions.h"
#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    web::http::experimental::listener::api_router make_unmounted_events_api(const nmos::node_model& model, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_events_api(const nmos::node_model& model, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router events_api;

        events_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, res));
            return pplx::task_from_result(true);
        });

        events_api.support(U("/x-nmos/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("events/") }, res));
            return pplx::task_from_result(true);
        });

        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is07_versions::from_settings(model.settings); });
        events_api.support(U("/x-nmos/") + nmos::patterns::events_api.pattern + U("/?"), methods::GET, [versions](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), res));
            return pplx::task_from_result(true);
        });

        events_api.mount(U("/x-nmos/") + nmos::patterns::events_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_events_api(model, gate));

        return events_api;
    }

    web::http::experimental::listener::api_router make_unmounted_events_api(const nmos::node_model& model, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router events_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is07_versions::from_settings(model.settings); });
        events_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        events_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("sources/") }, res));
            return pplx::task_from_result(true);
        });

        events_api.support(U("/") + nmos::patterns::sourceType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.events_resources;

            const string_t resourceType = parameters.at(nmos::patterns::sourceType.name);

            set_reply(res, status_codes::OK,
                web::json::serialize_if(resources,
                    [&](const nmos::resource& resource) { return resource.type == nmos::type_from_resourceType(resourceType); },
                    [](const nmos::resource& resource) { return value(resource.id + U("/")); }),
                web::http::details::mime_types::application_json);

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << resources.size() << " matching " << resourceType;

            return pplx::task_from_result(true);
        });

        events_api.support(U("/") + nmos::patterns::sourceType.pattern + U("/?") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.events_resources;

            const string_t resourceType = parameters.at(nmos::patterns::sourceType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("state/"), U("type/") }, res));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }
            return pplx::task_from_result(true);
        });

        events_api.support(U("/") + nmos::patterns::sourceType.pattern + U("/?") + nmos::patterns::resourceId.pattern + U("/") + nmos::patterns::eventTypeState.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.events_resources;

            const string_t resourceType = parameters.at(nmos::patterns::sourceType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };

            const string_t eventTypeState = parameters.at(nmos::patterns::eventTypeState.name);
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Returning " << eventTypeState << " for " << id_type;

                const web::json::field_as_value endpoint_eventTypeState{ eventTypeState };
                set_reply(res, status_codes::OK, endpoint_eventTypeState(resource->data));
            }
            else
            {
                // see https://github.com/AMWA-TV/nmos-event-tally/issues/23
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        return events_api;
    }
}

#include "nmos/events_api.h"

#include "nmos/api_utils.h"
#include "nmos/format.h"
#include "nmos/is07_versions.h"
#include "nmos/model.h"

namespace nmos
{

    web::http::experimental::listener::api_router make_unmounted_events_api(const nmos::node_model& model, slog::base_gate& gate_) {
        using namespace web::http::experimental::listener::api_router_using_declarations;
        using web::json::value;

        api_router events_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is07_versions::from_settings(model.settings); });
        events_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        events_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("sources/") }, res));
            return pplx::task_from_result(true);
        });

        events_api.support(U("/sources/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const auto match = [](const nmos::resources::value_type& resource) { return resource.type == nmos::types::source && resource.data.at(U("format")) == value::string(nmos::formats::data.name) && resource.data.has_field(U("event_type")); };

            set_reply(res, status_codes::OK,
                web::json::serialize_if(resources,
                    match,
                    [](const nmos::resources::value_type& resource) { return value::string(resource.data.at(U("id")).as_string() + "/"); }),
                U("application/json"));
            return pplx::task_from_result(true);
        });

        events_api.support(U("/sources/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.event_resources;

            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);
            auto resource = find_resource(resources, { resourceId, nmos::types::event_restapi });
            if (resources.end() != resource)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Returning resource: " << resourceId;
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("state/"), U("type/") }, res));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }
            return pplx::task_from_result(true);
        });

        events_api.support(U("/sources/") + nmos::patterns::resourceId.pattern + U("/") + nmos::patterns::eventsApiSubResource.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.event_resources;

            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);
            const string_t eventsApiSubResource = parameters.at(nmos::patterns::eventsApiSubResource.name);
            auto resource = find_resource(resources, { resourceId, nmos::types::event_restapi });
            if (resources.end() != resource)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Returning resource: " << resourceId;
                set_reply(res, status_codes::OK, resource->data.at(eventsApiSubResource));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }
            return pplx::task_from_result(true);
        });

        return events_api;
    }

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
        events_api.support(U("/x-nmos/events/?"), methods::GET, [versions](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), res));
            return pplx::task_from_result(true);
        });

        events_api.mount(U("/x-nmos/events/")+ nmos::patterns::version.pattern, make_unmounted_events_api(model, gate));

        return events_api;
    }


}

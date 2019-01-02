#include "nmos/node_api.h"

#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/json_validator.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/slog.h"
#include "cpprest/host_utils.h"

namespace nmos
{
    web::http::experimental::listener::api_router make_unmounted_node_api(const nmos::model& model, node_api_target_handler target_handler, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_node_api(const nmos::model& model, node_api_target_handler target_handler, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router node_api;

        node_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, res));
            return pplx::task_from_result(true);
        });

        node_api.support(U("/x-nmos/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("node/") }, res));
            return pplx::task_from_result(true);
        });

        node_api.support(U("/x-nmos/") + nmos::patterns::node_api.pattern + U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("v1.0/"), U("v1.1/"), U("v1.2/") }, res));
            return pplx::task_from_result(true);
        });

        node_api.mount(U("/x-nmos/") + nmos::patterns::node_api.pattern + U("/") + nmos::patterns::is04_version.pattern, make_unmounted_node_api(model, target_handler, gate));

        return node_api;
    }

    web::http::experimental::listener::api_router make_unmounted_node_api(const nmos::model& model, node_api_target_handler target_handler, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router node_api;

        node_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("self/"), U("devices/"), U("sources/"), U("flows/"), U("senders/"), U("receivers/") }, res));
            return pplx::task_from_result(true);
        });

        node_api.support(U("/self/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));

            auto resource = nmos::find_self_resource(resources);
            if (resources.end() != resource && nmos::is_permitted_downgrade(*resource, version))
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning self resource: " << resource->id;
                set_reply(res, status_codes::OK, nmos::downgrade(*resource, version));
            }
            else
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Self resource not found!";
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        node_api.support(U("/") + nmos::patterns::subresourceType.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceType = parameters.at(nmos::patterns::subresourceType.name);

            const auto match = [&](const nmos::resources::value_type& resource) { return resource.type == nmos::type_from_resourceType(resourceType) && nmos::is_permitted_downgrade(resource, version); };

            size_t count = 0;

            set_reply(res, status_codes::OK,
                web::json::serialize_if(resources,
                    match,
                    [&count, &version](const nmos::resources::value_type& resource) { ++count; return nmos::downgrade(resource, version); }),
                U("application/json"));

            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning " << count << " matching " << resourceType;

            return pplx::task_from_result(true);
        });

        node_api.support(U("/") + nmos::patterns::subresourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceType = parameters.at(nmos::patterns::subresourceType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource && nmos::is_permitted_downgrade(*resource, version))
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

        const web::json::experimental::json_validator validator
        {
            nmos::experimental::load_json_schema,
            boost::copy_range<std::vector<web::uri>>(is04_versions::all | boost::adaptors::transformed(experimental::make_nodeapi_receiver_target_put_request_schema_uri))
        };

        node_api.support(U("/receivers/") + nmos::patterns::resourceId.pattern + U("/target"), methods::PUT, [&model, target_handler, validator, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::types::receiver });
            if (resources.end() != resource && nmos::is_permitted_downgrade(*resource, version))
            {
                if (target_handler)
                {
                    return details::extract_json(req, parameters, gate).then([target_handler, validator, version, resourceId, res](value sender_data) mutable
                    {
                        validator.validate(sender_data, experimental::make_nodeapi_receiver_target_put_request_schema_uri(version));

                        return target_handler(resourceId, sender_data).then([res, sender_data]() mutable
                        {
                            set_reply(res, status_codes::Accepted, sender_data);
                            return true;
                        });
                    });
                }
                else
                {
                    set_reply(res, status_codes::NotImplemented);
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        return node_api;
    }
}

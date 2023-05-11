#include "nmos/rwnode_api.h"

#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_validator.h"
#include "nmos/api_utils.h"
#include "nmos/is13_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    web::http::experimental::listener::api_router make_unmounted_rwnode_api(nmos::model& model, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_rwnode_api(nmos::model& model, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router rwnode_api;

        rwnode_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
            return pplx::task_from_result(true);
        });

        rwnode_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("rwnode/") }, req, res));
            return pplx::task_from_result(true);
        });

        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is13_versions::from_settings(model.settings); });
        rwnode_api.support(U("/x-nmos/") + nmos::patterns::rwnode_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
            return pplx::task_from_result(true);
        });

        rwnode_api.mount(U("/x-nmos/") + nmos::patterns::rwnode_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_rwnode_api(model, gate));

        return rwnode_api;
    }

    web::json::value make_rwnode_response(const nmos::resource& resource)
    {
        using web::json::value_of;
        return value_of({
            { nmos::fields::id, resource.data.at(nmos::fields::id) },
            { nmos::fields::version, resource.data.at(nmos::fields::version) },
            { nmos::fields::label, resource.data.at(nmos::fields::label) },
            { nmos::fields::description, resource.data.at(nmos::fields::description) },
            { nmos::fields::tags, resource.data.at(nmos::fields::tags) }
        });
    }

    void merge_rwnode_request(web::json::value& value, const web::json::value& patch)
    {
        web::json::merge_patch(value, patch, true);
        web::json::insert(value, std::make_pair(nmos::fields::label, U("")));
        web::json::insert(value, std::make_pair(nmos::fields::description, U("")));
        web::json::insert(value, std::make_pair(nmos::fields::tags, web::json::value::object()));
    }

    web::http::experimental::listener::api_router make_unmounted_rwnode_api(nmos::model& model, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router rwnode_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is13_versions::from_settings(model.settings); });
        rwnode_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        rwnode_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("self/"), U("devices/"), U("sources/"), U("flows/"), U("senders/"), U("receivers/") }, req, res));
            return pplx::task_from_result(true);
        });

        rwnode_api.support(U("/self/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

            auto resource = nmos::find_self_resource(resources);
            if (resources.end() != resource)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Returning self resource: " << resource->id;
                set_reply(res, status_codes::OK, nmos::make_rwnode_response(*resource));
            }
            else
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Self resource not found!";
                set_reply(res, status_codes::InternalError); // rather than Not Found, since the Read/Write Node API doesn't allow a 404 response
            }

            return pplx::task_from_result(true);
        });

        const web::json::experimental::json_validator validator
        {
            nmos::experimental::load_json_schema,
            boost::copy_range<std::vector<web::uri>>(versions | boost::adaptors::transformed(experimental::make_rwnodeapi_resource_core_patch_request_schema_uri))
        };

        rwnode_api.support(U("/self/?"), methods::PATCH, [&model, validator, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);

            return details::extract_json(req, gate).then([&model, &validator, req, res, parameters, gate](value body) mutable
            {
                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

                validator.validate(body, experimental::make_rwnodeapi_resource_core_patch_request_schema_uri(version));

                auto lock = model.write_lock();
                auto& resources = model.node_resources;

                auto resource = nmos::find_self_resource(resources);
                if (resources.end() != resource)
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Patching self resource: " << resource->id;

                    modify_resource(resources, resource->id, [&body](nmos::resource& resource)
                    {
                        resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());
                        nmos::merge_rwnode_request(resource.data, body);
                    });

                    set_reply(res, status_codes::OK, nmos::make_rwnode_response(*resource));

                    model.notify();
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Self resource not found!";
                    set_reply(res, status_codes::InternalError); // rather than Not Found, since the Read/Write Node API doesn't allow a 404 response
                }

                return true;
            });
        });

        rwnode_api.support(U("/") + nmos::patterns::subresourceType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            const string_t resourceType = parameters.at(nmos::patterns::subresourceType.name);

            const auto match = [&](const nmos::resources::value_type& resource) { return resource.type == nmos::type_from_resourceType(resourceType); };

            size_t count = 0;

            set_reply(res, status_codes::OK,
                web::json::serialize_array(resources
                    | boost::adaptors::filtered(match)
                    | boost::adaptors::transformed(
                        [&count, &version](const nmos::resources::value_type& resource) { ++count; return nmos::make_rwnode_response(resource); }
                    )),
                web::http::details::mime_types::application_json);

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << count << " matching " << resourceType;

            return pplx::task_from_result(true);
        });

        rwnode_api.support(U("/") + nmos::patterns::subresourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            const string_t resourceType = parameters.at(nmos::patterns::subresourceType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Returning " << id_type;
                set_reply(res, status_codes::OK, nmos::make_rwnode_response(*resource));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        rwnode_api.support(U("/") + nmos::patterns::subresourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::PATCH, [&model, validator, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);

            return details::extract_json(req, gate).then([&model, &validator, req, res, parameters, gate](value body) mutable
            {
                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

                validator.validate(body, experimental::make_rwnodeapi_resource_core_patch_request_schema_uri(version));

                auto lock = model.write_lock();
                auto& resources = model.node_resources;

                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Patching " << id_type;

                    modify_resource(resources, resource->id, [&body](nmos::resource& resource)
                    {
                        resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());
                        nmos::merge_rwnode_request(resource.data, body);
                    });

                    set_reply(res, status_codes::OK, nmos::make_rwnode_response(*resource));

                    model.notify();
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return true;
            });
        });

        return rwnode_api;
    }
}

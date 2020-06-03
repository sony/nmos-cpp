#include "nmos/node_api.h"

#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_validator.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/is04_versions.h"
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

        node_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
            return pplx::task_from_result(true);
        });

        node_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("node/") }, req, res));
            return pplx::task_from_result(true);
        });

        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is04_versions::from_settings(model.settings); });
        node_api.support(U("/x-nmos/") + nmos::patterns::node_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
            return pplx::task_from_result(true);
        });

        node_api.mount(U("/x-nmos/") + nmos::patterns::node_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_node_api(model, target_handler, gate));

        return node_api;
    }

    inline utility::string_t make_node_api_resource_path(const nmos::resource& resource)
    {
        return nmos::types::node == resource.type
            ? U("/self")
            : U("/") + nmos::resourceType_from_type(resource.type) + U("/") + resource.id;
    }

    inline utility::string_t make_node_api_resource_location(const nmos::resource& resource, const utility::string_t& sub_path = {})
    {
        return U("/x-nmos/node/") + nmos::make_api_version(resource.version) + make_node_api_resource_path(resource) + sub_path;
    }

    web::http::experimental::listener::api_router make_unmounted_node_api(const nmos::model& model, node_api_target_handler target_handler, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router node_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is04_versions::from_settings(model.settings); });
        node_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        node_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("self/"), U("devices/"), U("sources/"), U("flows/"), U("senders/"), U("receivers/") }, req, res));
            return pplx::task_from_result(true);
        });

        node_api.support(U("/self/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

            auto resource = nmos::find_self_resource(resources);
            if (resources.end() != resource)
            {
                if (nmos::is_permitted_downgrade(*resource, version))
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Returning self resource: " << resource->id;
                    set_reply(res, status_codes::OK, nmos::downgrade(*resource, version));
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Self resource version is incorrect!";
                    set_error_reply(res, status_codes::InternalError, U("Internal Error; ") + details::make_permitted_downgrade_error(*resource, version));
                    // experimental extension, for debugging purposes
                    res.headers().add(web::http::header_names::location, make_node_api_resource_location(*resource));
                }
            }
            else
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Self resource not found!";
                set_reply(res, status_codes::InternalError); // rather than Not Found, since the Node API doesn't allow a 404 response
            }

            return pplx::task_from_result(true);
        });

        node_api.support(U("/") + nmos::patterns::subresourceType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            const string_t resourceType = parameters.at(nmos::patterns::subresourceType.name);

            const auto match = [&](const nmos::resources::value_type& resource) { return resource.type == nmos::type_from_resourceType(resourceType) && nmos::is_permitted_downgrade(resource, version); };

            size_t count = 0;

            set_reply(res, status_codes::OK,
                web::json::serialize_array(resources
                    | boost::adaptors::filtered(match)
                    | boost::adaptors::transformed(
                        [&count, &version](const nmos::resources::value_type& resource) { ++count; return nmos::downgrade(resource, version); }
                    )),
                web::http::details::mime_types::application_json);

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << count << " matching " << resourceType;

            return pplx::task_from_result(true);
        });

        node_api.support(U("/") + nmos::patterns::subresourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            const string_t resourceType = parameters.at(nmos::patterns::subresourceType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource)
            {
                if (nmos::is_permitted_downgrade(*resource, version))
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Returning resource: " << resourceId;
                    set_reply(res, status_codes::OK, nmos::downgrade(*resource, version));
                }
                else
                {
                    // experimental extension, proposed for v1.3, to distinguish from Not Found
                    set_error_reply(res, status_codes::Conflict, U("Conflict; ") + details::make_permitted_downgrade_error(*resource, version));
                    res.headers().add(web::http::header_names::location, make_node_api_resource_location(*resource));
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
            boost::copy_range<std::vector<web::uri>>(versions | boost::adaptors::transformed(experimental::make_nodeapi_receiver_target_put_request_schema_uri))
        };

        node_api.support(U("/receivers/") + nmos::patterns::resourceId.pattern + U("/target"), methods::PUT, [&model, target_handler, validator, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            // hmmm, fragile; make shared, and capture into continuation below, in order to extend lifetime until after target handler
            std::shared_ptr<nmos::api_gate> gate(new nmos::api_gate(gate_, req, parameters));

            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::types::receiver });
            if (resources.end() != resource)
            {
                if (nmos::is_permitted_downgrade(*resource, version))
                {
                    if (target_handler)
                    {
                        return details::extract_json(req, *gate).then([target_handler, validator, version, resourceId, res, gate](value sender_data) mutable
                        {
                            validator.validate(sender_data, experimental::make_nodeapi_receiver_target_put_request_schema_uri(version));

                            return target_handler(resourceId, sender_data, *gate).then([res, sender_data, gate]() mutable
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
                    // experimental extension, proposed for v1.3, to distinguish from Not Found
                    set_error_reply(res, status_codes::Conflict, U("Conflict; ") + details::make_permitted_downgrade_error(*resource, version));
                    res.headers().add(web::http::header_names::location, make_node_api_resource_location(*resource, U("/target")));
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

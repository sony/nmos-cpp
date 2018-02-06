#include "nmos/connection_api.h"

#include "nmos/activation_mode.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/slog.h"
#include "nmos/version.h"

namespace nmos
{
    web::http::experimental::listener::api_router make_unmounted_connection_api(nmos::resources& resources, nmos::mutex& mutex, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_connection_api(nmos::resources& resources, nmos::mutex& mutex, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router connection_api;

        connection_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("x-nmos/" })));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/x-nmos/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("connection/") }));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/x-nmos/") + nmos::patterns::connection_api.pattern + U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("v1.0/") }));
            return pplx::task_from_result(true);
        });

        connection_api.mount(U("/x-nmos/") + nmos::patterns::connection_api.pattern + U("/") + nmos::patterns::is05_version.pattern, make_unmounted_connection_api(resources, mutex, gate));

        nmos::add_api_finally_handler(connection_api, gate);

        return connection_api;
    }

    web::http::experimental::listener::api_router make_unmounted_connection_api(nmos::resources& resources, nmos::mutex& mutex, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router connection_api;

        connection_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("bulk/"), JU("single/") }));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/bulk/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("senders/"), JU("receivers/") }));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/bulk/") + nmos::patterns::connectorType.pattern + U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::MethodNotAllowed);
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/bulk/") + nmos::patterns::connectorType.pattern + U("/?"), methods::POST, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::NotImplemented);
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("senders/"), JU("receivers/") }));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/?"), methods::GET, [&resources, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::read_lock lock(mutex);

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);

            const auto match = [&](const nmos::resources::value_type& resource) { return resource.type == nmos::type_from_resourceType(resourceType) && nmos::is_permitted_downgrade(resource, nmos::is04_versions::v1_2); };

            size_t count = 0;

            set_reply(res, status_codes::OK,
                web::json::serialize_if(resources,
                    match,
                    [&count](const nmos::resources::value_type& resource) { ++count; return value(resource.id + U("/")); }),
                U("application/json"));

            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning " << count << " matching " << resourceType;

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&resources, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::read_lock lock(mutex);

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource && nmos::is_permitted_downgrade(*resource, nmos::is04_versions::v1_2))
            {
                if (nmos::types::sender == resource->type)
                {
                    set_reply(res, status_codes::OK, value_of({ JU("constraints/"), JU("staged/"), JU("active/"), JU("transportfile/") }));
                }
                else // if (nmos::types::receiver == resource->type)
                {
                    set_reply(res, status_codes::OK, value_of({ JU("constraints/"), JU("staged/"), JU("active/") }));
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/?"), methods::GET, [&resources, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            set_reply(res, status_codes::NotImplemented);
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/staged/?"), methods::GET, [&resources, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            set_reply(res, status_codes::NotImplemented);
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/staged/?"), methods::PATCH, [&resources, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            return req.extract_json().then([&, req, res, parameters](value body) mutable
            {
                // could start out as a shared/read lock, only upgraded to an exclusive/write lock when the sender/receiver in the resources is actually modified
                nmos::write_lock lock(mutex);

                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                // Validate request?

                const auto master_enable = nmos::fields::master_enable(body);
                const auto activation = nmos::fields::activation(body);
                const auto mode = nmos::fields::mode(activation);

                auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
                if (resources.end() != resource && nmos::is_permitted_downgrade(*resource, nmos::is04_versions::v1_2))
                {
                    if (nmos::activation_modes::activate_immediate == mode)
                    {
                        modify_resource(resources, resourceId, [&](nmos::resource& resource)
                        {
                            resource.data[U("version")] = value::string(nmos::make_version(resource.updated));

                            auto& subscription = resource.data[U("subscription")];
                            subscription[U("active")] = value::boolean(master_enable);
                            if (nmos::types::sender == resource.type)
                            {
                                subscription[U("receiver_id")] = master_enable ? body[U("receiver_id")] : value::null();
                            }
                            else // if (nmos::types::receiver == resource.type)
                            {
                                subscription[U("sender_id")] = master_enable ? body[U("sender_id")] : value::null();
                            }
                        });

                        set_reply(res, status_codes::OK, value{});
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

                return true;
            });
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/active/?"), methods::GET, [&resources, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            set_reply(res, status_codes::NotImplemented);
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/transportfile/?"), methods::GET, [&resources, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            set_reply(res, status_codes::OK, U("v=0\r\no=- 37 42 IN IP4 127.0.0.1 \r\ns= \r\nt=0 0\r\n"), U("application/sdp"));
            return pplx::task_from_result(true);
        });

        return connection_api;
    }
}

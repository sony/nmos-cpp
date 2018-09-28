#include "nmos/connection_api.h"

#include "nmos/activation_mode.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/model.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h"
#include "nmos/version.h"

namespace nmos
{
    web::http::experimental::listener::api_router make_unmounted_connection_api(nmos::node_model& model, nmos::activate_function activate, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, nmos::activate_function activate, slog::base_gate& gate)
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

        connection_api.mount(U("/x-nmos/") + nmos::patterns::connection_api.pattern + U("/") + nmos::patterns::is05_version.pattern, make_unmounted_connection_api(model, activate, gate));

        nmos::add_api_finally_handler(connection_api, gate);

        return connection_api;
    }


    template<typename pair_type, typename rtt>
    static bool patch_key_is_valid(const pair_type &pair, const rtt &resourceType)
    {
        return (pair.first == U("sender_id") && resourceType == U("receivers"))  ||
            (pair.first == U("receiver_id") && resourceType == U("senders"))  ||
            (pair.first == U("transport_file") && resourceType == U("receivers"))  ||
            pair.first == U("activation")  ||
            pair.first == U("master_enable")  ||
            pair.first == U("transport_params");
    }

    web::http::experimental::listener::api_router make_unmounted_connection_api(nmos::node_model& model, nmos::activate_function activate, slog::base_gate& gate)
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

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);

            const auto match = [&](const nmos::resources::value_type& resource) { return resource.type == nmos::type_from_resourceType(resourceType); };

            size_t count = 0;

            set_reply(res, status_codes::OK,
                web::json::serialize_if(resources,
                    match,
                    [&count](const nmos::resources::value_type& resource) { ++count; return value(resource.id + U("/")); }),
                U("application/json"));

            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning " << count << " matching " << resourceType;

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource)
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

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() == resource)
            {
                set_reply(res, status_codes::NotFound);
            }
            else
            {
                set_reply(res, status_codes::OK, nmos::fields::endpoint_constraints(resource->data));
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/staged/?"), methods::PATCH, [&model, activate, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            return details::extract_json(req, parameters, gate).then([&, req, res, parameters](value body) mutable
            {
                const auto& activation = nmos::fields::activation(body);
                const auto mode = !activation.is_null() ? nmos::activation_mode{ nmos::fields::mode(activation) } : nmos::activation_modes::none;

                if (nmos::activation_modes::activate_scheduled_absolute == mode || nmos::activation_modes::activate_scheduled_relative == mode)
                {
                    set_reply(res, status_codes::NotImplemented);
                    return true;
                }

                // could start out as a shared/read lock, only upgraded to an exclusive/write lock when the sender/receiver in the resources is actually modified
                auto lock = model.write_lock();
                auto& resources = model.connection_resources;

                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                auto type = nmos::type_from_resourceType(resourceType);
                auto resource = find_resource(resources, { resourceId, type });
                if (resources.end() != resource)
                {
                    // Validate JSON syntax according to the schema ideally...

                    // First, verify that every key is a valid field.
                    for (const auto& pair : body.as_object())
                    {
                        if (!patch_key_is_valid(pair, resourceType))
                        {
                            set_reply(res, status_codes::BadRequest);
                            return true;
                        }
                    }

                    // OK, now it's safe to update everything.
                    auto update = [&body](nmos::resource& resource)
                    {
                        auto& staged = nmos::fields::endpoint_staged(resource.data);

                        for (const auto& pair: body.as_object())
                        {
                            if (pair.first == nmos::fields::transport_file.key || pair.first == nmos::fields::activation.key)
                            {
                                for (const auto& opair : pair.second.as_object())
                                {
                                    staged[pair.first][opair.first] = opair.second;
                                }
                            }
                            else if (pair.first == nmos::fields::transport_params.key)
                            {
                                const auto& a = pair.second.as_array();
                                for (size_t i = 0; i < std::min<size_t>(2, a.size()); i++)
                                {
                                    for (const auto& tp : a.at(i).as_object())
                                    {
                                        staged[pair.first][i][tp.first] = tp.second;
                                    }
                                }
                            }
                            else
                            {
                                staged[pair.first] = pair.second;
                            }
                        }
                    };

                    modify_resource(resources, resourceId, update);

                    auto response = nmos::fields::endpoint_staged(resource->data);

                    if (nmos::activation_modes::none == mode)
                    {
                        // "If no activation was requested in the PATCH `activation_time` will be set `null`."
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml
                        response[nmos::fields::activation][nmos::fields::activation_time] = value();
                    }
                    else if (nmos::activation_modes::activate_immediate == mode)
                    {
                        // should be asynchronous (task-based), with a continuation to return the response
                        {
                            details::reverse_lock_guard<nmos::write_lock> unlock(lock);
                            activate(type, resourceId);
                        }

                        // "For immediate activations on the staged endpoint this property will be the time the
                        // activation actually occurred in the response to the PATCH request, but null in response
                        // to any GET requests thereafter."
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-activation-response-schema.json
                        response[nmos::fields::activation][nmos::fields::activation_time] = value::string(nmos::make_version());

                        // "For an immediate activation this field will always be null on the staged endpoint, even
                        // in the response to the PATCH request."
                        response[nmos::fields::activation][nmos::fields::requested_time] = value();

                        auto update = [&body, &mode](nmos::resource &resource)
                        {
                            auto& staged = nmos::fields::endpoint_staged(resource.data);
                            auto& activation = staged[nmos::fields::activation];

                            // "For immediate activations, in the response to the PATCH request this field
                            // will be set to 'activate_immediate', but will be null in response to any
                            // subsequent GET requests."
                            activation[nmos::fields::mode] = value();

                            // "For immediate activations on the staged endpoint this property will be the time the
                            // activation actually occurred in the response to the PATCH request, but null in response
                            // to any GET requests thereafter."
                            activation[nmos::fields::activation_time] = value();

                            // "This field returns to null once the activation is completed on the staged endpoint."
                            activation[nmos::fields::requested_time] = value();
                        };
                        modify_resource(resources, resourceId, update);
                    }
                    // else pass the buck to a scheduler thread to deal with the activation at the right time,
                    // and reply with the right HTTP status code.

                    set_reply(res, status_codes::OK, response);
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return true;
            });
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/") + nmos::patterns::stagingType.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);
            const string_t stagingType = parameters.at(nmos::patterns::stagingType.name);

            const auto& endpoint_data = stagingType == nmos::fields::active.key ? nmos::fields::endpoint_active : nmos::fields::endpoint_staged;

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() == resource)
            {
                set_reply(res, status_codes::NotFound);
            }
            else
            {
                set_reply(res, status_codes::OK, endpoint_data(resource->data));
            }
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/transportfile/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::types::sender });
            if (resources.end() != resource)
            {
                set_reply(res, status_codes::TemporaryRedirect);
                res.headers().add(web::http::header_names::location, nmos::fields::href(nmos::fields::transport_file(resource->data)));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        return connection_api;
    }
}

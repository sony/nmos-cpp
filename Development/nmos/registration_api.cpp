#include "nmos/registration_api.h"

#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/json_validator.h"
#include "nmos/api_utils.h"
#include "nmos/json_schema.h"
#include "nmos/log_manip.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/thread_utils.h"

namespace nmos
{
    void erase_expired_resources_thread(nmos::registry_model& model, slog::base_gate& gate)
    {
        // start out as a shared/read lock, only upgraded to an exclusive/write lock when an expired resource actually needs to be deleted from the resources
        auto lock = model.read_lock();
        auto& shutdown_condition = model.shutdown_condition;
        auto& shutdown = model.shutdown;
        auto& resources = model.registry_resources;

        auto least_health = nmos::least_health(resources);

        // wait until the next node could potentially expire, or the server is being shut down
        // (since health is truncated to seconds, and we want to be certain the expiry interval has passed, there's an extra second to wait here)
        while (!shutdown_condition.wait_until(lock, time_point_from_health(least_health.first + nmos::fields::registration_expiry_interval(model.settings) + 1), [&]{ return shutdown; }))
        {
            // hmmm, it needs to be possible to enable/disable periodic logging like this independently of the severity...
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "At " << nmos::make_version(nmos::tai_now()) << ", the registry contains " << nmos::put_resources_statistics(resources);

            // most nodes will have had a heartbeat during the wait, so the least health will have been increased
            // so this thread will be able to go straight back to waiting
            auto expire_health = health_now() - nmos::fields::registration_expiry_interval(model.settings);
            auto forget_health = expire_health - nmos::fields::registration_expiry_interval(model.settings);
            least_health = nmos::least_health(resources);
            if (least_health.first >= expire_health && least_health.second >= forget_health) continue;

            // otherwise, there's actually work to do...

            details::reverse_lock_guard<nmos::read_lock> unlock(lock);
            // note, without atomic upgrade, another thread may preempt hence the need to recalculate expire_health/forget_health and least_health
            auto upgrade = model.write_lock();

            expire_health = health_now() - nmos::fields::registration_expiry_interval(model.settings);
            forget_health = expire_health - nmos::fields::registration_expiry_interval(model.settings);

            // forget all resources expired in the previous interval
            forget_erased_resources(resources, forget_health);

            // expire all nodes for which there hasn't been a heartbeat in the last expiry interval
            const auto expired = erase_expired_resources(resources, expire_health, false);

            if (0 != expired)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << expired << " resources have expired";

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying query websockets thread"; // and anyone else who cares...
                model.notify();
            }

            least_health = nmos::least_health(resources);
        }
    }

    inline web::http::experimental::listener::api_router make_unmounted_registration_api(nmos::registry_model& model, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_registration_api(nmos::registry_model& model, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router registration_api;

        registration_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("x-nmos/") }));
            return pplx::task_from_result(true);
        });

        registration_api.support(U("/x-nmos/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("registration/") }));
            return pplx::task_from_result(true);
        });

        registration_api.support(U("/x-nmos/") + nmos::patterns::registration_api.pattern + U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("v1.0/"), JU("v1.1/"), JU("v1.2/") }));
            return pplx::task_from_result(true);
        });

        registration_api.mount(U("/x-nmos/") + nmos::patterns::registration_api.pattern + U("/") + nmos::patterns::is04_version.pattern, make_unmounted_registration_api(model, gate));

        nmos::add_api_finally_handler(registration_api, gate);

        return registration_api;
    }

    inline web::json::value make_health_response_body(health health)
    {
        web::json::value result;
        result[U("health")] = web::json::value::string(utility::ostringstreamed(health));
        return result;
    }

    inline utility::string_t make_registration_api_resource_location(const nmos::resource& resource)
    {
        return U("/x-nmos/registration/") + nmos::make_api_version(resource.version) + U("/resource/") + nmos::resourceType_from_type(resource.type) + U("/") + resource.id;
    }

    inline web::http::experimental::listener::api_router make_unmounted_registration_api(nmos::registry_model& model, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router registration_api;

        registration_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("resource/"), JU("health/") }));
            return pplx::task_from_result(true);
        });

        const web::json::experimental::json_validator validator
        {
            nmos::experimental::load_json_schema,
            boost::copy_range<std::vector<web::uri>>(is04_versions::all | boost::adaptors::transformed(experimental::make_registrationapi_resource_post_request_schema_uri))
        };

        registration_api.support(U("/resource/?"), methods::POST, [&model, validator, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            // note that, as elsewhere, http_exception and json_exception are handled by the exception handler added by add_api_finally_handler
            return details::extract_json(req, parameters, gate).then([&, req, res, parameters](value body) mutable
            {
                // could start out as a shared/read lock, only upgraded to an exclusive/write lock when the resource is actually modified or inserted into resources
                auto lock = model.write_lock();
                auto& resources = model.registry_resources;

                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));

                // Validate JSON syntax according to the schema

                const bool allow_invalid_resources = nmos::fields::allow_invalid_resources(model.settings);
                if (!allow_invalid_resources)
                {
                    validator.validate(body, experimental::make_registrationapi_resource_post_request_schema_uri(version));
                }
                else
                {
                    try
                    {
                        validator.validate(body, experimental::make_registrationapi_resource_post_request_schema_uri(version));
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "JSON error: " << e.what();
                    }
                }

                const value data = nmos::fields::data(body);
                const std::pair<nmos::id, nmos::type> id_type{ nmos::fields::id(data), nmos::type{ nmos::fields::type(body) } };
                const auto& id = id_type.first;
                const auto& type = id_type.second;

                // Validate request semantics, including referential integrity
                // such as the requested super-resource

                bool valid = true;

                // a modification request must not change the existing type
                const auto resource = nmos::find_resource(resources, id);
                const bool creating = resources.end() == resource;
                const bool valid_type = creating || resource->type == type;
                valid = valid && valid_type;

                // a modification request must not change the API version
                const bool valid_api_version = creating || resource->version == version;
                valid = valid && valid_api_version;

                // it must not change the super-resource either
                const std::pair<nmos::id, nmos::type> no_resource{};
                const auto super_id_type = nmos::get_super_resource(data, type);
                const bool valid_super_id_type = creating || nmos::get_super_resource(resource->data, resource->type) == super_id_type;
                valid = valid && valid_super_id_type;

                // the super-resource should exist in this registry (and must be of the right type)
                const auto super_resource = nmos::find_resource(resources, super_id_type.first);
                const bool no_super_resource = resources.end() == super_resource;
                const bool valid_super_resource = no_resource == super_id_type || !no_super_resource;
                valid = valid && valid_super_resource;

                const bool valid_super_type = no_resource == super_id_type || no_super_resource || super_resource->type == super_id_type.second;
                valid = valid && valid_super_type;

                // all the sub-resources of each node must have the same version
                const bool valid_super_api_version = no_resource == super_id_type || no_super_resource || super_resource->version == version;
                valid = valid && valid_super_api_version;

                // registration of an unchanged resource is considered as an acceptable "update" even though it's a no-op, but seems worth logging?
                const bool unchanged = !creating && data == resource->data;

                // each modification of a resource should update the version timestamp
                const bool valid_version = creating || unchanged || nmos::fields::version(data) > nmos::fields::version(resource->data);
                valid = valid && valid_version;

                if (!valid_type)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " would modify type from " << resource->type.name;
                else if (!valid_api_version)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " would modify API version from " << nmos::make_api_version(resource->version);
                else if (!valid_super_id_type)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " on " << super_id_type << " would modify super-resource from " << nmos::get_super_resource(resource->data, resource->type);
                else if (!valid_super_resource)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " on unknown " << super_id_type;
                else if (!valid_super_type)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " on " << super_id_type << " with inconsistent type of " << super_resource->type.name;
                else if (!valid_super_api_version)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " with API version inconsistent with super-resource " << nmos::make_api_version(super_resource->version);
                else if (!valid_version)
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " with invalid version";
                else if (no_resource == super_id_type) // i.e. just nodes, basically
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << (unchanged ? "unchanged " : "") << id_type;
                else
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << (unchanged ? "unchanged " : "") << id_type << " on " << super_id_type;

                if (nmos::types::node == type)
                {
                    // no extra validation yet
                }
                else if (nmos::types::device == type)
                {
                    // "The 'senders' and 'receivers' arrays in a Device have been deprecated, but will continue to be present until v2.0."
                    // Therefore, issue warnings rather than errors here and don't worry too much about other issues such as whether to
                    // merge senders and receivers with existing device if present?
                    // or remove previous senders and receivers?
                    // See https://github.com/AMWA-TV/nmos-discovery-registration/issues/24

                    for (auto& element : nmos::fields::senders(data))
                    {
                        const auto& sender_id = element.as_string();
                        const bool valid_sender = nmos::has_resource(resources, { sender_id, nmos::types::sender });
                        if (!valid_sender) slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " with unknown sender: " << sender_id;
                    }

                    for (auto& element : nmos::fields::receivers(data))
                    {
                        const auto& receiver_id = element.as_string();
                        const bool valid_receiver = nmos::has_resource(resources, { receiver_id, nmos::types::receiver });
                        if (!valid_receiver) slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " with unknown receiver: " << receiver_id;
                    }
                }
                else if (nmos::types::source == type)
                {
                    // the parent sources might not be registered in this registry, so issue a warning not an error, and don't treat this as invalid?
                    for (auto& element : nmos::fields::parents(data))
                    {
                        const auto& source_id = element.as_string();
                        const bool valid_parent = nmos::has_resource(resources, { source_id, nmos::types::source });
                        if (!valid_parent) slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " with unknown parent source: " << source_id;
                    }
                }
                else if (nmos::types::flow == type)
                {
                    // v1.1 introduced device_id for flow
                    if (nmos::is04_versions::v1_1 <= version)
                    {
                        const auto& device_id = nmos::fields::device_id(data);
                        const bool valid_device = nmos::has_resource(resources, { device_id, nmos::types::device });
                        if (!valid_device) slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " on unknown device: " << device_id;
                        valid = valid && valid_device;
                    }

                    // the parent flows might not be registered in this registry, so issue a warning not an error, and don't treat this as invalid?
                    for (auto& element : nmos::fields::parents(data))
                    {
                        const auto& flow_id = element.as_string();
                        const bool valid_parent = nmos::has_resource(resources, { flow_id, nmos::types::flow });
                        if (!valid_parent) slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " with unknown parent flow: " << flow_id;
                    }
                }
                else if (nmos::types::sender == type)
                {
                    const auto& flow_id = nmos::fields::flow_id(data);
                    const bool valid_flow = nmos::has_resource(resources, { flow_id, nmos::types::flow });
                    if (!valid_flow)
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " of unknown flow: " << flow_id;
                    else
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " of flow: " << flow_id;
                    valid = valid && valid_flow;

                    // v1.2 introduced subscription for sender
                    if (nmos::is04_versions::v1_2 <= version)
                    {
                        // the receiver might not be registered in this registry, so issue a warning not an error, and don't treat this as invalid?
                        const value& receiver_id = nmos::fields::receiver_id(nmos::fields::subscription(data));
                        const bool valid_receiver = receiver_id.is_null() || nmos::has_resource(resources, { receiver_id.as_string(), nmos::types::receiver });
                        if (!valid_receiver)
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " subscribed to unknown receiver: " << receiver_id.as_string();
                        else
                            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " subscribed to receiver: " << receiver_id.serialize();
                    }
                }
                else if (nmos::types::receiver == type)
                {
                    // the sender might not be registered in this registry, so issue a warning not an error, and don't treat this as invalid?
                    const value& sender_id = nmos::fields::sender_id(nmos::fields::subscription(data));
                    const bool valid_sender = sender_id.is_null() || nmos::has_resource(resources, { sender_id.as_string(), nmos::types::sender });
                    if (!valid_sender)
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " subscribed to unknown sender: " << sender_id.as_string();
                    else
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for " << id_type << " subscribed to sender: " << sender_id.serialize();
                }
                else // bad type
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for unrecognised resource type: " << type.name;
                    valid = false;
                }

                // always reject updates that would modify resource type or super-resource
                if (valid_type && valid_super_id_type && (valid || allow_invalid_resources))
                {
                    if (creating)
                    {
                        nmos::resource created_resource{ version, type, data, false };

                        set_reply(res, status_codes::Created, data);
                        res.headers().add(web::http::header_names::location, make_registration_api_resource_location(created_resource));

                        insert_resource(resources, std::move(created_resource), allow_invalid_resources);
                    }
                    else
                    {
                        set_reply(res, status_codes::OK, data);
                        res.headers().add(web::http::header_names::location, make_registration_api_resource_location(*resource));

                        modify_resource(resources, id, [&data](nmos::resource& resource)
                        {
                            resource.data = data;
                        });
                    }

                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "At " << nmos::make_version(nmos::tai_now()) << ", the registry contains " << nmos::put_resources_statistics(resources);

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Notifying query websockets thread"; // and anyone else who cares...
                    model.notify();
                }
                else if (!valid_type || !valid_api_version || !valid_super_id_type || !valid_version)
                {
                    // experimental extension, using a more specific status code to distinguish conflicts from validation errors
                    set_reply(res, status_codes::Conflict);

                    // the Location header would enable an HTTP DELETE to be performed to explicitly clear the registry of the conflicting registration
                    // (assert !creating, i.e. resources.end() != resource in all these cases)
                    res.headers().add(web::http::header_names::location, make_registration_api_resource_location(*resource));
                }
                else if (!valid_super_type || !valid_super_api_version)
                {
                    // the difference here is that it's the super-resource that conflicts
                    set_reply(res, status_codes::Conflict);

                    // since the conflict is with the super-resource, a single HTTP DELETE cannot be enough to resolve the issue in this case...
                    // (assert !no_super_resource, i.e. resources.end() != super_resource in all these cases)
                    res.headers().add(web::http::header_names::location, make_registration_api_resource_location(*super_resource));
                }
                else
                {
                    set_reply(res, status_codes::BadRequest);
                }

                return true;
            });
        });

        registration_api.support(U("/health/nodes/") + nmos::patterns::resourceId.pattern + U("/?"), [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            // since health is mutable, no need to get an exclusive/write lock even to handle a POST request
            auto lock = model.read_lock();
            auto& resources = model.registry_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::types::node });
            if (resources.end() != resource && resource->version == version)
            {
                if (methods::POST == req.method())
                {
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Heartbeat received for node: " << resourceId;

                    const auto health = nmos::health_now();
                    set_resource_health(resources, resourceId, health);

                    set_reply(res, web::http::status_codes::OK, make_health_response_body(health));
                }
                else if (methods::GET == req.method())
                {
                    set_reply(res, web::http::status_codes::OK, make_health_response_body(resource->health));
                }
                else
                {
                    set_reply(res, status_codes::MethodNotAllowed);
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        registration_api.support(U("/resource/") + nmos::patterns::resourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.registry_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceType = parameters.at(nmos::patterns::resourceType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource && resource->version == version)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning resource: " << resourceId;
                set_reply(res, status_codes::OK, resource->data);
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        registration_api.support(U("/resource/") + nmos::patterns::resourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::DEL, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            // could start out as a shared/read lock, only upgraded to an exclusive/write lock when the resource is actually deleted from resources
            auto lock = model.write_lock();
            auto& resources = model.registry_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));
            const string_t resourceType = parameters.at(nmos::patterns::resourceType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource && resource->version == version)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Deleting resource: " << resourceId;

                // remove this resource from its super-resource's sub-resources
                auto super_resource = nmos::find_resource(resources, nmos::get_super_resource(resource->data, resource->type));
                if (super_resource != resources.end())
                {
                    // this isn't modifying the visible data of the super_resouce though, so no resource events need to be generated
                    // hence resources.modify(...) rather than modify_resource(resources, ...)
                    resources.modify(super_resource, [&resource](nmos::resource& super_resource)
                    {
                        super_resource.sub_resources.erase(resource->id);
                    });
                }

                // "If a Node unregisters a resource in the incorrect order, the Registration API MUST clean up related child resources
                // on the Node's behalf in order to prevent stale entries remaining in the registry."
                // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#controlled-unregistration
                erase_resource(resources, resource->id, false);

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Notifying query websockets thread"; // and anyone else who cares...
                model.notify();

                set_reply(res, status_codes::NoContent);
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        return registration_api;
    }
}

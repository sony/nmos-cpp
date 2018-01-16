#include "nmos/registration_api.h"

#include "nmos/api_utils.h"
#include "nmos/log_manip.h"
#include "nmos/model.h"
#include "nmos/slog.h"
#include "nmos/query_utils.h"

namespace nmos
{
    void erase_expired_resources_thread(nmos::model& model, std::mutex& mutex, std::condition_variable& condition, bool& shutdown, std::condition_variable& query_ws_events_condition, slog::base_gate& gate)
    {
        std::unique_lock<std::mutex> lock(mutex);
        // wait until the next node could potentially expire, or the server is being shut down
        // (since health is truncated to seconds, and we want to be certain the expiry interval has passed, there's an extra second to wait here)
        while (!condition.wait_until(lock, time_point_from_health(least_health(model.resources) + nmos::fields::registration_expiry_interval(model.settings) + 1), [&]{ return shutdown; }))
        {
            auto before = model.resources.size();

            // expire all nodes for which there hasn't been a heartbeat in the last expiry interval
            erase_expired_resources(model.resources, health_now() - nmos::fields::registration_expiry_interval(model.settings));

            auto after = model.resources.size();

            if (before != after)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << (before - after) << " resources have expired, " << after << " remain";

                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "At " << nmos::make_version(nmos::tai_now()) << ", the registry contains " << nmos::put_resources_statistics(model.resources);

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying query websockets thread";
                query_ws_events_condition.notify_all();
            }
        }
    }

    inline web::http::experimental::listener::api_router make_unmounted_registration_api(nmos::model& model, std::mutex& mutex, std::condition_variable& query_ws_events_condition, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_registration_api(nmos::model& model, std::mutex& mutex, std::condition_variable& query_ws_events_condition, slog::base_gate& gate)
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

        registration_api.mount(U("/x-nmos/") + nmos::patterns::registration_api.pattern + U("/") + nmos::patterns::is04_version.pattern, make_unmounted_registration_api(model, mutex, query_ws_events_condition, gate));

        nmos::add_api_finally_handler(registration_api, gate);

        return registration_api;
    }

    inline web::json::value make_health_response_body(health health)
    {
        web::json::value result;
        result[U("health")] = web::json::value::string(utility::ostringstreamed(health));
        return result;
    }

    inline web::http::experimental::listener::api_router make_unmounted_registration_api(nmos::model& model, std::mutex& mutex, std::condition_variable& query_ws_events_condition, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router registration_api;

        registration_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("resource/"), JU("health/") }));
            return pplx::task_from_result(true);
        });

        registration_api.support(U("/resource/?"), methods::POST, [&model, &mutex, &query_ws_events_condition, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            return req.extract_json().then([&, req, res, parameters](value body) mutable
            {
                std::lock_guard<std::mutex> lock(mutex);

                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::is04_version.name));

                nmos::type type = { nmos::fields::type(body) };
                value data = nmos::fields::data(body);
                nmos::id id = nmos::fields::id(data);

                // Validate request, including referential integrity
                // such as the requested super-resource

                bool valid = true;

                resources::iterator resource = model.resources.find(id);
                const bool creating = model.resources.end() == resource;

                // a modification request must not change the existing type
                valid = valid && (creating || resource->type == type);
                // it shouldn't change the super-resource either
                const auto super_id_type = nmos::get_super_resource(data, type);
                valid = valid && (creating || nmos::get_super_resource(resource->data, resource->type) == super_id_type);

                resources::iterator super_resource = nmos::find_resource(model.resources, super_id_type);
                const bool valid_super_resource = model.resources.end() != super_resource;

                if (nmos::types::node == type)
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for node: " << id;
                }
                else if (nmos::types::device == type)
                {
                    valid = valid && valid_super_resource;

                    if (!valid_super_resource)
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for device: " << id << " on unknown node: " << nmos::fields::node_id(data);
                    else
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for device: " << id << " on node: " << nmos::fields::node_id(data);

                    // "The 'senders' and 'receivers' arrays in a Device have been deprecated, but will continue to be present until v2.0."
                    // Therefore, issue warnings rather than errors here and don't worry too much about other issues such as whether to
                    // merge senders and receivers with existing device if present?
                    // or remove previous senders and receivers?
                    // See https://github.com/AMWA-TV/nmos-discovery-registration/issues/24

                    for (auto& sender_id : nmos::fields::senders(data))
                    {
                        const bool valid_sender = nmos::has_resource(model.resources, { sender_id.as_string(), nmos::types::sender });
                        valid = valid && valid_sender;
                        if (!valid_sender) slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for device: " << id << " with unknown sender: " << sender_id.as_string();
                    }

                    for (auto& receiver_id : nmos::fields::receivers(data))
                    {
                        const bool valid_receiver = nmos::has_resource(model.resources, { receiver_id.as_string(), nmos::types::receiver });
                        valid = valid && valid_receiver;
                        if (!valid_receiver) slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for device: " << id << " with unknown receiver: " << receiver_id.as_string();
                    }
                }
                else if (nmos::types::source == type)
                {
                    valid = valid && valid_super_resource;

                    if (!valid_super_resource)
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for source: " << id << " on unknown device: " << nmos::fields::device_id(data);
                    else
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for source: " << id << " on device: " << nmos::fields::device_id(data);
                }
                else if (nmos::types::flow == type)
                {
                    valid = valid && valid_super_resource;

                    if (!valid_super_resource)
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for flow: " << id << " on unknown source: " << nmos::fields::source_id(data);
                    else
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for flow: " << id << " on source: " << nmos::fields::source_id(data);
                }
                else if (nmos::types::sender == type)
                {
                    const bool valid_flow = nmos::has_resource(model.resources, { nmos::fields::flow_id(data), nmos::types::flow });
                    valid = valid && valid_super_resource && valid_flow;

                    if (!valid_super_resource || !valid_flow)
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for sender: " << id << " on " << (valid_super_resource ? "device" : "unknown device") << ": " << nmos::fields::device_id(data) << " of " << (valid_flow ? "flow" : "unknown flow") << ": " << nmos::fields::flow_id(data);
                    else
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for sender: " << id << " on device: " << nmos::fields::device_id(data) << " of flow: " << nmos::fields::flow_id(data);
                }
                else if (nmos::types::receiver == type)
                {
                    const value sender_id = nmos::fields::sender_id(nmos::fields::subscription(data));
                    const bool valid_sender = sender_id.is_null() || nmos::has_resource(model.resources, { sender_id.as_string(), nmos::types::sender });
                    valid = valid && valid_super_resource && valid_sender;

                    if (!valid_super_resource || !valid_sender)
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for receiver: " << id << " on " << (valid_super_resource ? "device" : "unknown device") << ": " << nmos::fields::device_id(data) << " subscribed to " << (valid_sender ? "sender" : "unknown sender") << ": " << sender_id.serialize();
                    else
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for receiver: " << id << " on device: " << nmos::fields::device_id(data) << " subscribed to sender: " << sender_id.serialize();
                }
                else // bad type
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Registration requested for unrecognised resource type: " << type.name;
                    valid = false;
                }

                const bool allow_invalid_resources = nmos::fields::allow_invalid_resources(model.settings);
                if (valid || allow_invalid_resources)
                {
                    if (creating)
                    {
                        nmos::resource created_resource{ version, type, data, false };

                        insert_resource(model.resources, std::move(created_resource), allow_invalid_resources);
                    }
                    else
                    {
                        modify_resource(model.resources, id, [&data](nmos::resource& resource)
                        {
                            resource.data = data;
                        });
                    }

                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "At " << nmos::make_version(nmos::tai_now()) << ", the registry contains " << nmos::put_resources_statistics(model.resources);

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Notifying query websockets thread";
                    query_ws_events_condition.notify_all();

                    set_reply(res, creating ? status_codes::Created : status_codes::OK, data);
                    const string_t location(U("/x-nmos/registration/") + parameters.at(U("version")) + U("/resource/") + nmos::resourceType_from_type(type) + U("/") + id);
                    res.headers().add(web::http::header_names::location, location);
                }
                else
                {
                    set_reply(res, status_codes::BadRequest);
                }

                return true;
            });
        });

        registration_api.support(U("/health/nodes/") + nmos::patterns::resourceId.pattern + U("/?"), [&model, &mutex, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            std::lock_guard<std::mutex> lock(mutex);

            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            if (nmos::has_resource(model.resources, { resourceId, nmos::types::node }))
            {
                if (methods::POST == req.method())
                {
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Heartbeat received for node: " << resourceId;

                    const auto health = nmos::health_now();
                    set_resource_health(model.resources, resourceId, health);

                    set_reply(res, web::http::status_codes::OK, make_health_response_body(health));
                }
                else if (methods::GET == req.method())
                {
                    set_reply(res, web::http::status_codes::OK, make_health_response_body(model.resources.find(resourceId)->health));
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

        registration_api.support(U("/resource/") + nmos::patterns::resourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), [&model, &mutex, &query_ws_events_condition, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            std::lock_guard<std::mutex> lock(mutex);

            const string_t resourceType = parameters.at(nmos::patterns::resourceType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = model.resources.find(resourceId);
            if (model.resources.end() != resource)
            {
                if (resource->type == nmos::type_from_resourceType(resourceType))
                {
                    if (methods::GET == req.method())
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning resource: " << resourceId;
                        set_reply(res, status_codes::OK, resource->data);
                    }
                    else if (methods::DEL == req.method())
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Deleting resource: " << resourceId;

                        // remove this resource from its super-resource's sub-resources
                        resources::iterator super_resource = nmos::find_resource(model.resources, nmos::get_super_resource(resource->data, resource->type));
                        if (super_resource != model.resources.end())
                        {
                            // this isn't modifying the visible data of the super_resouce though, so no resource events need to be generated
                            // hence model.resources.modify(...) rather than modify_resource(model.resources, ...)
                            model.resources.modify(super_resource, [&resource](nmos::resource& super_resource)
                            {
                                super_resource.sub_resources.erase(resource->id);
                            });
                        }

                        // not sure if we're responsible for erasing sub-resources or whether the client is... play safe?
                        erase_resource(model.resources, resource->id);

                        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Notifying query websockets thread";
                        query_ws_events_condition.notify_all();

                        set_reply(res, status_codes::NoContent);
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

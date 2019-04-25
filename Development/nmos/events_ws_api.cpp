#include "nmos/events_ws_api.h"

#include "nmos/api_utils.h"
#include "nmos/is07_versions.h"
#include "nmos/log_manip.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/rational.h"
#include "nmos/thread_utils.h" // for wait_until
#include "nmos/slog.h"
#include "nmos/version.h"

namespace nmos
{
    class ws_api_gate : public details::omanip_gate
    {
    public:
        // apart from the gate, arguments are copied in order that this object is safely copyable
        ws_api_gate(slog::base_gate& gate, const utility::string_t& ws_resource_path)
            : details::omanip_gate(gate, slog::omanip([=](std::ostream& os) { os << nmos::stash_request_uri(ws_resource_path); }))
        {}
        virtual ~ws_api_gate() {}
    };

    resources::iterator find_subscription(resources& resources, const utility::string_t& ws_resource_path)
    {
        auto& by_type = resources.get<tags::type>();
        const auto subscriptions = by_type.equal_range(details::has_data(nmos::types::subscription));
        auto resource = std::find_if(subscriptions.first, subscriptions.second, [&ws_resource_path](const nmos::resources::value_type& resource)
        {
            return ws_resource_path == nmos::fields::ws_href(resource.data);
        });
        return subscriptions.second != resource ? resources.project<0>(resource) : resources.end();
    }

    web::websockets::experimental::listener::validate_handler make_eventntally_ws_validate_handler(nmos::node_model& model, slog::base_gate& gate_)
    {
        return [&model, &gate_](const utility::string_t& ws_resource_path)
        {
            nmos::ws_api_gate gate(gate_, ws_resource_path);
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating websocket connection to: " << ws_resource_path;

            auto& by_type = resources.get<nmos::tags::type>();
            const auto connection_senders = by_type.equal_range(nmos::details::has_data(nmos::types::sender));
            auto connection_sender = std::find_if(connection_senders.first, connection_senders.second, [&ws_resource_path](const nmos::resources::value_type& resource)
            {
                auto first_transport_param = resource.data.at(nmos::fields::endpoint_active).at(nmos::fields::transport_params).at(0);

                // Checking if connection resource have ws_resource:_path in its connection_uri
                return first_transport_param.has_field(nmos::fields::connection_uri) && first_transport_param.at(nmos::fields::connection_uri).as_string().find(ws_resource_path) != std::string::npos;
            });

            const bool has_subscription = connection_senders.second != connection_sender;

            if (!has_subscription) slog::log<slog::severities::error>(gate, SLOG_FLF) << "Invalid websocket connection to: " << ws_resource_path;
            return has_subscription;
        };
    }

    web::websockets::experimental::listener::open_handler make_eventntally_ws_open_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        using utility::string_t;
        using web::json::value;

        return [&model, &websockets, &gate_](const utility::string_t& ws_resource_path, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::ws_api_gate gate(gate_, ws_resource_path);
            auto lock = model.write_lock();
            auto& resources = model.event_resources;

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Opening websocket connection to: " << ws_resource_path;

            // Screating subscription
            value subscription_data;
            nmos::id subscription_id = nmos::make_id();
            subscription_data[nmos::fields::id] = value::string(subscription_id);
            subscription_data[nmos::fields::resource_path] = value::string(U("/nonexistentyet"));
            subscription_data[nmos::fields::params] = value::object();
            subscription_data[nmos::fields::max_update_rate_ms] = 0;
            resource subscription{ is07_versions::v1_0, nmos::types::subscription, subscription_data, false };

            //Creating grain, 1-1 relationship with subscription.
            value grain_data;
            nmos::id grain_id = nmos::make_id();
            grain_data[nmos::fields::id] = value::string(grain_id);
            grain_data[nmos::fields::subscription_id] = value::string(subscription_id);

            //Creating empty grain data
            grain_data[U("message")] = details::make_grain(subscription.id, subscription.id, U(""));
            nmos::fields::message_grain_data(grain_data) = make_resource_events(resources, subscription.version, subscription.data.at(nmos::fields::resource_path).as_string(), subscription.data.at(U("params")));

            // never expire the grain resource, they are only deleted when the connection is closed
            resource grain{ subscription.version, nmos::types::grain, grain_data, false };

            insert_resource(resources, std::move(subscription));
            insert_resource(resources, std::move(grain));

            // never expire a subscription while it has connections
            // note, since health is mutable, no need for:
            // model.resources.modify(subscription, [](nmos::resource& subscription){ subscription.health = health_forever; });
            subscription.health = health_forever;

            websockets.insert({ grain_id, connection_id });

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Creating websocket connection: " << grain_id << " to subscription: " << subscription.id;

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying events websockets thread"; // and anyone else who cares...
            model.notify();
        };
    }

  web::websockets::experimental::listener::message_handler make_eventntally_ws_message_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        using utility::string_t;
        using web::json::value;
        using web::json::value_of;
        return [&model, &websockets, &gate_](const utility::string_t& ws_resource_path, const web::websockets::experimental::listener::connection_id& connection_id, const utility::string_t& message_payload)
        {
            nmos::ws_api_gate gate(gate_, ws_resource_path);
            auto lock = model.write_lock();
            auto& resources = model.event_resources;

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Got message " << message_payload << " on path " << ws_resource_path;

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(resources, { websocket->second, nmos::types::grain });

                if (resources.end() != grain)
                {
                    const auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });
                    if (resources.end() != subscription) {
                        try {
                            auto body = value::parse(message_payload);
                            auto command = body[nmos::fields::command];
                            if (command == value::string(U("subscription"))) {
                                modify_resource(resources, subscription->id, [&body](nmos::resource& resource)
                                {
                                    resource.data[nmos::fields::resource_path] = value::string(U("/events"));
                                    utility::ostringstream_t rqlQuery;
                                    rqlQuery << U("in(id,(");

                                    for (const auto& sourceId : body.at("sources").as_array()) {
                                        rqlQuery << sourceId.as_string() << ",";
                                    }

                                    rqlQuery << U("))");

                                    resource.data[nmos::fields::params] = value_of({{ U("query.rql"), rqlQuery.str() }});
                                });

                                // Send back all events data when user sends "subscription" command
                                const string_t resource_path = nmos::fields::resource_path(subscription->data);
                                auto grain_data = make_resource_events(resources, subscription->version, resource_path, subscription->data.at(U("params")));
                                auto grain_message = details::make_grain(subscription->id, subscription->id, U("/events/"));
                                modify_resource(resources, grain->id, [&grain_data, &grain_message](nmos::resource& resource)
                                {
                                    resource.data[U("message")] = grain_message;
                                    nmos::fields::message_grain_data(resource.data) = grain_data;
                                });

                                // Starting health countdown here
                                const auto health = nmos::health_now();
                                set_resource_health(resources, subscription->id, health);

                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Received new IS-07 subscription command for sources " << body.at("sources").serialize();
                                model.notify();
                            } else if (command == value::string(U("health"))) {
                                const auto health = nmos::health_now();
                                set_resource_health(resources, subscription->id, health);

                                resources.modify(grain, [&resources](nmos::resource& grain)
                                {
                                    nmos::tai tai = nmos::tai_now();
                                    // Saving "health"-command response here, so the reply-thread can send the update to the client
                                    auto& message_grain = grain.data[U("message")][U("grain")];
                                    message_grain[U("health_reply")] = value_of ({
                                        { U("timing"), value_of ({
                                            { U("creation_timestamp"), std::to_string(tai.seconds) + U(":") + std::to_string(tai.nanoseconds) }
                                        })},
                                        { U("message_type"), U("health") }
                                    });
                                    grain.updated = strictly_increasing_update(resources);
                                });

                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Received new IS-07 health command command";

                                model.notify();
                            }
                        } catch (const web::json::json_exception& e) {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Got malformed IS-07 command json: " << e.what();
                        }
                    }
                }

            }
            
            return;            
        };
    }


    web::websockets::experimental::listener::close_handler make_eventntally_ws_close_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        return [&model, &websockets, &gate_](const utility::string_t& ws_resource_path, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::ws_api_gate gate(gate_, ws_resource_path);
            auto lock = model.write_lock();
            auto& resources = model.event_resources;

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing websocket connection to: " << ws_resource_path;

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(resources, { websocket->second, nmos::types::grain });

                if (grain != resources.end()) {
                    const auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });
                    if (resources.end() != subscription) {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Deleting websocket connection with subscription_id: " << subscription->id;
                        // This should erase grain too, as a subscription's subresource
                        erase_resource(resources, subscription->id, false);   
                    }
                }

                websockets.right.erase(websocket);
            }
        };
    }

    void send_eventntally_ws_events_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::send_eventntally_ws_events));

        using utility::string_t;
        using web::json::value;

        // could start out as a shared/read lock, only upgraded to an exclusive/write lock when a grain in the resources is actually modified
        auto lock = model.write_lock();
        auto& condition = model.condition;
        auto& shutdown = model.shutdown;
        auto& resources = model.event_resources;

        tai most_recent_message{};
        auto earliest_necessary_update = (tai_clock::time_point::max)();

        for (;;)
        {
            // wait for the thread to be interrupted either because there are resource changes, or because the server is being shut down
            // or because message sending was throttled earlier
            details::wait_until(condition, lock, earliest_necessary_update, [&]{ return shutdown || most_recent_message < most_recent_update(resources); });
            if (shutdown) break;
            most_recent_message = most_recent_update(resources);

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Got notification on query websockets thread";

            earliest_necessary_update = (tai_clock::time_point::max)();

            std::vector<std::pair<web::websockets::experimental::listener::connection_id, web::websockets::experimental::listener::websocket_outgoing_message>> outgoing_messages;

            for (const auto& websocket : websockets.left)
            {
                // for each websocket connection that has valid grain and subscription resources
                const auto grain = find_resource(resources, { websocket.first, nmos::types::grain });
                if (resources.end() == grain) {
                    // Connection has expired
                    listener.close(websocket.second);
                    continue;
                }
                const auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });
                if (resources.end() == subscription) continue;

                if (!grain->data.has_field(U("message"))) continue;
                auto& message_grain = grain->data.at(U("message")).at(U("grain"));
                if (message_grain.has_field(U("health_reply")) && !message_grain.at(U("health_reply")).is_null()) {
                    auto serialized = utility::us2s(message_grain.at(U("health_reply")).serialize());
                    web::websockets::experimental::listener::websocket_outgoing_message message;
                    message.set_utf8_message(serialized);    
                    outgoing_messages.push_back({ websocket.second, message });
                }

                auto events = nmos::fields::message_grain_data(grain->data);
                for (auto i = 0; i < events.size(); i++) {
                    auto serialized = utility::us2s(events.at(i).at("post").at(nmos::fields::event_restapi_state).serialize());
                    web::websockets::experimental::listener::websocket_outgoing_message message;
                    message.set_utf8_message(serialized);

                    outgoing_messages.push_back({ websocket.second, message });
                }

                // reset the grain for next time
                resources.modify(grain, [](nmos::resource& grain)
                {
                    // Removing all events here since we are interested in sending just the last event.
                    nmos::fields::message_grain_data(grain.data) = value::array();
                    grain.data[U("message")][U("grain")][U("health_reply")] = value::null();
                });
            }

            // send the messages without the lock on resources
            details::reverse_lock_guard<nmos::write_lock> unlock{ lock };

            if (!outgoing_messages.empty()) slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sending " << outgoing_messages.size() << " websocket messages";

            for (auto& outgoing_message : outgoing_messages)
            {
                // hmmm, no way to cancel this currently...
                auto send = listener.send(outgoing_message.first, outgoing_message.second).then([&](pplx::task<void> finally)
                {
                    try
                    {
                        finally.get();
                    }
                    catch (const web::websockets::experimental::listener::websocket_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "WebSockets error: " << e.what() << " [" << e.error_code() << "]";
                    }
                });
                // current websocket_listener implementation is synchronous in any case, but just to make clear...
                // for now, wait for the message to be sent
                send.wait();
            }
        }
    }

    void erase_expired_resources_thread(nmos::node_model& model, slog::base_gate& gate_) {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::eventntally_ws_expiry));

        // start out as a shared/read lock, only upgraded to an exclusive/write lock when an expired resource actually needs to be deleted from the resources
        auto lock = model.read_lock();
        auto& shutdown_condition = model.shutdown_condition;
        auto& shutdown = model.shutdown;
        auto& resources = model.event_resources;

        auto least_health = nmos::least_health(resources);

        // wait until the next node could potentially expire, or the server is being shut down
        // (since health is truncated to seconds, and we want to be certain the expiry interval has passed, there's an extra second to wait here)
        while (!shutdown_condition.wait_until(lock, time_point_from_health(least_health.first + nmos::fields::eventntally_expiry_interval(model.settings) + 1), [&]{ return shutdown; }))
        {
            // hmmm, it needs to be possible to enable/disable periodic logging like this independently of the severity...
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "At " << nmos::make_version(nmos::tai_now()) << ", the IS-07 event machinery contains " << nmos::put_resources_statistics(resources);

            // most nodes will have had a heartbeat during the wait, so the least health will have been increased
            // so this thread will be able to go straight back to waiting
            auto expire_health = health_now() - nmos::fields::eventntally_expiry_interval(model.settings);
            auto forget_health = expire_health - nmos::fields::eventntally_expiry_interval(model.settings);
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
}

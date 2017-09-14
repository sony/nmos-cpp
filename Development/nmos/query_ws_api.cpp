#include "nmos/query_ws_api.h"

#include "nmos/query_utils.h"
#include "nmos/rational.h"
#include "nmos/slog.h"
#include "nmos/version.h"

namespace nmos
{
    inline resources::iterator find_subscription(resources& resources, const utility::string_t& ws_resource_path)
    {
        auto resource = std::find_if(resources.begin(), resources.end(), [&ws_resource_path](const nmos::resources::value_type& resource)
        {
            return nmos::types::subscription == resource.type
                && ws_resource_path == web::uri(nmos::fields::ws_href(resource.data)).path();
        });
        return resource;
    }

    web::websockets::experimental::listener::validate_handler make_query_ws_validate_handler(nmos::model& model, std::mutex& mutex, slog::base_gate& gate)
    {
        return [&model, &mutex, &gate](const utility::string_t& ws_resource_path)
        {
            std::lock_guard<std::mutex> lock(mutex);

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating websocket connection to: " << ws_resource_path;

            const bool has_subscription = model.resources.end() != find_subscription(model.resources, ws_resource_path);

            if (!has_subscription) slog::log<slog::severities::error>(gate, SLOG_FLF) << "Invalid websocket connection to: " << ws_resource_path;
            return has_subscription;
        };
    }

    namespace details
    {
        void set_subscription_grain_timestamp(web::json::value& message, const nmos::tai& tai)
        {
            const auto timestamp = web::json::value::string(nmos::make_version(tai));
            message[U("origin_timestamp")] = timestamp;
            message[U("sync_timestamp")] = timestamp;
            message[U("creation_timestamp")] = timestamp;
        }

        nmos::tai get_subscription_grain_timestamp(const web::json::value& message)
        {
            return parse_version(nmos::fields::sync_timestamp(message));
        }

        web::json::value make_subscription_grain(const nmos::id& source_id, const nmos::id& flow_id, const utility::string_t& topic)
        {
            using web::json::value;

            // seems worthwhile to keep_order for simple visualisation
            value result = value::object(true);

            result[U("grain_type")] = JU("event");
            result[U("source_id")] = value::string(source_id);
            result[U("flow_id")] = value::string(flow_id);
            set_subscription_grain_timestamp(result, {});
            result[U("rate")] = make_rational();
            result[U("duration")] = make_rational();
            value& grain = result[U("grain")] = value::object(true);
            grain[U("type")] = JU("urn:x-nmos:format:data.event");
            grain[U("topic")] = value::string(topic);
            grain[U("data")] = value::array();

            return result;
        }
    }

    web::websockets::experimental::listener::open_handler make_query_ws_open_handler(nmos::model& model, nmos::websockets& websockets, std::mutex& mutex, std::condition_variable& query_ws_events_condition, slog::base_gate& gate)
    {
        using utility::string_t;
        using web::json::value;

        // Source ID of the Query API instance issuing the data Grain
        const nmos::id source_id = nmos::make_id();

        return [source_id, &model, &websockets, &mutex, &query_ws_events_condition, &gate](const utility::string_t& ws_resource_path, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            std::lock_guard<std::mutex> lock(mutex);

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Opening websocket connection to: " << ws_resource_path;

            auto subscription = find_subscription(model.resources, ws_resource_path);

            if (model.resources.end() != subscription)
            {
                // create a websocket connection resource

                value data;
                nmos::id id = nmos::make_id();
                data[U("id")] = value::string(id);
                data[U("subscription_id")] = value::string(subscription->id);

                // create an initial websocket message with no data

                const string_t resource_path = nmos::fields::resource_path(subscription->data);
                const string_t topic = resource_path + U('/');
                data[U("message")] = details::make_subscription_grain(source_id, subscription->id, topic);

                // populate it with the initial (unchanged, a.k.a. sync) data

                const resource_query match(subscription->version, resource_path, subscription->data.at(U("params")));
 
                std::vector<value> events;
                for (const auto& resource : model.resources)
                {
                    if (match(resource))
                    {
                        events.push_back(make_resource_event(resource_path, resource.type, resource.data, resource.data));
                    }
                }

                data[U("message")][U("grain")][U("data")] = web::json::value_from_elements(events);

                // track the websocket connection as a sub-resource of the subscription

                // never expire websocket connections, they are only deleted when the connection is closed
                resource websocket{ subscription->version, nmos::types::websocket, data, true };

                insert_resource(model.resources, std::move(websocket));
                model.resources.modify(subscription, [&id](nmos::resource& subscription)
                {
                    subscription.sub_resources.insert(id);
                    // never expire a subscription while it has connections
                    subscription.health = health_forever;
                });

                websockets.insert({ id, connection_id });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Creating websocket connection: " << id << " to subscription: " << subscription->id;

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying query websockets thread";
                query_ws_events_condition.notify_all();
            }
            else
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Invalid websocket connection to: " << ws_resource_path;
            }
        };
    }

    web::websockets::experimental::listener::close_handler make_query_ws_close_handler(nmos::model& model, nmos::websockets& websockets, std::mutex& mutex, slog::base_gate& gate)
    {
        return [&model, &websockets, &mutex, &gate](const utility::string_t& ws_resource_path, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            std::lock_guard<std::mutex> lock(mutex);

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing websocket connection to: " << ws_resource_path;

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto resource = find_resource(model.resources, { websocket->second, nmos::types::websocket });

                if (model.resources.end() != resource)
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Deleting websocket connection: " << resource->id;

                    // a non-persistent subscription for which this was the last websocket connection should now expire unless a new connection is made soon
                    modify_resource(model.resources, nmos::fields::subscription_id(resource->data), [&](nmos::resource& subscription)
                    {
                        subscription.sub_resources.erase(resource->id);
                        if (subscription.sub_resources.empty())
                        {
                            subscription.health = health_now();
                        }
                    });

                    erase_resource(model.resources, resource->id);
                }

                websockets.right.erase(websocket);
            }
        };
    }

    // std::condition_variable::wait_until seems unreliable
    template<typename Clock, typename Duration, typename Predicate>
    inline bool wait_until(std::condition_variable& condition, std::unique_lock<std::mutex>& lock, const std::chrono::time_point<Clock, Duration>& tp, Predicate predicate)
    {
        if ((std::chrono::time_point<Clock, Duration>::max)() == tp)
        {
            condition.wait(lock, predicate);
            return true;
        }
        else
        {
            return condition.wait_until(lock, tp, predicate);
        }
    }

    void send_query_ws_events_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::model& model, nmos::websockets& websockets, std::mutex& mutex, std::condition_variable& condition, bool& shutdown, slog::base_gate& gate)
    {
        using utility::string_t;
        using web::json::value;

        std::unique_lock<std::mutex> lock(mutex);
        tai most_recent_message{};
        auto earliest_necessary_update = (tai_clock::time_point::max)();

        for (;;)
        {
            // wait for the thread to be interrupted either because there are resource changes, or because the server is being shut down
            // or because message sending was throttled earlier
            wait_until(condition, lock, earliest_necessary_update, [&]{ return shutdown || most_recent_message < most_recent_update(model.resources); });
            if (shutdown) break;
            most_recent_message = most_recent_update(model.resources);

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Got notification on query websockets thread";

            const auto now = tai_clock::now();

            earliest_necessary_update = (tai_clock::time_point::max)();

            for (const auto& websocket : websockets.left)
            {
                // for each websocket connection that has valid websocket connection and subscription resources
                const auto resource = find_resource(model.resources, { websocket.first, nmos::types::websocket });
                if (model.resources.end() == resource) continue;
                const auto subscription = model.resources.find(nmos::fields::subscription_id(resource->data));
                if (model.resources.end() == subscription) continue;
                // and has events to send
                auto& events = websocket_resource_events(*resource);
                if (0 == events.size()) continue;

                // throttle messages according to the subscription's max_update_rate_ms
                const auto max_update_rate = std::chrono::milliseconds(nmos::fields::max_update_rate_ms(subscription->data));
                const auto earliest_allowed_update = time_point_from_tai(details::get_subscription_grain_timestamp(websocket_message(*resource))) + max_update_rate;
                if (earliest_allowed_update > now)
                {
                    // make sure to send a message as soon as allowed
                    if (earliest_allowed_update < earliest_necessary_update)
                    {
                        earliest_necessary_update = earliest_allowed_update;
                    }
                    // just don't do it now!
                    continue;
                }

                // set the message timestamp
                model.resources.modify(resource, [&most_recent_message](nmos::resource& websocket)
                {
                    details::set_subscription_grain_timestamp(websocket_message(websocket), most_recent_message);
                });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sending " << events.size() << " changes on websocket connection: " << resource->id;

                auto serialized = utility::us2s(websocket_message(*resource).serialize());
                web::websockets::experimental::listener::websocket_outgoing_message message;
                message.set_utf8_message(serialized);

                listener.send(websocket.second, message);

                // reset the message for next time
                model.resources.modify(resource, [&model](nmos::resource& websocket)
                {
                    auto& events = websocket_resource_events(websocket);
                    events = value::array();
                    websocket.updated = strictly_increasing_update(model.resources);
                });
            }
        }
    }
}

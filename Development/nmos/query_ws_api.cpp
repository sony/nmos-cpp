#include "nmos/query_ws_api.h"

#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/rational.h"
#include "nmos/thread_utils.h" // for wait_until
#include "nmos/slog.h"
#include "nmos/version.h"

namespace nmos
{
    inline resources::iterator find_subscription(resources& resources, const utility::string_t& ws_resource_path)
    {
        auto& by_type = resources.get<tags::type>();
        const auto subscriptions = by_type.equal_range(nmos::types::subscription);
        auto resource = std::find_if(subscriptions.first, subscriptions.second, [&ws_resource_path](const nmos::resources::value_type& resource)
        {
            return ws_resource_path == web::uri(nmos::fields::ws_href(resource.data)).path();
        });
        return subscriptions.second != resource ? resources.project<0>(resource) : resources.end();
    }

    web::websockets::experimental::listener::validate_handler make_query_ws_validate_handler(nmos::model& model, nmos::mutex& mutex, slog::base_gate& gate)
    {
        return [&model, &mutex, &gate](const utility::string_t& ws_resource_path)
        {
            nmos::read_lock lock(mutex);

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating websocket connection to: " << ws_resource_path;

            const bool has_subscription = model.resources.end() != find_subscription(model.resources, ws_resource_path);

            if (!has_subscription) slog::log<slog::severities::error>(gate, SLOG_FLF) << "Invalid websocket connection to: " << ws_resource_path;
            return has_subscription;
        };
    }

    web::websockets::experimental::listener::open_handler make_query_ws_open_handler(nmos::model& model, nmos::websockets& websockets, nmos::mutex& mutex, nmos::condition_variable& query_ws_events_condition, slog::base_gate& gate)
    {
        using utility::string_t;
        using web::json::value;

        // Source ID of the Query API instance issuing the data Grain
        const nmos::id source_id = nmos::make_id();

        return [source_id, &model, &websockets, &mutex, &query_ws_events_condition, &gate](const utility::string_t& ws_resource_path, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::write_lock lock(mutex);

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
                data[U("message")] = details::make_grain(source_id, subscription->id, topic);

                // populate it with the initial (unchanged, a.k.a. sync) data

                nmos::fields::message_grain_data(data) = make_resource_events(model.resources, subscription->version, resource_path, subscription->data.at(U("params")));

                // track the grain for the websocket connection as a sub-resource of the subscription

                // never expire the grain resource, they are only deleted when the connection is closed
                resource grain{ subscription->version, nmos::types::grain, data, true };

                insert_resource(model.resources, std::move(grain));

                // never expire a subscription while it has connections
                // note, since health is mutable, no need for:
                // model.resources.modify(subscription, [](nmos::resource& subscription){ subscription.health = health_forever; });
                subscription->health = health_forever;

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

    web::websockets::experimental::listener::close_handler make_query_ws_close_handler(nmos::model& model, nmos::websockets& websockets, nmos::mutex& mutex, slog::base_gate& gate)
    {
        return [&model, &websockets, &mutex, &gate](const utility::string_t& ws_resource_path, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::write_lock lock(mutex);

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing websocket connection to: " << ws_resource_path;

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(model.resources, { websocket->second, nmos::types::grain });

                if (model.resources.end() != grain)
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Deleting websocket connection: " << grain->id;

                    // a non-persistent subscription for which this was the last websocket connection should now expire unless a new connection is made soon
                    modify_resource(model.resources, nmos::fields::subscription_id(grain->data), [&](nmos::resource& subscription)
                    {
                        subscription.sub_resources.erase(grain->id);
                        if (subscription.sub_resources.empty())
                        {
                            subscription.health = health_now();
                        }
                    });

                    erase_resource(model.resources, grain->id);
                }

                websockets.right.erase(websocket);
            }
        };
    }

    void send_query_ws_events_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::model& model, nmos::websockets& websockets, nmos::mutex& mutex, nmos::condition_variable& condition, bool& shutdown, slog::base_gate& gate)
    {
        using utility::string_t;
        using web::json::value;

        // could start out as a shared/read lock, only upgraded to an exclusive/write lock when a grain in the resources is actually modified
        nmos::write_lock lock(mutex);
        tai most_recent_message{};
        auto earliest_necessary_update = (tai_clock::time_point::max)();

        for (;;)
        {
            // wait for the thread to be interrupted either because there are resource changes, or because the server is being shut down
            // or because message sending was throttled earlier
            details::wait_until(condition, lock, earliest_necessary_update, [&]{ return shutdown || most_recent_message < most_recent_update(model.resources); });
            if (shutdown) break;
            most_recent_message = most_recent_update(model.resources);

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Got notification on query websockets thread";

            const auto now = tai_clock::now();

            earliest_necessary_update = (tai_clock::time_point::max)();

            for (const auto& websocket : websockets.left)
            {
                // for each websocket connection that has valid grain and subscription resources
                const auto grain = find_resource(model.resources, { websocket.first, nmos::types::grain });
                if (model.resources.end() == grain) continue;
                const auto subscription = find_resource(model.resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });
                if (model.resources.end() == subscription) continue;
                // and has events to send
                if (0 == nmos::fields::message_grain_data(grain->data).size()) continue;

                // throttle messages according to the subscription's max_update_rate_ms
                const auto max_update_rate = std::chrono::milliseconds(nmos::fields::max_update_rate_ms(subscription->data));
                const auto earliest_allowed_update = time_point_from_tai(details::get_grain_timestamp(nmos::fields::message(grain->data))) + max_update_rate;
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

                // set the grain timestamp
                model.resources.modify(grain, [&most_recent_message](nmos::resource& grain)
                {
                    details::set_grain_timestamp(nmos::fields::message(grain.data), most_recent_message);
                });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sending " << nmos::fields::message_grain_data(grain->data).size() << " changes on websocket connection: " << grain->id;

                auto serialized = utility::us2s(nmos::fields::message(grain->data).serialize());
                web::websockets::experimental::listener::websocket_outgoing_message message;
                message.set_utf8_message(serialized);

                listener.send(websocket.second, message);

                // reset the grain for next time
                model.resources.modify(grain, [&model](nmos::resource& grain)
                {
                    nmos::fields::message_grain_data(grain.data) = value::array();
                    grain.updated = strictly_increasing_update(model.resources);
                });
            }
        }
    }
}

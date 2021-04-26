#include "nmos/query_ws_api.h"

#include "cpprest/json_storage.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/rational.h"
#include "nmos/thread_utils.h" // for wait_until
#include "nmos/slog.h"
#include "nmos/version.h"

namespace nmos
{
    web::websockets::experimental::listener::validate_handler make_query_ws_validate_handler(nmos::registry_model& model, slog::base_gate& gate_)
    {
        return [&model, &gate_](web::http::http_request req)
        {
            nmos::ws_api_gate gate(gate_, req.request_uri());
            auto lock = model.read_lock();
            auto& resources = model.registry_resources;

            // RFC 6750 defines two methods of sending bearer access tokens which are applicable to WebSocket
            // Clients SHOULD use the "Authorization Request Header Field" method.
            // Clients MAY use a "URI Query Parameter".
            // See https://tools.ietf.org/html/rfc6750#section-2

            // For now, to determine whether the "resource name" is valid, only look at the path, and ignore any query parameters
            const auto& ws_resource_path = req.request_uri().path();
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating websocket connection to: " << ws_resource_path;

            const bool has_ws_resource_path = resources.end() != find_resource_if(resources, nmos::types::subscription, [&ws_resource_path](const nmos::resource& resource)
            {
                return ws_resource_path == web::uri(nmos::fields::ws_href(resource.data)).path();
            });

            if (!has_ws_resource_path) slog::log<slog::severities::error>(gate, SLOG_FLF) << "Invalid websocket connection to: " << ws_resource_path;
            return has_ws_resource_path;
        };
    }

    // note, model mutex is assumed to also protect websockets
    web::websockets::experimental::listener::open_handler make_query_ws_open_handler(const nmos::id& source_id, nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        using web::json::value;

        return [source_id, &model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.registry_resources;

            const auto& ws_resource_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Opening websocket connection to: " << ws_resource_path;

            auto subscription = find_resource_if(resources, nmos::types::subscription, [&ws_resource_path](const nmos::resource& resource)
            {
                return ws_resource_path == web::uri(nmos::fields::ws_href(resource.data)).path();
            });

            if (resources.end() != subscription)
            {
                // create a websocket connection resource

                value data;
                nmos::id id = nmos::make_id();
                data[nmos::fields::id] = value::string(id);
                data[nmos::fields::subscription_id] = value::string(subscription->id);

                // create an initial websocket message with no data

                const auto resource_path = nmos::fields::resource_path(subscription->data);
                const auto topic = resource_path + U('/');
                data[U("message")] = details::make_grain(source_id, subscription->id, topic);

                // populate it with the initial (unchanged, a.k.a. sync) data

                nmos::fields::message_grain_data(data) = make_resource_events(resources, subscription->version, resource_path, nmos::fields::params(subscription->data));

                // track the grain for the websocket connection as a sub-resource of the subscription

                // never expire the grain resource, they are only deleted when the connection is closed
                resource grain{ subscription->version, nmos::types::grain, std::move(data), true };

                insert_resource(resources, std::move(grain));

                // never expire a subscription while it has connections
                // note, since health is mutable, no need for:
                // model.resources.modify(subscription, [](nmos::resource& subscription){ subscription.health = health_forever; });
                subscription->health = health_forever;

                websockets.insert({ id, connection_id });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Creating websocket connection: " << id << " to subscription: " << subscription->id;

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying query websockets thread"; // and anyone else who cares...
                model.notify();
            }
            else
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Invalid websocket connection to: " << ws_resource_path;
            }
        };
    }

    // note, model mutex is assumed to also protect websockets
    web::websockets::experimental::listener::close_handler make_query_ws_close_handler(nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, web::websockets::websocket_close_status close_status, const utility::string_t& close_reason)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.registry_resources;

            const auto& ws_resource_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing websocket connection to: " << ws_resource_path << " [" << (int)close_status << ": " << close_reason << "]";

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(resources, { websocket->second, nmos::types::grain });

                if (resources.end() != grain)
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Deleting websocket connection: " << grain->id;

                    // a non-persistent subscription for which this was the last websocket connection should now expire unless a new connection is made soon
                    modify_resource(resources, nmos::fields::subscription_id(grain->data), [&](nmos::resource& subscription)
                    {
                        subscription.sub_resources.erase(grain->id);
                        if (!nmos::fields::persist(subscription.data) && subscription.sub_resources.empty())
                        {
                            subscription.health = health_now();
                        }
                    });

                    erase_resource(resources, grain->id, false);
                }

                websockets.right.erase(websocket);

                model.notify();
            }
        };
    }

    // note, model mutex is assumed to also protect websockets
    void send_query_ws_events_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::registry_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::send_query_ws_events));

        using web::json::value;

        // could start out as a shared/read lock, only upgraded to an exclusive/write lock when a grain in the resources is actually modified
        auto lock = model.write_lock();
        auto& condition = model.condition;
        auto& shutdown = model.shutdown;
        auto& resources = model.registry_resources;

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

            const auto now = tai_clock::now();

            earliest_necessary_update = (tai_clock::time_point::max)();

            std::vector<std::pair<web::websockets::experimental::listener::connection_id, web::websockets::websocket_outgoing_message>> outgoing_messages;

            for (auto wit = websockets.left.begin(); websockets.left.end() != wit;)
            {
                const auto& websocket = *wit;

                // for each websocket connection that has valid grain and subscription resources
                const auto grain = find_resource(resources, { websocket.first, nmos::types::grain });
                if (resources.end() == grain)
                {
                    // theoretically blocking, but in fact not
                    listener.close(websocket.second, web::websockets::websocket_close_status::server_terminate, U("Deleted")).wait();

                    wit = websockets.left.erase(wit);
                    continue;
                }
                const auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });
                if (resources.end() == subscription)
                {
                    // a grain without a subscription shouldn't be possible, but let's be tidy
                    erase_resource(resources, grain->id);

                    // theoretically blocking, but in fact not
                    listener.close(websocket.second, web::websockets::websocket_close_status::server_terminate, U("Deleted")).wait();

                    wit = websockets.left.erase(wit);
                    continue;
                }
                // and has events to send
                if (0 == nmos::fields::message_grain_data(grain->data).size())
                {
                    ++wit;
                    continue;
                }

                // throttle messages according to the subscription's max_update_rate_ms
                // see discussion about creation_timestamp below...
                const auto max_update_rate = std::chrono::milliseconds(nmos::fields::max_update_rate_ms(subscription->data));
                const auto earliest_allowed_update = time_point_from_tai(nmos::fields::creation_timestamp(nmos::fields::message(grain->data))) + max_update_rate;
                if (earliest_allowed_update > now)
                {
                    // make sure to send a message as soon as allowed
                    if (earliest_allowed_update < earliest_necessary_update)
                    {
                        earliest_necessary_update = earliest_allowed_update;
                    }
                    // just don't do it now!
                    ++wit;
                    continue;
                }

                // experimental extension, to limit maximum number of events per message

                resource_paging paging(nmos::fields::params(subscription->data), most_recent_message, (size_t)nmos::experimental::fields::query_ws_paging_default(model.settings), (size_t)nmos::experimental::fields::query_ws_paging_limit(model.settings));
                auto next_events = value::array();

                // determine the grain timestamps

                // the meanings of each of these are being clarified in IS-04 v1.3
                // see https://github.com/AMWA-TV/nmos-discovery-registration/pull/102

                // origin_timestamp is like paging.until, the timestamp of the most recent update potentially included in this message
                // it has therefore been subject to the usual adjustments to make it unique and strictly increasing
                // another possibility would be to make it the timestamp of the most recent update *actually* included
                // or it could be the timestamp of (or the timestamp before, like paging.since) the least recent update included
                const auto origin_timestamp = value::string(nmos::make_version(most_recent_message));

                // creation_timestamp reflects the time that the message is actually being prepared
                // this may be more recent if messages have been throttled
                // or less recent since it hasn't been adjusted in the same way as the update timestamps
                const auto creation_timestamp = value::string(nmos::make_version(tai_from_time_point(now)));

                // prepare the message

                resources.modify(grain, [&paging, &next_events, &origin_timestamp, &creation_timestamp](nmos::resource& grain)
                {
                    auto& message = nmos::fields::message(grain.data);

                    // postpone all the events after the specified limit
                    auto& next_storage = web::json::storage_of(next_events.as_array());
                    auto& message_storage = web::json::storage_of(nmos::fields::grain_data(message).as_array());
                    if (paging.limit < message_storage.size())
                    {
                        const auto b = message_storage.begin() + paging.limit, e = message_storage.end();
                        next_storage.assign(std::make_move_iterator(b), std::make_move_iterator(e));
                        message_storage.erase(b, e);
                        // hmm, feels like origin_timestamp should be adjusted in this case, but how?
                    }

                    // set the timestamps
                    message[nmos::fields::origin_timestamp] = origin_timestamp;
                    message[nmos::fields::sync_timestamp] = origin_timestamp;
                    message[nmos::fields::creation_timestamp] = creation_timestamp;
                });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing to send " << nmos::fields::message_grain_data(grain->data).size() << " changes on websocket connection: " << grain->id;

                //+ additional logging, cf. nmos::details::request_registration
                // see nmos/node_behaviour.cpp
                const auto topic = nmos::fields::grain_topic(nmos::fields::message(grain->data));
                const auto message_origin_timestamp = nmos::fields::origin_timestamp(nmos::fields::message(grain->data));
                for (const auto& event : nmos::fields::message_grain_data(grain->data).as_array())
                {
                    const auto id_type = nmos::details::get_resource_event_resource(topic, event);
                    const auto event_type = nmos::details::get_resource_event_type(event);
                    const auto event_origin_timestamp = web::json::field_with_default<tai>{ nmos::fields::origin_timestamp, message_origin_timestamp }(event);

                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Sending registration " << slog::omanip([&event_type](std::ostream& s)
                    {
                        switch (event_type)
                        {
                        case nmos::details::resource_added_event: s << "creation"; break;
                        case nmos::details::resource_removed_event: s << "deletion"; break;
                        case nmos::details::resource_modified_event: s << "update"; break;
                        case nmos::details::resource_unchanged_event: s << "sync"; break;
                        default: s << "event"; break;
                        }
                    }) << " for " << id_type << " at: " << nmos::make_version(event_origin_timestamp);
                }
                //- additional logging, cf. nmos::details::request_registration

                auto serialized = utility::us2s(nmos::fields::message(grain->data).serialize());
                web::websockets::websocket_outgoing_message message;
                message.set_utf8_message(serialized);

                outgoing_messages.push_back({ websocket.second, message });

                if (0 != next_events.size())
                {
                    // make sure to send a message as soon as allowed
                    if (now + max_update_rate < earliest_necessary_update)
                    {
                        earliest_necessary_update = now + max_update_rate;
                    }
                }

                // reset the grain for next time
                resources.modify(grain, [&next_events, &resources](nmos::resource& grain)
                {
                    using std::swap;
                    swap(nmos::fields::message_grain_data(grain.data), next_events);
                    next_events = value::array(); // unnecessary
                    grain.updated = strictly_increasing_update(resources);
                });

                ++wit;
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
                    catch (const web::websockets::websocket_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "WebSocket error: " << e.what() << " [" << e.error_code() << "]";
                    }
                });
                // current websocket_listener implementation is synchronous in any case, but just to make clear...
                // for now, wait for the message to be sent
                send.wait();
            }
        }
    }
}

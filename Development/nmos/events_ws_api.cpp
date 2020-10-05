#include "nmos/events_ws_api.h"

#include <boost/algorithm/string/join.hpp>
#include "cpprest/json_storage.h"
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
    // IS-07 Events WebSocket API
    // There are strong similarities between the behaviour of this API and the IS-04 Query WebSocket API
    // although the structure of the WebSocket messages is different, and the latter has a two-step process
    // for creating and connecting to a subscription whereas endpoints on this API are advertised via the
    // "connection_uri" in the IS-05 Connection API sender's active transport parameters.
    // Expiry of connections in the Events WebSocket API is more or less identical to expiry as performed
    // by the IS-04 Registration API, so this implementation also shares much commonality.
    // See nmos/query_ws_api.cpp and nmos/registration_api.cpp

    web::websockets::experimental::listener::validate_handler make_events_ws_validate_handler(nmos::node_model& model, slog::base_gate& gate_)
    {
        return [&model, &gate_](web::http::http_request req)
        {
            nmos::ws_api_gate gate(gate_, req.request_uri());
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            // RFC 6750 defines two methods of sending bearer access tokens which are applicable to WebSocket
            // Clients SHOULD use the "Authorization Request Header Field" method.
            // Clients MAY use a "URI Query Parameter".
            // See https://tools.ietf.org/html/rfc6750#section-2

            // For now, to determine whether the "resource name" is valid, only look at the path, and ignore any query parameters
            const auto& ws_resource_path = req.request_uri().path();
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating websocket connection to: " << ws_resource_path;

            const bool has_ws_resource_path = resources.end() != find_resource_if(resources, nmos::types::sender, [&ws_resource_path](const nmos::resource& resource)
            {
                auto active = nmos::fields::master_enable(nmos::fields::endpoint_active(resource.data));
                auto& transport_params = nmos::fields::transport_params(nmos::fields::endpoint_active(resource.data));
                auto& connection_uri = nmos::fields::connection_uri(transport_params.at(0));
                return active
                    && !connection_uri.is_null()
                    && ws_resource_path == web::uri(connection_uri.as_string()).path();
            });

            if (!has_ws_resource_path) slog::log<slog::severities::error>(gate, SLOG_FLF) << "Invalid websocket connection to: " << ws_resource_path;
            return has_ws_resource_path;
        };
    }

    web::websockets::experimental::listener::open_handler make_events_ws_open_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        using web::json::value;
        using web::json::value_of;

        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.events_resources;

            const auto& ws_resource_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Opening websocket connection to: " << ws_resource_path;

            // create a subscription (1-1 relationship with the connection)
            resources::const_iterator subscription;

            {
                const bool secure = nmos::experimental::fields::client_secure(model.settings);

                const auto ws_href = web::uri_builder()
                    .set_scheme(web::ws_scheme(secure))
                    .set_host(nmos::get_host(model.settings))
                    .set_port(nmos::fields::events_ws_port(model.settings))
                    .set_path(ws_resource_path)
                    .to_uri();

                const bool non_persistent = false;
                value data = value_of({
                    { nmos::fields::id, nmos::make_id() },
                    { nmos::fields::max_update_rate_ms, 0 },
                    { nmos::fields::resource_path, U('/') + nmos::resourceType_from_type(nmos::types::source) },
                    { nmos::fields::params, value_of({ { U("query.rql"), U("in(id,())") } }) },
                    { nmos::fields::persist, non_persistent },
                    { nmos::fields::secure, secure },
                    { nmos::fields::ws_href, ws_href.to_string() }
                });

                // hm, could version be determined from ws_resource_path?
                nmos::resource subscription_{ is07_versions::v1_0, nmos::types::subscription, std::move(data), non_persistent };

                subscription = insert_resource(resources, std::move(subscription_)).first;
            }

            {
                // create a websocket connection resource

                value data;
                nmos::id id = nmos::make_id();
                data[nmos::fields::id] = value::string(id);
                data[nmos::fields::subscription_id] = value::string(subscription->id);

                // create an initial websocket message with no data

                const auto resource_path = nmos::fields::resource_path(subscription->data);
                const auto topic = resource_path + U('/');
                // source_id and flow_id are set per-message depending on the source, unlike Query WebSocket API
                data[nmos::fields::message] = details::make_grain({}, {}, topic);

                // there is no (unchanged, a.k.a. sync) data with which to populate the grain
                // until after a subscription command adds a source

                //nmos::fields::message_grain_data(data) = make_resource_events(resources, subscription->version, resource_path, nmos::fields::params(subscription->data));

                // track the grain for the websocket connection as a sub-resource of the subscription

                // like the subscription, the connection resource is not persistent
                // it is deleted if a health command is not received soon enough
                // or when the connection is closed
                resource grain{ subscription->version, nmos::types::grain, std::move(data), false };

                insert_resource(resources, std::move(grain));

                websockets.insert({ id, connection_id });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Creating websocket connection: " << id << " to subscription: " << subscription->id;

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying events websockets thread"; // and anyone else who cares...
                model.notify();
            }
        };
    }

    web::websockets::experimental::listener::close_handler make_events_ws_close_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, web::websockets::websocket_close_status close_status, const utility::string_t& close_reason)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.events_resources;

            const auto& ws_resource_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing websocket connection to: " << ws_resource_path << " [" << (int)close_status << ": " << close_reason << "]";

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(resources, { websocket->second, nmos::types::grain });

                if (resources.end() != grain)
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Deleting websocket connection: " << grain->id;

                    // subscriptions have a 1-1 relationship with the websocket connection and both should now be erased immediately
                    auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });

                    if (resources.end() != subscription)
                    {
                        // this should erase grain too, as a subscription's subresource
                        erase_resource(resources, subscription->id);
                    }
                    else
                    {
                        // a grain without a subscription shouldn't be possible, but let's be tidy
                        erase_resource(resources, grain->id);
                    }
                }

                websockets.right.erase(websocket);

                model.notify();
            }
        };
    }

    web::websockets::experimental::listener::message_handler make_events_ws_message_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        using web::json::value;
        using web::json::value_of;

        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, const web::websockets::websocket_incoming_message& msg_)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.events_resources;

            // theoretically blocking, but in fact not
            auto msg = msg_.extract_string().get();

            const auto& ws_resource_path = connection_uri.path();
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received websocket message: " << msg << " on connection to: " << ws_resource_path;

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(resources, { websocket->second, nmos::types::grain });

                if (resources.end() != grain)
                {
                    auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });

                    if (resources.end() != subscription)
                    {
                        try
                        {
                            const auto message = value::parse(utility::conversions::to_string_t(msg));

                            auto& command = nmos::fields::command(message);
                            if (U("subscription") == command)
                            {
                                // update the subscription

                                modify_resource(resources, subscription->id, [&message](nmos::resource& resource)
                                {
                                    // there is no way to send an error message back if the requested source id doesn't exist
                                    // hmm, this also won't take account of whether the associated sender is subsequently disabled by setting master_enable to false
                                    auto rql_query = U("in(id,(") + boost::algorithm::join(nmos::fields::sources(message) | boost::adaptors::transformed([](const value& v) { return v.as_string(); }), U(",")) + U("))");

                                    resource.data[nmos::fields::params] = value_of({ { U("query.rql"), rql_query } });
                                });

                                // update the grain with the current (sync) data for state messages
                                // without discarding any outstanding events (health messages, for example)

                                resources.modify(grain, [&](nmos::resource& grain)
                                {
                                    const auto& resource_path = nmos::fields::resource_path(subscription->data);
                                    const auto& params = nmos::fields::params(subscription->data);
                                    // these resource events are not structured as required for the state message
                                    // so will be transformed in nmos::send_events_ws_messages_thread
                                    auto events = make_resource_events(resources, subscription->version, resource_path, params);

                                    auto& events_storage = web::json::storage_of(events.as_array());
                                    auto& grain_storage = web::json::storage_of(nmos::fields::message_grain_data(grain.data).as_array());
                                    if (!grain_storage.empty())
                                    {
                                        events_storage.insert(events_storage.end(), std::make_move_iterator(grain_storage.begin()), std::make_move_iterator(grain_storage.end()));
                                        grain_storage.clear();
                                    }
                                    using std::swap;
                                    swap(grain_storage, events_storage);

                                    grain.updated = strictly_increasing_update(resources);
                                });

                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Received subscription command for " << nmos::fields::sources(message).size() << " sources";
                                model.notify();
                            }
                            else if (U("health") == command)
                            {
                                const auto health = nmos::health_now();
                                set_resource_health(resources, subscription->id, health);

                                // append a health message
                                // hmm, consider whether health messages ought to have higher priority?

                                resources.modify(grain, [&](nmos::resource& grain)
                                {
                                    web::json::push_back(nmos::fields::message_grain_data(grain.data), make_events_health_message({ nmos::tai_now(), nmos::fields::timestamp(message) }));

                                    grain.updated = strictly_increasing_update(resources);
                                });

                                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received health command";
                                model.notify();
                            }
                        }
                        catch (const web::json::json_exception& e)
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "JSON error: " << e.what();

                            // sending an error message back at this point would be nice?
                            // see https://github.com/AMWA-TV/nmos-event-tally/issues/41
                        }
                    }
                }
            }
        };
    }

    // reboot message
    // see https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/2.0.%20Message%20types.md#12-the-reboot-message-type
    web::json::value make_events_reboot_message(const nmos::details::events_state_identity& identity, const nmos::details::events_state_timing& timing)
    {
        using web::json::value_of;

        return value_of({
            { U("identity"), make_events_state_identity(identity) },
            { U("timing"), nmos::details::make_events_state_timing(timing) },
            { U("message_type"), U("reboot") }
        });
    }

    // shutdown message
    // see https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/2.0.%20Message%20types.md#13-the-shutdown-message-type
    web::json::value make_events_shutdown_message(const nmos::details::events_state_identity& identity, const nmos::details::events_state_timing& timing)
    {
        using web::json::value_of;

        return value_of({
            { U("identity"), make_events_state_identity(identity) },
            { U("timing"), nmos::details::make_events_state_timing(timing) },
            { U("message_type"), U("shutdown") }
        });
    }

    // health message
    // see https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/2.0.%20Message%20types.md#15-the-health-message
    web::json::value make_events_health_message(const nmos::details::events_state_timing& timing)
    {
        using web::json::value_of;

        return value_of({
            { U("timing"), nmos::details::make_events_state_timing(timing) },
            { U("message_type"), U("health") }
        });
    }

    namespace details
    {
        struct observe_websocket_exception
        {
            observe_websocket_exception(slog::base_gate& gate) : gate(gate) {}

            void operator()(pplx::task<void> finally)
            {
                try
                {
                    finally.get();
                }
                catch (const web::websockets::websocket_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "WebSocket error: " << e.what() << " [" << e.error_code() << "]";
                }
            }

            slog::base_gate& gate;
        };
    }

    void send_events_ws_messages_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::send_events_ws_messages));

        using web::json::value;
        using web::json::value_of;

        // could start out as a shared/read lock, only upgraded to an exclusive/write lock when a grain in the resources is actually modified
        auto lock = model.write_lock();
        auto& condition = model.condition;
        auto& shutdown = model.shutdown;
        auto& resources = model.events_resources;

        tai most_recent_message{};
        auto earliest_necessary_update = (tai_clock::time_point::max)();

        for (;;)
        {
            // wait for the thread to be interrupted either because there are resource changes, or because the server is being shut down
            // or because message sending was throttled earlier
            details::wait_until(condition, lock, earliest_necessary_update, [&] { return shutdown || most_recent_message < most_recent_update(resources); });
            if (shutdown) break;
            most_recent_message = most_recent_update(resources);

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Got notification on events websockets thread";

            earliest_necessary_update = (tai_clock::time_point::max)();

            std::vector<std::pair<web::websockets::experimental::listener::connection_id, web::websockets::websocket_outgoing_message>> outgoing_messages;

            for (auto wit = websockets.left.begin(); websockets.left.end() != wit;)
            {
                const auto& websocket = *wit;

                // for each websocket connection that has valid grain and subscription resources
                const auto grain = find_resource(resources, { websocket.first, nmos::types::grain });
                if (resources.end() == grain)
                {
                    auto close = listener.close(websocket.second, web::websockets::websocket_close_status::server_terminate, U("Expired"))
                        .then(details::observe_websocket_exception(gate));
                    // theoretically blocking, but in fact not
                    close.wait();

                    wit = websockets.left.erase(wit);
                    continue;
                }
                const auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });
                if (resources.end() == subscription)
                {
                    // a grain without a subscription shouldn't be possible, but let's be tidy
                    erase_resource(resources, grain->id);

                    auto close = listener.close(websocket.second, web::websockets::websocket_close_status::server_terminate, U("Expired"))
                        .then(details::observe_websocket_exception(gate));
                    // theoretically blocking, but in fact not
                    close.wait();

                    wit = websockets.left.erase(wit);
                    continue;
                }
                // and has events to send
                if (0 == nmos::fields::message_grain_data(grain->data).size())
                {
                    ++wit;
                    continue;
                }

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing to send " << nmos::fields::message_grain_data(grain->data).size() << " events on websocket connection: " << grain->id;

                for (const auto& event : nmos::fields::message_grain_data(grain->data).as_array())
                {
                    web::websockets::websocket_outgoing_message message;

                    if (event.has_field(U("message_type")))
                    {
                        // reboot, shutdown or health message
                        message.set_utf8_message(utility::us2s(event.serialize()));
                        outgoing_messages.push_back({ websocket.second, message });
                    }
                    else if (event.has_field(U("post")))
                    {
                        // state message
                        // see https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/2.0.%20Message%20types.md#11-the-state-message-type
                        // and nmos::make_events_boolean_state, nmos::make_events_number_state, etc.
                        // and nmos::details::make_resource_event
                        const web::json::value& state = nmos::fields::endpoint_state(event.at(U("post")));
                        message.set_utf8_message(utility::us2s(state.serialize()));
                        outgoing_messages.push_back({ websocket.second, message });
                    }
                }

                // reset the grain for next time
                resources.modify(grain, [&resources](nmos::resource& grain)
                {
                    // all messages have now been prepared
                    nmos::fields::message_grain_data(grain.data) = value::array();
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
                auto send = listener.send(outgoing_message.first, outgoing_message.second)
                    .then(details::observe_websocket_exception(gate));
                // current websocket_listener implementation is synchronous in any case, but just to make clear...
                // for now, wait for the message to be sent
                send.wait();
            }
        }
    }

    void erase_expired_events_resources_thread(nmos::node_model& model, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::events_expiry));

        // start out as a shared/read lock, only upgraded to an exclusive/write lock when an expired resource actually needs to be deleted from the resources
        auto lock = model.read_lock();
        auto& shutdown_condition = model.shutdown_condition;
        auto& shutdown = model.shutdown;
        auto& resources = model.events_resources;

        auto least_health = nmos::least_health(resources);

        // wait until the next connection could potentially expire, or the server is being shut down
        // (since health is truncated to seconds, and we want to be certain the expiry interval has passed, there's an extra second to wait here)
        while (!shutdown_condition.wait_until(lock, time_point_from_health(least_health.first + nmos::fields::events_expiry_interval(model.settings) + 1), [&] { return shutdown; }))
        {
            // hmmm, it needs to be possible to enable/disable periodic logging like this independently of the severity...
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "At " << nmos::make_version(nmos::tai_now()) << ", the node events' resources contains " << nmos::put_resources_statistics(resources);

            // most connections will have had a heartbeat during the wait, so the least health will have been increased
            // so this thread will be able to go straight back to waiting
            auto expire_health = health_now() - nmos::fields::events_expiry_interval(model.settings);
            auto forget_health = expire_health - nmos::fields::events_expiry_interval(model.settings);
            least_health = nmos::least_health(resources);
            if (least_health.first >= expire_health && least_health.second >= forget_health) continue;

            // otherwise, there's actually work to do...

            details::reverse_lock_guard<nmos::read_lock> unlock(lock);
            // note, without atomic upgrade, another thread may preempt hence the need to recalculate expire_health/forget_health and least_health
            auto upgrade = model.write_lock();

            expire_health = health_now() - nmos::fields::events_expiry_interval(model.settings);
            forget_health = expire_health - nmos::fields::events_expiry_interval(model.settings);

            // forget all resources expired in the previous interval
            forget_erased_resources(resources, forget_health);

            // expire all connections for which there hasn't been a heartbeat in the last expiry interval
            const auto expired = erase_expired_resources(resources, expire_health, false, true);

            if (0 != expired)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << expired << " resources have expired";

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying events websockets thread"; // and anyone else who cares...
                model.notify();
            }

            least_health = nmos::least_health(resources);
        }
    }
}

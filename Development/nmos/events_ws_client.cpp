#include "nmos/events_ws_client.h"

#include <unordered_map>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "pplx/pplx_utils.h" // for pplx::complete_after, etc.
#include "nmos/client_utils.h"
#include "nmos/events_resources.h"
#include "nmos/mutex.h"
#include "nmos/slog.h"

namespace std
{
    // Obvious specialization of std::hash for web::uri
    template <>
    struct hash<web::uri>
    {
        size_t operator()(const web::uri& uri) const
        {
            return std::hash<utility::string_t>()(uri.to_string());
        }
    };
}

// This declares a container type suitable for managing IS-07 Events WebSocket receiver subscriptions
// with indices to support the operations required by the IS-05 Connection API.
namespace nmos
{
    namespace tags
    {
        struct id;
        struct events_ws_subscription;
    }

    struct events_ws_subscription
    {
        nmos::id id;

        web::uri connection_uri;
        nmos::id source_id;
    };

    namespace details
    {
        typedef boost::multi_index::member<events_ws_subscription, nmos::id, &events_ws_subscription::id> events_ws_subscription_id_extractor;
        typedef boost::multi_index::member<events_ws_subscription, web::uri, &events_ws_subscription::connection_uri> events_ws_subscription_connection_uri_extractor;
        typedef boost::multi_index::member<events_ws_subscription, nmos::id, &events_ws_subscription::source_id> events_ws_subscription_source_id_extractor;
        typedef boost::multi_index::composite_key<events_ws_subscription, events_ws_subscription_connection_uri_extractor, events_ws_subscription_source_id_extractor> events_ws_subscription_extractor;
        typedef boost::tuple<web::uri, nmos::id> events_ws_subscription_extractor_tuple;
    }

    typedef boost::multi_index_container<
        events_ws_subscription,
        boost::multi_index::indexed_by<
        boost::multi_index::hashed_unique<boost::multi_index::tag<tags::id>, details::events_ws_subscription_id_extractor>,
        boost::multi_index::ordered_non_unique<boost::multi_index::tag<tags::events_ws_subscription>, details::events_ws_subscription_extractor>
        >
    > events_ws_subscriptions;
}

namespace nmos
{
    namespace details
    {
        struct events_ws_client_impl
        {
            events_ws_client_impl(web::websockets::client::websocket_client_config config, int events_heartbeat_interval, slog::base_gate& gate);

            pplx::task<void> subscribe(const nmos::id& id, const web::uri& connection_uri, const nmos::id& source_id);
            pplx::task<void> close(web::websockets::client::websocket_close_status close_status, const utility::string_t& close_reason);

            web::websockets::client::websocket_client_config config;
            int events_heartbeat_interval;
            nmos::details::omanip_gate gate;

            mutable nmos::mutex mutex;

            events_ws_close_handler user_close;

            events_ws_message_handler user_message;

            events_ws_subscriptions subscriptions;

            // until C++20, std::unordered_set::find need not support transparent key comparison (unlike std::set)
            std::unordered_map<web::uri, web::websockets::client::websocket_callback_client> connections;

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }

            static void wait_nothrow(pplx::task<void> t) { try { t.wait(); } catch (...) {} }

            static nmos::details::omanip_gate make_gate(slog::base_gate& gate) { return{ gate, nmos::stash_category(nmos::categories::send_events_ws_commands) }; }
        };

        events_ws_client_impl::events_ws_client_impl(web::websockets::client::websocket_client_config config, int events_heartbeat_interval, slog::base_gate& gate)
            : config(std::move(config))
            , events_heartbeat_interval(events_heartbeat_interval)
            , gate(make_gate(gate))
        {
        }

        pplx::task<void> events_ws_client_impl::subscribe(const nmos::id& id, const web::uri& connection_uri, const nmos::id& source_id)
        {
            pplx::task<void> result = pplx::task_from_result();

            auto lock = write_lock();

            auto subscription = subscriptions.find(id);

            if (subscriptions.end() == subscription)
            {
                // insert new subscription if valid

                if (!connection_uri.is_empty() && !source_id.empty())
                {
                    subscription = subscriptions.insert({ id, connection_uri, source_id }).first;
                }
            }
            else
            {
                // replace current subscription with new one if valid, or just erase old one

                const auto subscription_uri = subscription->connection_uri;

                if (!connection_uri.is_empty() && !source_id.empty())
                {
                    subscriptions.modify(subscription, [&](nmos::events_ws_subscription& subscription)
                    {
                        subscription.connection_uri = connection_uri;
                        subscription.source_id = source_id;
                    });
                }
                else
                {
                    subscriptions.erase(subscription);
                    subscription = subscriptions.end();
                }

                if (subscriptions.end() == subscription || subscription->connection_uri != subscription_uri)
                {
                    // send "subscription" command to old connection, or close it if no longer required

                    auto connection = connections.find(subscription_uri);
                    if (connections.end() != connection)
                    {
                        // "A disconnection IS-05 PATCH request should always trigger the client to remove the associated source id
                        // from the current WebSocket subscriptions list. If the source is the last item in the subscriptions list,
                        // then it is recommended for the client to close the underlying WebSocket connection."
                        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.1/docs/5.2.%20Transport%20-%20Websocket.md#35-disconnectingparking
                        // Doesn't seem much point in sending an empty subscription command, so just close the connection in that case...

                        auto& by_connection_uri = subscriptions.get<nmos::tags::events_ws_subscription>();
                        auto equal_connection_uris = by_connection_uri.equal_range(subscription_uri);
                        if (equal_connection_uris.second != equal_connection_uris.first)
                        {
                            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Sending subscription command";

                            auto sources = boost::copy_range<std::vector<nmos::id>>(equal_connection_uris
                                | boost::adaptors::transformed([](const nmos::events_ws_subscription& subscription) { return subscription.source_id; })
                                );
                            auto command = nmos::make_events_subscription_command(sources);
                            web::websockets::client::websocket_outgoing_message message;
                            message.set_utf8_message(utility::conversions::to_utf8string(command.serialize()));
                            result = connection->second.send(message);
                        }
                        else
                        {
                            auto client_after_erase = connection->second;
                            connections.erase(connection);
                            result = client_after_erase.close();
                        }
                    }
                }
            }

            if (subscriptions.end() != subscription)
            {
                // open new connection, if required

                auto connection = connections.find(connection_uri);
                if (connections.end() == connection)
                {
                    auto client = web::websockets::client::websocket_callback_client(config);

                    client.set_message_handler([this, connection_uri](const web::websockets::client::websocket_incoming_message& msg)
                    {
                        // theoretically blocking, but in fact not
                        msg.extract_string().then([this, connection_uri](const std::string& msg)
                        {
                            try
                            {
                                const auto message = web::json::value::parse(utility::conversions::to_string_t(msg));

                                // external message handler is only called once for each message, no matter how many receivers are subscribed to sources on this connection

                                // hmm, if the user wanted to implement subscription-specific behaviour for "shutdown", "reboot" and especially "state" messages (events),
                                // the callback might be simplified by passing a set of ids associated with the message source id, or perhaps a boost::any_range of the
                                // relevant event_ws_subscriptions, or access to the subscriptions member itself protected by the mutex
                                // ("heartbeat" messages are not source-specific so those could be handled separately; the user is less likely to be interested anyway...)

                                auto lock = read_lock();

                                if (user_message)
                                {
                                    user_message(connection_uri, message);
                                }
                            }
                            catch (const web::json::json_exception& e)
                            {
                                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "JSON error: " << e.what();

                                // sending an error message back at this point would be nice?
                                // should the connection be closed and re-opened?
                            }
                        }).wait();
                    });

                    auto cancellation_source = pplx::cancellation_token_source();
                    auto token = cancellation_source.get_token();

                    client.set_close_handler([this, connection_uri, cancellation_source](web::websockets::client::websocket_close_status close_status, const utility::string_t& close_reason, const std::error_code& error)
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Closing websocket connection to: " << connection_uri.to_string() << " [" << (int)close_status << ": " << close_reason << "]";

                        auto lock = write_lock();

                        // cancel heartbeats

                        cancellation_source.cancel();

                        // erase this connection (but not the associated subscriptions)

                        auto connection = connections.find(connection_uri);
                        if (connections.end() != connection)
                        {
                            // extend this client's lifetime beyond the close operation to avoid deadlock
                            auto client_after_erase = connection->second;
                            connections.erase(connection);
                            client_after_erase.close().then([client_after_erase] {});
                        }

                        // hmm, should probably try to re-make the connection, possibly with exponential back-off, for ephemeral error conditions
                        // see https://github.com/AMWA-TV/nmos-device-connection-management/issues/96

                        // for now, just signal via external handler

                        // hmm, if the user wanted to implement reconnection externally, they'd need to resubscribe all associated ids, which might be simplified
                        // by passing a bimap of associated ids and source ids, or perhaps a boost::any_range of the relevant event_ws_subscriptions, or access
                        // to the subscriptions member itself protected by the mutex

                        if (user_close)
                        {
                            user_close(connection_uri, close_status, close_reason);
                        }
                    });

                    result = result.then([this, client, connection_uri]() mutable
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Opening websocket connection to: " << connection_uri.to_string();

                        return client.connect(connection_uri);
                    });

                    auto heartbeats = result.then([this, client, token]() mutable
                    {
                        // "Upon connection, the client is required to report its health every 5 seconds in order to maintain its session and subscription."
                        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.2.%20Transport%20-%20Websocket.md#41-heartbeats

                        return pplx::do_while([this, client, token]() mutable
                        {
                            return pplx::complete_after(std::chrono::seconds(events_heartbeat_interval), token).then([this, client, token]() mutable
                            {
                                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Sending health command";

                                auto command = nmos::make_events_health_command();
                                web::websockets::client::websocket_outgoing_message message;
                                message.set_utf8_message(utility::conversions::to_utf8string(command.serialize()));
                                return client.send(message)
                                    .then([] { return true; });
                            });
                        }, token);
                    }).then(pplx::observe_exception<void>());

                    connection = connections.insert({ connection_uri, client }).first;
                }

                // send "subscription" command

                result = result.then([this, connection_uri]
                {
                    auto lock = read_lock();

                    auto connection = connections.find(connection_uri);
                    if (connections.end() != connection)
                    {
                        auto& by_connection_uri = subscriptions.get<nmos::tags::events_ws_subscription>();
                        auto equal_connection_uris = by_connection_uri.equal_range(connection_uri);
                        if (equal_connection_uris.second != equal_connection_uris.first)
                        {
                            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Sending subscription command";

                            auto sources = boost::copy_range<std::vector<nmos::id>>(equal_connection_uris
                                | boost::adaptors::transformed([](const nmos::events_ws_subscription& subscription) { return subscription.source_id; })
                                );
                            auto command = nmos::make_events_subscription_command(sources);
                            web::websockets::client::websocket_outgoing_message message;
                            message.set_utf8_message(utility::conversions::to_utf8string(command.serialize()));
                            return connection->second.send(message);
                        }
                    }

                    return pplx::task_from_result();
                });
            }

            return result;
        }

        pplx::task<void> events_ws_client_impl::close(web::websockets::client::websocket_close_status close_status, const utility::string_t& close_reason)
        {
            auto lock = write_lock();

            if (connections.empty()) return pplx::task_from_result();

            std::vector<pplx::task<void>> tasks;
            for (auto& connection : connections)
            {
                tasks.push_back(connection.second.close(close_status, close_reason));
            }

            subscriptions.clear();
            connections.clear();

            return pplx::when_all(tasks.begin(), tasks.end()).then([tasks](pplx::task<void> finally)
            {
                for (auto& task : tasks) wait_nothrow(task);
                finally.wait();
            });
        }
    }

    events_ws_client::events_ws_client(slog::base_gate& gate)
        : impl(new details::events_ws_client_impl({}, nmos::fields::events_heartbeat_interval.default_value, gate))
    {
    }

    events_ws_client::events_ws_client(web::websockets::client::websocket_client_config config, slog::base_gate& gate)
        : impl(new details::events_ws_client_impl(std::move(config), nmos::fields::events_heartbeat_interval.default_value, gate))
    {
    }

    events_ws_client::events_ws_client(web::websockets::client::websocket_client_config config, int events_heartbeat_interval, slog::base_gate& gate)
        : impl(new details::events_ws_client_impl(std::move(config), events_heartbeat_interval, gate))
    {
    }

    events_ws_client::events_ws_client(events_ws_client&& other)
        : impl(std::move(other.impl))
    {
    }

    events_ws_client& events_ws_client::operator=(events_ws_client&& other)
    {
        if (this != &other)
        {
            impl = std::move(other.impl);
        }
        return *this;
    }

    events_ws_client::~events_ws_client()
    {
        if (impl)
        {
            try
            {
                close(web::websockets::client::websocket_close_status::going_away, _XPLATSTR("Client shutting down")).wait();
            }
            catch (...)
            {
            }
        }
    }

    // update or create a subscription for the specified id, to the specified WebSocket URI and source id
    // by opening a new connection (and potentially closing an existing connection) and/or issuing subscription commands as required
    // heartbeat commands will also be issued at the appropriate intervals until the connection is closed
    pplx::task<void> events_ws_client::subscribe(const nmos::id& id, const web::uri& connection_uri, const nmos::id& source_id)
    {
        return impl->subscribe(id, connection_uri, source_id);
    }

    // remove the subscription for the specified id
    // by issuing a subscription command or closing the existing connection as required
    pplx::task<void> events_ws_client::unsubscribe(const nmos::id& id)
    {
        return impl->subscribe(id, {}, {});
    }

    // close all connections normally
    pplx::task<void> events_ws_client::close()
    {
        return close(web::websockets::client::websocket_close_status::normal, _XPLATSTR("Normal"));
    }

    // close all connections with the specified status and reason
    pplx::task<void> events_ws_client::close(web::websockets::client::websocket_close_status close_status, const utility::string_t& close_reason)
    {
        return impl->close(close_status, close_reason);
    }

    void events_ws_client::set_close_handler(events_ws_close_handler close_handler)
    {
        // updating the close handler with open connections is not supported, so no lock
        impl->user_close = close_handler;
    }

    void events_ws_client::set_message_handler(events_ws_message_handler message_handler)
    {
        // updating the message handler with open connections is not supported, so no lock
        impl->user_message = message_handler;
    }

    const web::websockets::client::websocket_client_config& events_ws_client::configuration() const
    {
        return impl->config;
    }
}

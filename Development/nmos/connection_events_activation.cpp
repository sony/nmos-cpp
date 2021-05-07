#include "nmos/connection_events_activation.h"

#include "pplx/pplx_utils.h"
#include "nmos/connection_api.h"
#include "nmos/client_utils.h"
#include "nmos/event_type.h"
#include "nmos/model.h"
#include "nmos/slog.h"
#include "nmos/transport.h"

namespace nmos
{
    // this handler can be used to (un)subscribe IS-07 Events WebSocket receivers with the specified handlers, when they are activated
    nmos::connection_activation_handler make_connection_events_websocket_activation_handler(load_ca_certificates_handler load_ca_certificates, events_ws_message_handler message_handler, events_ws_close_handler close_handler, const nmos::settings& settings, slog::base_gate& gate)
    {
        std::shared_ptr<nmos::events_ws_client> events_ws_client(new nmos::events_ws_client(nmos::make_websocket_client_config(settings, load_ca_certificates, gate), nmos::fields::events_heartbeat_interval(settings), gate));

        events_ws_client->set_message_handler(message_handler);
        events_ws_client->set_close_handler(close_handler);

        return [events_ws_client](const nmos::resource& resource, const nmos::resource& connection_resource)
        {
            if (nmos::types::receiver != resource.type) return;
            if (nmos::transports::websocket.name != nmos::fields::transport(resource.data)) return;

            // receiver must be active with a valid connection_uri and ext_is_07_source_id, otherwise unsubscribe

            const auto& endpoint_active = nmos::fields::endpoint_active(connection_resource.data);
            const bool active = nmos::fields::master_enable(endpoint_active);
            const auto& transport_params = nmos::fields::transport_params(endpoint_active).at(0);
            const auto& connection_uri_or_null = nmos::fields::connection_uri(transport_params);
            const auto& ext_is_07_source_id_or_null = nmos::fields::ext_is_07_source_id(transport_params);

            if (active && !connection_uri_or_null.is_null() && !ext_is_07_source_id_or_null.is_null())
            {
                events_ws_client->subscribe(connection_resource.id, connection_uri_or_null.as_string(), ext_is_07_source_id_or_null.as_string())
                    .then(pplx::observe_exception<void>());
            }
            else
            {
                events_ws_client->unsubscribe(connection_resource.id)
                    .then(pplx::observe_exception<void>());
            }
        };
    }

    namespace experimental
    {
        // since each Events WebSocket connection may potentially have subscriptions to a number of sources, for multiple receivers
        // this handler adaptor enables simple processing of "state" messages (events) per receiver
        nmos::events_ws_message_handler make_events_ws_message_handler(const nmos::node_model& model, events_ws_receiver_event_handler event_handler, slog::base_gate& gate)
        {
            return [&model, event_handler, &gate](const web::uri& connection_uri, const web::json::value& message)
            {
                const auto& message_type = nmos::fields::message_type(message);

                if (U("state") == message_type)
                {
                    const auto& source_id = nmos::fields::source_id(nmos::fields::identity(message));
                    const auto event_type = nmos::event_type(nmos::fields::state_event_type(message));

                    // external message handler is only called once for each message, no matter how many receivers are subscribed to sources on this connection

                    auto lock = model.read_lock();

                    // hmm, if the user wanted to implement subscription-specific behaviour for "shutdown", "reboot" and especially "state" messages (events),
                    // the callback might be simplified by passing a set of ids associated with the message source id, or perhaps a boost::any_range of the
                    // relevant event_ws_subscriptions, or access to the subscriptions member itself protected by the mutex

                    // for now, iterate over all receivers and just skip the ones that don't match

                    auto& by_type = model.node_resources.get<nmos::tags::type>();
                    const auto receivers = by_type.equal_range(nmos::details::has_data(nmos::types::receiver));

                    for (auto receiver = receivers.first; receivers.second != receiver; ++receiver)
                    {
                        if (nmos::transports::websocket.name != nmos::fields::transport(receiver->data)) continue;

                        const std::pair<nmos::id, nmos::type> id_type{ receiver->id, receiver->type };

                        auto connection_receiver = nmos::find_resource(model.connection_resources, id_type);
                        if (model.connection_resources.end() == connection_receiver) continue;

                        const auto& endpoint_active = nmos::fields::endpoint_active(connection_receiver->data);
                        const bool active = nmos::fields::master_enable(endpoint_active);
                        const auto& transport_params = nmos::fields::transport_params(endpoint_active).at(0);
                        const auto& connection_uri_or_null = nmos::fields::connection_uri(transport_params);
                        const auto& ext_is_07_source_id_or_null = nmos::fields::ext_is_07_source_id(transport_params);

                        if (!active) continue;
                        if (connection_uri_or_null.is_null() || connection_uri.to_string() != connection_uri_or_null.as_string()) continue;
                        if (ext_is_07_source_id_or_null.is_null() || source_id != ext_is_07_source_id_or_null.as_string()) continue;

                        // subscription-specific behaviour

                        const auto& event_type_caps_or_null = nmos::fields::event_types(nmos::fields::caps(receiver->data));

                        const bool match = event_type_caps_or_null.is_null() || [&]
                        {
                            const auto& event_type_caps = event_type_caps_or_null.as_array();
                            return event_type_caps.end() != std::find_if(event_type_caps.begin(), event_type_caps.end(), [event_type](const web::json::value& event_type_cap)
                            {
                                return nmos::is_matching_event_type(nmos::event_type(event_type_cap.as_string()), event_type);
                            });
                        }();

                        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received " << (match ? "" : "unexpected ") << "state message (" << event_type.name << ") for " << id_type;

                        if (match)
                        {
                            if (event_handler)
                            {
                                event_handler(*receiver, *connection_receiver, message);
                            }
                        }
                        // else, hmm, what now?
                    }
                }
                else // "health", "shutdown" or "reboot"
                {
                    // hmm, for "reboot", should probably try to re-make the connection, possibly with exponential back-off
                    // see https://github.com/AMWA-TV/nmos-device-connection-management/issues/96

                    // for now, just log all of these message types

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received " << message_type << " message";
                }
            };
        }

        // this handler reflects Events WebSocket connection errors into the /active endpoint of all associated receivers by setting master_enable to false
        nmos::events_ws_close_handler make_events_ws_close_handler(nmos::node_model& model, slog::base_gate& gate)
        {
            return [&model, &gate](const web::uri& connection_uri, web::websockets::client::websocket_close_status close_status, const utility::string_t& close_reason)
            {
                auto lock = model.write_lock();

                // hmm, should probably try to re-make the connection, possibly with exponential back-off, for ephemeral error conditions
                // see https://github.com/AMWA-TV/nmos-device-connection-management/issues/96

                // for now, just reflect this into the /active endpoint of all associated receivers by setting master_enable to false
                // see https://github.com/AMWA-TV/nmos-device-connection-management/pull/97

                const auto activation_time = nmos::tai_now();

                auto& by_type = model.node_resources.get<nmos::tags::type>();
                const auto receivers = by_type.equal_range(nmos::details::has_data(nmos::types::receiver));

                for (auto receiver = receivers.first; receivers.second != receiver; ++receiver)
                {
                    if (nmos::transports::websocket.name != nmos::fields::transport(receiver->data)) continue;

                    const std::pair<nmos::id, nmos::type> id_type{ receiver->id, receiver->type };

                    auto connection_receiver = nmos::find_resource(model.connection_resources, id_type);
                    if (model.connection_resources.end() == connection_receiver) continue;

                    const auto& endpoint_active = nmos::fields::endpoint_active(connection_receiver->data);
                    const bool active = nmos::fields::master_enable(endpoint_active);
                    const auto& transport_params = nmos::fields::transport_params(endpoint_active).at(0);
                    const auto& connection_uri_or_null = nmos::fields::connection_uri(transport_params);

                    if (!active) continue;
                    if (connection_uri_or_null.is_null() || connection_uri.to_string() != connection_uri_or_null.as_string()) continue;

                    // Update the IS-05 resource's /active endpoint

                    nmos::modify_resource(model.connection_resources, id_type.first, [&](nmos::resource& connection_resource)
                    {
                        using web::json::value;

                        const auto at = web::json::value::string(nmos::make_version(activation_time));

                        connection_resource.data[nmos::fields::version] = at;

                        auto& endpoint_active = nmos::fields::endpoint_active(connection_resource.data);
                        auto& active_activation = endpoint_active[nmos::fields::activation];

                        active_activation[nmos::fields::mode] = value::null();
                        active_activation[nmos::fields::requested_time] = value::null();
                        active_activation[nmos::fields::activation_time] = at;

                        endpoint_active[nmos::fields::master_enable] = value::boolean(false);
                    });

                    // Update the IS-04 resource's subscription

                    nmos::modify_resource(model.node_resources, id_type.first, [&activation_time](nmos::resource& resource)
                    {
                        nmos::set_resource_subscription(resource, false, {}, activation_time);
                    });
                }
            };
        }
    }
}

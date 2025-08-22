#include "nmos/control_protocol_ws_api.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/join.hpp>
#include "cpprest/json_validator.h"
#include "nmos/api_utils.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/is12_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace details
    {
        static const web::json::experimental::json_validator& controlprotocol_validator()
        {
            // hmm, could be based on supported API versions from settings, like other APIs' validators?
            static const web::json::experimental::json_validator validator
            {
                nmos::experimental::load_json_schema,
                boost::copy_range<std::vector<web::uri>>(boost::range::join(boost::range::join(
                    is12_versions::all | boost::adaptors::transformed(experimental::make_controlprotocolapi_base_message_schema_uri),
                    is12_versions::all | boost::adaptors::transformed(experimental::make_controlprotocolapi_command_message_schema_uri)),
                    is12_versions::all | boost::adaptors::transformed(experimental::make_controlprotocolapi_subscription_message_schema_uri)
                ))
            };
            return validator;
        }

        // Validate against specification schema
        // throws web::json::json_exception on failure, which results in a 400 Badly-formed command
        void validate_controlprotocolapi_base_message_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            controlprotocol_validator().validate(request_data, experimental::make_controlprotocolapi_base_message_schema_uri(version));
        }
        void validate_controlprotocolapi_command_message_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            controlprotocol_validator().validate(request_data, experimental::make_controlprotocolapi_command_message_schema_uri(version));
        }
        void validate_controlprotocolapi_subscription_message_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            controlprotocol_validator().validate(request_data, experimental::make_controlprotocolapi_subscription_message_schema_uri(version));
        }
    }

    // IS-12 Control Protocol WebSocket API

    web::websockets::experimental::listener::validate_handler make_control_protocol_ws_validate_handler(nmos::node_model& model, nmos::experimental::ws_validate_authorization_handler ws_validate_authorization, slog::base_gate& gate_)
    {
        return [&model, ws_validate_authorization, &gate_](web::http::http_request req)
        {
            nmos::ws_api_gate gate(gate_, req.request_uri());

            // RFC 6750 defines two methods of sending bearer access tokens which are applicable to WebSocket
            // Clients SHOULD use the "Authorization Request Header Field" method.
            // Clients MAY use a "URI Query Parameter".
            // See https://tools.ietf.org/html/rfc6750#section-2
            if (ws_validate_authorization)
            {
                if (!ws_validate_authorization(req, nmos::experimental::scopes::ncp)) { return false; }
            }

            // For now just return true
            const auto& ws_ncp_path = req.request_uri().path();
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating websocket connection to: " << ws_ncp_path;

            return true;
        };
    }

    web::websockets::experimental::listener::open_handler make_control_protocol_ws_open_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        using web::json::value;
        using web::json::value_of;

        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.control_protocol_resources;

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Opening websocket connection to: " << ws_ncp_path;

            // create a subscription (1-1 relationship with the connection)
            resources::const_iterator subscription;

            {
                const bool secure = nmos::experimental::fields::client_secure(model.settings);

                const auto ws_href = web::uri_builder()
                    .set_scheme(web::ws_scheme(secure))
                    .set_host(nmos::get_host(model.settings))
                    .set_port(nmos::fields::control_protocol_ws_port(model.settings))
                    .set_path(ws_ncp_path)
                    .to_uri();

                const utility::string_t control_protocol_resource_path;

                const bool non_persistent = false;
                value data = value_of({
                    { nmos::fields::id, nmos::make_id() },
                    { nmos::fields::max_update_rate_ms, 0 },
                    { nmos::fields::resource_path, control_protocol_resource_path },
                    { nmos::fields::params, value_of({ { U("query.rql"), U("in(id,())") } }) },
                    { nmos::fields::persist, non_persistent },
                    { nmos::fields::secure, secure },
                    { nmos::fields::ws_href, ws_href.to_string() }
                }, true);

                // hm, could version be determined from ws_resource_path?
                nmos::resource subscription_{ is12_versions::v1_0, nmos::types::subscription, std::move(data), non_persistent };

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
                data[nmos::fields::message] = details::make_grain({}, {}, topic);

                resource grain{ is12_versions::v1_0, nmos::types::grain, std::move(data), false };
                insert_resource(resources, std::move(grain));

                websockets.insert({ id, connection_id });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Creating websocket connection: " << id << " to subscription: " << subscription->id;

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying control protocol websockets thread"; // and anyone else who cares...
                model.notify();
            }
        };
    }

    web::websockets::experimental::listener::close_handler make_control_protocol_ws_close_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, web::websockets::websocket_close_status close_status, const utility::string_t& close_reason)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.control_protocol_resources;

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing websocket connection to: " << ws_ncp_path << " [" << (int)close_status << ": " << close_reason << "]";

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

    web::websockets::experimental::listener::message_handler make_control_protocol_ws_message_handler(nmos::node_model& model, nmos::websockets& websockets, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, nmos::get_control_protocol_method_descriptor_handler get_control_protocol_method_descriptor, nmos::control_protocol_property_changed_handler property_changed, slog::base_gate& gate_)
    {
        using web::json::value;
        using web::json::value_of;

        return [&model, &websockets, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_control_protocol_method_descriptor, property_changed, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, const web::websockets::websocket_incoming_message& msg_)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);

            auto lock = model.write_lock();
            auto& resources = model.control_protocol_resources;

            // theoretically blocking, but in fact not
            auto msg = msg_.extract_string().get();

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received websocket message: " << msg << " on connection: " << ws_ncp_path;

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
                            // extract the control protocol api version from the ws_ncp_path
                            if (web::uri::split_path(ws_ncp_path).empty()) { throw std::invalid_argument("empty URL"); }
                            const auto version = nmos::parse_api_version(web::uri::split_path(ws_ncp_path).back());

                            // convert message to JSON
                            const auto message = value::parse(utility::conversions::to_string_t(msg));

                            // validate the base-message
                            details::validate_controlprotocolapi_base_message_schema(version, message);

                            const auto msg_type = nmos::fields::nc::message_type(message);
                            switch (msg_type)
                            {
                            // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#command-message-type
                            case ncp_message_type::command:
                            {
                                // validate command-message
                                details::validate_controlprotocolapi_command_message_schema(version, message);

                                auto responses = value::array();
                                auto& commands = nmos::fields::nc::commands(message);
                                for (const auto& cmd : commands)
                                {
                                    const auto handle = nmos::fields::nc::handle(cmd);
                                    const auto oid = nmos::fields::nc::oid(cmd);

                                    // get methodId
                                    const auto& method_id = nmos::details::parse_nc_method_id(nmos::fields::nc::method_id(cmd));

                                    // get arguments
                                    const auto& arguments = nmos::fields::nc::arguments(cmd);

                                    value nc_method_result;

                                    auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                                    if (resources.end() != resource)
                                    {
                                        const auto& class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(resource->data));

                                        // find the relevant method handler to execute
                                        // method tuple definition described in control_protocol_handlers.h
                                        auto method = get_control_protocol_method_descriptor(class_id, method_id);
                                        auto& nc_method_descriptor = method.first;
                                        auto& control_method_handler = method.second;
                                        if (control_method_handler)
                                        {
                                            try
                                            {
                                                // do method arguments constraints validation
                                                nc::method_parameters_contraints_validation(arguments, nc_method_descriptor, get_control_protocol_datatype_descriptor);

                                                // execute the relevant control method handler, then accumulating up their response to responses
                                                // wrap the NcMethodResuls here
                                                nc_method_result = control_method_handler(resources, *resource, arguments, nmos::fields::nc::is_deprecated(nc_method_descriptor), gate);
                                            }
                                            catch (const nmos::control_protocol_exception& e)
                                            {
                                                // invalid arguments
                                                utility::ostringstream_t ss;
                                                ss << "invalid argument: " << arguments.serialize() << " error: " << e.what();
                                                slog::log<slog::severities::error>(gate, SLOG_FLF) << ss.str();
                                                nc_method_result = details::make_nc_method_result_error({ nmos::nc_method_status::parameter_error }, ss.str());
                                            }
                                        }
                                        else
                                        {
                                            // unknown methodId, or method not implemented
                                            utility::ostringstream_t ss;
                                            ss << U("unsupported method_id: ") << nmos::fields::nc::method_id(cmd).serialize()
                                                << U(" for control class class_id: ") << resource->data.at(nmos::fields::nc::class_id).serialize();
                                            slog::log<slog::severities::error>(gate, SLOG_FLF) << ss.str();
                                            nc_method_result = details::make_nc_method_result_error({ nc_method_status::method_not_implemented }, ss.str());
                                        }
                                    }
                                    else
                                    {
                                        // resource not found for the given oid
                                        utility::ostringstream_t ss;
                                        ss << U("unknown oid: ") << oid;
                                        slog::log<slog::severities::error>(gate, SLOG_FLF) << ss.str();
                                        nc_method_result = details::make_nc_method_result_error({ nc_method_status::bad_oid }, ss.str());
                                    }
                                    // accumulating up response
                                    auto response = make_control_protocol_response(handle, nc_method_result);

                                    web::json::push_back(responses, response);
                                }

                                // add command_response to the grain ready to transfer to the client in nmos::send_control_protocol_ws_messages_thread
                                resources.modify(grain, [&](nmos::resource& grain)
                                {
                                    web::json::push_back(nmos::fields::message_grain_data(grain.data), make_control_protocol_command_response(responses));

                                    grain.updated = strictly_increasing_update(resources);
                                });
                            }
                            break;
                            // See https://specs.amwa.tv/is-12/branches/v1.0.x/docs/Protocol_messaging.html#subscription-message-type
                            case ncp_message_type::subscription:
                            {
                                // validate subscription-message
                                details::validate_controlprotocolapi_subscription_message_schema(version, message);

                                // subscribing to multiple OIDs, and filtering out invalid OIDs which cannot be subscribed to
                                auto& subscriptions = nmos::fields::nc::subscriptions(message);
                                value valid_subscriptions = value::array();
                                for (const auto& subscription : subscriptions)
                                {
                                    const auto oid = subscription.as_integer();
                                    auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                                    if (resources.end() != resource)
                                    {
                                        // only add the valid OIDs which can be subscribed to
                                        web::json::push_back(valid_subscriptions, subscription);
                                    }
                                }

                                // update the subscription
                                modify_resource(resources, subscription->id, [&valid_subscriptions](nmos::resource& resource)
                                {
                                    auto rql_query = U("in(id,(") + boost::algorithm::join(valid_subscriptions.as_array() | boost::adaptors::transformed([](const value& v) { return U("string:") + utility::s2us(std::to_string(v.as_integer())); }), U(",")) + U("))");

                                    resource.data[nmos::fields::params] = value_of({ { U("query.rql"), rql_query } });
                                });

                                // add subscription_response to the grain ready to transfer to the client in nmos::send_control_protocol_ws_messages_thread
                                resources.modify(grain, [&](nmos::resource& grain)
                                {
                                    web::json::push_back(nmos::fields::message_grain_data(grain.data), make_control_protocol_subscription_response(valid_subscriptions));

                                    grain.updated = strictly_increasing_update(resources);
                                });

                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Received subscription command for " << valid_subscriptions.serialize();
                                model.notify();
                            }
                            break;
                            default:
                                // ignore unexpected message type
                                break;
                            }

                        }
                        catch (const web::json::json_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();

                            resources.modify(grain, [&](nmos::resource& grain)
                            {
                                web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                    make_control_protocol_error_message({ nc_method_status::bad_command_format }, utility::s2us(e.what())));

                                grain.updated = strictly_increasing_update(resources);
                            });
                        }
                        catch (const std::exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception while handing control protocol command: " << e.what();

                            resources.modify(grain, [&](nmos::resource& grain)
                            {
                                web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                    make_control_protocol_error_message({ nc_method_status::bad_command_format }, utility::s2us(std::string("Unexpected exception while handing control protocol command : ") + e.what())));

                                grain.updated = strictly_increasing_update(resources);
                            });
                        }
                        catch (...)
                        {
                            slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception for handing control protocol command";

                            resources.modify(grain, [&](nmos::resource& grain)
                            {
                                web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                    make_control_protocol_error_message({ nc_method_status::bad_command_format }, U("Unexpected unknown exception while handing control protocol command")));

                                grain.updated = strictly_increasing_update(resources);
                            });
                        }
                        model.notify();
                    }
                }
            }
        };
    }

    // observe_websocket_exception is the same as the one defined in events_ws_api
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

    void send_control_protocol_ws_messages_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::send_control_protocol_ws_messages));

        using web::json::value;
        using web::json::value_of;

        // could start out as a shared/read lock, only upgraded to an exclusive/write lock when a grain in the resources is actually modified
        auto lock = model.write_lock();
        auto& condition = model.condition;
        auto& shutdown = model.shutdown;
        auto& resources = model.control_protocol_resources;

        tai most_recent_message{};
        auto earliest_necessary_update = (tai_clock::time_point::max)();

        for (;;)
        {
            // wait for the thread to be interrupted either because there are resource changes, or because the server is being shut down
            // or because message sending was throttled earlier
            details::wait_until(condition, lock, earliest_necessary_update, [&] { return shutdown || most_recent_message < most_recent_update(resources); });
            if (shutdown) break;
            most_recent_message = most_recent_update(resources);

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Got notification on control protocol websockets thread";

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

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "outgoing_message: " << event.serialize();
                    message.set_utf8_message(utility::us2s(event.serialize()));
                    outgoing_messages.push_back({ websocket.second, message });
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
}

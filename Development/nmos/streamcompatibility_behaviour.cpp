#include "nmos/streamcompatibility_behaviour.h"

#include <vector>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/activation_mode.h"
#include "nmos/activation_utils.h"
#include "nmos/capabilities.h" // for constraint_sets
#include "nmos/connection_api.h" // for get_transport_type_data
#include "nmos/constraints.h"
#include "nmos/id.h"
#include "nmos/media_type.h"
#include "nmos/model.h"
#include "nmos/resources.h"
#include "nmos/sdp_utils.h"
#include "nmos/slog.h"
#include "nmos/streamcompatibility_state.h"
#include "nmos/streamcompatibility_utils.h"
#include "sdp/sdp.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            std::vector<nmos::id> get_resources_ids(const nmos::resources& resources, const nmos::type& type)
            {
                return boost::copy_range<std::vector<nmos::id>>(resources | boost::adaptors::filtered([&type](const nmos::resource& resource)
                {
                    return type == resource.type;
                }) | boost::adaptors::transformed([](const nmos::resource& resource)
                    {
                        return std::vector<nmos::id>::value_type{ resource.id };
                    }));
            }

            nmos::sender_state update_sender_status(nmos::node_model& model, const nmos::id& sender_id, const details::streamcompatibility_sender_validator& validate_sender_resources, slog::base_gate& gate)
            {
                using web::json::value;

                auto& node_resources = model.node_resources;
                auto& connection_resources = model.connection_resources;
                auto& streamcompatibility_resources = model.streamcompatibility_resources;

                const std::pair<nmos::id, nmos::type> sender_id_type{ sender_id, nmos::types::sender };
                auto sender = find_resource(node_resources, sender_id_type);
                if (node_resources.end() == sender) throw std::logic_error("matching IS-04 sender not found");

                const std::pair<nmos::id, nmos::type> flow_id_type{ nmos::fields::flow_id(sender->data).as_string(), nmos::types::flow };
                auto flow = find_resource(node_resources, flow_id_type);
                if (node_resources.end() == flow) throw std::logic_error("matching IS-04 flow not found");

                const std::pair<nmos::id, nmos::type> source_id_type{ nmos::fields::source_id(flow->data), nmos::types::source };
                auto source = find_resource(node_resources, source_id_type);
                if (node_resources.end() == source) throw std::logic_error("matching IS-04 source not found");

                auto connection_sender = find_resource(connection_resources, sender_id_type);
                if (connection_resources.end() == connection_sender) throw std::logic_error("matching IS-05 sender not found");

                auto streamcompatibility_sender = find_resource(streamcompatibility_resources, sender_id_type);
                if (streamcompatibility_resources.end() == streamcompatibility_sender) throw std::logic_error("matching IS-11 sender not found");

                nmos::sender_state sender_state(nmos::fields::state(nmos::fields::status(streamcompatibility_sender->data)));
                utility::string_t sender_state_debug;

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "IS-11 " << sender_id_type << " status before active constraints validation: " << sender_state.name;

                // Setting the State to any value except for "no_essence" or "awaiting_essence" triggers Active Constraints validation
                if (sender_state != nmos::sender_states::no_essence && sender_state != nmos::sender_states::awaiting_essence)
                {
                    const auto& constraint_sets = nmos::fields::constraint_sets(nmos::fields::active_constraint_sets(nmos::fields::endpoint_active_constraints(streamcompatibility_sender->data))).as_array();

                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating " << sender_id_type << " active constraints against with its sender, flow, source and transport file";
                    if (validate_sender_resources)
                    {
                        // do custom sender_source_flow constraints validation to update the sender_state and sender_state_debug
                        std::tie(sender_state, sender_state_debug) = validate_sender_resources(*source, *flow, *sender, *connection_sender, constraint_sets);
                    }
                }

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "IS-11 " << sender_id_type << " status after active constraints validation: " << sender_state.name;

                // update sender state and sender_state_debug if state has changed
                if (nmos::fields::state(nmos::fields::status(streamcompatibility_sender->data)) != sender_state.name)
                {
                    utility::string_t updated_timestamp{ nmos::make_version() };

                    set_sender_status(node_resources, streamcompatibility_resources, sender_id, sender_state, sender_state_debug, gate);
                }

                return sender_state;
            }

            nmos::receiver_state update_receiver_status(nmos::node_model& model, const nmos::id& receiver_id, const details::streamcompatibility_receiver_validator& validate_receiver, slog::base_gate& gate)
            {
                using web::json::value;

                auto& node_resources = model.node_resources;
                auto& connection_resources = model.connection_resources;
                auto& streamcompatibility_resources = model.streamcompatibility_resources;

                const std::pair<nmos::id, nmos::type> receiver_id_type{ receiver_id, nmos::types::receiver };

                auto node_receiver = find_resource(node_resources, receiver_id_type);
                if (node_resources.end() == node_receiver) throw std::logic_error("matching IS-04 receiver not found");

                auto connection_receiver = find_resource(connection_resources, receiver_id_type);
                if (connection_resources.end() == connection_receiver) throw std::logic_error("matching IS-05 receiver not found");

                auto streamcompatibility_receiver = find_resource(streamcompatibility_resources, receiver_id_type);
                if (streamcompatibility_resources.end() == streamcompatibility_receiver) throw std::logic_error("matching IS-11 receiver not found");

                nmos::receiver_state receiver_state(nmos::receiver_states::unknown);
                utility::string_t receiver_state_debug;

                if (nmos::fields::master_enable(nmos::fields::endpoint_active(connection_receiver->data)))
                {
                    const auto& transport_file = nmos::fields::transport_file(nmos::fields::endpoint_active(connection_receiver->data));

                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating " << receiver_id_type << " against with its transport file";
                    if (validate_receiver)
                    {
                        std::tie(receiver_state, receiver_state_debug) = validate_receiver(*node_receiver, transport_file);
                    }
                }

                if (nmos::fields::state(nmos::fields::status(streamcompatibility_receiver->data)) != receiver_state.name)
                {
                    set_receiver_status(node_resources, streamcompatibility_resources, receiver_id, receiver_state, receiver_state_debug, gate);
                }

                return receiver_state;
            }
        }

        void streamcompatibility_behaviour_thread(nmos::node_model& model, details::streamcompatibility_sender_validator validate_sender_resources, details::streamcompatibility_receiver_validator validate_receiver, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_of;

            auto lock = model.write_lock(); // in order to update state of Sender/Receiver
            auto& node_resources = model.node_resources;
            auto& connection_resources = model.connection_resources;
            auto& streamcompatibility_resources = model.streamcompatibility_resources;

            auto most_recent_update = nmos::tai_min();

            for (;;)
            {
                model.wait(lock, [&] { return model.shutdown || most_recent_update < nmos::most_recent_update(node_resources); }); // check for any IS-04 resources updated, shouldn't be checking IS-11 instead of IS-04 ???
                if (model.shutdown) break;

                // find Senders with recently updated IS-11 properties, associated Flow or Source
                auto streamcompatibility_senders_ids = details::get_resources_ids(streamcompatibility_resources, nmos::types::sender);
                for (const nmos::id& sender_id : streamcompatibility_senders_ids)
                {
                    const std::pair<nmos::id, nmos::type> sender_id_type{ sender_id, nmos::types::sender };

                    try
                    {
                        auto sender = find_resource(node_resources, sender_id_type);
                        if (node_resources.end() == sender) throw std::logic_error("matching IS-04 sender not found");

                        const std::pair<nmos::id, nmos::type> flow_id_type{ nmos::fields::flow_id(sender->data).as_string(), nmos::types::flow };
                        auto flow = find_resource(node_resources, flow_id_type);
                        if (node_resources.end() == flow) throw std::logic_error("matching IS-04 flow not found");

                        const std::pair<nmos::id, nmos::type> source_id_type{ nmos::fields::source_id(flow->data), nmos::types::source };
                        auto source = find_resource(node_resources, source_id_type);
                        if (node_resources.end() == source) throw std::logic_error("matching IS-04 source not found");

                        // check any updated on IS-04 source/flow/sender which is related to the IS-11 sender
                        const auto updated = most_recent_update < sender->updated ||
                            most_recent_update < flow->updated ||
                            most_recent_update < source->updated;

                        if (updated)
                        {
                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "The " << sender_id_type << ", its Flow, or its Source has been updated recently, and now's the time to update its IS-11 status.";

                            // update sender status against sender constraints
                            const auto sender_state = details::update_sender_status(model, sender_id, validate_sender_resources, gate);

                            // `At any time if State of an active Sender becomes active_constraints_violation, the Sender MUST become inactive. An inactive Sender in this state MUST NOT allow activations.`
                            // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Behaviour_-_Server_Side.html#preventing-restrictions-violation
                            // hmm, todo check sender current activation status before stop sender when active Sender becomes active_constraints_violation
                            if (nmos::sender_states::active_constraints_violation == sender_state)
                            {
                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "IS-11 " << sender_id_type << " status changed to " << sender_state.name << " -> Stopping " << sender_id_type;
                                web::json::value merged;
                                nmos::modify_resource(connection_resources, sender_id, [&merged](nmos::resource& connection_resource)
                                {
                                    connection_resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());
                                    merged = nmos::fields::endpoint_staged(connection_resource.data);
                                    merged[nmos::fields::master_enable] = value::boolean(false);
                                    auto activation = nmos::make_activation();
                                    activation[nmos::fields::mode] = value::string(nmos::activation_modes::activate_immediate.name);
                                    nmos::details::merge_activation(merged[nmos::fields::activation], activation, nmos::tai_now());
                                    connection_resource.data[nmos::fields::endpoint_staged] = merged;
                                });

                                nmos::details::handle_immediate_activation_pending(model, lock, sender_id_type, merged[nmos::fields::activation], gate);
                            }
                        }
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Failed to update IS-11 " << sender_id_type << " status, unexpected exception: " << e.what();
                        continue;
                    }
                }

                // find IS-11 Receivers with recently updated "caps" to check whether the active transport file still satisfies them
                auto streamcompatibility_receivers_ids = details::get_resources_ids(streamcompatibility_resources, nmos::types::receiver);
                for (const nmos::id& receiver_id : streamcompatibility_receivers_ids)
                {
                    const std::pair<nmos::id, nmos::type> receiver_id_type{ receiver_id, nmos::types::receiver };

                    try
                    {
                        auto receiver = find_resource(node_resources, receiver_id_type);
                        if (node_resources.end() == receiver) throw std::logic_error("matching IS-04 receiver not found");

                        const auto updated = most_recent_update < receiver->updated;
                        if (updated)
                        {
                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "The " << receiver_id_type << " has been updated recently, and now's the time to update its IS-11 status.";

                            // update receiver status against receiver constraints
                            const auto receiver_state = details::update_receiver_status(model, receiver_id, validate_receiver, gate);

                            // `At any time if State of an active Receiver becomes non_compliant_stream, the Receiver SHOULD become inactive. An inactive Receiver in this state SHOULD NOT allow activations.`
                            // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Behaviour_-_Server_Side.html#preventing-restrictions-violation
                            // hmm, todo check receiver current activation status before stop sender when active Receiver becomes non_compliant_stream
                            if (nmos::receiver_states::non_compliant_stream == receiver_state)
                            {
                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "IS-11 " << receiver_id_type << " status changed to " << receiver_state.name << " -> Stopping " << receiver_id_type;
                                web::json::value merged;
                                nmos::modify_resource(connection_resources, receiver_id, [&merged](nmos::resource& connection_resource)
                                {
                                    connection_resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());
                                    merged = nmos::fields::endpoint_staged(connection_resource.data);
                                    merged[nmos::fields::master_enable] = value::boolean(false);
                                    auto activation = nmos::make_activation();
                                    activation[nmos::fields::mode] = value::string(nmos::activation_modes::activate_immediate.name);
                                    nmos::details::merge_activation(merged[nmos::fields::activation], activation, nmos::tai_now());
                                    connection_resource.data[nmos::fields::endpoint_staged] = merged;
                                });

                                nmos::details::handle_immediate_activation_pending(model, lock, receiver_id_type, merged[nmos::fields::activation], gate);
                            }
                        }
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Failed to update IS-11 " << receiver_id_type << " status, unexpected exception: " << e.what();
                        continue;
                    }
                }

                model.notify();
                most_recent_update = nmos::most_recent_update(node_resources);
            }
        }
    }
}

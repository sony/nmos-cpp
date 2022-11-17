#include "nmos/streamcompatibility_behaviour.h"
#include "nmos/streamcompatibility_state.h"
#include "nmos/streamcompatibility_utils.h"

#include <vector>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/activation_mode.h"
#include "nmos/activation_utils.h"
#include "nmos/capabilities.h" // for constraint_sets
#include "nmos/constraints.h"
#include "nmos/id.h"
#include "nmos/media_type.h"
#include "nmos/model.h"
#include "nmos/resources.h"
#include "nmos/sdp_utils.h"
#include "nmos/slog.h"
#include "sdp/sdp.h"

namespace nmos
{
    namespace experimental
    {
        utility::string_t get_sdp_data(const web::json::value& transport_file)
        {
            // "'data' and 'type' must both be strings or both be null"
            // See https://specs.amwa.tv/is-05/releases/v1.0.2/APIs/schemas/with-refs/v1.0-receiver-response-schema.html
            // and https://specs.amwa.tv/is-05/releases/v1.1.0/APIs/schemas/with-refs/receiver-transport-file.html

            if (!transport_file.has_field(nmos::fields::data)) throw std::logic_error("data is required");

            auto& transport_data = transport_file.at(nmos::fields::data);
            if (transport_data.is_null()) throw std::logic_error("data is required");

            if (!transport_file.has_field(nmos::fields::type)) throw std::logic_error("type is required");

            auto& transport_type = transport_file.at(nmos::fields::type);
            if (transport_type.is_null() || transport_type.as_string().empty()) throw std::logic_error("type is required");

            if (nmos::media_types::application_sdp.name != transport_type.as_string()) throw std::logic_error("transport file type is not SDP");

            return transport_data.as_string();
        }

        std::vector<nmos::id> get_resources_ids(const nmos::resources& resources, const nmos::type& type)
        {
            return boost::copy_range<std::vector<nmos::id>>(resources | boost::adaptors::filtered([&type] (const nmos::resource& resource)
            {
                return type == resource.type;
            }) | boost::adaptors::transformed([](const nmos::resource& resource)
            {
                return std::vector<nmos::id>::value_type{ resource.id };
            }));
        }

        std::pair<nmos::sender_state, utility::string_t> validate_sender_resources(const web::json::value& transport_file, const web::json::value& sender, const web::json::value& flow, const web::json::value& source, const web::json::array& constraint_sets)
        {
            nmos::sender_state sender_state;

            if (!web::json::empty(constraint_sets))
            {
                bool constrained = true;

                auto source_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return match_source_parameters_constraint_set(source, constraint_set); });
                auto flow_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return match_flow_parameters_constraint_set(flow, constraint_set); });
                auto sender_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return match_sender_parameters_constraint_set(sender, constraint_set); });

                constrained = constraint_sets.end() != source_found && constraint_sets.end() != flow_found && constraint_sets.end() != sender_found;

                if (!transport_file.is_null() && !transport_file.as_object().empty())
                {
                    utility::string_t sdp_data = get_sdp_data(transport_file);
                    const auto session_description = sdp::parse_session_description(utility::us2s(sdp_data));
                    auto sdp_params = nmos::parse_session_description(session_description).first;

                    const auto format_params = nmos::details::get_format_parameters(sdp_params);
                    const auto sdp_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return nmos::details::match_sdp_parameters_constraint_set(nmos::details::format_constraints, sdp_params, format_params, constraint_set); });

                    constrained = constrained && constraint_sets.end() != sdp_found;
                }

                sender_state = constrained ? nmos::sender_states::constrained : nmos::sender_states::active_constraints_violation;
            }
            else
            {
                sender_state = nmos::sender_states::unconstrained;
            }

            return { sender_state, {} };
        }

        std::pair<nmos::receiver_state, utility::string_t> validate_receiver_resources(const web::json::value& transport_file, const web::json::value& receiver)
        {
            nmos::receiver_state receiver_state;
            utility::string_t receiver_state_debug;

            if (!transport_file.is_null() && !transport_file.as_object().empty())
            {
                utility::string_t sdp_data = get_sdp_data(transport_file);
                const auto session_description = sdp::parse_session_description(utility::us2s(sdp_data));
                auto sdp_params = nmos::parse_session_description(session_description).first;

                receiver_state = nmos::receiver_states::compliant_stream;

                try
                {
                    validate_sdp_parameters(receiver, sdp_params);
                }
                catch (const std::runtime_error& e)
                {
                    receiver_state = nmos::receiver_states::non_compliant_stream;
                    receiver_state_debug = utility::conversions::to_string_t(e.what());
                }
            }
            else
            {
                receiver_state = nmos::receiver_states::unknown;
            }

            return { receiver_state, receiver_state_debug };
        }

        void streamcompatibility_behaviour_thread(nmos::node_model& model, details::streamcompatibility_sender_validator validate_sender, details::streamcompatibility_receiver_validator validate_receiver, slog::base_gate& gate)
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
                model.wait(lock, [&] { return model.shutdown || most_recent_update < nmos::most_recent_update(node_resources); });
                if (model.shutdown) break;

                auto streamcompatibility_senders_ids = get_resources_ids(streamcompatibility_resources, nmos::types::sender);
                auto streamcompatibility_receivers_ids = get_resources_ids(streamcompatibility_resources, nmos::types::receiver);

                // find Senders with recently updated IS-11 properties, associated Flow or Source
                for (const nmos::id& sender_id : streamcompatibility_senders_ids)
                {
                    try
                    {
                        bool updated = false;

                        const std::pair<nmos::id, nmos::type> sender_id_type{ sender_id, nmos::types::sender };
                        auto sender = find_resource(node_resources, sender_id_type);
                        if (node_resources.end() == sender) throw std::logic_error("Matching IS-04 Sender not found");

                        const std::pair<nmos::id, nmos::type> flow_id_type{ nmos::fields::flow_id(sender->data).as_string(), nmos::types::flow };
                        auto flow = find_resource(node_resources, flow_id_type);
                        if (node_resources.end() == flow) throw std::logic_error("Matching IS-04 Flow not found");

                        const std::pair<nmos::id, nmos::type> source_id_type{ nmos::fields::source_id(flow->data), nmos::types::source };
                        auto source = find_resource(node_resources, source_id_type);
                        if (node_resources.end() == source) throw std::logic_error("Matching IS-04 Source not found");

                        updated = most_recent_update < sender->updated ||
                            most_recent_update < flow->updated ||
                            most_recent_update < source->updated;

                        if (updated)
                        {
                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sender " << sender_id << " or its Flow or Source has been updated recently and the state of this Sender is being updated as well";

                            auto streamcompatibility_sender = find_resource(streamcompatibility_resources, sender_id_type);
                            if (streamcompatibility_resources.end() == streamcompatibility_sender) throw std::logic_error("Matching IS-11 Sender not found");

                            auto connection_sender = find_resource(connection_resources, sender_id_type);
                            if (connection_resources.end() == connection_sender) throw std::logic_error("Matching IS-05 Sender not found");

                            nmos::sender_state sender_state(nmos::fields::state(nmos::fields::status(streamcompatibility_sender->data)));
                            utility::string_t sender_state_debug;

                            // Setting the State to any value except for "no_essence" or "awaiting_essence" triggers Active Constraints validation
                            if (sender_state != nmos::sender_states::no_essence && sender_state != nmos::sender_states::awaiting_essence)
                            {
                                const auto& constraint_sets = nmos::fields::constraint_sets(nmos::fields::active_constraint_sets(nmos::fields::endpoint_active_constraints(streamcompatibility_sender->data))).as_array();
                                auto& transport_file = nmos::fields::endpoint_transportfile(connection_sender->data);

                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sender " << sender_id << " is being validated with its Flow, Source and transport file";
                                if (validate_sender)
                                {
                                    std::tie(sender_state, sender_state_debug) = validate_sender(transport_file, sender->data, flow->data, source->data, constraint_sets);
                                }
                            }

                            if (nmos::fields::state(nmos::fields::status(streamcompatibility_sender->data)) != sender_state.name)
                            {
                                utility::string_t updated_timestamp;

                                modify_resource(streamcompatibility_resources, sender_id, [&sender_state, &sender_state_debug, &updated_timestamp, &gate](nmos::resource& sender)
                                {
                                    nmos::fields::status(sender.data)[nmos::fields::state] = web::json::value::string(sender_state.name);
                                    if (!sender_state_debug.empty())
                                    {
                                        nmos::fields::status(sender.data)[nmos::fields::debug] = web::json::value::string(sender_state_debug);
                                    }

                                    updated_timestamp = nmos::make_version();
                                    sender.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                                });

                                update_version(node_resources, sender_id, updated_timestamp);
                            }

                            if (sender_state == nmos::sender_states::active_constraints_violation)
                            {
                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Stopping Sender " << sender->id;
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
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "An exception appeared while updating the state of Sender " << sender_id << ": " << e.what();
                        continue;
                    }
                }

                // find IS-11 Receivers with recently updated "caps" to check whether the active transport file still satisfies them
                for (const nmos::id& receiver_id : streamcompatibility_receivers_ids)
                {
                    try
                    {
                        bool updated = false;

                        const std::pair<nmos::id, nmos::type> receiver_id_type{ receiver_id, nmos::types::receiver };
                        auto receiver = find_resource(node_resources, receiver_id_type);
                        if (node_resources.end() == receiver) throw std::logic_error("Matching IS-04 receiver not found");

                        updated = most_recent_update < nmos::fields::version(nmos::fields::caps(receiver->data));

                        if (updated)
                        {
                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Receiver " << receiver_id << " has been updated recently and Receiver State is being updated as well";

                            const std::pair<nmos::id, nmos::type> streamcompatibility_receiver_id_type{ receiver_id, nmos::types::receiver };
                            auto streamcompatibility_receiver = find_resource(streamcompatibility_resources, streamcompatibility_receiver_id_type);
                            if (streamcompatibility_resources.end() == streamcompatibility_receiver) throw std::logic_error("Matching IS-11 receiver not found");

                            nmos::receiver_state receiver_state(nmos::receiver_states::unknown);
                            utility::string_t receiver_state_debug;

                            const std::pair<nmos::id, nmos::type> connection_receiver_id_type{ receiver_id, nmos::types::receiver };
                            auto connection_receiver = find_resource(connection_resources, connection_receiver_id_type);
                            if (connection_resources.end() == connection_receiver) throw std::logic_error("Matching IS-05 receiver not found");

                            auto& transport_file = nmos::fields::transport_file(nmos::fields::endpoint_staged(connection_receiver->data));

                            if (validate_receiver)
                            {
                                std::tie(receiver_state, receiver_state_debug) = validate_receiver(transport_file, receiver->data);
                            }

                            if (nmos::fields::state(nmos::fields::status(streamcompatibility_receiver->data)) != receiver_state.name)
                            {
                                utility::string_t updated_timestamp;

                                modify_resource(streamcompatibility_resources, receiver_id, [&receiver_state, &receiver_state_debug, &updated_timestamp, &gate](nmos::resource& receiver)
                                {
                                    nmos::fields::status(receiver.data)[nmos::fields::state] = web::json::value::string(receiver_state.name);
                                    if (!receiver_state_debug.empty())
                                    {
                                        nmos::fields::status(receiver.data)[nmos::fields::debug] = web::json::value::string(receiver_state_debug);
                                    }

                                    updated_timestamp = nmos::make_version();
                                    receiver.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                                });

                                update_version(node_resources, receiver_id, updated_timestamp);
                            }

                            if (receiver_state == nmos::receiver_states::non_compliant_stream)
                            {
                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Stopping Receiver " << receiver->id;
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
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Updating receiver status for " << receiver_id << " raised exception: " << e.what();
                        continue;
                    }
                }

                model.notify();
                most_recent_update = nmos::most_recent_update(node_resources);
            }
        }
    }
}

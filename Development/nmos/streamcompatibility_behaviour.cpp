#include "nmos/streamcompatibility_behaviour.h"
#include "nmos/streamcompatibility_state.h"
#include "nmos/streamcompatibility_utils.h"

#include <vector>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
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

        nmos::sender_state validate_sender_resources(const web::json::value& transport_file, const web::json::value& flow, const web::json::value& source, const web::json::array& constraint_sets)
        {
            nmos::sender_state sender_state;

            if (!web::json::empty(constraint_sets))
            {
                bool constrained = true;

                auto source_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return match_source_parameters_constraint_set(source, constraint_set); });
                auto flow_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return match_flow_parameters_constraint_set(flow, constraint_set); });

                constrained = constraint_sets.end() != source_found && constraint_sets.end() != flow_found;

                if (!transport_file.is_null() && !transport_file.as_object().empty())
                {
                    utility::string_t sdp_data = get_sdp_data(transport_file);
                    const auto session_description = sdp::parse_session_description(utility::us2s(sdp_data));
                    auto sdp_params = nmos::parse_session_description(session_description).first;

                    const auto format_params = details::get_format_parameters(sdp_params);
                    const auto sdp_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return nmos::details::match_sdp_parameters_constraint_set(details::format_constraints, sdp_params, format_params, constraint_set); });

                    constrained = constrained && constraint_sets.end() != sdp_found;
                }

                sender_state = constrained ? nmos::sender_states::constrained : nmos::sender_states::active_constraints_violation;
            }
            else
            {
                sender_state = nmos::sender_states::unconstrained;
            }

            return sender_state;
        }

        nmos::receiver_state validate_receiver_resources(const web::json::value& transport_file, const web::json::value& receiver)
        {
            nmos::receiver_state receiver_state;

            if (!transport_file.is_null() && !transport_file.as_object().empty())
            {
                utility::string_t sdp_data = get_sdp_data(transport_file);
                const auto session_description = sdp::parse_session_description(utility::us2s(sdp_data));
                auto sdp_params = nmos::parse_session_description(session_description).first;

                receiver_state = nmos::receiver_states::ok;

                try
                {
                    validate_sdp_parameters(receiver, sdp_params);
                }
                catch (const std::runtime_error& e)
                {
                    receiver_state = nmos::receiver_states::receiver_capabilities_violation;
                }
            }
            else
            {
                receiver_state = nmos::receiver_states::no_transport_file;
            }

            return receiver_state;
        }

        void streamcompatibility_behaviour_thread(nmos::node_model& model, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_of;

            auto lock = model.write_lock(); // in order to update the resources
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

                // find IS-11 recently updated Senders and Senders with recently updated Flow or Source
                for (const nmos::id& sender_id : streamcompatibility_senders_ids)
                {
                    try
                    {
                        bool updated = false;

                        const std::pair<nmos::id, nmos::type> sender_id_type{ sender_id, nmos::types::sender };
                        auto sender = find_resource(node_resources, sender_id_type);
                        if (node_resources.end() == sender) throw std::logic_error("Matching IS-04 sender not found");

                        const std::pair<nmos::id, nmos::type> flow_id_type{ nmos::fields::flow_id(sender->data).as_string(), nmos::types::flow };
                        auto flow = find_resource(node_resources, flow_id_type);
                        if (node_resources.end() == flow) throw std::logic_error("Matching IS-04 flow not found");

                        const std::pair<nmos::id, nmos::type> source_id_type{ nmos::fields::source_id(flow->data), nmos::types::source };
                        auto source = find_resource(node_resources, source_id_type);
                        if (node_resources.end() == source) throw std::logic_error("Matching IS-04 source not found");

                        updated = most_recent_update < sender->updated ||
                            most_recent_update < flow->updated ||
                            most_recent_update < source->updated;

                        if (updated)
                        {
                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sender " << sender_id << " or its Flow or Source has been updated recently and Sender State is being updated as well";

                            const std::pair<nmos::id, nmos::type> streamcompatibility_sender_id_type{ sender_id, nmos::types::sender };
                            auto streamcompatibility_sender = find_resource(streamcompatibility_resources, streamcompatibility_sender_id_type);
                            if (streamcompatibility_resources.end() == streamcompatibility_sender) throw std::logic_error("Matching IS-11 sender not found");

                            nmos::signal_state signal_state(nmos::fields::signal_state(nmos::fields::endpoint_status(streamcompatibility_sender->data)));
                            nmos::sender_state sender_state(signal_state.name);

                            if (signal_state == nmos::signal_states::signal_is_present)
                            {
                                const auto& constraint_sets = nmos::fields::constraint_sets(nmos::fields::active_constraint_sets(nmos::fields::endpoint_active_constraints(streamcompatibility_sender->data))).as_array();

                                const std::pair<nmos::id, nmos::type> connection_sender_id_type{ sender_id, nmos::types::sender };
                                auto connection_sender = find_resource(connection_resources, connection_sender_id_type);
                                if (connection_resources.end() == connection_sender) throw std::logic_error("Matching IS-05 sender not found");

                                auto& transport_file = nmos::fields::endpoint_transportfile(connection_sender->data);

                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sender " << sender_id << " is being validated with its Flow, Source and transport file";
                                sender_state = validate_sender_resources(transport_file, flow->data, source->data, constraint_sets);
                            }

                            if (nmos::fields::state(nmos::fields::status(nmos::fields::endpoint_status(streamcompatibility_sender->data))) != sender_state.name)
                            {
                                utility::string_t updated_timestamp;

                                modify_resource(streamcompatibility_resources, sender_id, [&sender_state, &updated_timestamp, &gate](nmos::resource& sender)
                                {
                                    nmos::fields::status(nmos::fields::endpoint_status(sender.data))[nmos::fields::state] = web::json::value::string(sender_state.name);

                                    updated_timestamp = nmos::make_version();
                                    sender.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                                });

                                update_version(node_resources, sender_id, updated_timestamp);
                            }
                        }
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Updating sender status for " << sender_id << " raised exception: " << e.what();
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

                            nmos::receiver_state receiver_state(nmos::receiver_states::no_transport_file);

                            const std::pair<nmos::id, nmos::type> connection_receiver_id_type{ receiver_id, nmos::types::receiver };
                            auto connection_receiver = find_resource(connection_resources, connection_receiver_id_type);
                            if (connection_resources.end() == connection_receiver) throw std::logic_error("Matching IS-05 receiver not found");

                            auto& transport_file = nmos::fields::transport_file(nmos::fields::endpoint_staged(connection_receiver->data));

                            receiver_state = validate_receiver_resources(transport_file, receiver->data);

                            if (nmos::fields::state(nmos::fields::status(nmos::fields::endpoint_status(streamcompatibility_receiver->data))) != receiver_state.name)
                            {
                                utility::string_t updated_timestamp;

                                modify_resource(streamcompatibility_resources, receiver_id, [&receiver_state, &updated_timestamp, &gate](nmos::resource& receiver)
                                {
                                    nmos::fields::status(nmos::fields::endpoint_status(receiver.data))[nmos::fields::state] = web::json::value::string(receiver_state.name);

                                    updated_timestamp = nmos::make_version();
                                    receiver.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                                });

                                update_version(node_resources, receiver_id, updated_timestamp);
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

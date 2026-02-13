#include "nmos/streamcompatibility_validation.h"

#include "nmos/connection_api.h"
#include "nmos/json_fields.h"
#include "nmos/model.h"
#include "nmos/resource.h"
#include "nmos/resources.h"
#include "nmos/slog.h"
#include "nmos/streamcompatibility_state.h"
#include "sdp/sdp.h"

namespace nmos
{
    namespace experimental
    {
        namespace detail
        {
            bool match_resource_parameters_constraint_set(const parameter_constraints& constraints, const web::json::value& resource, const web::json::value& constraint_set_)
            {
                if (!nmos::caps::meta::enabled(constraint_set_)) return false;

                const auto& constraint_set = constraint_set_.as_object();
                return constraint_set.end() == std::find_if(constraint_set.begin(), constraint_set.end(), [&](const std::pair<utility::string_t, web::json::value>& constraint)
                {
                    const auto found = constraints.find(constraint.first);
                    return constraints.end() != found && !found->second(resource, constraint.second);
                });
            }
        }

        void validate_rtp_transport_file(const nmos::resource& receiver, const utility::string_t& transport_file_type, const utility::string_t& transport_file_data)
        {
            return details::validate_rtp_transport_file(&validate_sdp_parameters, receiver, transport_file_type, transport_file_data);
        }

        void details::validate_rtp_transport_file(nmos::details::sdp_parameters_validator validate_sdp_parameters, const nmos::resource& receiver, const utility::string_t& transport_file_type, const utility::string_t& transport_file_data)
        {
            if (transport_file_type != nmos::media_types::application_sdp.name)
            {
                throw std::runtime_error("unexpected type: " + utility::us2s(transport_file_type));
            }

            const auto session_description = sdp::parse_session_description(utility::us2s(transport_file_data));
            auto sdp_transport_params = nmos::parse_session_description(session_description);

            // Validate transport file according to the IS-04 receiver, throws an exception when validation fails

            validate_sdp_parameters(receiver.data, sdp_transport_params.first);
        }

        // "At any time if State of an active Sender becomes active_constraints_violation, the Sender MUST become inactive.
        // An inactive Sender in this state MUST NOT allow activations.
        // At any time if State of an active Receiver becomes non_compliant_stream, the Receiver SHOULD become inactive.
        // An inactive Receiver in this state SHOULD NOT allow activations."
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Behaviour_-_Server_Side.html#preventing-restrictions-violation
        nmos::details::connection_resource_patch_validator make_connection_streamcompatibility_validator(nmos::node_model& model)
        {
            return [&model] (const nmos::resource& resource, const nmos::resource& connection_resource, const web::json::value& endpoint_staged, slog::base_gate& gate)
            {
                if (nmos::fields::master_enable(endpoint_staged))
                {
                    const auto id = nmos::fields::id(resource.data);
                    const auto type = resource.type;

                    auto streamcompatibility_resource = find_resource(model.streamcompatibility_resources, {id, type});
                    if (model.streamcompatibility_resources.end() != streamcompatibility_resource)
                    {
                        auto resource_state = nmos::fields::state(nmos::fields::status(streamcompatibility_resource->data));

                        if (resource_state == nmos::sender_states::active_constraints_violation.name || resource_state == nmos::receiver_states::non_compliant_stream.name)
                        {
                            throw std::logic_error("activation failed due to Status of the connector");
                        }
                    }
                }
            };
        }

        details::transport_file_constraint_sets_matcher make_streamcompatibility_sdp_constraint_sets_matcher(const details::sdp_constraint_sets_matcher& match_sdp_parameters_constraint_sets)
        {
            return [match_sdp_parameters_constraint_sets](const std::pair<utility::string_t, utility::string_t>& transport_file, const web::json::array& constraint_sets) -> bool
            {
                if (nmos::media_types::application_sdp.name != transport_file.first)
                {
                    throw std::runtime_error("unknown transport file type");
                }
                const auto session_description = sdp::parse_session_description(utility::us2s(transport_file.second));
                auto sdp_params = nmos::parse_session_description(session_description).first;

                return match_sdp_parameters_constraint_sets(constraint_sets, sdp_params);
            };

        }

        bool match_resource_parameters_constraint_set(const nmos::resource& resource, const web::json::value& constraint_set)
        {
            const std::map<nmos::type, parameter_constraints> resource_parameter_constraints
            {
                { nmos::types::source, source_parameter_constraints },
                { nmos::types::flow, flow_parameter_constraints },
                { nmos::types::sender, sender_parameter_constraints }
            };

            if (0 == resource_parameter_constraints.count(resource.type))
            {
                throw std::logic_error("wrong resource type");
            }

            return detail::match_resource_parameters_constraint_set(resource_parameter_constraints.at(resource.type), resource.data, constraint_set);
        }

        details::streamcompatibility_sender_validator make_streamcompatibility_sender_resources_validator(const details::resource_constraints_matcher& match_resource_constraint_set, const details::transport_file_constraint_sets_matcher& match_transport_file_constraint_sets)
        {
            return [match_resource_constraint_set, match_transport_file_constraint_sets](const nmos::resource& source, const nmos::resource& flow, const nmos::resource& sender, const nmos::resource& connection_sender, const web::json::array& constraint_sets) -> std::pair<nmos::sender_state, utility::string_t>
            {
                nmos::sender_state sender_state;

                if (!web::json::empty(constraint_sets))
                {
                    bool constrained = true;
                    auto source_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return match_resource_constraint_set(source, constraint_set); });
                    auto flow_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return match_resource_constraint_set(flow, constraint_set); });
                    auto sender_found = std::find_if(constraint_sets.begin(), constraint_sets.end(), [&](const web::json::value& constraint_set) { return match_resource_constraint_set(sender, constraint_set); });

                    constrained = constraint_sets.end() != source_found && constraint_sets.end() != flow_found && constraint_sets.end() != sender_found;

                    const auto& transport_file = nmos::fields::endpoint_transportfile(connection_sender.data);

                    if (!transport_file.is_null() && !transport_file.as_object().empty())
                    {
                        constrained = constrained && match_transport_file_constraint_sets(nmos::details::get_transport_type_data(transport_file), constraint_sets);
                    }

                    sender_state = constrained ? nmos::sender_states::constrained : nmos::sender_states::active_constraints_violation;
                }
                else
                {
                    sender_state = nmos::sender_states::unconstrained;
                }

                return { sender_state, {} };
            };
        }

        details::streamcompatibility_receiver_validator make_streamcompatibility_receiver_validator(const details::transport_file_validator& validate_transport_file)
        {
            return [validate_transport_file](const nmos::resource& receiver, const web::json::value& transport_file_) -> std::pair<nmos::receiver_state, utility::string_t>
            {
                nmos::receiver_state receiver_state = nmos::receiver_states::unknown;
                utility::string_t receiver_state_debug;

                if (!transport_file_.is_null() && !transport_file_.as_object().empty())
                {
                    const auto transport_file = nmos::details::get_transport_type_data(transport_file_);
                    if (std::pair<utility::string_t, utility::string_t>{} != transport_file)
                    {
                        receiver_state = nmos::receiver_states::compliant_stream;

                        try
                        {
                            validate_transport_file(receiver, transport_file.first, transport_file.second);
                        }
                        catch (const std::exception& e)
                        {
                            receiver_state = nmos::receiver_states::non_compliant_stream;
                            receiver_state_debug = utility::conversions::to_string_t(e.what());
                        }
                    }
                }

                return { receiver_state, receiver_state_debug };
            };
        }
    }
}

#include "nmos/channelmapping_api.h"

#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_validator.h"
#include "cpprest/json_visit.h"
#include "nmos/activation_mode.h"
#include "nmos/activation_utils.h"
#include "nmos/api_utils.h"
#include "nmos/is08_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/slog.h"
#include "nmos/tai.h"

namespace nmos
{
    web::http::experimental::listener::api_router make_unmounted_channelmapping_api(nmos::node_model& model, details::channelmapping_output_map_validator validate_merged, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_channelmapping_api(nmos::node_model& model, details::channelmapping_output_map_validator validate_merged, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router channelmapping_api;

        channelmapping_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
            return pplx::task_from_result(true);
        });

        channelmapping_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("channelmapping/") }, req, res));
            return pplx::task_from_result(true);
        });

        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is08_versions::from_settings(model.settings); });
        channelmapping_api.support(U("/x-nmos/") + nmos::patterns::channelmapping_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
            return pplx::task_from_result(true);
        });

        // At the moment, it doesn't seem necessary to enable support multiple API instances via the API selector mechanism
        // so therefore just mount the Channel Mapping API instance directly at /x-nmos/channelmapping/{version}/
        // If it becomes necessary, nmos::node_mode::channelmapping_resources could become a map from {selector} to nmos::resources
        // and the 'unmounted' API instance handler also mounted after a "child resources" handler based on the API selectors
        // See https://github.com/AMWA-TV/nmos-audio-channel-mapping/blob/v1.0.x/docs/2.0.%20APIs.md#api-paths
        channelmapping_api.mount(U("/x-nmos/") + nmos::patterns::channelmapping_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_channelmapping_api(model, validate_merged, gate));

        return channelmapping_api;
    }

    namespace details
    {
        static const web::json::experimental::json_validator& activation_validator()
        {
            // hmm, could be based on supported API versions from settings, like other APIs' validators?
            static const web::json::experimental::json_validator validator
            {
                nmos::experimental::load_json_schema,
                boost::copy_range<std::vector<web::uri>>(is08_versions::all | boost::adaptors::transformed(experimental::make_channelmappingapi_map_activations_post_request_schema_uri))
            };
            return validator;
        }

        // Validate against specification schema
        // throws web::json::json_exception on failure, which results in a 400 Bad Request
        void validate_activation_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            activation_validator().validate(request_data, experimental::make_channelmappingapi_map_activations_post_request_schema_uri(version));
        }

        inline channelmapping_activation_post_response make_channelmapping_activation_post_error_response(web::http::status_code code, const utility::string_t& error = {}, const utility::string_t& debug = {})
        {
            return{ code, make_error_response_body(code, error, debug) };
        }

        inline channelmapping_activation_post_response make_channelmapping_activation_post_error_response(web::http::status_code code, const std::exception& debug)
        {
            return make_channelmapping_activation_post_error_response(code, {}, utility::s2us(debug.what()));
        }

        inline utility::string_t make_channelmapping_input_error(const nmos::channelmapping_id& input_id)
        {
            return U("unknown input '") + input_id + U("'");
        }

        inline utility::string_t make_channelmapping_output_error(const nmos::channelmapping_id& output_id)
        {
            return U("unknown output '") + output_id + U("'");
        }

        inline utility::string_t make_channelmapping_unrouted_output_error(const nmos::channelmapping_id& output_id)
        {
            return U("output '") + output_id + U("' must be routed from an input");
        }

        inline utility::string_t make_channelmapping_routable_input_error(const nmos::channelmapping_id& output_id, const nmos::channelmapping_id& input_id)
        {
            return U("output '") + output_id + U("' cannot be routed from input '") + input_id + U("'");
        }

        inline utility::string_t make_channelmapping_input_channel_error(const nmos::channelmapping_id& input_id, const utility::string_t& channel_index)
        {
            return U("input '") + input_id + U("' does not have a channel ") + channel_index;
        }

        inline utility::string_t make_channelmapping_output_channel_error(const nmos::channelmapping_id& output_id, const utility::string_t& channel_index)
        {
            return U("output '") + output_id + U("' does not have a channel ") + channel_index;
        }

        inline utility::string_t make_channelmapping_output_not_locked_error(const nmos::channelmapping_id& output_id)
        {
            return U("output '") + output_id + U("' is locked by another activation");
        }

        inline utility::string_t make_channelmapping_input_block_size_error(const nmos::channelmapping_id& input_id, int block_size)
        {
            return U("input '") + input_id + U("' channels must be routed together in blocks of ") + utility::ostringstreamed(block_size);
        }

        inline utility::string_t make_channelmapping_input_block_not_reorderable_error(const nmos::channelmapping_id& input_id)
        {
            return U("input '") + input_id + U("' channels must be routed in order");
        }

        inline utility::string_t make_channelmapping_input_block_duplicate_channel_error(const nmos::channelmapping_id& input_id, const utility::string_t& channel_index)
        {
            return U("input '") + input_id + U("' channel ") + channel_index + U(" cannot be duplicated in a block");
        }

        channelmapping_activation_post_response validate_output_caps(const nmos::resources& channelmapping_resources, const nmos::resource& output, const web::json::value& output_action)
        {
            using web::http::status_codes;

            const auto& routable_inputs_or_null = nmos::fields::routable_inputs(nmos::fields::caps(nmos::fields::endpoint_io(output.data)));

            const auto& active_map = nmos::fields::map(nmos::fields::endpoint_active(output.data));

            for (const auto& channel_action : output_action.as_object())
            {
                // check the output channel is valid
                if (!active_map.has_field(channel_action.first))
                {
                    return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_output_channel_error(nmos::fields::channelmapping_id(output.data), channel_action.first));
                }

                const auto& input_id_or_null = nmos::fields::input(channel_action.second);

                const auto& input_channel_index_or_null = nmos::fields::channel_index(channel_action.second);
                if (input_id_or_null.is_null())
                {
                    // When "one field of Output channel object is null but other is not", the entire request must be rejected
                    // with the 400 Bad Request response.
                    // See https://nmos.amwa.tv/nmos-audio-channel-mapping/branches/v1.0.x/docs/4.0._Behaviour.html#activation-responses
                    if (!input_channel_index_or_null.is_null())
                    {
                        throw web::json::json_exception("invalid channel_index when input is null");
                    }

                    // "If no [routing] restrictions exist, the routable_inputs field MUST be set to null."
                    // See https://nmos.amwa.tv/nmos-audio-channel-mapping/branches/v1.0.x/docs/4.0._Behaviour.html#output-routing-constraints
                    if (!routable_inputs_or_null.is_null())
                    {
                        // "If the Device allows the Output to have unrouted channels, the list SHOULD also include null."
                        const auto& routable_inputs = routable_inputs_or_null.as_array();
                        if (routable_inputs.end() == std::find(routable_inputs.begin(), routable_inputs.end(), input_id_or_null))
                        {
                            return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_unrouted_output_error(nmos::fields::channelmapping_id(output.data)));
                        }
                    }
                }
                else
                {
                    const auto& input_id = input_id_or_null.as_string();

                    // When "one field of Output channel object is null but other is not", the entire request must be rejected
                    // with the 400 Bad Request response.
                    // See https://nmos.amwa.tv/nmos-audio-channel-mapping/branches/v1.0.x/docs/4.0._Behaviour.html#activation-responses
                    if (input_channel_index_or_null.is_null())
                    {
                        throw web::json::json_exception("invalid channel_index when input is not null");
                    }
                    const auto input_channel_index = input_channel_index_or_null.as_integer();

                    // "If no [routing] restrictions exist, the routable_inputs field MUST be set to null."
                    // See https://nmos.amwa.tv/nmos-audio-channel-mapping/branches/v1.0.x/docs/4.0._Behaviour.html#output-routing-constraints
                    if (!routable_inputs_or_null.is_null())
                    {
                        // "If an Input is listed in an Output's routable input list then channels from that input can be routed
                        // to that Output in the Map table."
                        const auto& routable_inputs = routable_inputs_or_null.as_array();
                        if (routable_inputs.end() == std::find(routable_inputs.begin(), routable_inputs.end(), input_id_or_null))
                        {
                            return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_routable_input_error(nmos::fields::channelmapping_id(output.data), input_id));
                        }
                    }

                    // check the input actually exists
                    const auto input = find_resource(channelmapping_resources, { nmos::make_channelmapping_resource_id({ input_id, nmos::types::input }), nmos::types::input });
                    if (input == channelmapping_resources.end())
                    {
                        return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_error(input_id));
                    }

                    // check the input channel index is valid
                    const auto& input_channels = nmos::fields::channels(nmos::fields::endpoint_io(input->data));
                    if (input_channel_index >= static_cast<int>(input_channels.size()))
                    {
                        return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_channel_error(input_id, utility::ostringstreamed(input_channel_index)));
                    }
                }
            }

            return{ status_codes::OK, {} };
        }

        channelmapping_activation_post_response validate_output_not_locked(const nmos::resource& output)
        {
            using web::http::status_codes;

            // When an "Output is modified by an already scheduled activation", the entire request must be rejected
            // with the 423 Locked response.
            // See https://nmos.amwa.tv/nmos-audio-channel-mapping/branches/v1.0.x/docs/4.0._Behaviour.html#activation-responses

            const auto& mode = nmos::fields::mode(nmos::fields::activation(nmos::fields::endpoint_staged(output.data)));

            // testing against null means that an in-flight immediate activation will also cause any subsequent activation
            // of the same output(s) to receive 423 Locked until the immediate activation is finished, but that seems reasonable
            if (!mode.is_null())
            {
                return make_channelmapping_activation_post_error_response(status_codes::Locked, U("Locked; ") + make_channelmapping_output_not_locked_error(nmos::fields::channelmapping_id(output.data)));
            }

            return{ status_codes::OK, {} };
        }

        channelmapping_activation_post_response validate_input_caps(const nmos::resources& channelmapping_resources, const nmos::channelmapping_id& output_id, const web::json::value& active_map)
        {
            using web::http::status_codes;

            nmos::channelmapping_id current_input_id;

            // "If the Input [...] cannot perform re-ordering, [...] there MUST be a fixed offset
            // for all Input and Output channel indexes in the mapping."
            // See https://nmos.amwa.tv/nmos-audio-channel-mapping/branches/v1.0.x/docs/4.0._Behaviour.html#re-ordering
            bool current_reorderable = false;
            std::map<nmos::channelmapping_id, int> channel_offsets;

            // "It MUST be that either all Input channels in a block are routed to an Output, or none are routed.
            // All Input channels in a block MUST be routed to the same Output when routed."
            // See https://nmos.amwa.tv/nmos-audio-channel-mapping/branches/v1.0.x/docs/4.0._Behaviour.html#channel-block-sizes
            unsigned int current_block_size = 0;
            std::set<int> channels_in_current_block;

            for (int output_channel_index = 0; output_channel_index < static_cast<int>(active_map.size()); ++output_channel_index)
            {
                const web::json::field_as_value output_channel_field{ utility::ostringstreamed(output_channel_index) };

                if (!active_map.has_field(output_channel_field))
                {
                    return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_output_channel_error(output_id, output_channel_field.key));
                }

                const auto& output_channel = output_channel_field(active_map);

                const auto& input_id_or_null = nmos::fields::input(output_channel);
                if (input_id_or_null.is_null()) continue;
                const auto& input_id = input_id_or_null.as_string();

                const auto& input_channel_index_or_null = nmos::fields::channel_index(output_channel);
                if (input_channel_index_or_null.is_null())
                {
                    throw web::json::json_exception("invalid channel_index when input is not null");
                }
                const auto input_channel_index = input_channel_index_or_null.as_integer();

                if (input_id != current_input_id)
                {
                    if (!current_input_id.empty())
                    {
                        // check the last input's block size constraint
                        if (channels_in_current_block.size() != current_block_size)
                        {
                            return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_block_size_error(current_input_id, current_block_size));
                        }
                    }

                    const auto input = find_resource(channelmapping_resources, { nmos::make_channelmapping_resource_id({ input_id, nmos::types::input }), nmos::types::input });
                    if (input == channelmapping_resources.end())
                    {
                        return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_error(input_id));
                    }

                    current_input_id = input_id;

                    const auto& input_caps = nmos::fields::caps(nmos::fields::endpoint_io(input->data));
                    current_block_size = nmos::fields::block_size(input_caps);
                    current_reorderable = nmos::fields::reordering(input_caps);

                    if (current_block_size == 0)
                    {
                        // 500 Internal Server Error
                        throw std::logic_error("invalid block_size");
                    }

                    // if this input cannot perform re-ordering, check this first channel index is at a block boundary
                    if (!current_reorderable)
                    {
                        if (input_channel_index % current_block_size != 0)
                        {
                            // "All blocks MUST start with an input channel where channel_index is zero or a multiple of the block_size parameter."
                            // See https://nmos.amwa.tv/nmos-audio-channel-mapping/branches/v1.0.x/docs/4.0._Behaviour.html#channel-block-sizes
                            return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_block_not_reorderable_error(input_id));
                        }
                    }

                    // if this input cannot perform re-ordering, save the fixed offset for the input
                    if (!current_reorderable)
                    {
                        // note map::insert does not overwrite an existing value for the same key
                        channel_offsets.insert({ input_id, input_channel_index - output_channel_index });
                    }

                    channels_in_current_block.clear();
                }
                else if (channels_in_current_block.size() == current_block_size)
                {
                    channels_in_current_block.clear();
                }

                // if this input cannot perform re-ordering, check the fixed offset for the input
                if (!current_reorderable)
                {
                    if (channel_offsets.at(input_id) != input_channel_index - output_channel_index)
                    {
                        return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_block_not_reorderable_error(input_id));
                    }
                }

                if (!channels_in_current_block.empty())
                {
                    // check this channel comes from the same block
                    if (input_channel_index / current_block_size != *channels_in_current_block.begin() / current_block_size)
                    {
                        return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_block_size_error(current_input_id, current_block_size));
                    }

                    // if this input cannot perform re-ordering, check the fixed offset or equivalently contiguous input channel indexes
                    if (!current_reorderable)
                    {
                        if (input_channel_index != 1 + *channels_in_current_block.rbegin())
                        {
                            return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_block_not_reorderable_error(input_id));
                        }
                    }
                }

                // check for a repeated channel index
                const auto inserted = channels_in_current_block.insert(input_channel_index).second;
                if (!inserted)
                {
                    return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_block_duplicate_channel_error(current_input_id, utility::ostringstreamed(input_channel_index)));
                }
            }

            // check the last input's block size constraint
            if (!current_input_id.empty())
            {
                if (channels_in_current_block.size() != current_block_size)
                {
                    return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_input_block_size_error(current_input_id, current_block_size));
                }
            }

            return{ status_codes::OK, {} };
        }

        channelmapping_activation_post_response handle_channelmapping_activation_post(nmos::node_model& model, nmos::write_lock& lock, const nmos::api_version& version, const nmos::id& activation_id, const web::json::value& request_data, const nmos::tai& request_time, details::channelmapping_output_map_validator validate_merged, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            // lock.owns_lock() must be true initially
            auto& resources = model.channelmapping_resources;

            // Validate JSON syntax according to the schema

            validate_activation_schema(version, request_data);

            const auto request_state = details::get_activation_state(nmos::fields::activation(request_data));

            // Validate constraints

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating requested action against constraints";

            const auto& request_action = nmos::fields::action(request_data);

            if (web::json::empty(request_action))
            {
                // since activations are represented in the data model by distributing them across the outputs' /staged 'endpoints'
                // handling scheduled activations with empty actions is awkward
                throw std::runtime_error("empty actions are not supported");
            }

            for (const auto& output_action : request_action.as_object())
            {
                // convert channelmapping_id to id
                const auto id = nmos::make_channelmapping_resource_id({ output_action.first, nmos::types::output });

                // check output exists
                const auto output = find_resource(resources, { id, nmos::types::output });

                if (resources.end() == output)
                {
                    return make_channelmapping_activation_post_error_response(status_codes::BadRequest, U("Bad Request; ") + make_channelmapping_output_error(output_action.first));
                }

                auto result = details::validate_output_not_locked(*output);

                if (!web::http::is_success_status_code(result.first))
                {
                    return result;
                }

                result = details::validate_output_caps(resources, *output, output_action.second);

                if (!web::http::is_success_status_code(result.first))
                {
                    return result;
                }

                // Merge this post request into a *copy* of the current active 'endpoint' map
                // so that the merged map can be validated against the constraints

                auto merged = nmos::fields::map(nmos::fields::endpoint_active(output->data));

                web::json::merge_patch(merged, output_action.second);

                // Validate merged JSON according to the constraints

                result = details::validate_input_caps(resources, output_action.first, merged);

                if (!web::http::is_success_status_code(result.first))
                {
                    return result;
                }

                // Perform any final validation

                if (validate_merged)
                {
                    validate_merged(*output, merged, gate);
                }
            }

            // Finally, update the staged 'endpoint' of each of the requested outputs

            const auto& request_activation = nmos::fields::activation(request_data);

            for (const auto& output_action : request_action.as_object())
            {
                // convert channelmapping_id to id
                const auto id = nmos::make_channelmapping_resource_id({ output_action.first, nmos::types::output });

                modify_resource(resources, id, [&](nmos::resource& resource)
                {
                    auto& staged = nmos::fields::endpoint_staged(resource.data);

                    staged[nmos::fields::activation_id] = web::json::value::string(activation_id);

                    details::merge_activation(staged[nmos::fields::activation], request_activation, request_time);

                    staged[nmos::fields::action] = output_action.second;
                });
            }

            if (details::scheduled_activation_pending == request_state)
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Scheduled channel mapping activation requested for " << activation_id;
            else if (details::immediate_activation_pending == request_state)
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Immediate channel mapping activation requested for " << activation_id;

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying channel mapping activation thread";

            model.notify();

            // Prepare the response

            auto response_data = request_data;
            details::merge_activation(response_data[nmos::fields::activation], request_activation, request_time);
            // in the case of an immediate activation, the activation_time should be updated before the response is sent
            // so the /map/activations/ POST request handler uses details::handle_pending_immediate_activation

            return{ details::scheduled_activation_pending == request_state ? status_codes::Accepted : status_codes::OK, std::move(response_data) };
        }

        void serialize_channelmapping_io_resources_object(utility::ostringstream_t& os, const nmos::resources& resources, const nmos::type& type, slog::base_gate& gate)
        {
            size_t count = 0;

            web::json::serialize_object(os, resources
                | boost::adaptors::filtered([&](const nmos::resource& resource) { return type == resource.type; })
                | boost::adaptors::transformed(
                    [&count](const nmos::resource& resource) { ++count; return std::make_pair(nmos::fields::channelmapping_id(resource.data), std::cref(nmos::fields::endpoint_io(resource.data))); }
            ));

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << count << " matching " << nmos::resourceType_from_type(type);
        }
   }

   // Activate an IS-08 output by transitioning the 'staged' action into the active map
   void set_channelmapping_output_active(nmos::resource& channelmapping_output, const nmos::tai& activation_time)
   {
       using web::json::value;
       using web::json::value_of;

       auto& resource = channelmapping_output;
       const auto at = value::string(nmos::make_version(activation_time));

       auto& staged = nmos::fields::endpoint_staged(resource.data);
       auto& staged_activation = staged[nmos::fields::activation];
       const nmos::activation_mode staged_mode{ nmos::fields::mode(staged_activation).as_string() };

       // Set the time of activation (will be included in the POST response for an immediate activation)
       staged_activation[nmos::fields::activation_time] = at;

       auto& active = nmos::fields::endpoint_active(resource.data);

       // Apply the staged action
       web::json::merge_patch(active[nmos::fields::map], staged[nmos::fields::action]);
       active[nmos::fields::activation] = staged_activation;

       if (nmos::activation_modes::activate_scheduled_absolute == staged_mode ||
           nmos::activation_modes::activate_scheduled_relative == staged_mode)
       {
           set_channelmapping_output_not_pending(resource);
       }
       // nmos::details::handle_immediate_activation_pending finishes the transition of the staged parameters back to null
       // in the case of an immediate activation!
   }

   // Clear any pending activation of an IS-08 output
   // (This function should not be called after nmos::set_channelmapping_output_active.)
   void set_channelmapping_output_not_pending(nmos::resource& channelmapping_output)
   {
       auto& staged = nmos::fields::endpoint_staged(channelmapping_output.data);
       auto& staged_activation = staged[nmos::fields::activation];

       staged_activation = nmos::make_activation();
   }

   // Update the IS-04 source or device after the active map is changed in any way
   // (This function should be called after nmos::set_channelmapping_output_active.)
   void set_resource_version(nmos::resource& node_resource, const nmos::tai& activation_time)
   {
       using web::json::value;

       auto& resource = node_resource;
       const auto at = value::string(nmos::make_version(activation_time));

       resource.data[nmos::fields::version] = at;
   }

    web::http::experimental::listener::api_router make_unmounted_channelmapping_api(nmos::node_model& model, details::channelmapping_output_map_validator validate_merged, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router channelmapping_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is08_versions::from_settings(model.settings); });
        channelmapping_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        channelmapping_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("inputs/"), U("outputs/"), U("map/"), U("io/") }, req, res));
            return pplx::task_from_result(true);
        });

        // inputs and outputs endpoints

        channelmapping_api.support(U("/") + nmos::patterns::inputOutputType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.channelmapping_resources;

            const string_t inputOutputType = parameters.at(nmos::patterns::inputOutputType.name);

            const auto type = nmos::type_from_resourceType(inputOutputType);
            const auto match = [&](const nmos::resource& resource) { return resource.type == type; };

            size_t count = 0;

            // experimental extension, to support human-readable HTML rendering of NMOS responses
            if (experimental::details::is_html_response_preferred(req, web::http::details::mime_types::application_json))
            {
                set_reply(res, status_codes::OK,
                    web::json::serialize_array(resources
                        | boost::adaptors::filtered(match)
                        | boost::adaptors::transformed(
                            [&count, &req](const nmos::resource& resource) { ++count; return experimental::details::make_html_response_a_tag(nmos::fields::channelmapping_id(resource.data) + U("/"), req); }
                            )),
                    web::http::details::mime_types::application_json);
            }
            else
            {
                set_reply(res, status_codes::OK,
                    web::json::serialize_array(resources
                        | boost::adaptors::filtered(match)
                        | boost::adaptors::transformed(
                            [&count](const nmos::resource& resource) { ++count; return value(nmos::fields::channelmapping_id(resource.data) + U("/")); }
                            )
                        ),
                    web::http::details::mime_types::application_json);
            }

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << count << " matching " << inputOutputType;

            return pplx::task_from_result(true);
        });

        channelmapping_api.support(U("/") + nmos::patterns::inputOutputType.pattern + U("/") + nmos::patterns::inputOutputId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.channelmapping_resources;

            const string_t inputOutputType = parameters.at(nmos::patterns::inputOutputType.name);
            const string_t inputOutputId = parameters.at(nmos::patterns::inputOutputId.name);

            const auto type = nmos::type_from_resourceType(inputOutputType);
            auto resource = find_resource(resources, { nmos::make_channelmapping_resource_id({ inputOutputId, type }), type });
            if (resources.end() != resource)
            {
                if (nmos::types::input == type)
                {
                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("properties/"), U("parent/"), U("channels/"), U("caps/") }, req, res));
                }
                else // if (nmos::types::output == type)
                {
                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("properties/"), U("sourceid/"), U("channels/"), U("caps/") }, req, res));
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        const auto handle_input_output_subroute = [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.channelmapping_resources;

            const string_t inputOutputType = parameters.at(nmos::patterns::inputOutputType.name);
            const string_t inputOutputId = parameters.at(nmos::patterns::inputOutputId.name);

            const auto type = nmos::type_from_resourceType(inputOutputType);
            auto resource = find_resource(resources, { nmos::make_channelmapping_resource_id({ inputOutputId, type }), type });
            if (resources.end() != resource)
            {
                const auto& io = nmos::fields::endpoint_io(resource->data);
                if (nmos::types::input == type)
                {
                    const string_t& subroute = parameters.at(nmos::patterns::inputSubroute.name);
                    // subroute names are exactly the same as the /io endpoint property names
                    set_reply(res, status_codes::OK, io.at(subroute));
                }
                else // if (nmos::types::output == type)
                {
                    const string_t& subroute = parameters.at(nmos::patterns::outputSubroute.name);
                    // subroute names are exactly the same as the /io endpoint property names
                    // apart from /sourceid, which corresponds to "source_id" per the usual NMOS conventions
                    const string_t field = subroute == U("sourceid") ? U("source_id") : subroute;
                    set_reply(res, status_codes::OK, io.at(field));
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        };

        channelmapping_api.support(U("/") + nmos::patterns::inputType.pattern + U("/") + nmos::patterns::inputOutputId.pattern + U("/") + nmos::patterns::inputSubroute.pattern + U("/?"), methods::GET, handle_input_output_subroute);
        channelmapping_api.support(U("/") + nmos::patterns::outputType.pattern + U("/") + nmos::patterns::inputOutputId.pattern + U("/") + nmos::patterns::outputSubroute.pattern + U("/?"), methods::GET, handle_input_output_subroute);

        // io endpoint

        channelmapping_api.support(U("/io/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.channelmapping_resources;

            utility::ostringstream_t os;
            web::json::ostream_visitor visitor(os);

            visitor(web::json::enter_object_tag{});
            {
                visitor(web::json::enter_field_tag{});
                web::json::visit(visitor, value::string(U("inputs")));
                visitor(web::json::field_separator_tag{});
                details::serialize_channelmapping_io_resources_object(os, resources, nmos::types::input, gate);
                visitor(web::json::leave_field_tag{});
            }
            visitor(web::json::object_separator_tag{});
            {
                visitor(web::json::enter_field_tag{});
                web::json::visit(visitor, value::string(U("outputs")));
                visitor(web::json::field_separator_tag{});
                details::serialize_channelmapping_io_resources_object(os, resources, nmos::types::output, gate);
                visitor(web::json::leave_field_tag{});
            }
            visitor(web::json::leave_object_tag{});

            set_reply(res, status_codes::OK,
                os.str(),
                web::http::details::mime_types::application_json);

            return pplx::task_from_result(true);
        });

        // map endpoints

        channelmapping_api.support(U("/map/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("activations/"), U("active/") }, req, res));

            return pplx::task_from_result(true);
        });

        channelmapping_api.support(U("/map/active/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.channelmapping_resources;

            utility::ostringstream_t os;
            web::json::ostream_visitor visitor(os);

            auto most_recent_activation = nmos::make_activation();
            auto most_recent_activation_time = nmos::tai_min();

            visitor(web::json::enter_object_tag{});
            {
                visitor(web::json::enter_field_tag{});
                web::json::visit(visitor, value::string(nmos::fields::map));
                visitor(web::json::field_separator_tag{});
                {
                    // hm, how can we do something like nmos::experimental::details::make_html_response_a_tag to make the output ids 'clickable'?

                    web::json::serialize_object(os, resources
                        | boost::adaptors::filtered([&](const nmos::resource& resource) { return nmos::types::output == resource.type; })
                        | boost::adaptors::transformed(
                            [&](const nmos::resource& resource)
                            {
                                // check if this output's /active endpoint's activation is the most recent yet
                                const auto& activation = nmos::fields::activation(nmos::fields::endpoint_active(resource.data));
                                const auto& activation_time_or_null = nmos::fields::activation_time(activation);
                                if (!activation_time_or_null.is_null())
                                {
                                    const auto activation_time = nmos::parse_version(activation_time_or_null.as_string());
                                    if (activation_time > most_recent_activation_time)
                                    {
                                        most_recent_activation = activation;
                                        most_recent_activation_time = activation_time;
                                    }
                                }

                                return std::make_pair(nmos::fields::channelmapping_id(resource.data), std::cref(nmos::fields::map(nmos::fields::endpoint_active(resource.data))));
                            }
                    ));
                }
                visitor(web::json::leave_field_tag{});
            }
            visitor(web::json::object_separator_tag{});
            {
                visitor(web::json::enter_field_tag{});
                web::json::visit(visitor, value::string(nmos::fields::activation));
                visitor(web::json::field_separator_tag{});
                web::json::visit(visitor, most_recent_activation);
                visitor(web::json::leave_field_tag{});
            }
            visitor(web::json::leave_object_tag{});

            set_reply(res, status_codes::OK,
                os.str(),
                web::http::details::mime_types::application_json);

            return pplx::task_from_result(true);
        });

        channelmapping_api.support(U("/map/active/") + nmos::patterns::inputOutputId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.channelmapping_resources;

            const string_t outputId = parameters.at(nmos::patterns::inputOutputId.name);

            auto resource = find_resource(resources, { nmos::make_channelmapping_resource_id({ outputId, nmos::types::output }), nmos::types::output });
            if (resources.end() != resource)
            {
                const auto& active = nmos::fields::endpoint_active(resource->data);

                utility::ostringstream_t os;
                web::json::ostream_visitor visitor(os);

                visitor(web::json::enter_object_tag{});
                {
                    visitor(web::json::enter_field_tag{});
                    web::json::visit(visitor, value::string(nmos::fields::activation));
                    visitor(web::json::field_separator_tag{});
                    web::json::visit(visitor, nmos::fields::activation(active));
                    visitor(web::json::leave_field_tag{});
                }
                visitor(web::json::object_separator_tag{});
                {
                    visitor(web::json::enter_field_tag{});
                    web::json::visit(visitor, value::string(nmos::fields::map));
                    visitor(web::json::field_separator_tag{});
                    {
                        visitor(web::json::enter_object_tag{});
                        {
                            visitor(web::json::enter_field_tag{});
                            web::json::visit(visitor, value::string(nmos::fields::channelmapping_id(resource->data)));
                            visitor(web::json::field_separator_tag{});
                            web::json::visit(visitor, nmos::fields::map(active));
                            visitor(web::json::leave_field_tag{});
                        }
                        visitor(web::json::leave_object_tag{});
                    }
                    visitor(web::json::leave_field_tag{});
                }
                visitor(web::json::leave_object_tag{});

                set_reply(res, status_codes::OK,
                    os.str(),
                    web::http::details::mime_types::application_json);
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }
            return pplx::task_from_result(true);
        });

        channelmapping_api.support(U("/map/activations/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.channelmapping_resources;

            // for now at least, build a temporary object to return
            auto result = value::object();

            auto& by_type = resources.get<nmos::tags::type>();
            const auto outputs = by_type.equal_range(nmos::details::has_data(nmos::types::output));

            for (auto output = outputs.first; outputs.second != output; ++output)
            {
                const auto& staged = nmos::fields::endpoint_staged(output->data);
                const auto& activation = nmos::fields::activation(staged);

                if (details::scheduled_activation_pending != details::get_activation_state(activation)) continue;

                const auto& activation_id = nmos::fields::activation_id(staged);

                const auto& action = nmos::fields::action(staged);
                result[activation_id][nmos::fields::action][nmos::fields::channelmapping_id(output->data)] = action;

                // only needs doing once...
                result[activation_id][nmos::fields::activation] = activation;
            }

            set_reply(res, status_codes::OK, result);

            return pplx::task_from_result(true);
        });

        channelmapping_api.support(U("/map/activations/?"), methods::POST, [&model, validate_merged, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            return details::extract_json(req, gate).then([&model, req, res, parameters, validate_merged, gate](value data) mutable
            {
                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

                // Note that the activation identifiers used in the Channel Mapping API are not required to be universally unique
                // but this implementation chooses to use identifiers that are
                const auto activation_id = nmos::make_id();

                // cf. nmos::details::handle_connection_resource_patch

                auto lock = model.write_lock();
                const auto request_time = tai_now(); // during write lock to ensure uniqueness

                auto result = details::handle_channelmapping_activation_post(model, lock, version, activation_id, data, request_time, validate_merged, gate);

                if (web::http::is_success_status_code(result.first))
                {
                    // immediate activations need to be processed before sending the response
                    if (details::immediate_activation_pending == details::get_activation_state(nmos::fields::activation(data)))
                    {
                        // hmm, details::handle_immediate_activation_pending waits for the staged activation to have changed from the specified initial value, and then updates it
                        // hence the back and forth here...
                        web::json::value response_activation = result.second[nmos::fields::activation];

                        const auto& action = nmos::fields::action(data);
                        for (const auto& output : action.as_object())
                        {
                            response_activation = result.second[nmos::fields::activation];
                            details::handle_immediate_activation_pending(model, lock, { nmos::make_channelmapping_resource_id({ output.first, nmos::types::output }), nmos::types::output }, response_activation, gate);
                        }

                        result.second[nmos::fields::activation] = response_activation;
                    }

                    set_reply(res, result.first, value_of({ { activation_id, std::move(result.second) } }));
                }
                else
                {
                    // don't replace an existing response body which might contain richer error information
                    set_reply(res, result.first, !result.second.is_null() ? result.second : nmos::make_error_response_body(result.first));
                }

                return true;
            });
        });

        channelmapping_api.support(U("/map/activations/") + nmos::patterns::activationId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.channelmapping_resources;

            const string_t activationId = parameters.at(nmos::patterns::activationId.name);

            auto result = value::null();

            auto& by_type = resources.get<nmos::tags::type>();
            const auto outputs = by_type.equal_range(nmos::details::has_data(nmos::types::output));

            for (auto output = outputs.first; outputs.second != output; ++output)
            {
                const auto& staged = nmos::fields::endpoint_staged(output->data);
                const auto& activation = nmos::fields::activation(staged);

                if (details::scheduled_activation_pending != details::get_activation_state(activation)) continue;

                const auto& activation_id = nmos::fields::activation_id(staged);
                if (activationId != activation_id) continue;

                const auto& action = nmos::fields::action(staged);
                result[nmos::fields::action][nmos::fields::channelmapping_id(output->data)] = action;

                // only needs doing once...
                result[nmos::fields::activation] = activation;
            }

            if (!result.is_null())
            {
                set_reply(res, status_codes::OK, result);
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        channelmapping_api.support(U("/map/activations/") + nmos::patterns::activationId.pattern + U("/?"), methods::DEL, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.write_lock();
            auto& resources = model.channelmapping_resources;

            const string_t activationId = parameters.at(nmos::patterns::activationId.name);

            auto found = false;

            auto& by_type = resources.get<nmos::tags::type>();
            const auto outputs = by_type.equal_range(nmos::details::has_data(nmos::types::output));

            for (auto output = outputs.first; outputs.second != output; ++output)
            {
                const auto& staged = nmos::fields::endpoint_staged(output->data);
                const auto& activation = nmos::fields::activation(staged);

                if (details::scheduled_activation_pending != details::get_activation_state(activation)) continue;

                const auto& activation_id = nmos::fields::activation_id(staged);
                if (activationId != activation_id) continue;

                found = true;

                modify_resource(resources, output->id, [&](nmos::resource& resource)
                {
                    auto& staged = nmos::fields::endpoint_staged(resource.data);
                    staged[nmos::fields::activation] = nmos::make_activation();
                    staged.erase(nmos::fields::action);
                    staged.erase(nmos::fields::activation_id);
                });
            }

            if (found)
            {
                set_reply(res, status_codes::NoContent);
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        return channelmapping_api;
    }
}

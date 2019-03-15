#include "nmos/connection_api.h"

#include <boost/range/join.hpp>
#include "cpprest/http_utils.h"
#include "cpprest/json_validator.h"
#include "nmos/activation_mode.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/is04_versions.h"
#include "nmos/is05_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/sdp_utils.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h"
#include "nmos/version.h"
#include "sdp/sdp.h"

namespace nmos
{
    web::http::experimental::listener::api_router make_unmounted_connection_api(nmos::node_model& model, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router connection_api;

        connection_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, res));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/x-nmos/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("connection/") }, res));
            return pplx::task_from_result(true);
        });

        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is05_versions::from_settings(model.settings); });
        connection_api.support(U("/x-nmos/") + nmos::patterns::connection_api.pattern + U("/?"), methods::GET, [versions](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), res));
            return pplx::task_from_result(true);
        });

        connection_api.mount(U("/x-nmos/") + nmos::patterns::connection_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_connection_api(model, gate));

        return connection_api;
    }

    namespace details
    {
        static const web::json::experimental::json_validator& staged_core_validator()
        {
            // hmm, could be based on supported API versions from settings, like other APIs' validators?
            static const web::json::experimental::json_validator validator
            {
                nmos::experimental::load_json_schema,
                boost::copy_range<std::vector<web::uri>>(boost::range::join(
                    is05_versions::all | boost::adaptors::transformed(boost::bind(experimental::make_connectionapi_staged_patch_request_schema_uri, _1, nmos::types::sender)),
                    is05_versions::all | boost::adaptors::transformed(boost::bind(experimental::make_connectionapi_staged_patch_request_schema_uri, _1, nmos::types::receiver))
                ))
            };
            return validator;
        }

        // Validate against specification schema
        // throws web::json::json_exception on failure, which results in a 400 Bad Request
        void validate_staged_core(const nmos::api_version& version, const nmos::type& type, const web::json::value& staged)
        {
            // Validate JSON syntax according to the schema

            staged_core_validator().validate(staged, experimental::make_connectionapi_staged_patch_request_schema_uri(version, type));
        }

        // Extend an existing schema with "auto" as a valid value
        web::json::value make_auto_schema(const web::json::value& schema)
        {
            using web::json::value_of;

            const bool keep_order = true;

            return value_of({
                { U("anyOf"), value_of({
                    value_of({
                        { U("type"), U("string") },
                        { U("pattern"), U("^auto$") }
                    }, keep_order),
                    schema
                }) }
            });
        }

        static const std::map<nmos::type, std::set<utility::string_t>>& auto_constraints()
        {
            // These are the constraints that support "auto" in /staged; cf. resolve_auto
            // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0_sender_transport_params_rtp.json
            // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0_receiver_transport_params_rtp.json
            static const std::map<nmos::type, std::set<utility::string_t>> auto_constraints
            {
                {
                    nmos::types::sender,
                    {
                        nmos::fields::source_ip,
                        nmos::fields::destination_ip,
                        nmos::fields::source_port,
                        nmos::fields::destination_port,
                        nmos::fields::fec_destination_ip,
                        nmos::fields::fec1D_destination_port,
                        nmos::fields::fec2D_destination_port,
                        nmos::fields::fec1D_source_port,
                        nmos::fields::fec2D_source_port,
                        nmos::fields::rtcp_destination_ip,
                        nmos::fields::rtcp_destination_port,
                        nmos::fields::rtcp_source_port
                    }
                },
                {
                    nmos::types::receiver,
                    {
                        nmos::fields::interface_ip,
                        nmos::fields::destination_port,
                        nmos::fields::fec_destination_ip,
                        nmos::fields::fec_mode,
                        nmos::fields::fec1D_destination_port,
                        nmos::fields::fec2D_destination_port,
                        nmos::fields::rtcp_destination_ip,
                        nmos::fields::rtcp_destination_port
                    }
                }
            };
            return auto_constraints;
        }

        // Make a json schema from /constraints
        web::json::value make_constraints_schema(const nmos::type& type, const web::json::value& constraints)
        {
            using web::json::value;
            using web::json::value_of;

            const bool keep_order = true;

            auto items = value::array();

            auto& type_auto_constraints = auto_constraints().at(type);

            for (const auto& leg : constraints.as_array())
            {
                auto properties = value::object();

                for (const auto& constraint : leg.as_object())
                {
                    if (type_auto_constraints.end() != type_auto_constraints.find(constraint.first))
                    {
                        properties[constraint.first] = make_auto_schema(constraint.second);
                    }
                    else
                    {
                        properties[constraint.first] = constraint.second;
                    }
                }

                web::json::push_back(items, value_of({
                    { U("type"), U("object") },
                    { U("properties"), properties }
                }, keep_order));
            }

            return value_of({
                { U("$schema"), U("http://json-schema.org/draft-04/schema#") },
                { U("type"), U("object") },
                { U("properties"), value_of({
                    { U("transport_params"),  value_of({
                        { U("type"), U("array") },
                        { U("items"), items }
                    }, keep_order) }
                }, keep_order) }
            }, keep_order);
        }

        // Validate staged endpoint against constraints
        void validate_staged_constraints(const nmos::type& type, const web::json::value& constraints, const web::json::value& staged)
        {
            const auto schema = make_constraints_schema(type, constraints);
            const auto uri = web::uri{U("/constraints")};

            // Validate staged JSON syntax according to the schema

            const web::json::experimental::json_validator validator
            {
                [&](const web::uri& ) { return schema; },
                { uri }
            };

            // Validate JSON syntax according to the schema

            validator.validate(staged, uri);
        }

        enum activation_state { immediate_activation_pending, scheduled_activation_pending, activation_not_pending, staging_only };

        // Discover which kind of activation this is, or whether it is only a request for staging
        activation_state get_activation_state(const web::json::value& activation)
        {
            if (activation.is_null()) return staging_only;

            const auto& mode_or_null = nmos::fields::mode(activation);
            if (mode_or_null.is_null()) return activation_not_pending;

            const auto mode = nmos::activation_mode{ mode_or_null.as_string() };

            if (nmos::activation_modes::activate_scheduled_absolute == mode || nmos::activation_modes::activate_scheduled_relative == mode)
            {
                return scheduled_activation_pending;
            }
            else if (nmos::activation_modes::activate_immediate == mode)
            {
                return immediate_activation_pending;
            }
            else
            {
                throw web::json::json_exception(U("invalid activation mode"));
            }
        }

        // Calculate the absolute TAI from the requested time of a scheduled activation
        nmos::tai get_absolute_requested_time(const web::json::value& activation, const nmos::tai& request_time)
        {
            const auto& mode_or_null = nmos::fields::mode(activation);
            const auto& requested_time_or_null = nmos::fields::requested_time(activation);

            const nmos::activation_mode mode{ mode_or_null.as_string() };

            if (nmos::activation_modes::activate_scheduled_absolute == mode)
            {
                return web::json::as<nmos::tai>(requested_time_or_null);
            }
            else if (nmos::activation_modes::activate_scheduled_relative == mode)
            {
                return tai_from_time_point(time_point_from_tai(request_time) + duration_from_tai(web::json::as<nmos::tai>(requested_time_or_null)));
            }
            else
            {
                throw std::logic_error("cannot get absolute requested time for mode: " + utility::us2s(mode.name));
            }
        }

        // Set the appropriate fields of the response/staged activation from the specified patch
        void merge_activation(web::json::value& activation, const web::json::value& patch_activation, const nmos::tai& request_time)
        {
            using web::json::value;

            switch (details::get_activation_state(patch_activation))
            {
            case details::staging_only:
                // All three merged values should be null (already)

                activation[nmos::fields::mode] = value::null();
                activation[nmos::fields::requested_time] = value::null();

                // "If no activation was requested in the PATCH `activation_time` will be set `null`."
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml
                activation[nmos::fields::activation_time] = value::null();

                break;
            case details::activation_not_pending:
                // Merged "mode" should be null (already)

                activation[nmos::fields::mode] = value::null();

                // It doesn't make sense for either of the TAI timestamps to be set when the mode is null
                // See https://github.com/AMWA-TV/nmos-device-connection-management/issues/30
                activation[nmos::fields::requested_time] = value::null();
                activation[nmos::fields::activation_time] = value::null();

                break;
            case details::immediate_activation_pending:
                // Merged "mode" should be "activate_immediate", and "requested_time" should be null (already)

                // "For immediate activations, in the response to the PATCH request this field
                // will be set to 'activate_immediate'"
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-activation-response-schema.json
                activation[nmos::fields::mode] = value::string(nmos::activation_modes::activate_immediate.name);

                // "For an immediate activation this field will always be null on the staged endpoint,
                // even in the response to the PATCH request."
                // However, here it is set to indicate an in-flight immediate activation
                activation[nmos::fields::requested_time] = value::string(nmos::make_version(request_time));

                // "For immediate activations on the staged endpoint this property will be the time the activation actually
                // occurred in the response to the PATCH request, but null in response to any GET requests thereafter."
                // Therefore, this value will be set later
                activation[nmos::fields::activation_time] = value::null();

                break;
            case details::scheduled_activation_pending:
                // Merged "mode" and "requested_time" should be set (already)

                activation[nmos::fields::mode] = patch_activation.at(nmos::fields::mode);
                activation[nmos::fields::requested_time] = patch_activation.at(nmos::fields::requested_time);

                // "For scheduled activations `activation_time` should be the absolute TAI time the parameters will actually transition."
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml
                auto absolute_requested_time = get_absolute_requested_time(activation, request_time);
                activation[nmos::fields::activation_time] = value::string(nmos::make_version(absolute_requested_time));

                break;
            }
        }

        // Apparently, the response to errors in the transport file should be 500 Internal Error rather than 400 Bad Request
        // See https://github.com/AMWA-TV/nmos-device-connection-management/issues/40
        inline std::logic_error transport_file_error(const std::string& message)
        {
            return std::logic_error("Transport file error - " + message);
        }

        std::pair<utility::string_t, utility::string_t> get_transport_type_data(const web::json::value& transport_file)
        {
            // "Why would you want to set the transport file without also setting it's type? I mean, IS-05 doesn't
            // strictly prohibit it but I don't imagine you can expect the receiver to do anything useful if you do."
            // See https://github.com/AMWA-TV/nmos-device-connection-management/issues/47
            if (!transport_file.has_field(nmos::fields::data)) throw transport_file_error("data is required");

            auto& transport_data = transport_file.at(nmos::fields::data);
            if (!transport_data.is_null())
            {
                if (!transport_file.has_field(nmos::fields::type)) throw transport_file_error("type is required");

                auto& transport_type = transport_file.at(nmos::fields::type);
                if (transport_type.is_null() || transport_type.as_string().empty()) throw transport_file_error("type is required");

                return{ transport_type.as_string(), transport_data.as_string() };
            }
            else
            {
                // Check for unexpected transport file type? In the future, could be a range of supported types...
                // Or only allow the type to be omitted? Or null?
                return{};
            }
        }

        // wait for the staged activation mode of the specified resource to be set to something other than "activate_immediate"
        // or for the resource to have vanished (unexpected!)
        // or for the server to be shut down
        // or for the timeout to expire
        template <class ReadOrWriteLock>
        bool wait_immediate_activation_not_pending(nmos::node_model& model, ReadOrWriteLock& lock, std::pair<nmos::id, nmos::type> id_type)
        {
            return model.wait_for(lock, std::chrono::seconds(nmos::fields::immediate_activation_max(model.settings)), [&]
            {
                if (model.shutdown) return true;

                auto resource = find_resource(model.connection_resources, id_type);
                if (model.connection_resources.end() == resource) return true;

                auto& mode = nmos::fields::mode(nmos::fields::activation(nmos::fields::endpoint_staged(resource->data)));
                return mode.is_null() || mode.as_string() != nmos::activation_modes::activate_immediate.name;
            });
        }

        // wait for the staged activation of the specified resource to have changed
        // or for the resource to have vanished (unexpected!)
        // or for the server to be shut down
        // or for the timeout to expire
        template <class ReadOrWriteLock>
        bool wait_activation_modified(nmos::node_model& model, ReadOrWriteLock& lock, std::pair<nmos::id, nmos::type> id_type, web::json::value initial_activation)
        {
            return model.wait_for(lock, std::chrono::seconds(nmos::fields::immediate_activation_max(model.settings)), [&]
            {
                if (model.shutdown) return true;

                auto resource = find_resource(model.connection_resources, id_type);
                if (model.connection_resources.end() == resource) return true;

                auto& activation = nmos::fields::activation(nmos::fields::endpoint_staged(resource->data));
                return activation != initial_activation;
            });
        }

        void handle_immediate_activation_pending(nmos::node_model& model, nmos::write_lock& lock, const std::pair<nmos::id, nmos::type>& id_type, web::json::value& response_activation, slog::base_gate& gate)
        {
            using web::json::value;

            // lock.owns_lock() must be true initially; waiting releases and reacquires the lock
            if (!wait_activation_modified(model, lock, id_type, response_activation) || model.shutdown)
            {
                throw std::logic_error("timed out waiting for in-flight immediate activation to complete");
            }

            auto& resources = model.connection_resources;

            // after releasing and reacquiring the lock, must find the resource again!
            const auto found = find_resource(resources, id_type);
            if (resources.end() == found)
            {
                // since this has happened while the activation was in flight, the response is 500 Internal Error rather than 404 Not Found
                throw std::logic_error("resource vanished during in-flight immediate activation");
            }
            else
            {
                auto& staged = nmos::fields::endpoint_staged(found->data);
                auto& staged_activation = staged.at(nmos::fields::activation);

                // use requested time to identify it's still the same in-flight immediate activation
                if (staged_activation.at(nmos::fields::requested_time) != response_activation.at(nmos::fields::requested_time))
                {
                    throw std::logic_error("activation modified during in-flight immediate activation");
                }
            }

            modify_resource(resources, id_type.first, [&response_activation](nmos::resource& resource)
            {
                auto& staged = nmos::fields::endpoint_staged(resource.data);
                auto& staged_activation = staged[nmos::fields::activation];

                resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());

                // "For immediate activations, [the `mode` field] will be null in response to any subsequent GET requests."
                staged_activation[nmos::fields::mode] = value::null();

                // "For an immediate activation this field will always be null on the staged endpoint,
                // even in the response to the PATCH request."
                response_activation[nmos::fields::requested_time] = value::null();
                staged_activation[nmos::fields::requested_time] = value::null();

                // "For immediate activations on the staged endpoint this property will be the time the activation actually
                // occurred in the response to the PATCH request, but null in response to any GET requests thereafter."
                response_activation[nmos::fields::activation_time] = staged_activation[nmos::fields::activation_time];
                staged_activation[nmos::fields::activation_time] = value::null();
            });

            // this is especially important to unblock other patch requests in details::wait_immediate_activation_not_pending
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying connection API - immediate activation completed";
            model.notify();
        }

        typedef std::pair<web::http::status_code, web::json::value> connection_resource_patch_response;

        inline connection_resource_patch_response make_connection_resource_patch_error_response(web::http::status_code code, const utility::string_t& error = {}, const utility::string_t& debug = {})
        {
            return{ code, make_error_response_body(code, error, debug) };
        }

        inline connection_resource_patch_response make_connection_resource_patch_error_response(web::http::status_code code, const std::exception& debug)
        {
            return make_connection_resource_patch_error_response(code,{}, utility::s2us(debug.what()));
        }

        // Basic theory of implementation of PATCH /staged
        //
        // 1. Reject any patch, other than cancellation, when a scheduled activation is outstanding.
        // 2. Validate the patch, apply it and notify anyone who cares via model condition variable, e.g. a scheduling thread.
        // 3. Then return the response.
        //
        // That would work, except for those pesky immediate activations...
        //
        // Actual implementation of PATCH /staged
        //
        // Since immediate activations may take some time, we want to release the model lock so we can e.g. accept patch requests
        // on *other* senders or receivers.
        // But if we release the model lock at any point during an in-flight immediate activation, then we also have to handle
        // patch requests on the *same* sender or receiver arriving while this immediate activation is in flight.
        // And (according to the spec, probably) we're supposed to block rather than reject them...
        //
        // Similarly we also have to handle get requests while the immediate activation is in flight. Here there are two choices,
        // we need to either return the previous staged parameters or block until the immediate activation is completed.
        // Might as well do the latter, having necessarily implemented it for patch...
        //
        // To achieve this, we need to write something into the model to be able to identify when an immediate activation is in
        // flight, and when it's completed, i.e. a kind of 'per-resource lock'.
        //
        // We could choose a number of different ways of storing the in-flight activation 'lock' and data in the model, but what
        // I've done is to use the staged endpoint like this:
        //
        // * Activation mode is set to "activate_immediate". This is the per-resource 'lock' which allows concurrent patch and
        //   get requests to be blocked (and therefore this in-flight state never to be returned).
        // * Activation requested_time is set to current timestamp. This uniquely identifies this immediate activation (in case
        //   some other thread decides to trample all over our in-flight activation).
        // * Activation activation_time is set to null. This indicates this activation is not complete.
        //
        // Then notify the scheduling thread, release the model lock and wait for the immediate activation to be done.
        //
        // If it's successful, the activation_time should be set (e.g. by nmos::set_connection_resource_active) and will be used
        // in the response.
        // If it fails, the requested_time should be changed (e.g. set to null) so that an error response is sent; the mode must
        // also be set back to null to unblock concurrent patch operations (nmos::set_connection_resource_not_pending does both).
        //
        // This obviously requires co-operation from other threads that are manipulating these connection resources.
        // nmos::experimental::node_resources_thread currently serves as an example of how to handle sender/receiver activations.
        //
        // By the time we reacquire the model lock anything may have happened, but we can identify with the above whether to send
        // a success response or an error, and in the success case, release the 'per-resource lock' by updating the staged
        // activation mode, requested_time and activation_time.
        connection_resource_patch_response handle_connection_resource_patch(nmos::node_model& model, nmos::write_lock& lock, const nmos::api_version& version, const std::pair<nmos::id, nmos::type>& id_type, const web::json::value& patch, const nmos::tai& request_time, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            // lock.owns_lock() must be true initially
            auto& resources = model.connection_resources;

            // Validate JSON syntax according to the schema
            details::validate_staged_core(version, id_type.second, patch);

            const auto patch_state = details::get_activation_state(nmos::fields::activation(patch));

            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                // Prevent changes to already-scheduled activations and in-flight immediate activations

                const auto& staged_activation = nmos::fields::activation(nmos::fields::endpoint_staged(resource->data));
                const auto staged_state = details::get_activation_state(staged_activation);

                if (details::scheduled_activation_pending == staged_state && details::activation_not_pending != patch_state)
                {
                    // "When the resource is locked because an activation has been scheduled[, 423 Locked is returned].
                    // A resource may be unlocked by setting `mode` in `activation` to `null`."

                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Rejecting PATCH request for " << id_type << " due to a pending scheduled activation";

                    return details::make_connection_resource_patch_error_response(status_codes::Locked);
                }
                else if (details::immediate_activation_pending == staged_state)
                {
                    const auto& requested_time_or_null = nmos::fields::requested_time(staged_activation);

                    if (!requested_time_or_null.is_null() && request_time == web::json::as<nmos::tai>(requested_time_or_null))
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Rejecting PATCH request for " << id_type << " due to a pending immediate activation from the same bulk request";

                        return details::make_connection_resource_patch_error_response(status_codes::BadRequest);
                    }
                    // See https://github.com/AMWA-TV/nmos-device-connection-management/issues/49
                    else if (!details::wait_immediate_activation_not_pending(model, lock, id_type) || model.shutdown)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Rejecting PATCH request for " << id_type << " due to a pending immediate activation";

                        return details::make_connection_resource_patch_error_response(status_codes::InternalError); // or ServiceUnavailable? probably not NotFound even if that's true after the timeout?
                    }
                    // find resource again just in case, since waiting releases and reacquires the lock
                    resource = find_resource(resources, id_type);
                }
            }

            if (resources.end() != resource)
            {
                // Merge this patch request into a *copy* of the current staged endpoint
                // so that the merged parameters can be validated against the constraints
                // before the current values are overwritten.
                // On successful staging/activation, this copy will also be used for the response.

                auto merged = nmos::fields::endpoint_staged(resource->data);

                // "In the case where the transport file and transport parameters are updated in the same PATCH request
                // transport parameters specified in the request object take precedence over those in the transport file."
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L369
                // "In all other cases the most recently received PATCH request takes priority."
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#interpretation-of-sdp-files

                // First, validate and merge the transport file (this resource must be a receiver)
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L344-L363

                auto& transport_file = nmos::fields::transport_file(patch);
                if (!transport_file.is_null() && !transport_file.as_object().empty())
                {
                    const auto transport_type_data = details::get_transport_type_data(transport_file);

                    if (!transport_type_data.first.empty())
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Processing transport file";

                        if (transport_type_data.first != U("application/sdp"))
                        {
                            throw transport_file_error("unexpected type: " + utility::us2s(transport_type_data.first));
                        }

                        try
                        {
                            const auto session_description = sdp::parse_session_description(utility::us2s(transport_type_data.second));
                            auto sdp_transport_params = nmos::parse_session_description(session_description);

                            // Validate transport file according to the IS-04 receiver

                            auto receiver = find_resource(model.node_resources, id_type);
                            if (model.node_resources.end() != receiver)
                            {
                                validate_sdp_parameters(receiver->data, sdp_transport_params.first);
                            }

                            // Merge the transport file into the transport parameters

                            auto& transport_params = nmos::fields::transport_params(merged);

                            if (1 == transport_params.size() && 2 == sdp_transport_params.second.size())
                            {
                                web::json::pop_back(sdp_transport_params.second);
                            }

                            // "Where a Receiver supports SMPTE 2022-7 but is required to Receive a non-SMPTE 2022-7 stream,
                            // only the first set of transport parameters should be used. rtp_enabled in the second set of parameters
                            // must be set to false"
                            // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#operation-with-smpte-2022-7
                            if (2 == transport_params.size() && 1 == sdp_transport_params.second.size())
                            {
                                web::json::push_back(sdp_transport_params.second, web::json::value_of({ { U("rtp_enabled"), false } }));
                            }

                            web::json::merge_patch(transport_params, sdp_transport_params.second);
                        }
                        catch (const web::json::json_exception& e)
                        {
                            throw transport_file_error(e.what());
                        }
                        catch (const std::runtime_error& e)
                        {
                            throw transport_file_error(e.what());
                        }
                    }
                }

                // Second, merge the transport parameters (in fact, all fields, including "sender_id", "master_enabled", etc.)

                web::json::merge_patch(merged, patch);

                // Then, prepare the activation response

                details::merge_activation(merged[nmos::fields::activation], nmos::fields::activation(patch), request_time);

                // Validate merged JSON according to the constraints

                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating staged transport parameters against constraints";

                details::validate_staged_constraints(resource->type, nmos::fields::endpoint_constraints(resource->data), merged);

                // Finally, update the staged endpoint

                modify_resource(resources, id_type.first, [&patch_state, &merged](nmos::resource& resource)
                {
                    resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());

                    nmos::fields::endpoint_staged(resource.data) = merged;
                    // In the case of an immediate activation, this is not yet a valid response
                    // to a 'subsequent' GET request on the staged endpoint; that should be dealt with
                    // by calling details::handle_immediate_activation_pending
                });

                if (details::staging_only == patch_state)
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Staged endpoint updated for " << id_type;
                else if (details::activation_not_pending == patch_state)
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Scheduled activation cancelled for " << id_type;
                else if (details::scheduled_activation_pending == patch_state)
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Scheduled activation requested for " << id_type;
                else if (details::immediate_activation_pending == patch_state)
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Immediate activation requested for " << id_type;

                // details::notify_connection_resource_patch also needs to be called!

                return{ details::scheduled_activation_pending == patch_state ? status_codes::Accepted : status_codes::OK, merged };
            }
            else
            {
                return details::make_connection_resource_patch_error_response(status_codes::NotFound);
            }
        }

        void notify_connection_resource_patch(const nmos::node_model& model, slog::base_gate& gate)
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying connection thread";

            model.notify();
        }

        void handle_connection_resource_patch(web::http::http_response res, nmos::node_model& model, const nmos::api_version& version, const std::pair<nmos::id, nmos::type>& id_type, const web::json::value& patch, slog::base_gate& gate)
        {
            auto lock = model.write_lock();
            const auto request_time = tai_now(); // during write lock to ensure uniqueness

            auto result = handle_connection_resource_patch(model, lock, version, id_type, patch, request_time, gate);

            if (web::http::is_success_status_code(result.first))
            {
                notify_connection_resource_patch(model, gate);

                // immediate activations need to be processed before sending the response
                if (details::immediate_activation_pending == details::get_activation_state(nmos::fields::activation(patch)))
                {
                    details::handle_immediate_activation_pending(model, lock, id_type, result.second[nmos::fields::activation], gate);
                }

                set_reply(res, result.first, result.second);
            }
            else
            {
                // don't replace an existing response body which might contain richer error information
                set_reply(res, result.first, !result.second.is_null() ? result.second : nmos::make_error_response_body(result.first));
            }
        }
    }

    // Activate an IS-05 sender or receiver by transitioning the 'staged' settings into the 'active' resource
    void set_connection_resource_active(nmos::resource& connection_resource, std::function<void(web::json::value&)> resolve_auto, const nmos::tai& activation_time)
    {
        using web::json::value;
        using web::json::value_of;

        auto& resource = connection_resource;
        const auto at = value::string(nmos::make_version(activation_time));

        resource.data[nmos::fields::version] = at;

        auto& staged = nmos::fields::endpoint_staged(resource.data);
        auto& staged_activation = staged[nmos::fields::activation];
        const nmos::activation_mode staged_mode{ nmos::fields::mode(staged_activation).as_string() };

        // Set the time of activation (will be included in the PATCH response for an immediate activation)
        staged_activation[nmos::fields::activation_time] = at;

        // "When a set of 'staged' settings is activated, these settings transition into the 'active' resource."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/1.0.%20Overview.md#active
        resource.data[nmos::fields::endpoint_active] = resource.data[nmos::fields::endpoint_staged];

        // "On activation all instances of "auto" should be resolved into the actual values that will be used"
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L257
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
        resolve_auto(resource.data[nmos::fields::endpoint_active]);

        // Unclear whether the activation in the active endpoint should have values for mode, requested_time
        // (and even activation_time?) or whether they should be null? The examples have them with values.
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/examples/v1.0-receiver-active-get-200.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/examples/v1.0-sender-active-get-200.json

        if (nmos::activation_modes::activate_scheduled_absolute == staged_mode ||
            nmos::activation_modes::activate_scheduled_relative == staged_mode)
        {
            set_connection_resource_not_pending(resource);
        }
        // nmos::details::handle_immediate_activation_pending finishes the transition of the staged parameters back to null
        // in the case of an immediate activation, as explained above nmos::details::handle_connection_resource_patch!
    }

    // Clear any pending activation of an IS-05 sender or receiver
    // (This function should not be called after nmos::set_connection_resource_active.)
    void set_connection_resource_not_pending(nmos::resource& connection_resource)
    {
        using web::json::value;
        using web::json::value_of;

        auto& resource = connection_resource;

        auto& staged = nmos::fields::endpoint_staged(resource.data);
        auto& staged_activation = staged[nmos::fields::activation];

        // "This parameter returns to null on the staged endpoint once an activation is completed."
        // "This field returns to null once the activation is completed on the staged endpoint."
        // "On the staged endpoint this field returns to null once the activation is completed."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-activation-response-schema.json

        // "A resource may be unlocked by setting `mode` in `activation` to `null`, which will cancel the pending activation."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L244

        staged_activation = value_of({
            { nmos::fields::mode, value::null() },
            { nmos::fields::requested_time, value::null() },
            { nmos::fields::activation_time, value::null() }
        });
    }

    // Update the IS-04 sender or receiver after the active connection is changed in any way
    // (This function should be called after nmos::set_connection_resource_active.)
    void set_resource_subscription(nmos::resource& node_resource, bool active, const nmos::id& connected_id, const nmos::tai& activation_time)
    {
        using web::json::value;
        using web::json::value_of;

        auto& resource = node_resource;
        const auto at = value::string(nmos::make_version(activation_time));
        // "It isn't completely clear how a Sender/Receiver's subscribed ID should be set when the Sender or Receiver is in a
        // 'parked' or unsubscribed state. Whilst the 'active' flag is the authoritative reference for this in v1.2+, in order
        // to maintain backwards compatibility for v1.1/v1.0 the Receiver's subscription sender_id needs to be set to 'null'
        // when it is not subscribed to anything."
        // Therefore consider active as well as connected_id
        // See https://github.com/AMWA-TV/nmos-discovery-registration/issues/76
        const auto ci = active && !connected_id.empty() ? value::string(connected_id) : value::null();

        // "When the 'active' parameters of a Sender or Receiver are modified, or when a re-activation of the same parameters
        // is performed, the 'version' attribute of the relevant IS-04 Sender or Receiver must be incremented."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/3.1.%20Interoperability%20-%20NMOS%20IS-04.md#version-increments
        resource.data[nmos::fields::version] = at;

        // Senders indicate the connected receiver_id, receivers indicate the connected sender_id
        // (depending on the API version)
        if (nmos::is04_versions::v1_2 <= resource.version)
        {
            nmos::fields::subscription(resource.data) = value_of({
                { nmos::fields::active, active },
                { nmos::types::sender == resource.type ? nmos::fields::receiver_id : nmos::fields::sender_id, ci }
            });
        }
        else if (nmos::types::receiver == resource.type)
        {
            nmos::fields::subscription(resource.data) = value_of({
                { nmos::fields::sender_id, ci }
            });
        }
    }

    // "On activation all instances of "auto" should be resolved into the actual values that will be used"
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L257
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0_sender_transport_params_rtp.json
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0_receiver_transport_params_rtp.json
    // "In some cases the behaviour is more complex, and may be determined by the vendor."
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
    // This function therefore does not select a value for e.g. sender "source_ip" or receiver "interface_ip".
    void resolve_auto(const nmos::type& type, web::json::value& transport_params, int auto_rtp_port)
    {
        if (nmos::types::sender == type)
        {
            for (auto& params : transport_params.as_array())
            {
                // nmos::fields::source_ip
                // nmos::fields::destination_ip
                details::resolve_auto(params, nmos::fields::source_port, [&] { return auto_rtp_port; });
                details::resolve_auto(params, nmos::fields::destination_port, [&] { return auto_rtp_port; });
                details::resolve_auto(params, nmos::fields::fec_destination_ip, [&] { return params[nmos::fields::destination_ip]; });
                details::resolve_auto(params, nmos::fields::fec1D_destination_port, [&] { return params[nmos::fields::destination_port].as_integer() + 2; });
                details::resolve_auto(params, nmos::fields::fec2D_destination_port, [&] { return params[nmos::fields::destination_port].as_integer() + 4; });
                details::resolve_auto(params, nmos::fields::fec1D_source_port, [&] { return params[nmos::fields::source_port].as_integer() + 2; });
                details::resolve_auto(params, nmos::fields::fec2D_source_port, [&] { return params[nmos::fields::source_port].as_integer() + 4; });
                details::resolve_auto(params, nmos::fields::rtcp_destination_ip, [&] { return params[nmos::fields::destination_ip]; });
                details::resolve_auto(params, nmos::fields::rtcp_destination_port, [&] { return params[nmos::fields::destination_port].as_integer() + 1; });
                details::resolve_auto(params, nmos::fields::rtcp_source_port, [&] { return params[nmos::fields::source_port].as_integer() + 1; });
            }
        }
        else if (nmos::types::receiver == type)
        {
            for (auto& params : transport_params.as_array())
            {
                // nmos::fields::interface_ip
                details::resolve_auto(params, nmos::fields::destination_port, [&] { return auto_rtp_port; });
                details::resolve_auto(params, nmos::fields::fec_destination_ip, [&] { return !nmos::fields::multicast_ip(params).is_null() ? params[nmos::fields::multicast_ip] : params[nmos::fields::interface_ip]; });
                // nmos::fields::fec_mode
                details::resolve_auto(params, nmos::fields::fec1D_destination_port, [&] { return params[nmos::fields::destination_port].as_integer() + 2; });
                details::resolve_auto(params, nmos::fields::fec2D_destination_port, [&] { return params[nmos::fields::destination_port].as_integer() + 4; });
                details::resolve_auto(params, nmos::fields::rtcp_destination_ip, [&] { return !nmos::fields::multicast_ip(params).is_null() ? params[nmos::fields::multicast_ip] : params[nmos::fields::interface_ip]; });
                details::resolve_auto(params, nmos::fields::rtcp_destination_port, [&] { return params[nmos::fields::destination_port].as_integer() + 1; });
            }
        }
    }

    web::http::experimental::listener::api_router make_unmounted_connection_api(nmos::node_model& model, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router connection_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is05_versions::from_settings(model.settings); });
        connection_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        connection_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("bulk/"), U("single/") }, res));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/bulk/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("senders/"), U("receivers/") }, res));
            return pplx::task_from_result(true);
        });

        // "The API should actively return an HTTP 405 if a GET is called on the [/bulk/senders and /bulk/receivers] endpoint[s]."
        // This is provided for free by the api_router which also identifies the handler for POST as a "near miss"
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L39-L44
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L73-L78

        connection_api.support(U("/bulk/") + nmos::patterns::connectorType.pattern + U("/?"), methods::POST, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            return details::extract_json(req, gate).then([&model, req, res, parameters, gate](value body) mutable
            {
                auto lock = model.write_lock();
                const auto request_time = tai_now(); // during write lock to ensure uniqueness

                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);

                auto& patches = body.as_array();

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Bulk operation requested for " << patches.size() << " " << resourceType;

                std::vector<details::connection_resource_patch_response> results;
                results.reserve(patches.size());

                // "Where a server implementation supports concurrent application of settings changes to
                // underlying Senders and Receivers, it may choose to perform 'bulk' resource operations
                // in a parallel fashion internally. This is an implementation decision and is not a
                // requirement of this specification."
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.0.%20Behaviour.md

                const auto type = nmos::type_from_resourceType(resourceType);

                const auto handle_connection_resource_exception = [&](const nmos::id& id)
                {
                    // try-catch based on the exception handler in nmos::add_api_finally_handler
                    try
                    {
                        throw;
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "JSON error for " << id << " in bulk request: " << e.what();
                        return details::make_connection_resource_patch_error_response(status_codes::BadRequest, e);
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "HTTP error for " << id << " in bulk request: " << e.what() << " [" << e.error_code() << "]";
                        return details::make_connection_resource_patch_error_response(status_codes::BadRequest, e);
                    }
                    catch (const std::runtime_error& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error for " << id << " in bulk request: " << e.what();
                        return details::make_connection_resource_patch_error_response(status_codes::NotImplemented, e);
                    }
                    catch (const std::logic_error& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error for " << id << " in bulk request: " << e.what();
                        return details::make_connection_resource_patch_error_response(status_codes::InternalError, e);
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception for " << id << " in bulk request: " << e.what();
                        return details::make_connection_resource_patch_error_response(status_codes::InternalError, e);
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception for " << id << " in bulk request";
                        return details::make_connection_resource_patch_error_response(status_codes::InternalError);
                    }
                };

                for (auto& patch : patches)
                {
                    const auto id = nmos::fields::id(patch);

                    details::connection_resource_patch_response result;

                    try
                    {
                        result = details::handle_connection_resource_patch(model, lock, version, { id, type }, patch[nmos::fields::params], request_time, gate);
                    }
                    catch (...)
                    {
                        result = handle_connection_resource_exception(id);
                    }

                    results.push_back(result);
                }

                if (0 != patches.size()) details::notify_connection_resource_patch(model, gate);

                auto rit = results.begin();
                for (auto pit = patches.begin(); patches.end() != pit; ++pit, ++rit)
                {
                    auto& patch = *pit;
                    auto& result = *rit;

                    const auto id = nmos::fields::id(patch);

                    if (web::http::is_success_status_code(result.first))
                    {
                        auto& response_activation = result.second[nmos::fields::activation];

                        // only pending immediate activations need to be processed before sending the response
                        if (details::immediate_activation_pending == details::get_activation_state(response_activation))
                        {
                            try
                            {
                                details::handle_immediate_activation_pending(model, lock, { id, type }, response_activation, gate);
                            }
                            catch (...)
                            {
                                result = handle_connection_resource_exception(id);
                            }
                        }
                    }

                    if (web::http::is_success_status_code(result.first))
                    {
                        // make a bulk response success item
                        // see https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-bulk-response-schema.json
                        result.second = value_of({
                            { nmos::fields::id, id },
                            { U("code"), result.first }
                        });
                    }
                    else
                    {
                        // don't replace an existing response body which might contain richer error information
                        if (result.second.is_null())
                        {
                            result.second = nmos::make_error_response_body(result.first);
                        }

                        // make a bulk response error item from the standard NMOS error response
                        result.second[nmos::fields::id] = value::string(id);
                    }
                }

                set_reply(res, status_codes::OK,
                    web::json::serialize(results,
                        [](const details::connection_resource_patch_response& result) { return result.second; }),
                    U("application/json"));
                return true;
            });
        });

        connection_api.support(U("/single/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("senders/"), U("receivers/") }, res));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);

            const auto match = [&](const nmos::resources::value_type& resource) { return resource.type == nmos::type_from_resourceType(resourceType); };

            size_t count = 0;

            set_reply(res, status_codes::OK,
                web::json::serialize_if(resources,
                    match,
                    [&count](const nmos::resources::value_type& resource) { ++count; return value(resource.id + U("/")); }),
                U("application/json"));

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << count << " matching " << resourceType;

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource)
            {
                std::set<utility::string_t> sub_routes{ U("constraints/"), U("staged/"), U("active/") };
                if (nmos::types::sender == resource->type) sub_routes.insert(U("transportfile/"));

                // The transporttype endpoint is introduced in v1.1
                if (nmos::is05_versions::v1_1 <= version) sub_routes.insert(U("transporttype/"));

                set_reply(res, status_codes::OK, nmos::make_sub_routes_body(sub_routes, res));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning constraints for " << id_type;

                const auto accept = req.headers().find(web::http::header_names::accept);
                if (req.headers().end() != accept && U("application/schema+json") == accept->second)
                {
                    // Experimental extension - constraints as JSON Schema
                    set_reply(res, status_codes::OK, nmos::details::make_constraints_schema(resource->type, nmos::fields::endpoint_constraints(resource->data)));
                }
                else
                {
                    set_reply(res, status_codes::OK, nmos::fields::endpoint_constraints(resource->data));
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/staged/?"), methods::PATCH, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            return details::extract_json(req, gate).then([&model, req, res, parameters, gate](value body) mutable
            {
                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Operation requested for single " << id_type;

                details::handle_connection_resource_patch(res, model, version, id_type, body, gate);

                return true;
            });
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/") + nmos::patterns::stagingType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);
            const string_t stagingType = parameters.at(nmos::patterns::stagingType.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                if (nmos::fields::endpoint_staged.key == stagingType)
                {
                    const auto staged_state = details::get_activation_state(nmos::fields::activation(nmos::fields::endpoint_staged(resource->data)));

                    if (details::immediate_activation_pending == staged_state)
                    {
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/issues/49
                        if (!details::wait_immediate_activation_not_pending(model, lock, id_type) || model.shutdown)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Rejecting GET request for " << id_type << " due to a pending immediate activation";

                            set_reply(res, status_codes::InternalError); // or ServiceUnavailable? probably not NotFound even if that's true after the timeout?
                            return pplx::task_from_result(true);
                        }
                        // find resource again just in case, since waiting releases and reacquires the lock
                        resource = find_resource(resources, id_type);
                    }
                }
            }

            if (resources.end() != resource)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << stagingType << " data for " << id_type;

                const web::json::field_as_value endpoint_staging{ stagingType };
                set_reply(res, status_codes::OK, endpoint_staging(resource->data));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/transportfile/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                if (nmos::fields::master_enable(nmos::fields::endpoint_active(resource->data)))
                {
                    // The transportfile endpoint data in the resource must have either "data" and "type", or an "href" for the redirect

                    auto& transportfile = nmos::fields::endpoint_transportfile(resource->data);
                    auto& data = nmos::fields::transportfile_data(transportfile);

                    if (!data.is_null())
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning transport file for " << id_type;

                        const auto accept = req.headers().find(web::http::header_names::accept);
                        if (req.headers().end() != accept && U("application/json") == accept->second && U("application/sdp") == nmos::fields::transportfile_type(transportfile))
                        {
                            // Experimental extension - SDP as JSON
                            set_reply(res, status_codes::OK, sdp::parse_session_description(utility::us2s(data.as_string())));
                        }
                        else
                        {
                            // This automatically performs conversion to UTF-8 if required (i.e. on Windows)
                            set_reply(res, status_codes::OK, data.as_string(), nmos::fields::transportfile_type(transportfile));
                        }

                        // "It is strongly recommended that the following caching headers are included via the /transportfile endpoint (or whatever this endpoint redirects to).
                        // This is important to ensure that connection management clients do not cache the contents of transport files which are liable to change."
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.0.%20Behaviour.md#transport-files--caching
                        res.headers().set_cache_control(U("no-cache"));
                    }
                    else
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Redirecting to transport file for " << id_type;

                        set_reply(res, status_codes::TemporaryRedirect);
                        res.headers().add(web::http::header_names::location, nmos::fields::transportfile_href(transportfile));
                    }
                }
                else
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Transport file requested for disabled " << id_type;

                    // "When the `master_enable` parameter is false [...] the `/transportfile` endpoint should return an HTTP 404 response."
                    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L163-L165
                    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L277
                    set_error_reply(res, status_codes::NotFound, U("Sender is not currently configured"));
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/transporttype/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            // The transporttype endpoint is introduced in v1.1
            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            if (nmos::is05_versions::v1_1 > version)
            {
                set_reply(res, status_codes::NotFound);
                return pplx::task_from_result(true);
            }

            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                auto matching_resource = find_resource(model.node_resources, id_type);
                if (model.node_resources.end() == matching_resource)
                {
                    throw std::logic_error("matching IS-04 resource not found");
                }
                else
                {
                    // hmm, currently unclear whether subclassifications such as e.g. "urn:x-nmos:transport:rtp.mcast"
                    // should be presented as the top-level category, e.g. "urn:x-nmos:transport:rtp"
                    // proposed solution is to trim to the first '.' after the last ':'
                    // see https://github.com/AMWA-TV/nmos-device-connection-management/issues/57
                    const auto& transport_subclassification = nmos::fields::transport(matching_resource->data);
                    const auto last_colon = transport_subclassification.find_last_of(U(':'));
                    const auto next_dot = transport_subclassification.find(U('.'), last_colon + 1);
                    set_reply(res, status_codes::OK, web::json::value::string(transport_subclassification.substr(0, next_dot)));
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        return connection_api;
    }
}

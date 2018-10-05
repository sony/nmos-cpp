#include "nmos/connection_api.h"

#include "cpprest/http_utils.h"
#include "nmos/activation_mode.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/log_manip.h"
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
            set_reply(res, status_codes::OK, value_of({ JU("x-nmos/" })));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/x-nmos/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("connection/") }));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/x-nmos/") + nmos::patterns::connection_api.pattern + U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("v1.0/") }));
            return pplx::task_from_result(true);
        });

        connection_api.mount(U("/x-nmos/") + nmos::patterns::connection_api.pattern + U("/") + nmos::patterns::is05_version.pattern, make_unmounted_connection_api(model, gate));

        nmos::add_api_finally_handler(connection_api, gate);

        return connection_api;
    }

    namespace details
    {
        // A poor substitute for schema validation
        // throws web::json::json_exception on failure, which results in a 400 Bad Request
        void validate_staged_core(const nmos::type& type, const web::json::value& staged)
        {
            static const std::map<nmos::type, std::set<utility::string_t>> stage_fields
            {
                {
                    // https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-sender-stage-schema.json
                    nmos::types::sender,
                    {
                        nmos::fields::receiver_id,
                        nmos::fields::master_enable,
                        nmos::fields::activation,
                        nmos::fields::transport_params
                    }
                },
                {
                    // https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-receiver-stage-schema.json
                    nmos::types::receiver,
                    {
                        nmos::fields::sender_id,
                        nmos::fields::master_enable,
                        nmos::fields::activation,
                        nmos::fields::transport_file,
                        nmos::fields::transport_params
                    }
                }
            };
            const auto& type_fields = stage_fields.at(type);
            const auto& o = staged.as_object();
            const auto found_invalid = std::find_if_not(o.begin(), o.end(), [&](const std::pair<utility::string_t, web::json::value>& field)
            {
                return type_fields.end() != type_fields.find(field.first);
            });
            if (o.end() != found_invalid)
            {
                throw web::json::json_exception((U("invalid field - ") + found_invalid->first).c_str());
            }
        }

        // Validate staged endpoint against constraints
        // This is a potential customisation point e.g. to validate semantics not representable by the constraints endpoint
        void validate_staged_constraints(const web::json::value& constraints, const web::json::value& staged)
        {
            // hmm, should do this...
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
            else // if (nmos::activation_modes::activate_immediate == mode)
            {
                return immediate_activation_pending;
            }
        }

        // Calculate the absolute TAI from the requested time of a scheduled activation
        nmos::tai get_absolute_requested_time(const web::json::value& activation)
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
                return tai_from_time_point(tai_clock::now() + duration_from_tai(web::json::as<nmos::tai>(requested_time_or_null)));
            }
            else
            {
                throw std::logic_error("cannot get absolute requested time for mode: " + utility::us2s(mode.name));
            }
        }

        // Set the appropriate fields of the response/staged activation from the specified patch
        void merge_activation(web::json::value& activation, const web::json::value& patch_activation)
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
                activation[nmos::fields::requested_time] = value::string(nmos::make_version());

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
                auto absolute_requested_time = get_absolute_requested_time(activation);
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

        // Set appropriate transport parameters based on the specified transport file
        // This is a potential customisation point e.g. to change the handling of "application/sdp", or handle more transport types
        void merge_transport_file(web::json::value& transport_params, const utility::string_t& transport_data, const utility::string_t& transport_type)
        {
            if (transport_type != U("application/sdp"))
            {
                throw transport_file_error("unexpected type: " + utility::us2s(transport_type));
            }

            try
            {
                const auto session_description = sdp::parse_session_description(utility::us2s(transport_data));
                const bool smpte2022_7 = 2 == transport_params.size();
                const auto patch = nmos::get_session_description_transport_params(session_description, smpte2022_7);
                web::json::merge_patch(transport_params, patch);
            }
            catch (const web::json::json_exception& e)
            {
                throw transport_file_error(e.what());
            }
        }

        // Set appropriate transport parameters based on the specified transport file
        void merge_transport_file(web::json::value& transport_params, const web::json::value& transport_file)
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
                if (transport_type.is_null()) throw transport_file_error("type is required");

                merge_transport_file(transport_params, transport_data.as_string(), transport_type.as_string());
            }
            else
            {
                // Check for unexpected transport file type? In the future, could be a range of supported types...
                // Or only allow the type to be omitted? Or null?
            }
        }

        void set_staged_activation_time(nmos::resources& connection_resources, const nmos::id& id, const nmos::tai& tai)
        {
            using web::json::value;

            modify_resource(connection_resources, id, [&tai](nmos::resource& resource)
            {
                resource.data[nmos::fields::version] = value::string(nmos::make_version());

                auto& staged = nmos::fields::endpoint_staged(resource.data);
                auto& staged_activation = staged[nmos::fields::activation];

                staged_activation[nmos::fields::activation_time] = value::string(nmos::make_version(tai));
            });
        }

        void set_staged_activation_not_pending(nmos::resources& connection_resources, const nmos::id& id)
        {
            using web::json::value;

            modify_resource(connection_resources, id, [](nmos::resource& resource)
            {
                resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());

                auto& staged = nmos::fields::endpoint_staged(resource.data);
                auto& staged_activation = staged[nmos::fields::activation];

                staged_activation[nmos::fields::mode] = value::null();
                staged_activation[nmos::fields::requested_time] = value::null();
                staged_activation[nmos::fields::activation_time] = value::null();
            });
        }

        // wait for the staged activation mode of the specified resource to be set to something other than "activate_immediate"
        // or for the resource to have vanished (unexpected!)
        // or for the server to be shut down
        void wait_immediate_activation_not_pending(nmos::node_model& model, nmos::write_lock& lock, std::pair<nmos::id, nmos::type> id_type)
        {
            model.wait_for(lock, std::chrono::seconds(nmos::fields::immediate_activation_max(model.settings)), [&]
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
        void wait_activation_modified(nmos::node_model& model, nmos::write_lock& lock, std::pair<nmos::id, nmos::type> id_type, web::json::value initial_activation)
        {
            model.wait_for(lock, std::chrono::seconds(nmos::fields::immediate_activation_max(model.settings)), [&]
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

            // we need to use wait_activation_modified here

            // ...while over in the connection thread...
            {
                // on success, update the active endpoint and the relevant IS-04 resource
                // and update the staged endpoint to indicate the immediate activation has been processed
                set_staged_activation_time(model.connection_resources, id_type.first);

                // on failure, set the activation not pending (or to anything else...)

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Notifying connection API - immediate activation processed";
                model.notify();
            }

            auto& resources = model.connection_resources;

            // after releasing and reacquiring the lock, must find the resource again!
            if (resources.end() == find_resource(resources, id_type))
            {
                // since this has happened while the activation was in flight, the response is 500 Internal Error rather than 404 Not Found
                throw std::logic_error("resource vanished during in-flight immediate activation");
            }

            modify_resource(resources, id_type.first, [&response_activation](nmos::resource& resource)
            {
                auto& staged = nmos::fields::endpoint_staged(resource.data);
                auto& staged_activation = staged[nmos::fields::activation];

                // use requested time to identify it's still the same in-flight immediate activation
                if (staged_activation[nmos::fields::requested_time] != response_activation[nmos::fields::requested_time])
                {
                    throw std::logic_error("activation modified during in-flight immediate activation");
                }

                resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());

                // "For immediate activations, [the `mode` field] will be null in response to any subsequent GET requests."
                staged_activation[nmos::fields::mode] = value::null();

                // "For an immediate activation this field will always be null on the staged endpoint,
                // even in the response to the PATCH request."
                response_activation[nmos::fields::requested_time] = value::null();

                // "For immediate activations on the staged endpoint this property will be the time the activation actually
                // occurred in the response to the PATCH request, but null in response to any GET requests thereafter."
                response_activation[nmos::fields::activation_time] = staged_activation[nmos::fields::activation_time];
                staged_activation[nmos::fields::activation_time] = value::null();
            });

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying connection API - immediate activation completed";
            model.notify();
        }

        void handle_patch(web::http::http_response res, nmos::node_model& model, const std::pair<nmos::id, nmos::type>& id_type, const web::json::value& patch, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            // could start out as a shared/read lock, only upgraded to an exclusive/write lock when the sender/receiver in the resources is actually modified
            auto lock = model.write_lock();
            auto& resources = model.connection_resources;

            // Validate JSON syntax according to the schema
            details::validate_staged_core(id_type.second, patch);

            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                const auto patch_state = details::get_activation_state(nmos::fields::activation(patch));

                // Prevent changes to already-scheduled activations and in-flight immediate activations

                const auto staged_state = details::get_activation_state(nmos::fields::activation(nmos::fields::endpoint_staged(resource->data)));

                if (details::scheduled_activation_pending == staged_state && details::activation_not_pending != patch_state)
                {
                    // "When the resource is locked because an activation has been scheduled[, 423 Locked is returned].
                    // A resource may be unlocked by setting `mode` in `activation` to `null`."

                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Rejecting PATCH request for " << id_type << " due to a pending scheduled activation";

                    set_reply(res, status_codes::Locked);
                    return;
                }
                else if (details::immediate_activation_pending == staged_state)
                {
                    // We actually need to use wait_immediate_activation_not_pending and then find the resource again!
                    // See https://github.com/AMWA-TV/nmos-device-connection-management/issues/49

                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Rejecting PATCH request for " << id_type << " due to a pending immediate activation";

                    set_reply(res, status_codes::InternalError);
                    return;
                }

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

                // First, merge the transport file (it must be a receiver)

                auto& transport_file = nmos::fields::transport_file(patch);
                if (!transport_file.is_null() && !transport_file.as_object().empty())
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Processing transport file";

                    details::merge_transport_file(nmos::fields::transport_params(merged), transport_file);
                }

                // Second, merge the transport parameters (in fact, all fields, including "sender_id", "master_enabled", etc.)

                web::json::merge_patch(merged, patch);

                // Then, prepare the activation response

                details::merge_activation(merged[nmos::fields::activation], nmos::fields::activation(patch));

                // Validate merged JSON according to the constraints

                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating staged transport parameters against constraints";

                details::validate_staged_constraints(nmos::fields::endpoint_constraints(resource->data), merged);

                // Finally, update the staged endpoint

                modify_resource(resources, id_type.first, [&patch_state, &merged](nmos::resource& resource)
                {
                    resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());

                    nmos::fields::endpoint_staged(resource.data) = merged;
                    // In the case of an immediate activation, this is not yet a valid response
                    // to a 'subsequent' GET request on the staged endpoint; that is dealt with below!
                });

                if (details::staging_only == patch_state)
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying connection thread - staged endpoint updated";
                else if (details::activation_not_pending == patch_state)
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Notifying connection thread - scheduled activation cancelled";
                else if (details::scheduled_activation_pending == patch_state)
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Notifying connection thread - scheduled activation requested";
                else if (details::immediate_activation_pending == patch_state)
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying connection thread - immediate activation requested";

                model.notify();

                // OK, that wasn't finally; immediate activations need to be processed...
                if (details::immediate_activation_pending == patch_state)
                {
                    details::handle_immediate_activation_pending(model, lock, id_type, merged[nmos::fields::activation], gate);
                }

                // Finally, really finally, send the response

                set_reply(res, details::scheduled_activation_pending == patch_state ? status_codes::Accepted : status_codes::OK, merged);
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }
        };
    }

    web::http::experimental::listener::api_router make_unmounted_connection_api(nmos::node_model& model, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router connection_api;

        connection_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("bulk/"), JU("single/") }));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/bulk/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("senders/"), JU("receivers/") }));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/bulk/") + nmos::patterns::connectorType.pattern + U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::MethodNotAllowed);
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/bulk/") + nmos::patterns::connectorType.pattern + U("/?"), methods::POST, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            return details::extract_json(req, parameters, gate).then([&, req, res, parameters](value body) mutable
            {
                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);

                auto& patches = body.as_array();

                slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Bulk operation requested for " << patches.size() << " " << resourceType;

                std::vector<value> results;
                results.reserve(patches.size());

                // "Where a server implementation supports concurrent application of settings changes to
                // underlying Senders and Receivers, it may choose to perform 'bulk' resource operations
                // in a parallel fashion internally. This is an implementation decision and is not a
                // requirement of this specification."
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.0.%20Behaviour.md
                // Choose life. Just say no.
                const auto type = nmos::type_from_resourceType(resourceType);
                for (auto& patch : patches)
                {
                    const auto id = nmos::fields::id(patch);

                    web::http::http_response res;

                    // try-catch based on the exception handler in nmos::add_api_finally_handler
                    try
                    {
                        details::handle_patch(res, model, { id, type }, patch[nmos::fields::params], gate);
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "JSON error for " << id << " in bulk request: " << e.what();
                        details::set_error_reply(res, status_codes::BadRequest, utility::s2us(e.what()));
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "HTTP error for " << id << " in bulk request: " << e.what() << " [" << e.error_code() << "]";
                        details::set_error_reply(res, status_codes::BadRequest, utility::s2us(e.what()));
                    }
                    catch (const std::runtime_error& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Implementation error for " << id << " in bulk request: " << e.what();
                        details::set_error_reply(res, status_codes::NotImplemented, utility::s2us(e.what()));
                    }
                    catch (const std::logic_error& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Implementation error for " << id << " in bulk request: " << e.what();
                        details::set_error_reply(res, status_codes::InternalError, utility::s2us(e.what()));
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Unexpected exception for " << id << " in bulk request: " << e.what();
                        details::set_error_reply(res, status_codes::InternalError, utility::s2us(e.what()));
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Unexpected unknown exception for " << id << " in bulk request";
                        details::set_error_reply(res, status_codes::InternalError);
                    }

                    if (web::http::is_success_status_code(res.status_code()))
                    {
                        // make a bulk response success item
                        // see https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-bulk-response-schema.json
                        results.push_back(value_of({
                            { nmos::fields::id, id },
                            { U("code"), res.status_code() }
                        }));
                    }
                    else
                    {
                        // make a bulk response error item from the standard NMOS error response
                        auto result = res.extract_json().get();
                        result[nmos::fields::id] = value::string(id);
                        results.push_back(std::move(result));
                    }
                }

                set_reply(res, status_codes::OK, web::json::serialize(results), U("application/json"));
                return true;
            });
        });

        connection_api.support(U("/single/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, value_of({ JU("senders/"), JU("receivers/") }));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
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

            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning " << count << " matching " << resourceType;

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            auto resource = find_resource(resources, { resourceId, nmos::type_from_resourceType(resourceType) });
            if (resources.end() != resource)
            {
                if (nmos::types::sender == resource->type)
                {
                    set_reply(res, status_codes::OK, value_of({ JU("constraints/"), JU("staged/"), JU("active/"), JU("transportfile/") }));
                }
                else // if (nmos::types::receiver == resource->type)
                {
                    set_reply(res, status_codes::OK, value_of({ JU("constraints/"), JU("staged/"), JU("active/") }));
                }
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
            auto resource = find_resource(resources, id_type);
            if (resources.end() == resource)
            {
                set_reply(res, status_codes::NotFound);
            }
            else
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning constraints for " << id_type;
                set_reply(res, status_codes::OK, nmos::fields::endpoint_constraints(resource->data));
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/staged/?"), methods::PATCH, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            return details::extract_json(req, parameters, gate).then([&, req, res, parameters](value body) mutable
            {
                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };

                slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Operation requested for single " << id_type;

                details::handle_patch(res, model, id_type, body, gate);

                return true;
            });
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/") + nmos::patterns::stagingType.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
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
                        // We actually need to use wait_immediate_activation_not_pending and then must find the resource again!
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/issues/49

                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Rejecting GET request for " << id_type << " due to a pending immediate activation";

                        set_reply(res, status_codes::InternalError);
                        return pplx::task_from_result(true);
                    }
                }

                slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Returning " << stagingType << " data for " << id_type;

                const web::json::field_as_value endpoint_staging{ stagingType };
                set_reply(res, status_codes::OK, endpoint_staging(resource->data));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/transportfile/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                if (nmos::fields::master_enable(nmos::fields::endpoint_active(resource->data)))
                {
                    // The transportfile endpoint data in the resource could support "data", "type" and an optional "encoding" field
                    // as an alternative to "href"; data conversion to the specified encoding would be needed here

                    slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Redirecting to transport file for " << id_type;

                    set_reply(res, status_codes::TemporaryRedirect);
                    res.headers().add(web::http::header_names::location, nmos::fields::href(nmos::fields::endpoint_transportfile(resource->data)));
                }
                else
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Transport file requested for disabled " << id_type;

                    // "When the `master_enable` parameter is false [...] the `/transportfile` endpoint should return an HTTP 404 response."
                    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L163-L165
                    set_reply(res, status_codes::NotFound);
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

#include "nmos/connection_api.h"

#include <boost/range/join.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/http_utils.h"
#include "cpprest/json_validator.h"
#include "nmos/activation_mode.h"
#include "nmos/activation_utils.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h"
#include "nmos/is04_versions.h"
#include "nmos/is05_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/sdp_utils.h"
#include "nmos/slog.h"
#include "nmos/transport.h"
#include "nmos/thread_utils.h"
#include "nmos/version.h"
#include "sdp/sdp.h"

namespace nmos
{
    web::http::experimental::listener::api_router make_unmounted_connection_api(nmos::node_model& model, transport_file_parser parse_transport_file, details::connection_resource_patch_validator validate_merged, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, transport_file_parser parse_transport_file, details::connection_resource_patch_validator validate_merged, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router connection_api;

        connection_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("connection/") }, req, res));
            return pplx::task_from_result(true);
        });

        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is05_versions::from_settings(model.settings); });
        connection_api.support(U("/x-nmos/") + nmos::patterns::connection_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
            return pplx::task_from_result(true);
        });

        connection_api.mount(U("/x-nmos/") + nmos::patterns::connection_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_connection_api(model, parse_transport_file, validate_merged, gate));

        return connection_api;
    }

    web::http::experimental::listener::api_router make_connection_api(nmos::node_model& model, slog::base_gate& gate)
    {
        return make_connection_api(model, &parse_rtp_transport_file, {}, gate);
    }

    inline bool is_connection_api_permitted_downgrade(const nmos::resource& resource, const nmos::resource& connection_resource, const nmos::api_version& version)
    {
        if (resource.version.major != version.major) return false;
        if (resource.version.minor <= version.minor) return true;

        // "Where a transport type is added in a new version of the Connection Management API specification, earlier versioned APIs must not list any Senders or Receivers which make use of this new transport type."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/5.0.%20Upgrade%20Path.md#requirements-for-connection-management-apis

        typedef const std::map<nmos::api_version, std::set<nmos::transport>> versions_transport_bases_t;
        versions_transport_bases_t versions_transport_bases
        {
            {
                nmos::is05_versions::v1_0, { nmos::transports::rtp }
            },
            {
                nmos::is05_versions::v1_1, { nmos::transports::websocket, nmos::transports::mqtt }
            }
        };

        const nmos::transport transport_subclassification(nmos::fields::transport(resource.data));
        const nmos::transport transport_base = nmos::transport_base(transport_subclassification);

        const auto first = versions_transport_bases.begin();
        const auto last = versions_transport_bases.upper_bound(version);

        return last != std::find_if(first, last, [&transport_base](const versions_transport_bases_t::value_type& vtb) { return vtb.second.end() != vtb.second.find(transport_base); });
    }

    inline utility::string_t make_connection_api_resource_location(const nmos::resource& resource, const utility::string_t& sub_path = {})
    {
        return U("/x-nmos/connection/") + nmos::make_api_version(resource.version) + U("/single/") + nmos::resourceType_from_type(resource.type) + U("/") + resource.id + sub_path;
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
                    is05_versions::all | boost::adaptors::transformed(experimental::make_connectionapi_sender_staged_patch_request_schema_uri),
                    is05_versions::all | boost::adaptors::transformed(experimental::make_connectionapi_receiver_staged_patch_request_schema_uri)
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

        static const std::map<nmos::type, std::set<utility::string_t>>& rtp_auto_constraints()
        {
            // These are the constraints that support "auto" in /staged; cf. resolve_rtp_auto
            // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_rtp.json
            // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_rtp.json
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

        static const std::map<nmos::type, std::set<utility::string_t>>& websocket_auto_constraints()
        {
            // These are the constraints that support "auto" in /staged
            // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_websocket.json
            // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_websocket.json
            static const std::map<nmos::type, std::set<utility::string_t>> auto_constraints
            {
                {
                    nmos::types::sender,
                    {
                        nmos::fields::connection_uri,
                        nmos::fields::connection_authorization
                    }
                },
                {
                    nmos::types::receiver,
                    {
                        nmos::fields::connection_authorization
                    }
                }
            };
            return auto_constraints;
        }

        static const std::map<nmos::type, std::set<utility::string_t>>& mqtt_auto_constraints()
        {
            // These are the constraints that support "auto" in /staged
            // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_mqtt.json
            // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_mqtt.json
            static const std::map<nmos::type, std::set<utility::string_t>> auto_constraints
            {
                {
                    nmos::types::sender,
                    {
                        nmos::fields::destination_host,
                        nmos::fields::destination_port,
                        nmos::fields::broker_protocol,
                        nmos::fields::broker_authorization
                    }
                },
                {
                    nmos::types::receiver,
                    {
                        nmos::fields::source_host,
                        nmos::fields::source_port,
                        nmos::fields::broker_protocol,
                        nmos::fields::broker_authorization
                    }
                }
            };
            return auto_constraints;
        }

        static const std::map<nmos::type, std::set<utility::string_t>>& auto_constraints(const nmos::transport& transport_base)
        {
            if (nmos::transports::rtp == transport_base) return rtp_auto_constraints();
            if (nmos::transports::websocket == transport_base) return websocket_auto_constraints();
            if (nmos::transports::mqtt == transport_base) return mqtt_auto_constraints();

            static const std::map<nmos::type, std::set<utility::string_t>> no_auto_constraints;
            return no_auto_constraints;
        }

        // Make a json schema from /constraints and /transporttype
        web::json::value make_constraints_schema(const nmos::type& type, const web::json::value& constraints, const nmos::transport& transport_base)
        {
            using web::json::value;
            using web::json::value_of;

            const bool keep_order = true;

            auto items = value::array();

            auto& type_auto_constraints = auto_constraints(transport_base).at(type);

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
        void validate_staged_constraints(const nmos::type& type, const web::json::value& constraints, const nmos::transport& transport_base, const web::json::value& staged)
        {
            const auto schema = make_constraints_schema(type, constraints, transport_base);
            const auto uri = web::uri{ U("/constraints") };

            // Validate staged JSON syntax according to the schema

            const web::json::experimental::json_validator validator
            {
                [&](const web::uri&) { return schema; },
                { uri }
            };

            // Validate JSON syntax according to the schema

            validator.validate(staged, uri);
        }

        // Apparently, the response to errors in the transport file should be 500 Internal Error rather than 400 Bad Request
        // See https://github.com/AMWA-TV/nmos-device-connection-management/issues/40
        inline std::logic_error transport_file_error(const std::string& message)
        {
            return std::logic_error("Transport file error - " + message);
        }

        std::pair<utility::string_t, utility::string_t> get_transport_type_data(const web::json::value& transport_file)
        {
            // "'data' and 'type' must both be strings or both be null"
            // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0.2/APIs/schemas/v1.0-receiver-response-schema.json
            // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver-transport-file.json

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

        typedef std::pair<web::http::status_code, web::json::value> connection_resource_patch_response;

        inline connection_resource_patch_response make_connection_resource_patch_error_response(web::http::status_code code, const utility::string_t& error = {}, const utility::string_t& debug = {})
        {
            return{ code, make_error_response_body(code, error, debug) };
        }

        inline connection_resource_patch_response make_connection_resource_patch_error_response(web::http::status_code code, const std::exception& debug)
        {
            return make_connection_resource_patch_error_response(code, {}, utility::s2us(debug.what()));
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
        // nmos::connection_activation_thread can usually be used to schedule sender/receiver activations.
        //
        // By the time we reacquire the model lock anything may have happened, but we can identify with the above whether to send
        // a success response or an error, and in the success case, release the 'per-resource lock' by updating the staged
        // activation mode, requested_time and activation_time.
        connection_resource_patch_response handle_connection_resource_patch(nmos::node_model& model, nmos::write_lock& lock, const nmos::api_version& version, const std::pair<nmos::id, nmos::type>& id_type, const web::json::value& patch, const nmos::tai& request_time, transport_file_parser parse_transport_file, details::connection_resource_patch_validator validate_merged, slog::base_gate& gate)
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
                auto matching_resource = find_resource(model.node_resources, id_type);
                if (model.node_resources.end() == matching_resource)
                {
                    throw std::logic_error("matching IS-04 and IS-05 resources not found");
                }

                if (nmos::is_connection_api_permitted_downgrade(*matching_resource, *resource, version))
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

                        // "If a 'bulk' request includes multiple sets of parameters for the same Sender or Receiver ID the behaviour is defined by the implementation.
                        // In order to maximise interoperability clients are encouraged not to include the same Sender or Receiver ID multiple times in the same 'bulk' request."
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0.2/docs/4.0.%20Behaviour.md#salvo-operation
                        if (!requested_time_or_null.is_null() && request_time == web::json::as<nmos::tai>(requested_time_or_null))
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Rejecting PATCH request for " << id_type << " due to a pending immediate activation from the same bulk request";

                            return details::make_connection_resource_patch_error_response(status_codes::BadRequest);
                        }
                        // "If an API implementation receives a new PATCH request to the /staged resource while an activation is in progress it SHOULD block the request until the previous activation is complete."
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.0.%20Behaviour.md#in-progress-activations
                        else if (!details::wait_immediate_activation_not_pending(model, lock, id_type) || model.shutdown)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Rejecting PATCH request for " << id_type << " due to a pending immediate activation";

                            return details::make_connection_resource_patch_error_response(status_codes::InternalError); // or ServiceUnavailable? probably not NotFound even if that's true after the timeout?
                        }
                        // find resource again just in case, since waiting releases and reacquires the lock
                        resource = find_resource(resources, id_type);
                    }
                }
                else
                {
                    // experimental extension, proposed for v1.1, to distinguish from Not Found
                    return details::make_connection_resource_patch_error_response(status_codes::Conflict, U("Conflict; ") + details::make_permitted_downgrade_error(*resource, version));
                    // hmm, no way to pass the Location header back
                }
            }

            if (resources.end() != resource)
            {
                auto matching_resource = find_resource(model.node_resources, id_type);
                if (model.node_resources.end() == matching_resource)
                {
                    throw std::logic_error("matching IS-04 and IS-05 resources not found");
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

                // First, validate and merge the transport file (this resource must be a receiver)
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L344-L363

                auto& transport_file = nmos::fields::transport_file(patch);
                if (!transport_file.is_null() && !transport_file.as_object().empty())
                {
                    const auto transport_type_data = details::get_transport_type_data(transport_file);

                    if (!transport_type_data.first.empty())
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Processing transport file";

                        try
                        {
                            // Validate and parse the transport file for this receiver

                            const auto transport_file_params = parse_transport_file(*matching_resource, *resource, transport_type_data.first, transport_type_data.second, gate);

                            // Merge the transport file into the transport parameters

                            auto& transport_params = nmos::fields::transport_params(merged);

                            web::json::merge_patch(transport_params, transport_file_params);
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

                const nmos::transport transport_subclassification(nmos::fields::transport(matching_resource->data));
                details::validate_staged_constraints(resource->type, nmos::fields::endpoint_constraints(resource->data), nmos::transport_base(transport_subclassification), merged);

                // Perform any final validation

                if (validate_merged)
                {
                    validate_merged(*matching_resource, *resource, merged, gate);
                }

                // Finally, update the staged endpoint

                modify_resource(resources, id_type.first, [&merged](nmos::resource& resource)
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

                return{ details::scheduled_activation_pending == patch_state ? status_codes::Accepted : status_codes::OK, std::move(merged) };
            }
            else
            {
                return details::make_connection_resource_patch_error_response(status_codes::NotFound);
            }
        }

        void notify_connection_resource_patch(const nmos::node_model& model, slog::base_gate& gate)
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying connection activation thread";

            model.notify();
        }

        void handle_connection_resource_patch(web::http::http_response res, nmos::node_model& model, const nmos::api_version& version, const std::pair<nmos::id, nmos::type>& id_type, const web::json::value& patch, transport_file_parser parse_transport_file, details::connection_resource_patch_validator validate_merged, slog::base_gate& gate)
        {
            auto lock = model.write_lock();
            const auto request_time = tai_now(); // during write lock to ensure uniqueness

            auto result = handle_connection_resource_patch(model, lock, version, id_type, patch, request_time, parse_transport_file, validate_merged, gate);

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

        void handle_connection_resource_transportfile(web::http::http_response res, const nmos::node_model& model, const nmos::api_version& version, const std::pair<nmos::id, nmos::type>& id_type, const utility::string_t& accept, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                auto matching_resource = find_resource(model.node_resources, id_type);
                if (model.node_resources.end() == matching_resource)
                {
                    throw std::logic_error("matching IS-04 and IS-05 resources not found");
                }

                if (nmos::is_connection_api_permitted_downgrade(*matching_resource, *resource, version))
                {
                    auto& transportfile = nmos::fields::endpoint_transportfile(resource->data);

                    if (!transportfile.is_null())
                    {
                        // The transportfile endpoint data in the resource must have either "data" and "type", or an "href" for the redirect
                        auto& data = nmos::fields::transportfile_data(transportfile);

                        if (!data.is_null())
                        {
                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning transport file for " << id_type;

                            // hmm, parsing of the Accept header could be much better and should take account of quality values
                            if (!accept.empty() && web::http::details::mime_types::application_json == web::http::details::get_mime_type(accept) && U("application/sdp") == nmos::fields::transportfile_type(transportfile))
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
                        // Either this sender uses a transport type which does not require a transport file in which case a 404 response is required
                        // or the sender does use a transport file, but is inactive and not currently configured in which case a 404 is also allowed
                        // (or this is an internal server error, but since a 5xx response is not defined, assume one of the former cases)
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Transport file requested for " << id_type << " which does not have one";

                        // An HTTP 404 response may be returned if "the transport type does not require a transport file".
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/ConnectionAPI.raml#L339-L340
                        // "When the `master_enable` parameter is false [...] the `/transportfile` endpoint should return an HTTP 404 response."
                        // In other words an HTTP 404 response is returned "if the sender is not currently configured".
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L163-L165
                        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L277
                        // Those statements are going to be combined and adjusted slightly in the next specification patch release to say:
                        // An HTTP 404 response is returned if "the transport type does not require a transport file, or if the sender is not currently configured"
                        // and "may also be returned when the `master_enable` parameter is false in /active, if the sender only maintains a transport file when transmitting."
                        // See https://github.com/AMWA-TV/nmos-device-connection-management/pull/111
                        set_error_reply(res, status_codes::NotFound, U("Sender is not configured with a transport file"));
                    }
                }
                else
                {
                    // experimental extension, proposed for v1.1, to distinguish from Not Found
                    set_error_reply(res, status_codes::Conflict, U("Conflict; ") + details::make_permitted_downgrade_error(*resource, version));
                    res.headers().add(web::http::header_names::location, make_connection_api_resource_location(*resource, U("/transportfile")));
                }
            }
            else if (details::is_erased_resource(resources, id_type))
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + details::make_erased_resource_error());
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }
        }
    }

    // Activate an IS-05 sender or receiver by transitioning the 'staged' settings into the 'active' resource
    void set_connection_resource_active(nmos::resource& connection_resource, std::function<void(web::json::value& endpoint_active)> resolve_auto, const nmos::tai& activation_time)
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

        auto& active = nmos::fields::endpoint_active(resource.data);

        // "On activation all instances of "auto" must be resolved into the actual values that will be used, unless
        // there is an error condition. If there is an error condition that means `auto` cannot be resolved, the active
        // transport parameters must not change, and the underlying sender must continue as before."
        // Therefore, in case it throws an exception, resolve_auto is called on a copy of the /staged resource data,
        // before making any changes to the /active resource data.
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/ConnectionAPI.raml#L300-L309
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
        auto activating = staged;
        resolve_auto(activating);

        // "When a set of 'staged' settings is activated, these settings transition into the 'active' resource."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/1.0.%20Overview.md#active
        active = activating;

        // Unclear whether the activation in the active endpoint should have values for mode, requested_time
        // (and even activation_time?) or whether they should be null? The examples have them with values.
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/examples/v1.0-receiver-active-get-200.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/examples/v1.0-sender-active-get.json

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
        auto& staged = nmos::fields::endpoint_staged(connection_resource.data);
        auto& staged_activation = staged[nmos::fields::activation];

        // "This parameter returns to null on the staged endpoint once an activation is completed."
        // "This field returns to null once the activation is completed on the staged endpoint."
        // "On the staged endpoint this field returns to null once the activation is completed."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-activation-response-schema.json

        // "A resource may be unlocked by setting `mode` in `activation` to `null`, which will cancel the pending activation."
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L244

        staged_activation = nmos::make_activation();
    }

    // Update the IS-04 sender or receiver after the active connection is changed in any way
    // (This function should be called after nmos::set_connection_resource_active.)
    void set_resource_subscription(nmos::resource& node_resource, bool active, const nmos::id& connected_id, const nmos::tai& activation_time)
    {
        using web::json::value;
        using web::json::value_of;

        auto& resource = node_resource;
        const auto at = value::string(nmos::make_version(activation_time));

        // "The 'receiver_id' key MUST be set to `null` in all cases except where a unicast push-based Sender is configured to transmit to an NMOS Receiver, and the 'active' key is set to 'true'."
        // "The 'sender_id' key MUST be set to `null` in all cases except where the Receiver is currently configured to receive from an NMOS Sender, and the 'active' key is set to 'true'.
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2.2/docs/4.3.%20Behaviour%20-%20Nodes.md#api-resources
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

    // Validate and parse the specified transport file for the specified receiver
    web::json::value parse_rtp_transport_file(const nmos::resource& receiver, const nmos::resource& connection_receiver, const utility::string_t& transport_file_type, const utility::string_t& transport_file_data, slog::base_gate& gate)
    {
        if (transport_file_type != U("application/sdp"))
        {
            throw std::runtime_error("unexpected type: " + utility::us2s(transport_file_type));
        }

        const auto session_description = sdp::parse_session_description(utility::us2s(transport_file_data));
        auto sdp_transport_params = nmos::parse_session_description(session_description);

        // Validate transport file according to the IS-04 receiver

        validate_sdp_parameters(receiver.data, sdp_transport_params.first);

        // could equally use nmos::fields::interface_bindings(receiver.data) or nmos::fields::transport_params(nmos::fields::endpoint_staged(connection_receiver.data))
        const auto legs = nmos::fields::endpoint_constraints(connection_receiver.data).size();

        if (1 == legs && 2 == sdp_transport_params.second.size())
        {
            web::json::pop_back(sdp_transport_params.second);
        }

        // "Where a Receiver supports SMPTE 2022-7 but is required to Receive a non-SMPTE 2022-7 stream,
        // only the first set of transport parameters should be used. rtp_enabled in the second set of parameters
        // must be set to false"
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#operation-with-smpte-2022-7
        if (2 == legs && 1 == sdp_transport_params.second.size())
        {
            web::json::push_back(sdp_transport_params.second, web::json::value_of({ { U("rtp_enabled"), false } }));
        }

        return sdp_transport_params.second;
    }

    // "On activation all instances of "auto" should be resolved into the actual values that will be used"
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/ConnectionAPI.raml#L300-L301
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_rtp.json
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_rtp.json
    // "In many cases this is a simple operation, and the behaviour is very clearly defined in the relevant transport parameter schemas.
    // For example a port number may be offset from the RTP port number by a pre-determined value. The specification makes suggestions
    // of a sensible default value for "auto" to resolve to, but the Sender or Receiver may choose any value permitted by the schema
    // and constraints."
    // This function implements those sensible defaults for the RTP transport type.
    // "In some cases the behaviour is more complex, and may be determined by the vendor."
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#use-of-auto
    // This function therefore does not select a value for e.g. sender "source_ip" or receiver "interface_ip".
    void resolve_rtp_auto(const nmos::type& type, web::json::value& transport_params, int auto_rtp_port)
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

    web::http::experimental::listener::api_router make_unmounted_connection_api(nmos::node_model& model, transport_file_parser parse_transport_file, details::connection_resource_patch_validator validate_merged, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router connection_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is05_versions::from_settings(model.settings); });
        connection_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        connection_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("bulk/"), U("single/") }, req, res));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/bulk/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("senders/"), U("receivers/") }, req, res));
            return pplx::task_from_result(true);
        });

        // "The API should actively return an HTTP 405 if a GET is called on the [/bulk/senders and /bulk/receivers] endpoint[s]."
        // This is provided for free by the api_router which also identifies the handler for POST as a "near miss"
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L39-L44
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml#L73-L78

        connection_api.support(U("/bulk/") + nmos::patterns::connectorType.pattern + U("/?"), methods::POST, [&model, parse_transport_file, validate_merged, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            return details::extract_json(req, gate).then([&model, req, res, parameters, parse_transport_file, validate_merged, gate](value body) mutable
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

                const auto handle_connection_resource_exception = [&](const std::pair<nmos::id, nmos::type>& id_type)
                {
                    // try-catch based on the exception handler in nmos::add_api_finally_handler
                    try
                    {
                        throw;
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "JSON error for " << id_type << " in bulk request: " << e.what();
                        return details::make_connection_resource_patch_error_response(status_codes::BadRequest, e);
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "HTTP error for " << id_type << " in bulk request: " << e.what() << " [" << e.error_code() << "]";
                        return details::make_connection_resource_patch_error_response(status_codes::BadRequest, e);
                    }
                    catch (const std::runtime_error& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error for " << id_type << " in bulk request: " << e.what();
                        return details::make_connection_resource_patch_error_response(status_codes::NotImplemented, e);
                    }
                    catch (const std::logic_error& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error for " << id_type << " in bulk request: " << e.what();
                        return details::make_connection_resource_patch_error_response(status_codes::InternalError, e);
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception for " << id_type << " in bulk request: " << e.what();
                        return details::make_connection_resource_patch_error_response(status_codes::InternalError, e);
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception for " << id_type << " in bulk request";
                        return details::make_connection_resource_patch_error_response(status_codes::InternalError);
                    }
                };

                for (auto& patch : patches)
                {
                    const auto id = nmos::fields::id(patch);

                    details::connection_resource_patch_response result;

                    try
                    {
                        result = details::handle_connection_resource_patch(model, lock, version, { id, type }, patch[nmos::fields::params], request_time, parse_transport_file, validate_merged, gate);
                    }
                    catch (...)
                    {
                        result = handle_connection_resource_exception({ id, type });
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
                                result = handle_connection_resource_exception({ id, type });
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
                    web::json::serialize_array(results
                        | boost::adaptors::transformed(
                            [](const details::connection_resource_patch_response& result) { return result.second; }
                        )),
                    web::http::details::mime_types::application_json);
                return true;
            });
        });

        connection_api.support(U("/single/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("senders/"), U("receivers/") }, req, res));
            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);

            const auto match = [&](const nmos::resource& resource)
            {
                auto matching_resource = find_resource(model.node_resources, { resource.id, resource.type });
                if (model.node_resources.end() == matching_resource)
                {
                    throw std::logic_error("matching IS-04 resource not found");
                }

                return resource.type == nmos::type_from_resourceType(resourceType) && nmos::is_connection_api_permitted_downgrade(*matching_resource, resource, version);
            };

            size_t count = 0;

            // experimental extension, to support human-readable HTML rendering of NMOS responses
            if (experimental::details::is_html_response_preferred(req, web::http::details::mime_types::application_json))
            {
                set_reply(res, status_codes::OK,
                    web::json::serialize_array(resources
                        | boost::adaptors::filtered(match)
                        | boost::adaptors::transformed(
                            [&count, &req](const nmos::resource& resource) { ++count; return experimental::details::make_html_response_a_tag(resource.id + U("/"), req); }
                        )),
                    web::http::details::mime_types::application_json);
            }
            else
            {
                set_reply(res, status_codes::OK,
                    web::json::serialize_array(resources
                        | boost::adaptors::filtered(match)
                        | boost::adaptors::transformed(
                            [&count](const nmos::resource& resource) { ++count; return value(resource.id + U("/")); }
                        )
                    ),
                    web::http::details::mime_types::application_json);
            }

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

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                auto matching_resource = find_resource(model.node_resources, id_type);
                if (model.node_resources.end() == matching_resource)
                {
                    throw std::logic_error("matching IS-04 resource not found");
                }

                if (nmos::is_connection_api_permitted_downgrade(*matching_resource, *resource, version))
                {
                    std::set<utility::string_t> sub_routes{ U("constraints/"), U("staged/"), U("active/") };
                    if (nmos::types::sender == resource->type) sub_routes.insert(U("transportfile/"));

                    // The transporttype endpoint is introduced in v1.1
                    if (nmos::is05_versions::v1_1 <= version) sub_routes.insert(U("transporttype/"));

                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body(std::move(sub_routes), req, res));
                }
                else
                {
                    // experimental extension, proposed for v1.1, to distinguish from Not Found
                    set_error_reply(res, status_codes::Conflict, U("Conflict; ") + details::make_permitted_downgrade_error(*resource, version));
                    res.headers().add(web::http::header_names::location, make_connection_api_resource_location(*resource));
                }
            }
            else if (details::is_erased_resource(resources, id_type))
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + details::make_erased_resource_error());
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

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
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

                if (nmos::is_connection_api_permitted_downgrade(*matching_resource, *resource, version))
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning constraints for " << id_type;

                    // hmm, parsing of the Accept header could be much better and should take account of quality values
                    const auto accept = req.headers().find(web::http::header_names::accept);
                    if (req.headers().end() != accept && U("application/schema+json") == web::http::details::get_mime_type(accept->second))
                    {
                        // Experimental extension - constraints as JSON Schema

                        const nmos::transport transport_subclassification(nmos::fields::transport(matching_resource->data));
                        res.headers().set_content_type(U("application/schema+json"));
                        set_reply(res, status_codes::OK, nmos::details::make_constraints_schema(resource->type, nmos::fields::endpoint_constraints(resource->data), nmos::transport_base(transport_subclassification)));
                    }
                    else
                    {
                        set_reply(res, status_codes::OK, nmos::fields::endpoint_constraints(resource->data));
                    }
                }
                else
                {
                    // experimental extension, proposed for v1.1, to distinguish from Not Found
                    set_error_reply(res, status_codes::Conflict, U("Conflict; ") + details::make_permitted_downgrade_error(*resource, version));
                    res.headers().add(web::http::header_names::location, make_connection_api_resource_location(*resource, U("/constraints")));
                }
            }
            else if (details::is_erased_resource(resources, id_type))
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + details::make_erased_resource_error());
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/staged/?"), methods::PATCH, [&model, parse_transport_file, validate_merged, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            return details::extract_json(req, gate).then([&model, req, res, parameters, parse_transport_file, validate_merged, gate](value body) mutable
            {
                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Operation requested for single " << id_type;

                details::handle_connection_resource_patch(res, model, version, id_type, body, parse_transport_file, validate_merged, gate);

                return true;
            });
        });

        connection_api.support(U("/single/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/") + nmos::patterns::stagingType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.connection_resources;

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);
            const string_t stagingType = parameters.at(nmos::patterns::stagingType.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                auto matching_resource = find_resource(model.node_resources, id_type);
                if (model.node_resources.end() == matching_resource)
                {
                    throw std::logic_error("matching IS-04 resource not found");
                }

                if (nmos::is_connection_api_permitted_downgrade(*matching_resource, *resource, version))
                {
                    if (nmos::fields::endpoint_staged.key == stagingType)
                    {
                        const auto staged_state = details::get_activation_state(nmos::fields::activation(nmos::fields::endpoint_staged(resource->data)));

                        if (details::immediate_activation_pending == staged_state)
                        {
                            // "Any GET requests to `/staged` during this time [while an activation is in progress] MAY also be blocked until the activation is complete."
                            // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.0.%20Behaviour.md#in-progress-activations
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
                else
                {
                    // experimental extension, proposed for v1.1, to distinguish from Not Found
                    set_error_reply(res, status_codes::Conflict, U("Conflict; ") + details::make_permitted_downgrade_error(*resource, version));
                    res.headers().add(web::http::header_names::location, make_connection_api_resource_location(*resource, U("/") + stagingType));

                    return pplx::task_from_result(true);
                }
            }

            if (resources.end() != resource)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << stagingType << " data for " << id_type;

                const web::json::field_as_value endpoint_staging{ stagingType };
                set_reply(res, status_codes::OK, endpoint_staging(resource->data));
            }
            else if (details::is_erased_resource(resources, id_type))
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + details::make_erased_resource_error());
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

            const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };

            const auto accept = req.headers().find(web::http::header_names::accept);
            const auto accept_or_empty = req.headers().end() != accept ? accept->second : utility::string_t{};

            details::handle_connection_resource_transportfile(res, model, version, id_type, accept_or_empty, gate);

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

                if (nmos::is_connection_api_permitted_downgrade(*matching_resource, *resource, version))
                {
                    // "Returns the URN base for the transport type employed by this sender with any subclassifications or versions removed."
                    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/ConnectionAPI.raml#L349
                    const nmos::transport transport_subclassification(nmos::fields::transport(matching_resource->data));
                    set_reply(res, status_codes::OK, web::json::value::string(nmos::transport_base(transport_subclassification).name));
                }
                else
                {
                    // experimental extension, proposed for v1.1, to distinguish from Not Found
                    set_error_reply(res, status_codes::Conflict, U("Conflict; ") + details::make_permitted_downgrade_error(*resource, version));
                    res.headers().add(web::http::header_names::location, make_connection_api_resource_location(*resource, U("/transporttype")));
                }
            }
            else if (details::is_erased_resource(resources, id_type))
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + details::make_erased_resource_error());
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

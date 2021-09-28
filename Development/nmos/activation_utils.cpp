#include "nmos/activation_utils.h"

#include "cpprest/basic_utils.h"
#include "nmos/activation_mode.h"
#include "nmos/json_fields.h"
#include "nmos/model.h"
#include "slog/all_in_one.h"

namespace nmos
{
    // Construct a 'not pending' activation response object with all null values
    web::json::value make_activation()
    {
        using web::json::value;
        using web::json::value_of;

        return value_of({
            { nmos::fields::mode, value::null() },
            { nmos::fields::requested_time, value::null() },
            { nmos::fields::activation_time, value::null() }
        });
    }

    namespace details
    {
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

        // Set the appropriate fields of the response/staged activation from the specified request
        void merge_activation(web::json::value& activation, const web::json::value& request_activation, const nmos::tai& request_time)
        {
            using web::json::value;

            switch (details::get_activation_state(request_activation))
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

                // Each of these fields "returns to null [...] when the resource is unlocked by setting the activation mode to null."
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-activation-response-schema.json
                // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/activation-response-schema.json
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

                activation[nmos::fields::mode] = request_activation.at(nmos::fields::mode);
                activation[nmos::fields::requested_time] = request_activation.at(nmos::fields::requested_time);

                // "For scheduled activations `activation_time` should be the absolute TAI time the parameters will actually transition."
                // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/ConnectionAPI.raml
                auto absolute_requested_time = get_absolute_requested_time(activation, request_time);
                activation[nmos::fields::activation_time] = value::string(nmos::make_version(absolute_requested_time));

                break;
            }
        }

        // this is bit of a dirty hack to support both Connection API and Channel Mapping API
        // without passing additional arguments around
        inline nmos::resources& get_resources_for_type(nmos::node_model& model, const nmos::type& type)
        {
            return nmos::types::input == type || nmos::types::output == type
                ? model.channelmapping_resources
                : model.connection_resources;
        }

        struct immediate_activation_not_pending
        {
            nmos::node_model& model;
            std::pair<nmos::id, nmos::type> id_type;

            inline bool operator()() const
            {
                if (model.shutdown) return true;

                auto& resources = get_resources_for_type(model, id_type.second);

                auto resource = find_resource(resources, id_type);
                if (resources.end() == resource) return true;

                auto& mode = nmos::fields::mode(nmos::fields::activation(nmos::fields::endpoint_staged(resource->data)));
                return mode.is_null() || mode.as_string() != nmos::activation_modes::activate_immediate.name;
            }
        };

        // wait for the staged activation mode of the specified resource to be set to something other than "activate_immediate"
        // or for the resource to have vanished (unexpected!)
        // or for the server to be shut down
        // or for the timeout to expire
        bool wait_immediate_activation_not_pending(nmos::node_model& model, nmos::read_lock& lock, const std::pair<nmos::id, nmos::type>& id_type)
        {
            return model.wait_for(lock, std::chrono::seconds(nmos::fields::immediate_activation_max(model.settings)), immediate_activation_not_pending{ model, id_type });
        }

        bool wait_immediate_activation_not_pending(nmos::node_model& model, nmos::write_lock& lock, const std::pair<nmos::id, nmos::type>& id_type)
        {
            return model.wait_for(lock, std::chrono::seconds(nmos::fields::immediate_activation_max(model.settings)), immediate_activation_not_pending{ model, id_type });
        }

        // wait for the staged activation of the specified resource to have changed
        // or for the resource to have vanished (unexpected!)
        // or for the server to be shut down
        // or for the timeout to expire
        bool wait_activation_modified(nmos::node_model& model, nmos::write_lock& lock, std::pair<nmos::id, nmos::type> id_type, web::json::value initial_activation)
        {
            return model.wait_for(lock, std::chrono::seconds(nmos::fields::immediate_activation_max(model.settings)), [&]
            {
                if (model.shutdown) return true;

                auto& resources = get_resources_for_type(model, id_type.second);

                auto resource = find_resource(resources, id_type);
                if (resources.end() == resource) return true;

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

            auto& resources = get_resources_for_type(model, id_type.second);

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
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying API - immediate activation completed";
            model.notify();
        }
    }
}

#include "nmos/streamcompatibility_utils.h"

#include "cpprest/basic_utils.h" // for utility::us2s

namespace nmos
{
    namespace experimental
    {
        // it's expected that write lock is already catched for the model
        bool update_version(nmos::resources& resources, const nmos::id& resource_id, const utility::string_t& new_version)
        {
            using web::json::value;

            return modify_resource(resources, resource_id, [&new_version](nmos::resource& resource)
            {
                resource.data[nmos::fields::version] = value::string(new_version);
            });
        }

        // it's expected that write lock is already catched for the model
        bool update_version(nmos::resources& resources, const web::json::array& resource_ids, const utility::string_t& new_version)
        {
            for (const auto& resource_id : resource_ids)
            {
                if (!update_version(resources, resource_id.as_string(), new_version))
                {
                    return false;
                }
            }
            return true;
        }

        bool set_sender_status(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& sender_id, const nmos::sender_state& state, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_of;

            bool result{ false };
            const auto version = nmos::make_version();
            try
            {
                result = modify_resource(streamcompatibility_resources, sender_id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::status] = value_of({ { nmos::fields::state, state.name } });
                    resource.data[nmos::fields::version] = value::string(version);
                });

                if (result)
                {
                    result = update_version(node_resources, sender_id, version);
                }
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set sender " << utility::us2s(sender_id) << " status: {state=" << utility::us2s(state.name) << "}  error: " << e.what();
                return false;
            }

            return result;
        }

        bool set_receiver_status(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& receiver_id, const nmos::receiver_state& state, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_of;

            bool result{ false };
            const auto version = nmos::make_version();
            try
            {
                result = modify_resource(streamcompatibility_resources, receiver_id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::status] = value_of({ { nmos::fields::state, state.name } });
                    resource.data[nmos::fields::version] = value::string(version);
                });

                if (result)
                {
                    result = update_version(node_resources, receiver_id, version);
                }
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set receiver: " << utility::us2s(receiver_id) << " status: {state=" << utility::us2s(state.name) << "}  error: " << e.what();
                return false;
            }

            return result;
        }

        bool set_sender_inputs(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& sender_id, const std::vector<nmos::id>& input_ids, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_from_elements;

            bool result{ false };
            const auto version = nmos::make_version();
            try
            {
                result = modify_resource(streamcompatibility_resources, sender_id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::inputs] = value_from_elements(input_ids);
                    resource.data[nmos::fields::version] = value::string(version);
                });

                if (result)
                {
                    result = update_version(node_resources, sender_id, version);
                }
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set sender: " << utility::us2s(sender_id) << " inputs error: " << e.what();
                return false;
            }

            return result;

        }

        bool set_receiver_outputs(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& receiver_id, const std::vector<nmos::id>& output_ids, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_from_elements;

            bool result{ false };
            const auto version = nmos::make_version();
            try
            {
                result = modify_resource(streamcompatibility_resources, receiver_id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::outputs] = value_from_elements(output_ids);
                    resource.data[nmos::fields::version] = value::string(version);
                });

                if (result)
                {
                    result = update_version(node_resources, receiver_id, version);
                }
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set receiver: " << utility::us2s(receiver_id) << " outputs error: " << e.what();
                return false;
            }

            return result;

        }
    }
}

#include "nmos/streamcompatibility_utils.h"

#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/basic_utils.h" // for utility::us2s

namespace nmos
{
    namespace experimental
    {
        void update_version(nmos::resources& resources, const nmos::id& resource_id, const utility::string_t& version)
        {
            using web::json::value;

            modify_resource(resources, resource_id, [&version](nmos::resource& resource)
            {
                resource.data[nmos::fields::version] = value::string(version);
            });
        }

        void update_version(nmos::resources& resources, const std::set<nmos::id>& resource_ids, const utility::string_t& version)
        {
            for (const auto& resource_id : resource_ids)
            {
                update_version(resources, resource_id, version);
            }
        }

        bool set_sender_status(resources& node_resources, resources& streamcompatibility_resources, const nmos::id& sender_id, const nmos::sender_state& state, const utility::string_t& state_debug, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_of;

            const auto version = nmos::make_version();
            try
            {
                modify_resource(streamcompatibility_resources, sender_id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::status] = value_of({ { nmos::fields::state, state.name } });

                    if (!state_debug.empty())
                    {
                        nmos::fields::status(resource.data)[nmos::fields::debug] = value::string(state_debug);
                    }
                    else if (nmos::fields::status(resource.data).has_field(nmos::fields::debug) && !nmos::fields::debug(nmos::fields::status(resource.data)).empty())
                    {
                        nmos::fields::status(resource.data).erase(nmos::fields::debug);
                    }

                    resource.data[nmos::fields::version] = value::string(version);
                });

                // `When State of Sender is changed, then the version attribute of the relevant IS-04 Sender MUST be incremented.`
                // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
                update_version(node_resources, sender_id, version);

                // hmmmm, should the version of the associated Device be incremented?
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set sender " << utility::us2s(sender_id) << " status: {state=" << utility::us2s(state.name) << "}  error: " << e.what();
                return false;
            }
            return true;
        }

        bool set_receiver_status(resources& node_resources, resources& streamcompatibility_resources, const nmos::id& receiver_id, const nmos::receiver_state& state, const utility::string_t& state_debug, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_of;

            const auto version = nmos::make_version();
            try
            {
                modify_resource(streamcompatibility_resources, receiver_id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::status] = value_of({ { nmos::fields::state, state.name } });

                    if (!state_debug.empty())
                    {
                        nmos::fields::status(resource.data)[nmos::fields::debug] = value::string(state_debug);
                    }
                    else if (nmos::fields::status(resource.data).has_field(nmos::fields::debug) && !nmos::fields::debug(nmos::fields::status(resource.data)).empty())
                    {
                        nmos::fields::status(resource.data).erase(nmos::fields::debug);
                    }

                    resource.data[nmos::fields::version] = value::string(version);
                });

                // `When State of Receiver is changed, then the version attribute of the relevant IS-04 Receiver MUST be incremented.`
                // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
                update_version(node_resources, receiver_id, version);

                // hmmmm, should the version of the associated Device be incremented?
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set receiver: " << utility::us2s(receiver_id) << " status: {state=" << utility::us2s(state.name) << "}  error: " << e.what();
                return false;
            }
            return true;
        }

        bool set_sender_inputs(resources& node_resources, resources& streamcompatibility_resources, const nmos::id& sender_id, const std::vector<nmos::id>& input_ids, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_from_elements;

            const auto version = nmos::make_version();
            try
            {
                modify_resource(streamcompatibility_resources, sender_id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::inputs] = value_from_elements(input_ids);
                    resource.data[nmos::fields::version] = value::string(version);
                });

                // `When the set of Inputs associated with the Sender is changed, then the version attribute of the relevant IS-04 Sender MUST be incremented.`
                // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
                update_version(node_resources, sender_id, version);

                // hmmmm, should the version of the associated Device be incremented?
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set sender: " << utility::us2s(sender_id) << " inputs error: " << e.what();
                return false;
            }
            return true;
        }

        bool set_receiver_outputs(resources& node_resources, resources& streamcompatibility_resources, const nmos::id& receiver_id, const std::vector<nmos::id>& output_ids, slog::base_gate& gate)
        {
            using web::json::value;
            using web::json::value_from_elements;

            const auto version = nmos::make_version();
            try
            {
                modify_resource(streamcompatibility_resources, receiver_id, [&](nmos::resource& resource)
                {
                    resource.data[nmos::fields::outputs] = value_from_elements(output_ids);
                    resource.data[nmos::fields::version] = value::string(version);
                });

                // `When the set of Outputs associated with the Receiver is changed, then the version attribute of the relevant IS-04 Receiver MUST be incremented.`
                // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
                update_version(node_resources, receiver_id, version);

                // hmmmm, should the version of the associated Device be incremented?
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Set receiver: " << utility::us2s(receiver_id) << " outputs error: " << e.what();
                return false;
            }
            return true;
        }
    }
}

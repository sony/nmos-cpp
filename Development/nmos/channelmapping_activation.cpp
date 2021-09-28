#include "nmos/channelmapping_activation.h"

#include "detail/for_each_reversed.h"
#include "nmos/activation_mode.h"
#include "nmos/channelmapping_api.h" // for nmos::set_channelmapping_output_active, etc.
#include "nmos/model.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h"

namespace nmos
{
    // cf. nmos/connection_activation.cpp
    void channelmapping_activation_thread(nmos::node_model& model, channelmapping_activation_handler channelmapping_activated, slog::base_gate& gate)
    {
        using web::json::value;
        using web::json::value_of;

        auto lock = model.write_lock(); // in order to update the resources

        auto most_recent_update = nmos::tai_min();
        auto earliest_scheduled_activation = (nmos::tai_clock::time_point::max)();

        for (;;)
        {
            // wait for the thread to be interrupted because there may be new scheduled activations, or immediate activations to process
            // or because the server is being shut down
            // or because it's time for the next scheduled activation
            model.wait_until(lock, earliest_scheduled_activation, [&] { return model.shutdown || most_recent_update < nmos::most_recent_update(model.channelmapping_resources); });
            if (model.shutdown) break;

            auto& by_updated = model.channelmapping_resources.get<nmos::tags::updated>();

            // go through all channelmapping resources
            // process any immediate activations
            // process any scheduled activations whose requested_time has passed
            // identify the next scheduled activation

            const auto now = nmos::tai_clock::now();

            earliest_scheduled_activation = (nmos::tai_clock::time_point::max)();

            bool notify = false;

            // since modify reorders the resource in this index, use for_each_reversed
            detail::for_each_reversed(by_updated.begin(), by_updated.end(), [&](const nmos::resource& resource)
            {
                if (!resource.has_data()) return;

                if (nmos::types::output != resource.type) return;

                const std::pair<nmos::id, nmos::type> id_type{ resource.id, resource.type };

                auto& staged = nmos::fields::endpoint_staged(resource.data);
                auto& staged_activation = nmos::fields::activation(staged);
                auto& staged_mode_or_null = nmos::fields::mode(staged_activation);

                if (staged_mode_or_null.is_null()) return;

                const nmos::activation_mode staged_mode{ staged_mode_or_null.as_string() };

                if (nmos::activation_modes::activate_scheduled_absolute == staged_mode ||
                    nmos::activation_modes::activate_scheduled_relative == staged_mode)
                {
                    auto& staged_activation_time = nmos::fields::activation_time(staged_activation);
                    const auto scheduled_activation = nmos::time_point_from_tai(nmos::parse_version(staged_activation_time.as_string()));

                    if (scheduled_activation < now)
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Processing scheduled channel mapping activation for " << id_type;
                    }
                    else
                    {
                        if (scheduled_activation < earliest_scheduled_activation)
                        {
                            earliest_scheduled_activation = scheduled_activation;
                        }

                        return;
                    }
                }
                else if (nmos::activation_modes::activate_immediate == staged_mode)
                {
                    // check for cancelled in-flight immediate activation
                    if (nmos::fields::requested_time(staged_activation).is_null()) return;
                    // check for processed in-flight immediate activation
                    if (!nmos::fields::activation_time(staged_activation).is_null()) return;

                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Processing immediate channel mapping activation for " << id_type;
                }
                else
                {
                    slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected channel mapping activation mode for " << id_type;
                    return;
                }

                // hmm, should all outputs that are actioned part of the same activation get the same activation time or not?
                const auto activation_time = nmos::tai_now();

                nmos::id source_id;

                // Update the IS-08 and IS-04 resources

                // ensure to 'unlock' an immediate activation resource for the Channel Mapping API implementation
                // after an exception; this will cause a 500 Internal Error status code to be returned
                const auto handle_channelmapping_activation_exception = [&](const std::pair<nmos::id, nmos::type>& id_type)
                {
                    // try-catch based on the exception handler in nmos::add_api_finally_handler
                    try
                    {
                        throw;
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error for " << id_type << " during channel mapping activation: " << e.what();
                    }
                    catch (const std::runtime_error& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error for " << id_type << " during channel mapping activation: " << e.what();
                    }
                    catch (const std::logic_error& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error for " << id_type << " during channel mapping activation: " << e.what();
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception for " << id_type << " during channel mapping activation: " << e.what();
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception for " << id_type << " during channel mapping activation";
                    }

                    nmos::modify_resource(model.channelmapping_resources, id_type.first, [&](nmos::resource& output)
                    {
                        nmos::set_channelmapping_output_not_pending(output);
                    });
                };

                try
                {
                    nmos::modify_resource(model.channelmapping_resources, id_type.first, [&](nmos::resource& output)
                    {
                        nmos::set_channelmapping_output_active(output, activation_time);
                    });

                    // Update the IS-04 source's version

                    auto& source_id_or_null = nmos::fields::endpoint_io(resource.data).at(nmos::fields::source_id);

                    if (!source_id_or_null.is_null())
                    {
                        nmos::modify_resource(model.node_resources, source_id_or_null.as_string(), [&activation_time](nmos::resource& source)
                        {
                            nmos::set_resource_version(source, activation_time);
                        });
                    }

                    // Synchronous notification that the active map for the specified IS-08 output has changed

                    if (channelmapping_activated)
                    {
                        // this callback should not throw exceptions, as the active map will already have been changed and those changes will not be rolled back
                        channelmapping_activated(resource);
                    }
                }
                catch (...)
                {
                    handle_channelmapping_activation_exception(id_type);
                }

                notify = true;
            });

            if (notify)
            {
                // Update the IS-04 devices' versions

                // At the moment, it doesn't seem necessary to enable support multiple API instances via the API selector mechanism
                // so therefore just a single Channel Mapping API instance is mounted directly at /x-nmos/channelmapping/{version}/
                // If it becomes necessary, each device could associated with a specific API selector
                // See https://github.com/AMWA-TV/nmos-audio-channel-mapping/blob/v1.0.x/docs/2.0.%20APIs.md#api-paths

                // hmm, should all devices get the same activation time or not?
                const auto activation_time = nmos::tai_now();

                auto& by_type = model.node_resources.get<nmos::tags::type>();
                const auto devices = by_type.equal_range(nmos::details::has_data(nmos::types::device));

                for (auto device = devices.first; devices.second != device; ++device)
                {
                    nmos::modify_resource(model.node_resources, device->id, [&activation_time](nmos::resource& source)
                    {
                        nmos::set_resource_version(source, activation_time);
                    });
                }
            }

            if ((nmos::tai_clock::time_point::max)() != earliest_scheduled_activation)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Next scheduled channel mapping activation is at " << nmos::make_version(nmos::tai_from_time_point(earliest_scheduled_activation))
                    << " in about " << std::fixed << std::setprecision(3) << std::chrono::duration_cast<std::chrono::duration<double>>(earliest_scheduled_activation - now).count() << " seconds time";
            }

            if (notify)
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying node behaviour thread"; // and anyone else who cares...
                model.notify();
            }

            most_recent_update = nmos::most_recent_update(model.channelmapping_resources);
        }
    }
}

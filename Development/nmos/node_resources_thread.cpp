#include "nmos/node_resources_thread.h"

#include "nmos/model.h"
#include "nmos/node_resource.h"
#include "nmos/node_resources.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h"
#include "nmos/transport.h"

namespace nmos
{
    namespace experimental
    {
        // insert a node resource, and sub-resources, according to the settings, and then wait for sender/receiver activations or shutdown
        void node_resources_thread(nmos::node_model& model, slog::base_gate& gate)
        {
            const auto seed_id = nmos::with_read_lock(model.mutex, [&] { return nmos::experimental::fields::seed_id(model.settings); });
            auto node_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/self"));
            auto device_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/device/0"));
            auto source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/0"));
            auto flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/0"));
            auto sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/0"));
            auto receiver_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/receiver/0"));

            auto lock = model.write_lock(); // in order to update the resources

            // any delay between updates to the model resources is unnecessary
            // this just serves as a slightly more realistic example!
            const unsigned int delay_millis{ 50 };

            const auto insert_resource_after = [&](unsigned int milliseconds, nmos::resource&& resource, slog::base_gate& gate)
            {
                // using wait_until rather than wait_for as a workaround for an awful bug in VS2015, resolved in VS2017
                if (!model.shutdown_condition.wait_until(lock, std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds), [&] { return model.shutdown; }))
                {
                    const std::pair<nmos::id, nmos::type> id_type{ resource.id, resource.type };
                    const bool success = insert_resource(model.node_resources, std::move(resource)).second;

                    if (success)
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Updated model with " << id_type;
                    else
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Model update error: " << id_type;

                    if (nmos::types::sender == id_type.second) insert_resource(model.connection_resources, make_connection_sender(id_type.first, {}));
                    else if (nmos::types::receiver == id_type.second) insert_resource(model.connection_resources, make_connection_receiver(id_type.first));

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying node behaviour thread"; // and anyone else who cares...
                    model.notify();
                }
            };

            insert_resource_after(delay_millis, make_node(node_id, model.settings), gate);
            insert_resource_after(delay_millis, make_device(device_id, node_id, { sender_id }, { receiver_id }, model.settings), gate);
            insert_resource_after(delay_millis, make_video_source(source_id, device_id, { 25, 1 }, model.settings), gate);
            insert_resource_after(delay_millis, make_raw_video_flow(flow_id, source_id, device_id, model.settings), gate);
            insert_resource_after(delay_millis, make_sender(sender_id, flow_id, device_id, {}, model.settings), gate);
            insert_resource_after(delay_millis, make_video_receiver(receiver_id, device_id, nmos::transports::rtp_mcast, {}, model.settings), gate);

            model.shutdown_condition.wait(lock, [&] { return model.shutdown; });
        }
    }
}

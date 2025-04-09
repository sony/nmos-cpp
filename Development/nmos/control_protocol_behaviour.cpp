#include "nmos/control_protocol_behaviour.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        struct domain_status_properties
        {
            domain_status_properties(const nc_property_id& status_property_id, const nc_property_id& status_message_property_id, const nc_property_id& status_transition_counter_property_id, const utility::string_t& status_pending_field_name, const utility::string_t& status_message_pending_field_name, const utility::string_t& status_pending_received_time_field_name)
                : status_property_id(status_property_id)
                , status_message_property_id(status_message_property_id)
                , status_transition_counter_property_id(status_transition_counter_property_id)
                , status_pending_field_name(status_pending_field_name)
                , status_message_pending_field_name(status_message_pending_field_name)
                , status_pending_received_time_field_name(status_pending_received_time_field_name)
            {}

            nc_property_id status_property_id;
            nc_property_id status_message_property_id;
            nc_property_id status_transition_counter_property_id;
            utility::string_t status_pending_field_name;
            utility::string_t status_message_pending_field_name;
            utility::string_t status_pending_received_time_field_name;
        };

        void control_protocol_behaviour_thread(nmos::node_model& model, control_protocol_state& state, slog::base_gate& gate_)
        {
            nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::control_protocol_behaviour));

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting control protocol behaviour thread";

            std::vector<domain_status_properties> domain_statuses;
            domain_statuses.push_back(domain_status_properties(nc_receiver_monitor_stream_status_property_id, nc_receiver_monitor_stream_status_message_property_id, nc_receiver_monitor_stream_status_transition_counter_property_id, nmos::fields::nc::stream_status_pending, nmos::fields::nc::stream_status_message_pending, nmos::fields::nc::stream_status_pending_received_time));
            domain_statuses.push_back(domain_status_properties(nc_receiver_monitor_connection_status_property_id, nc_receiver_monitor_connection_status_message_property_id, nc_receiver_monitor_connection_status_transition_counter_property_id, nmos::fields::nc::connection_status_pending, nmos::fields::nc::connection_status_message_pending, nmos::fields::nc::connection_status_pending_received_time));
            domain_statuses.push_back(domain_status_properties(nc_receiver_monitor_link_status_property_id, nc_receiver_monitor_link_status_message_property_id, nc_receiver_monitor_link_status_transition_counter_property_id, nmos::fields::nc::link_status_pending, nmos::fields::nc::link_status_message_pending, nmos::fields::nc::link_status_pending_received_time));
            domain_statuses.push_back(domain_status_properties(nc_receiver_monitor_external_synchronization_status_property_id, nc_receiver_monitor_external_synchronization_status_message_property_id, nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, nmos::fields::nc::external_synchronization_status_pending, nmos::fields::nc::external_synchronization_status_message_pending, nmos::fields::nc::external_synchronization_status_pending_received_time));

            auto lock = model.read_lock();
            auto& condition = model.condition;

            auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(state);
            // continue until the server is being shut down
            for (;;)
            {
                condition.wait(lock, [&] { return model.shutdown || state.receiver_monitor_status_pending; });

                if (model.shutdown) break;

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Receiver monitor status pending";

                // Check statuses of receivers
                // Get root block
                auto found = nmos::find_resource_if(model.control_protocol_resources, nmos::types::nc_block, [&](const nmos::resource& resource)
                    {
                        return nmos::root_block_oid == nmos::fields::nc::oid(resource.data);
                    });

                // Get all receiver monitors
                auto descriptors = web::json::value::array();
                nmos::find_members_by_class_id(model.control_protocol_resources, *found, nmos::nc_receiver_monitor_class_id, true, true, descriptors.as_array());

                bool receiver_monitors_updates_pending = false;

                auto current_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

                for (const auto& descriptor : descriptors.as_array())
                {
                    auto oid = nmos::fields::nc::oid(descriptor);

                    auto status_reporting_delay = get_control_protocol_property(model.control_protocol_resources, oid, nc_status_monitor_status_reporting_delay, get_control_protocol_class_descriptor, gate);

                    for (const auto& domain_status : domain_statuses)
                    {
                        auto received_time = get_control_protocol_property(model.control_protocol_resources, oid, domain_status.status_pending_received_time_field_name, gate);

                        if (received_time.as_integer() > 0)
                        {
                            auto threshold_time = static_cast<long long>(received_time.as_integer()) + status_reporting_delay.as_integer();

                            if (current_time > threshold_time)
                            {
                                // copy pending status to status property
                                auto status = get_control_protocol_property(model.control_protocol_resources, oid, domain_status.status_pending_field_name, gate);
                                auto status_message = get_control_protocol_property(model.control_protocol_resources, oid, domain_status.status_message_pending_field_name, gate);

                                details::set_receiver_monitor_status(model.control_protocol_resources, oid, status, status_message.as_string(),
                                    domain_status.status_property_id,
                                    domain_status.status_message_property_id,
                                    domain_status.status_transition_counter_property_id,
                                    domain_status.status_pending_received_time_field_name,
                                    state, get_control_protocol_class_descriptor, gate);

                                model.notify();
                            }
                            else
                            {
                                receiver_monitors_updates_pending = true;
                            }
                        }
                    }
                }
                {
                    auto lock = state.write_lock();
                    if (!receiver_monitors_updates_pending)
                    {
                        state.receiver_monitor_status_pending = false;
                    }
                }
                model.wait_for(lock, bst::chrono::milliseconds(bst::chrono::milliseconds::rep(1000)), [&] { return model.shutdown; });
            }
        }
    }
}
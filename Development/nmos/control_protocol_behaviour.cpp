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

            std::vector<domain_status_properties> receiver_monitor_domain_statuses;
            receiver_monitor_domain_statuses.push_back(domain_status_properties(nc_receiver_monitor_stream_status_property_id, nc_receiver_monitor_stream_status_message_property_id, nc_receiver_monitor_stream_status_transition_counter_property_id, nmos::fields::nc::stream_status_pending, nmos::fields::nc::stream_status_message_pending, nmos::fields::nc::stream_status_pending_received_time));
            receiver_monitor_domain_statuses.push_back(domain_status_properties(nc_receiver_monitor_connection_status_property_id, nc_receiver_monitor_connection_status_message_property_id, nc_receiver_monitor_connection_status_transition_counter_property_id, nmos::fields::nc::connection_status_pending, nmos::fields::nc::connection_status_message_pending, nmos::fields::nc::connection_status_pending_received_time));
            receiver_monitor_domain_statuses.push_back(domain_status_properties(nc_receiver_monitor_link_status_property_id, nc_receiver_monitor_link_status_message_property_id, nc_receiver_monitor_link_status_transition_counter_property_id, nmos::fields::nc::link_status_pending, nmos::fields::nc::link_status_message_pending, nmos::fields::nc::link_status_pending_received_time));
            receiver_monitor_domain_statuses.push_back(domain_status_properties(nc_receiver_monitor_external_synchronization_status_property_id, nc_receiver_monitor_external_synchronization_status_message_property_id, nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, nmos::fields::nc::external_synchronization_status_pending, nmos::fields::nc::external_synchronization_status_message_pending, nmos::fields::nc::external_synchronization_status_pending_received_time));

            std::vector<domain_status_properties> sender_monitor_domain_statuses;
            sender_monitor_domain_statuses.push_back(domain_status_properties(nc_sender_monitor_essence_status_property_id, nc_sender_monitor_essence_status_message_property_id, nc_sender_monitor_essence_status_transition_counter_property_id, nmos::fields::nc::essence_status_pending, nmos::fields::nc::essence_status_message_pending, nmos::fields::nc::essence_status_pending_received_time));
            sender_monitor_domain_statuses.push_back(domain_status_properties(nc_sender_monitor_transmission_status_property_id, nc_sender_monitor_transmission_status_message_property_id, nc_sender_monitor_transmission_status_transition_counter_property_id, nmos::fields::nc::transmission_status_pending, nmos::fields::nc::transmission_status_message_pending, nmos::fields::nc::transmission_status_pending_received_time));
            sender_monitor_domain_statuses.push_back(domain_status_properties(nc_sender_monitor_link_status_property_id, nc_sender_monitor_link_status_message_property_id, nc_sender_monitor_link_status_transition_counter_property_id, nmos::fields::nc::link_status_pending, nmos::fields::nc::link_status_message_pending, nmos::fields::nc::link_status_pending_received_time));
            sender_monitor_domain_statuses.push_back(domain_status_properties(nc_sender_monitor_external_synchronization_status_property_id, nc_sender_monitor_external_synchronization_status_message_property_id, nc_sender_monitor_external_synchronization_status_transition_counter_property_id, nmos::fields::nc::external_synchronization_status_pending, nmos::fields::nc::external_synchronization_status_message_pending, nmos::fields::nc::external_synchronization_status_pending_received_time));

            auto lock = model.write_lock();
            auto& condition = model.condition;
            auto& shutdown = model.shutdown;
            auto& control_protocol_resources = model.control_protocol_resources;

            auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(state);
            // continue until the server is being shut down
            for (;;)
            {
                condition.wait(lock, [&] { return shutdown || nmos::with_read_lock(state.mutex, [&] { return state.monitor_status_pending; }); });

                if (shutdown) break;

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Monitor status pending";

                // Check statuses of receivers and senders
                // Get root block
                const auto found = nmos::find_resource_if(control_protocol_resources, nmos::types::nc_block, [&](const nmos::resource& resource)
                {
                    return nmos::root_block_oid == nmos::fields::nc::oid(resource.data);
                });

                if (control_protocol_resources.end() != found) // ensure root block is present
                {
                    // Get all status monitors - including receiver and sender monitors
                    auto descriptors = web::json::value::array();
                    nmos::find_members_by_class_id(control_protocol_resources, *found, nmos::nc_status_monitor_class_id, true, true, descriptors.as_array());

                    bool monitors_updates_pending = false;

                    do
                    {
                        auto current_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

                        // find delay until next pending update
                        auto minimum_delay = LLONG_MAX;

                        for (const auto& descriptor : descriptors.as_array())
                        {
                            auto oid = nmos::fields::nc::oid(descriptor);
                            const auto& class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(descriptor));

                            auto status_reporting_delay = get_control_protocol_property(control_protocol_resources, oid, nc_status_monitor_status_reporting_delay, get_control_protocol_class_descriptor, gate);

                            const auto& domain_statuses = nmos::is_nc_sender_monitor(class_id) ? sender_monitor_domain_statuses : receiver_monitor_domain_statuses;

                            for (const auto& domain_status : domain_statuses)
                            {
                                auto received_time = get_control_protocol_property(control_protocol_resources, oid, domain_status.status_pending_received_time_field_name, gate);

                                if (received_time.as_integer() > 0)
                                {
                                    auto threshold_time = static_cast<long long>(received_time.as_integer()) + status_reporting_delay.as_integer();

                                    auto delay = std::max(threshold_time - current_time, static_cast<long long>(0));

                                    minimum_delay = std::min(delay, minimum_delay);

                                    monitors_updates_pending = true;
                                }
                            }
                        }

                        if (!monitors_updates_pending) continue;

                        // wait until pending update due
                        model.wait_for(lock, bst::chrono::seconds(bst::chrono::seconds::rep(minimum_delay)), [&] { return shutdown; });
                        if (shutdown) break;

                        current_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

                        monitors_updates_pending = false;

                        // update statuses
                        for (const auto& descriptor : descriptors.as_array())
                        {
                            const auto& oid = nmos::fields::nc::oid(descriptor);
                            const auto& class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(descriptor));

                            const auto status_reporting_delay = get_control_protocol_property(control_protocol_resources, oid, nc_status_monitor_status_reporting_delay, get_control_protocol_class_descriptor, gate);

                            const auto& domain_statuses = nmos::is_nc_sender_monitor(class_id) ? sender_monitor_domain_statuses : receiver_monitor_domain_statuses;

                            for (const auto& domain_status : domain_statuses)
                            {
                                const auto received_time = get_control_protocol_property(control_protocol_resources, oid, domain_status.status_pending_received_time_field_name, gate);

                                if (received_time.as_integer() > 0)
                                {
                                    auto threshold_time = static_cast<long long>(received_time.as_integer()) + status_reporting_delay.as_integer();

                                    if (current_time >= threshold_time)
                                    {
                                        // copy pending status to status property
                                        const auto& status = get_control_protocol_property(control_protocol_resources, oid, domain_status.status_pending_field_name, gate);
                                        const auto& status_message = get_control_protocol_property(control_protocol_resources, oid, domain_status.status_message_pending_field_name, gate);
                                        const auto& status_message_string = status_message == web::json::value::null() ? U("") : status_message.as_string();
                                        details::set_monitor_status(control_protocol_resources, oid, status, status_message_string,
                                            domain_status.status_property_id,
                                            domain_status.status_message_property_id,
                                            domain_status.status_transition_counter_property_id,
                                            domain_status.status_pending_received_time_field_name,
                                            get_control_protocol_class_descriptor,
                                            gate);

                                        model.notify();
                                    }
                                    else
                                    {
                                        monitors_updates_pending = true;
                                    }
                                }
                            }
                        }
                    } while (monitors_updates_pending);

                    if (shutdown) break;

                    if (!monitors_updates_pending)
                    {
                        auto lock = state.write_lock();
                        state.monitor_status_pending = false;
                        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "No more receiver/sender monitors statuses are pending";
                    }
                }
            }
        }
    }
}
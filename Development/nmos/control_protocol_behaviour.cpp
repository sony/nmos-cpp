#include "nmos/control_protocol_behaviour.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace nc
    {
        namespace details
        {
            // Declared here rather than in the public header; used only by the behaviour thread
            bool set_monitor_status_internal(resources& resources, nc_oid oid, int status, const utility::string_t& status_message, const nc_property_id& status_property_id,
                const nc_property_id& status_message_property_id,
                const nc_property_id& status_transition_counter_property_id,
                const utility::string_t& status_pending_received_time_field_name,
                get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor,
                get_monitor_domains_handler get_monitor_domains,
                slog::base_gate& gate);
        }
    }

    namespace experimental
    {
        void control_protocol_behaviour_thread(nmos::node_model& model, control_protocol_state& state, slog::base_gate& gate_)
        {
            nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::control_protocol_behaviour));

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting control protocol behaviour thread";

            auto lock = model.write_lock();
            auto& condition = model.condition;
            auto& shutdown = model.shutdown;
            auto& control_protocol_resources = model.control_protocol_resources;

            auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(state);
            auto get_monitor_domains = nmos::make_get_monitor_domains_handler(state);
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
                    nmos::nc::find_members_by_class_id(control_protocol_resources, *found, nmos::nc_status_monitor_class_id, true, true, descriptors.as_array());

                    bool monitors_updates_pending = false;

                    do
                    {
                        auto current_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

                        // find delay until next pending update
                        auto minimum_delay = LLONG_MAX;

                        for (const auto& descriptor : descriptors.as_array())
                        {
                            auto oid = nmos::fields::nc::oid(descriptor);
                            const auto& class_id = nc::details::parse_class_id(nmos::fields::nc::class_id(descriptor));

                            auto status_reporting_delay = nc::get_property(control_protocol_resources, oid, nc_status_monitor_status_reporting_delay, get_control_protocol_class_descriptor, gate);

                            const auto domain_statuses = nmos::nc::get_monitor_domains(class_id, get_monitor_domains);

                            for (const auto& domain_status : domain_statuses)
                            {
                                auto received_time = nc::get_property(control_protocol_resources, oid, domain_status.status_pending_received_time_field_name, gate);

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
                            const auto& class_id = nc::details::parse_class_id(nmos::fields::nc::class_id(descriptor));

                            const auto status_reporting_delay = nc::get_property(control_protocol_resources, oid, nc_status_monitor_status_reporting_delay, get_control_protocol_class_descriptor, gate);

                            const auto domain_statuses = nmos::nc::get_monitor_domains(class_id, get_monitor_domains);

                            for (const auto& domain_status : domain_statuses)
                            {
                                const auto received_time = nc::get_property(control_protocol_resources, oid, domain_status.status_pending_received_time_field_name, gate);

                                if (received_time.as_integer() > 0)
                                {
                                    auto threshold_time = static_cast<long long>(received_time.as_integer()) + status_reporting_delay.as_integer();

                                    if (current_time >= threshold_time)
                                    {
                                        // copy pending status to status property
                                        const auto& status = nc::get_property(control_protocol_resources, oid, domain_status.status_pending_field_name, gate);
                                        const auto& status_message = nc::get_property(control_protocol_resources, oid, domain_status.status_message_pending_field_name, gate);
                                        const auto& status_message_string = status_message == web::json::value::null() ? U("") : status_message.as_string();
                                        nc::details::set_monitor_status_internal(control_protocol_resources, oid, status.as_integer(), status_message_string,
                                            domain_status.status_property_id,
                                            domain_status.status_message_property_id,
                                            domain_status.status_transition_counter_property_id,
                                            domain_status.status_pending_received_time_field_name,
                                            get_control_protocol_class_descriptor,
                                            get_monitor_domains,
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
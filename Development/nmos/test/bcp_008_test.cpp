// The first "test" is of course whether the header compiles standalone
#include "boost/iostreams/stream.hpp"
#include "nmos/control_protocol_behaviour.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/log_gate.h"
#include "nmos/model.h"
#include "nmos/slog.h"

#include "bst/test/test.h"

//////////////////////////////////////////////////////////////////////////////////////////////
// Helper template for testing status transitions with different status types and monitor types
namespace
{
    // Monitor type enumeration
    enum class MonitorType
    {
        Receiver,
        Sender
    };

    // Status transition configuration
    template<typename StatusType>
    struct StatusTransitionConfig
    {
        StatusType status;
        utility::string_t message;
        bool immediate_update; // Whether expect immediate/delayed update before transitioning to the next status
    };

    // Traits for different status types to provide type-specific information
    template<typename StatusType, MonitorType MT>
    struct StatusTraits;

    // ========== Receiver Monitor Specializations ==========

    // Specialization for receiver monitor nc_link_status
    template<>
    struct StatusTraits<nmos::nc_link_status::status, MonitorType::Receiver>
    {
        static const char* test_name() { return "Receiver Link Status"; }
        static nmos::nc_link_status::status initial_status() { return nmos::nc_link_status::status::all_down; }
        static utility::string_t initial_message() { return U("Initial all down"); }

        static std::vector<StatusTransitionConfig<nmos::nc_link_status::status>> transitions()
        {
            return {
                { nmos::nc_link_status::status::some_down, U("Transitioning to some down"), false },
                { nmos::nc_link_status::status::all_up, U("Transitioning to all up"), false },
                { nmos::nc_link_status::status::some_down, U("Transitioning to some down"), true },
                { nmos::nc_link_status::status::all_down, U("Transitioning to all down"), true }
            };
        }

        static auto make_set_status_handler(nmos::resources& resources, nmos::experimental::control_protocol_state& state, slog::base_gate& gate)
        {
            return nmos::make_set_receiver_monitor_link_status_handler(resources, state, gate);
        }

        static auto get_status_property_id() { return nmos::nc_receiver_monitor_link_status_property_id; }
        static auto get_message_property_id() { return nmos::nc_receiver_monitor_link_status_message_property_id; }
        static auto get_pending_field_name() { return nmos::fields::nc::link_status_pending; }
        static auto get_pending_time_field_name() { return nmos::fields::nc::link_status_pending_received_time; }
    };

    // Specialization for receiver monitor nc_connection_status
    template<>
    struct StatusTraits<nmos::nc_connection_status::status, MonitorType::Receiver>
    {
        static const char* test_name() { return "Receiver Connection Status"; }
        static nmos::nc_connection_status::status initial_status() { return nmos::nc_connection_status::status::unhealthy; }
        static utility::string_t initial_message() { return U("Initial unhealthy status"); }

        static std::vector<StatusTransitionConfig<nmos::nc_connection_status::status>> transitions()
        {
            return {
                { nmos::nc_connection_status::status::partially_healthy, U("Transitioning to partially healthy"), false },
                { nmos::nc_connection_status::status::healthy, U("Transitioning to healthy"), false },
                { nmos::nc_connection_status::status::inactive, U("Transitioning to inactive"), true },
                { nmos::nc_connection_status::status::healthy, U("Transitioning to healthy"), true },
                { nmos::nc_connection_status::status::partially_healthy, U("Transitioning to partially healthy"), true },
                { nmos::nc_connection_status::status::unhealthy, U("Transitioning to unhealthy"), true }
            };
        }

        static auto make_set_status_handler(nmos::resources& resources, nmos::experimental::control_protocol_state& state, slog::base_gate& gate)
        {
            return nmos::make_set_receiver_monitor_connection_status_handler(resources, state, gate);
        }

        static auto get_status_property_id() { return nmos::nc_receiver_monitor_connection_status_property_id; }
        static auto get_message_property_id() { return nmos::nc_receiver_monitor_connection_status_message_property_id; }
        static auto get_pending_field_name() { return nmos::fields::nc::connection_status_pending; }
        static auto get_pending_time_field_name() { return nmos::fields::nc::connection_status_pending_received_time; }
    };

    // Specialization for receiver monitor nc_synchronization_status
    template<>
    struct StatusTraits<nmos::nc_synchronization_status::status, MonitorType::Receiver>
    {
        static const char* test_name() { return "Receiver External Synchronization Status"; }
        static nmos::nc_synchronization_status::status initial_status() { return nmos::nc_synchronization_status::status::unhealthy; }
        static utility::string_t initial_message() { return U("Initial unhealthy status"); }

        static std::vector<StatusTransitionConfig<nmos::nc_synchronization_status::status>> transitions()
        {
            return {
                { nmos::nc_synchronization_status::status::partially_healthy, U("Transitioning to partially healthy"), false },
                { nmos::nc_synchronization_status::status::healthy, U("Transitioning to healthy"), false },
                { nmos::nc_synchronization_status::status::not_used, U("Transitioning to not used"), true },
                { nmos::nc_synchronization_status::status::healthy, U("Transitioning to healthy"), true },
                { nmos::nc_synchronization_status::status::partially_healthy, U("Transitioning to partially healthy"), true },
                { nmos::nc_synchronization_status::status::unhealthy, U("Transitioning to unhealthy"), true }
            };
        }

        static auto make_set_status_handler(nmos::resources& resources, nmos::experimental::control_protocol_state& state, slog::base_gate& gate)
        {
            return nmos::make_set_receiver_monitor_external_synchronization_status_handler(resources, state, gate);
        }

        static auto get_status_property_id() { return nmos::nc_receiver_monitor_external_synchronization_status_property_id; }
        static auto get_message_property_id() { return nmos::nc_receiver_monitor_external_synchronization_status_message_property_id; }
        static auto get_pending_field_name() { return nmos::fields::nc::external_synchronization_status_pending; }
        static auto get_pending_time_field_name() { return nmos::fields::nc::external_synchronization_status_pending_received_time; }
    };

    // Specialization for receiver monitor nc_stream_status
    template<>
    struct StatusTraits<nmos::nc_stream_status::status, MonitorType::Receiver>
    {
        static const char* test_name() { return "Receiver Stream Status"; }
        static nmos::nc_stream_status::status initial_status() { return nmos::nc_stream_status::status::unhealthy; }
        static utility::string_t initial_message() { return U("Initial unhealthy status"); }

        static std::vector<StatusTransitionConfig<nmos::nc_stream_status::status>> transitions()
        {
            return {
                { nmos::nc_stream_status::status::partially_healthy, U("Transitioning to partially healthy"), false },
                { nmos::nc_stream_status::status::healthy, U("Transitioning to healthy"), false },
                { nmos::nc_stream_status::status::inactive, U("Transitioning to inactive"), true },
                { nmos::nc_stream_status::status::healthy, U("Transitioning to healthy"), true },
                { nmos::nc_stream_status::status::partially_healthy, U("Transitioning to partially healthy"), true },
                { nmos::nc_stream_status::status::unhealthy, U("Transitioning to unhealthy"), true }
            };
        }

        static auto make_set_status_handler(nmos::resources& resources, nmos::experimental::control_protocol_state& state, slog::base_gate& gate)
        {
            return nmos::make_set_receiver_monitor_stream_status_handler(resources, state, gate);
        }

        static auto get_status_property_id() { return nmos::nc_receiver_monitor_stream_status_property_id; }
        static auto get_message_property_id() { return nmos::nc_receiver_monitor_stream_status_message_property_id; }
        static auto get_pending_field_name() { return nmos::fields::nc::stream_status_pending; }
        static auto get_pending_time_field_name() { return nmos::fields::nc::stream_status_pending_received_time; }
    };

    // ========== Sender Monitor Specializations ==========

    // Specialization for sender monitor nc_link_status
    template<>
    struct StatusTraits<nmos::nc_link_status::status, MonitorType::Sender>
    {
        static const char* test_name() { return "Sender Link Status"; }
        static nmos::nc_link_status::status initial_status() { return nmos::nc_link_status::status::all_down; }
        static utility::string_t initial_message() { return U("Initial all down"); }

        static std::vector<StatusTransitionConfig<nmos::nc_link_status::status>> transitions()
        {
            return {
                { nmos::nc_link_status::status::some_down, U("Transitioning to some down"), false },
                { nmos::nc_link_status::status::all_up, U("Transitioning to all up"), false },
                { nmos::nc_link_status::status::some_down, U("Transitioning to some down"), true },
                { nmos::nc_link_status::status::all_down, U("Transitioning to all down"), true }
            };
        }

        static auto make_set_status_handler(nmos::resources& resources, nmos::experimental::control_protocol_state& state, slog::base_gate& gate)
        {
            return nmos::make_set_sender_monitor_link_status_handler(resources, state, gate);
        }

        static auto get_status_property_id() { return nmos::nc_sender_monitor_link_status_property_id; }
        static auto get_message_property_id() { return nmos::nc_sender_monitor_link_status_message_property_id; }
        static auto get_pending_field_name() { return nmos::fields::nc::link_status_pending; }
        static auto get_pending_time_field_name() { return nmos::fields::nc::link_status_pending_received_time; }
    };

    // Specialization for sender monitor nc_transmission_status
    template<>
    struct StatusTraits<nmos::nc_transmission_status::status, MonitorType::Sender>
    {
        static const char* test_name() { return "Sender Transmission Status"; }
        static nmos::nc_transmission_status::status initial_status() { return nmos::nc_transmission_status::status::unhealthy; }
        static utility::string_t initial_message() { return U("Initial unhealthy status"); }

        static std::vector<StatusTransitionConfig<nmos::nc_transmission_status::status>> transitions()
        {
            return {
                { nmos::nc_transmission_status::status::partially_healthy, U("Transitioning to partially healthy"), false },
                { nmos::nc_transmission_status::status::healthy, U("Transitioning to healthy"), false },
                { nmos::nc_transmission_status::status::inactive, U("Transitioning to inactive"), true },
                { nmos::nc_transmission_status::status::healthy, U("Transitioning to healthy"), true },
                { nmos::nc_transmission_status::status::partially_healthy, U("Transitioning to partially healthy"), true },
                { nmos::nc_transmission_status::status::unhealthy, U("Transitioning to unhealthy"), true }
            };
        }

        static auto make_set_status_handler(nmos::resources& resources, nmos::experimental::control_protocol_state& state, slog::base_gate& gate)
        {
            return nmos::make_set_sender_monitor_transmission_status_handler(resources, state, gate);
        }

        static auto get_status_property_id() { return nmos::nc_sender_monitor_transmission_status_property_id; }
        static auto get_message_property_id() { return nmos::nc_sender_monitor_transmission_status_message_property_id; }
        static auto get_pending_field_name() { return nmos::fields::nc::transmission_status_pending; }
        static auto get_pending_time_field_name() { return nmos::fields::nc::transmission_status_pending_received_time; }
    };

    // Specialization for sender monitor nc_synchronization_status
    template<>
    struct StatusTraits<nmos::nc_synchronization_status::status, MonitorType::Sender>
    {
        static const char* test_name() { return "Sender External Synchronization Status"; }
        static nmos::nc_synchronization_status::status initial_status() { return nmos::nc_synchronization_status::status::unhealthy; }
        static utility::string_t initial_message() { return U("Initial unhealthy status"); }

        static std::vector<StatusTransitionConfig<nmos::nc_synchronization_status::status>> transitions()
        {
            return {
                { nmos::nc_synchronization_status::status::partially_healthy, U("Transitioning to partially healthy"), false },
                { nmos::nc_synchronization_status::status::healthy, U("Transitioning to healthy"), false },
                { nmos::nc_synchronization_status::status::not_used, U("Transitioning to not used"), true },
                { nmos::nc_synchronization_status::status::healthy, U("Transitioning to healthy"), true },
                { nmos::nc_synchronization_status::status::partially_healthy, U("Transitioning to partially healthy"), true },
                { nmos::nc_synchronization_status::status::unhealthy, U("Transitioning to unhealthy"), true }
            };
        }

        static auto make_set_status_handler(nmos::resources& resources, nmos::experimental::control_protocol_state& state, slog::base_gate& gate)
        {
            return nmos::make_set_sender_monitor_external_synchronization_status_handler(resources, state, gate);
        }

        static auto get_status_property_id() { return nmos::nc_sender_monitor_external_synchronization_status_property_id; }
        static auto get_message_property_id() { return nmos::nc_sender_monitor_external_synchronization_status_message_property_id; }
        static auto get_pending_field_name() { return nmos::fields::nc::external_synchronization_status_pending; }
        static auto get_pending_time_field_name() { return nmos::fields::nc::external_synchronization_status_pending_received_time; }
    };

    // Specialization for sender monitor nc_essence_status
    template<>
    struct StatusTraits<nmos::nc_essence_status::status, MonitorType::Sender>
    {
        static const char* test_name() { return "Sender Essence Status"; }
        static nmos::nc_essence_status::status initial_status() { return nmos::nc_essence_status::status::unhealthy; }
        static utility::string_t initial_message() { return U("Initial unhealthy status"); }

        static std::vector<StatusTransitionConfig<nmos::nc_essence_status::status>> transitions()
        {
            return {
                { nmos::nc_essence_status::status::partially_healthy, U("Transitioning to partially healthy"), false },
                { nmos::nc_essence_status::status::healthy, U("Transitioning to healthy"), false },
                { nmos::nc_essence_status::status::inactive, U("Transitioning to inactive"), true },
                { nmos::nc_essence_status::status::healthy, U("Transitioning to healthy"), true },
                { nmos::nc_essence_status::status::partially_healthy, U("Transitioning to partially healthy"), true },
                { nmos::nc_essence_status::status::unhealthy, U("Transitioning to unhealthy"), true }
            };
        }

        static auto make_set_status_handler(nmos::resources& resources, nmos::experimental::control_protocol_state& state, slog::base_gate& gate)
        {
            return nmos::make_set_sender_monitor_essence_status_handler(resources, state, gate);
        }

        static auto get_status_property_id() { return nmos::nc_sender_monitor_essence_status_property_id; }
        static auto get_message_property_id() { return nmos::nc_sender_monitor_essence_status_message_property_id; }
        static auto get_pending_field_name() { return nmos::fields::nc::essence_status_pending; }
        static auto get_pending_time_field_name() { return nmos::fields::nc::essence_status_pending_received_time; }
    };

    // Generic test function template
    template<typename StatusType, MonitorType mointor_type>
    void test_status_transition_impl()
    {
        using web::json::value;
        using Traits = StatusTraits<StatusType, mointor_type>;

        boost::iostreams::stream<boost::iostreams::null_sink> null_ostream((boost::iostreams::null_sink()));
        nmos::experimental::log_model log_model;
        nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

        nmos::node_model model;
        nmos::experimental::control_protocol_state control_protocol_state;
        auto& control_protocol_resources = model.control_protocol_resources;
        auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
        auto set_status = Traits::make_set_status_handler(control_protocol_resources, control_protocol_state, gate);

        // Create Device Model
        auto root_block = nmos::make_root_block();
        auto root_block_oid = nmos::root_block_oid;
        auto class_manager_oid = ++root_block_oid;
        auto class_manager = nmos::make_class_manager(class_manager_oid, control_protocol_state);
        auto monitor_oid = ++class_manager_oid;

        // Create appropriate monitor type
        auto monitor = mointor_type == MonitorType::Receiver
            ? nmos::make_receiver_monitor(monitor_oid, true, root_block_oid, U("monitor"), U("monitor"), U("monitor"), web::json::value::null())
            : nmos::make_sender_monitor(monitor_oid, true, root_block_oid, U("monitor"), U("monitor"), U("monitor"), web::json::value::null());

        nmos::nc::push_back(root_block, class_manager);
        nmos::nc::push_back(root_block, monitor);
        insert_resource(control_protocol_resources, std::move(root_block));
        insert_resource(control_protocol_resources, std::move(class_manager));
        insert_resource(control_protocol_resources, std::move(monitor));

        // Set status reporting delay to 3 seconds
        const int32_t status_reporting_delay = 3;
        BST_REQUIRE(nmos::nc::set_property(control_protocol_resources, monitor_oid,
            nmos::fields::nc::status_reporting_delay,
            value::number(status_reporting_delay),
            gate));

        // Set initial activation time to simulate monitor has been active long enough
        auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        auto activation_time = now_time - 10;
        BST_REQUIRE(nmos::nc::set_property(control_protocol_resources, monitor_oid,
            nmos::fields::nc::monitor_activation_time,
            value::number(activation_time),
            gate));

        // Start control_protocol_behaviour_thread
        std::thread behaviour_thread([&model, &control_protocol_state, &gate]()
            {
                nmos::experimental::control_protocol_behaviour_thread(model, control_protocol_state, gate);
            });

        auto status_transitions = Traits::transitions();
        auto expected_current_status = Traits::initial_status();

        // Set initial status
        BST_REQUIRE(set_status(monitor_oid, expected_current_status, Traits::initial_message()));

        // Verify initial status
        auto current_status = nmos::nc::get_property(control_protocol_resources, monitor_oid,
            Traits::get_status_property_id(),
            get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_current_status, current_status.as_integer());

        // Test each status transition
        for (auto& status_transition : status_transitions)
        {
            BST_REQUIRE(set_status(monitor_oid, status_transition.status, status_transition.message));
            model.notify();

            if (status_transition.immediate_update)
            {
                expected_current_status = status_transition.status;
            }

            // Verify current status
            current_status = nmos::nc::get_property(control_protocol_resources, monitor_oid,
                Traits::get_status_property_id(),
                get_control_protocol_class_descriptor, gate);
            BST_CHECK_EQUAL(expected_current_status, current_status.as_integer());

            if (status_transition.immediate_update)
            {
                auto pending_received_time = nmos::nc::get_property(control_protocol_resources, monitor_oid,
                    Traits::get_pending_time_field_name(),
                    gate);
                BST_CHECK_EQUAL(0, pending_received_time.as_integer());
            }
            else
            {
                auto pending_status = nmos::nc::get_property(control_protocol_resources, monitor_oid,
                    Traits::get_pending_field_name(),
                    gate);
                BST_CHECK_EQUAL(status_transition.status, pending_status.as_integer());

                auto pending_received_time = nmos::nc::get_property(control_protocol_resources, monitor_oid,
                    Traits::get_pending_time_field_name(),
                    gate);
                BST_CHECK(pending_received_time.as_integer() > 0);
            }

            // Wait for behaviour thread to process
            std::this_thread::sleep_for(std::chrono::seconds(4));

            // Verify final status
            auto final_status = nmos::nc::get_property(control_protocol_resources, monitor_oid,
                Traits::get_status_property_id(),
                get_control_protocol_class_descriptor, gate);
            BST_CHECK_EQUAL(status_transition.status, final_status.as_integer());

            auto final_status_message = nmos::nc::get_property(control_protocol_resources, monitor_oid,
                Traits::get_message_property_id(),
                get_control_protocol_class_descriptor, gate);
            BST_CHECK_EQUAL(status_transition.message, final_status_message.as_string());

            expected_current_status = status_transition.status;

            auto pending_received_time = nmos::nc::get_property(control_protocol_resources, monitor_oid,
                Traits::get_pending_time_field_name(),
                gate);
            BST_CHECK_EQUAL(0, pending_received_time.as_integer());
        }

        model.controlled_shutdown();
        behaviour_thread.join();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Receiver Monitor Tests
//////////////////////////////////////////////////////////////////////////////////////////////

BST_TEST_CASE(testReceiverLinkStatusTransition)
{
    test_status_transition_impl<nmos::nc_link_status::status, MonitorType::Receiver>();
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testReceiverConnectionStatusTransition)
{
    test_status_transition_impl<nmos::nc_connection_status::status, MonitorType::Receiver>();
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testReceiverExtSyncStatusTransition)
{
    test_status_transition_impl<nmos::nc_synchronization_status::status, MonitorType::Receiver>();
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testReceiverStreamStatusTransition)
{
    test_status_transition_impl<nmos::nc_stream_status::status, MonitorType::Receiver>();
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Sender Monitor Tests
//////////////////////////////////////////////////////////////////////////////////////////////

BST_TEST_CASE(testSenderLinkStatusTransition)
{
    test_status_transition_impl<nmos::nc_link_status::status, MonitorType::Sender>();
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSenderTransmissionStatusTransition)
{
    test_status_transition_impl<nmos::nc_transmission_status::status, MonitorType::Sender>();
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSenderExtSyncStatusTransition)
{
    test_status_transition_impl<nmos::nc_synchronization_status::status, MonitorType::Sender>();
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSenderEssenceStatusTransition)
{
    test_status_transition_impl<nmos::nc_essence_status::status, MonitorType::Sender>();
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testStatusTransitionWithinReportDelay)
{
    // Test scenario: Status transitions from unhealthy (3) to healthy (1) then back to unhealthy (3)
    // within the 3 second report delay. Final status should be unhealthy (3).

    boost::iostreams::stream<boost::iostreams::null_sink> null_ostream((boost::iostreams::null_sink()));
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

    nmos::node_model model;
    nmos::experimental::control_protocol_state control_protocol_state;
    auto& control_protocol_resources = model.control_protocol_resources;
    auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    auto set_receiver_monitor_stream_status = nmos::make_set_receiver_monitor_stream_status_handler(control_protocol_resources, control_protocol_state, gate);
    auto monitor_status_pending = nmos::make_monitor_status_pending_handler(control_protocol_state);

    // Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto root_block_oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager_oid = ++root_block_oid;
    auto class_manager = nmos::make_class_manager(class_manager_oid, control_protocol_state);
    auto monitor_oid = ++class_manager_oid;

    // Create receiver monitor with 3 second status reporting delay
    auto monitor = nmos::make_receiver_monitor(monitor_oid, true, root_block_oid, U("monitor"), U("monitor"), U("monitor"), web::json::value::null());

    nmos::nc::push_back(root_block, class_manager);
    nmos::nc::push_back(root_block, monitor);
    insert_resource(control_protocol_resources, std::move(root_block));
    insert_resource(control_protocol_resources, std::move(class_manager));
    insert_resource(control_protocol_resources, std::move(monitor));

    // Set status reporting delay to 3 seconds
    const int32_t status_reporting_delay = 3;
    BST_REQUIRE(nmos::nc::set_property_and_notify(control_protocol_resources, monitor_oid,
        nmos::nc_status_monitor_status_reporting_delay,
        web::json::value::number(status_reporting_delay),
        get_control_protocol_class_descriptor, gate));

    // Set initial activation time to simulate monitor has been active long enough
    auto now_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    auto activation_time = now_time - 10; // Set activation time to 10 seconds ago
    BST_REQUIRE(nmos::nc::set_property(control_protocol_resources, monitor_oid,
        nmos::fields::nc::monitor_activation_time,
        web::json::value::number(activation_time),
        gate));

    // Start control_protocol_behaviour_thread to process the pending status
    std::thread behaviour_thread([&model, &control_protocol_state, &gate]()
        {
            nmos::experimental::control_protocol_behaviour_thread(model, control_protocol_state, gate);
        });

    // unhealthy (3) -> { healthy (1) -> unhealthy (3) : within the 3 second status reporting delay }

    // Step 1: Set initial status to unhealthy (3)
    BST_REQUIRE(nmos::nc::set_receiver_monitor_stream_status(control_protocol_resources, monitor_oid,
        nmos::nc_stream_status::status::unhealthy,
        U("Initial unhealthy status"),
        get_control_protocol_class_descriptor, gate));

    // Verify initial status is unhealthy
    auto current_status = nmos::nc::get_property(control_protocol_resources, monitor_oid,
        nmos::nc_receiver_monitor_stream_status_property_id,
        get_control_protocol_class_descriptor, gate);
    BST_CHECK_EQUAL(nmos::nc_stream_status::status::unhealthy, current_status.as_integer());

    // Step 2: Transition to healthy (1) with delay
    // This should set pending status
    BST_REQUIRE(set_receiver_monitor_stream_status(monitor_oid, nmos::nc_stream_status::status::healthy, U("Transitioning to healthy")));
    model.notify();

    // Verify status is still unhealthy (pending change not yet applied)
    current_status = nmos::nc::get_property(control_protocol_resources, monitor_oid,
        nmos::nc_receiver_monitor_stream_status_property_id,
        get_control_protocol_class_descriptor, gate);
    BST_CHECK_EQUAL(nmos::nc_stream_status::status::unhealthy, current_status.as_integer());

    // Verify pending status is healthy
    auto pending_status = nmos::nc::get_property(control_protocol_resources, monitor_oid,
        nmos::fields::nc::stream_status_pending,
        gate);
    BST_CHECK_EQUAL(nmos::nc_stream_status::status::healthy, pending_status.as_integer());

    // Verify pending received time is set
    auto pending_received_time = nmos::nc::get_property(control_protocol_resources, monitor_oid,
        nmos::fields::nc::stream_status_pending_received_time,
        gate);
    BST_CHECK(pending_received_time.as_integer() > 0);

    // Step 3: Within the delay window (< 3 seconds), transition back to unhealthy (3)
    // Wait 1 second to simulate time passing but still within the 3 second window
    std::this_thread::sleep_for(std::chrono::seconds(1));

    BST_REQUIRE(set_receiver_monitor_stream_status(monitor_oid, nmos::nc_stream_status::status::unhealthy, U("Back to unhealthy")));
    model.notify();

    // Wait for the behaviour thread to process the status (initial delay + processing time)
    // Wait slightly more than 3 seconds from the first pending status
    std::this_thread::sleep_for(std::chrono::seconds(4));

    // Shutdown the model to stop the behaviour thread
    model.controlled_shutdown();
    behaviour_thread.join();

    // Step 4: Verify final status is unhealthy (3)
    auto final_status = nmos::nc::get_property(control_protocol_resources, monitor_oid,
        nmos::nc_receiver_monitor_stream_status_property_id,
        get_control_protocol_class_descriptor, gate);
    BST_CHECK_EQUAL(nmos::nc_stream_status::status::unhealthy, final_status.as_integer());

    // Verify final status message
    auto final_status_message = nmos::nc::get_property(control_protocol_resources, monitor_oid,
        nmos::nc_receiver_monitor_stream_status_message_property_id,
        get_control_protocol_class_descriptor, gate);
    BST_CHECK_EQUAL(U("Back to unhealthy"), final_status_message.as_string());

    // Verify pending received time has been reset to 0
    pending_received_time = nmos::nc::get_property(control_protocol_resources, monitor_oid,
        nmos::fields::nc::stream_status_pending_received_time,
        gate);
    BST_CHECK_EQUAL(0, pending_received_time.as_integer());
}

// The first "test" is of course whether the header compiles standalone
#include "boost/iostreams/stream.hpp"
#include "boost/iostreams/device/null.hpp"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/is04_versions.h"
#include "nmos/is12_versions.h"
#include "nmos/log_gate.h"
#include "nmos/slog.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testGetSetControlProtocolProperty)
{
    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor = nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

    boost::iostreams::stream< boost::iostreams::null_sink > null_ostream((boost::iostreams::null_sink()));
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

    // Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto root_block_oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager_oid = ++root_block_oid;
    auto class_manager = nmos::make_class_manager(class_manager_oid, control_protocol_state);
    nmos::nc::push_back(root_block, class_manager);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));

	auto oid = nmos::nc::get_property(resources, class_manager_oid, nmos::nc_object_oid_property_id, get_control_protocol_class_descriptor, gate);

	BST_CHECK_EQUAL(class_manager_oid, static_cast<uint32_t>(oid.as_integer()));

	auto role = nmos::nc::get_property(resources, class_manager_oid, nmos::nc_object_role_property_id, get_control_protocol_class_descriptor, gate);

	BST_CHECK_EQUAL(U("ClassManager"), role.as_string());

    auto test_label = U("ThisIsATest");

	bool result = nmos::nc::set_property_and_notify(resources, class_manager_oid, nmos::nc_object_user_label_property_id, web::json::value::string(test_label), get_control_protocol_class_descriptor, gate);

	BST_REQUIRE(result);

	auto label = nmos::nc::get_property(resources, class_manager_oid, nmos::nc_object_user_label_property_id, get_control_protocol_class_descriptor, gate);

	BST_CHECK_EQUAL(test_label, label.as_string());
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSetReceiverMonitorStatuses)
{
    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor = nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

    boost::iostreams::stream< boost::iostreams::null_sink > null_ostream((boost::iostreams::null_sink()));
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

	// Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto root_block_oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager_oid = ++root_block_oid;
    auto class_manager = nmos::make_class_manager(class_manager_oid, control_protocol_state);
	auto monitor_oid = ++class_manager_oid;

	auto monitor = nmos::make_receiver_monitor(monitor_oid, true, root_block_oid, U("monitor"), U("monitor"), U("monitor"), web::json::value::null());

    nmos::nc::push_back(root_block, class_manager);
    nmos::nc::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        auto link_status = nmos::nc_link_status::status::all_up;
        auto link_status_message = U("All links are up");
        auto expected_link_status_transition_counter = 0;
        auto connection_status = nmos::nc_connection_status::status::inactive;
        auto connection_status_message = U("Connection healthy");
        auto expected_connection_status_transition_counter = 0;
        auto external_synchronization_status = nmos::nc_synchronization_status::status::unhealthy;
        auto external_synchronization_status_message = U("No external synchronization");
        auto expected_external_synchronization_status_transition_counter = 1;
        auto stream_status = nmos::nc_stream_status::status::inactive;
        auto stream_status_message = U("Stream status healthy");
        auto expected_stream_status_transition_counter = 0;

        BST_REQUIRE(nmos::nc::set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status, actual_connection_status.as_integer());
        auto actual_connection_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status_message, actual_connection_status_message.as_string());
        auto actual_connection_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_connection_status_transition_counter, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_stream_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status, actual_stream_status.as_integer());
        auto actual_stream_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status_message, actual_stream_status_message.as_string());
        auto actual_stream_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_stream_status_transition_counter, actual_stream_status_transition_counter.as_integer());

        auto overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, overall_status.as_integer());
    }
    {
        auto link_status = nmos::nc_link_status::status::all_up;
        auto link_status_message = U("All links are up");
        auto expected_link_status_transition_counter = 0;
        auto connection_status = nmos::nc_connection_status::status::healthy;
        auto connection_status_message = U("Connection healthy");
        auto expected_connection_status_transition_counter = 0;
        auto external_synchronization_status = nmos::nc_synchronization_status::status::not_used;
        auto external_synchronization_status_message = U("No external synchronization");
        auto expected_external_synchronization_status_transition_counter = 1;
        auto stream_status = nmos::nc_stream_status::status::healthy;
        auto stream_status_message = U("Stream status healthy");
        auto expected_stream_status_transition_counter = 0;

        BST_REQUIRE(nmos::nc::set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status, actual_connection_status.as_integer());
        auto actual_connection_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status_message, actual_connection_status_message.as_string());
        auto actual_connection_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_connection_status_transition_counter, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_stream_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status, actual_stream_status.as_integer());
        auto actual_stream_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status_message, actual_stream_status_message.as_string());
        auto actual_stream_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_stream_status_transition_counter, actual_stream_status_transition_counter.as_integer());

        auto overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, overall_status.as_integer());
    }
    {
        auto link_status = nmos::nc_link_status::status::some_down;
        auto link_status_message = U("All links are up");
        auto expected_link_status_transition_counter = 1;
        auto connection_status = nmos::nc_connection_status::status::healthy;
        auto connection_status_message = U("Connection healthy");
        auto expected_connection_status_transition_counter = 0;
        auto external_synchronization_status = nmos::nc_synchronization_status::status::not_used;
        auto external_synchronization_status_message = U("No external synchronization");
        auto expected_external_synchronization_status_transition_counter = 1;
        auto stream_status = nmos::nc_stream_status::status::partially_healthy;
        auto stream_status_message = U("Stream status healthy");
        auto expected_stream_status_transition_counter = 1;

        BST_REQUIRE(nmos::nc::set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status, actual_connection_status.as_integer());
        auto actual_connection_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status_message, actual_connection_status_message.as_string());
        auto actual_connection_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_connection_status_transition_counter, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_stream_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status, actual_stream_status.as_integer());
        auto actual_stream_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status_message, actual_stream_status_message.as_string());
        auto actual_stream_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_stream_status_transition_counter, actual_stream_status_transition_counter.as_integer());

        auto overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::partially_healthy, overall_status.as_integer());
    }
    {
        auto link_status = nmos::nc_link_status::status::some_down;
        auto link_status_message = U("All links are up");
        auto expected_link_status_transition_counter = 1;
        auto connection_status = nmos::nc_connection_status::status::unhealthy;
        auto connection_status_message = U("Connection healthy");
        auto expected_connection_status_transition_counter = 1;
        auto external_synchronization_status = nmos::nc_synchronization_status::status::healthy;
        auto external_synchronization_status_message = U("No external synchronization");
        auto expected_external_synchronization_status_transition_counter = 1;
        auto stream_status = nmos::nc_stream_status::status::partially_healthy;
        auto stream_status_message = U("Stream status healthy");
        auto expected_stream_status_transition_counter = 1;

        BST_REQUIRE(nmos::nc::set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status, actual_connection_status.as_integer());
        auto actual_connection_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status_message, actual_connection_status_message.as_string());
        auto actual_connection_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_connection_status_transition_counter, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_stream_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status, actual_stream_status.as_integer());
        auto actual_stream_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status_message, actual_stream_status_message.as_string());
        auto actual_stream_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_stream_status_transition_counter, actual_stream_status_transition_counter.as_integer());

        auto overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::unhealthy, overall_status.as_integer());
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSetSynchronizationSourceId)
{
    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor = nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

    boost::iostreams::stream< boost::iostreams::null_sink > null_ostream((boost::iostreams::null_sink()));
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

    // Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto root_block_oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager_oid = ++root_block_oid;
    auto class_manager = nmos::make_class_manager(class_manager_oid, control_protocol_state);
    auto monitor_oid = ++class_manager_oid;

    auto monitor = nmos::make_receiver_monitor(monitor_oid, true, root_block_oid, U("monitor"), U("monitor"), U("monitor"), web::json::value::null());

    nmos::nc::push_back(root_block, class_manager);
    nmos::nc::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        utility::string_t sync_source_id = U("SYNCID");
        nmos::nc::set_monitor_synchronization_source_id(resources, monitor_oid, sync_source_id, get_control_protocol_class_descriptor, gate);
        auto actual_sync_source_id = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_synchronization_source_id_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(sync_source_id, actual_sync_source_id.as_string());
    }
    {
        nmos::nc::set_monitor_synchronization_source_id(resources, monitor_oid, {}, get_control_protocol_class_descriptor, gate);
        auto actual_sync_source_id = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_synchronization_source_id_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK(actual_sync_source_id.is_null());
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testActivateDeactivateReceiverMonitor)
{
    nmos::resources resources;

    bool reset_monitor_called = false;

    nmos::reset_monitor_handler reset_monitor = [&reset_monitor_called]()
    {
        // check that the property changed handler gets called
        reset_monitor_called = true;

        return nmos::nc::details::make_method_result({ nmos::nc_method_status::ok });
    };

    nmos::experimental::control_protocol_state control_protocol_state(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, reset_monitor);
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor = nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_method_descriptor_handler get_control_protocol_method_descriptor = nmos::make_get_control_protocol_method_descriptor_handler(control_protocol_state);

    boost::iostreams::stream< boost::iostreams::null_sink > null_ostream((boost::iostreams::null_sink()));
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

    // Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto root_block_oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager_oid = ++root_block_oid;
    auto class_manager = nmos::make_class_manager(class_manager_oid, control_protocol_state);
    auto monitor_oid = ++class_manager_oid;

    auto monitor = nmos::make_receiver_monitor(monitor_oid, true, root_block_oid, U("monitor"), U("monitor"), U("monitor"), web::json::value::null());

    nmos::nc::push_back(root_block, class_manager);
    nmos::nc::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        // autoResetCounterAndMessages will reset all counters and messages on activate, including calling back into application code
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_auto_reset_monitor_property_id, web::json::value::boolean(true), get_control_protocol_class_descriptor, gate);

        uint32_t transition_count = 10;
        // set transition counters
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::nc::activate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::healthy, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::healthy, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, actual_overall_status.as_integer());

        // Check transition counters
        auto actual_stream_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_stream_status_transition_counter.as_integer());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_external_synchronization_status_transition_counter.as_integer());

        // Check that reset_monitor handler was invoked
        BST_CHECK(reset_monitor_called);
    }
    {
        // Do deactivation
        nmos::nc::deactivate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, actual_overall_status.as_integer());
    }
    {
        reset_monitor_called = false;

        // disable autoResetCounterAndMessages
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_auto_reset_monitor_property_id, web::json::value::boolean(false), get_control_protocol_class_descriptor, gate);

        int32_t transition_count = 10;
        // set transition counters
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::nc::activate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::healthy, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::healthy, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, actual_overall_status.as_integer());

        // Check transition counters
        auto actual_stream_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_stream_status_transition_counter.as_integer());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_external_synchronization_status_transition_counter.as_integer());

        // Check that reset_monitor handler was NOT invoked
        BST_CHECK(!reset_monitor_called);
    }
    {
        // Do deactivation
        nmos::nc::deactivate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, actual_overall_status.as_integer());
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSetSenderMonitorStatuses)
{
    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor = nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

    boost::iostreams::stream< boost::iostreams::null_sink > null_ostream((boost::iostreams::null_sink()));
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

	// Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto root_block_oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager_oid = ++root_block_oid;
    auto class_manager = nmos::make_class_manager(class_manager_oid, control_protocol_state);
	auto monitor_oid = ++class_manager_oid;

	auto monitor = nmos::make_sender_monitor(monitor_oid, true, root_block_oid, U("monitor"), U("monitor"), U("monitor"), web::json::value::null());

    nmos::nc::push_back(root_block, class_manager);
    nmos::nc::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        auto link_status = nmos::nc_link_status::status::all_up;
        auto link_status_message = U("All links are up");
        auto expected_link_status_transition_counter = 0;
        auto transmission_status = nmos::nc_transmission_status::status::inactive;
        auto transmission_status_message = U("transmission healthy");
        auto expected_transmission_status_transition_counter = 0;
        auto external_synchronization_status = nmos::nc_synchronization_status::status::unhealthy;
        auto external_synchronization_status_message = U("No external synchronization");
        auto expected_external_synchronization_status_transition_counter = 1;
        auto essence_status = nmos::nc_essence_status::status::inactive;
        auto essence_status_message = U("essence status healthy");
        auto expected_essence_status_transition_counter = 0;

        BST_REQUIRE(nmos::nc::set_sender_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_transmission_status(resources, monitor_oid, transmission_status, transmission_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_essence_status(resources, monitor_oid, essence_status, essence_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status, actual_transmission_status.as_integer());
        auto actual_transmission_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status_message, actual_transmission_status_message.as_string());
        auto actual_transmission_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_transmission_status_transition_counter, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_essence_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status, actual_essence_status.as_integer());
        auto actual_essence_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status_message, actual_essence_status_message.as_string());
        auto actual_essence_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_essence_status_transition_counter, actual_essence_status_transition_counter.as_integer());

        auto overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, overall_status.as_integer());
    }
    {
        auto link_status = nmos::nc_link_status::status::all_up;
        auto link_status_message = U("All links are up");
        auto expected_link_status_transition_counter = 0;
        auto transmission_status = nmos::nc_transmission_status::status::healthy;
        auto transmission_status_message = U("transmission healthy");
        auto expected_transmission_status_transition_counter = 0;
        auto external_synchronization_status = nmos::nc_synchronization_status::status::not_used;
        auto external_synchronization_status_message = U("No external synchronization");
        auto expected_external_synchronization_status_transition_counter = 1;
        auto essence_status = nmos::nc_essence_status::status::healthy;
        auto essence_status_message = U("essence status healthy");
        auto expected_essence_status_transition_counter = 0;

        BST_REQUIRE(nmos::nc::set_sender_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_transmission_status(resources, monitor_oid, transmission_status, transmission_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_essence_status(resources, monitor_oid, essence_status, essence_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status, actual_transmission_status.as_integer());
        auto actual_transmission_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status_message, actual_transmission_status_message.as_string());
        auto actual_transmission_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_transmission_status_transition_counter, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_essence_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status, actual_essence_status.as_integer());
        auto actual_essence_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status_message, actual_essence_status_message.as_string());
        auto actual_essence_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_essence_status_transition_counter, actual_essence_status_transition_counter.as_integer());

        auto overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, overall_status.as_integer());
    }
    {
        auto link_status = nmos::nc_link_status::status::some_down;
        auto link_status_message = U("All links are up");
        auto expected_link_status_transition_counter = 1;
        auto transmission_status = nmos::nc_transmission_status::status::healthy;
        auto transmission_status_message = U("transmission healthy");
        auto expected_transmission_status_transition_counter = 0;
        auto external_synchronization_status = nmos::nc_synchronization_status::status::not_used;
        auto external_synchronization_status_message = U("No external synchronization");
        auto expected_external_synchronization_status_transition_counter = 1;
        auto essence_status = nmos::nc_essence_status::status::partially_healthy;
        auto essence_status_message = U("essence status healthy");
        auto expected_essence_status_transition_counter = 1;

        BST_REQUIRE(nmos::nc::set_sender_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_transmission_status(resources, monitor_oid, transmission_status, transmission_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_essence_status(resources, monitor_oid, essence_status, essence_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status, actual_transmission_status.as_integer());
        auto actual_transmission_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status_message, actual_transmission_status_message.as_string());
        auto actual_transmission_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_transmission_status_transition_counter, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_essence_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status, actual_essence_status.as_integer());
        auto actual_essence_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status_message, actual_essence_status_message.as_string());
        auto actual_essence_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_essence_status_transition_counter, actual_essence_status_transition_counter.as_integer());

        auto overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::partially_healthy, overall_status.as_integer());
    }
    {
        auto link_status = nmos::nc_link_status::status::some_down;
        auto link_status_message = U("All links are up");
        auto expected_link_status_transition_counter = 1;
        auto transmission_status = nmos::nc_transmission_status::status::unhealthy;
        auto transmission_status_message = U("transmission healthy");
        auto expected_transmission_status_transition_counter = 1;
        auto external_synchronization_status = nmos::nc_synchronization_status::status::healthy;
        auto external_synchronization_status_message = U("No external synchronization");
        auto expected_external_synchronization_status_transition_counter = 1;
        auto essence_status = nmos::nc_essence_status::status::partially_healthy;
        auto essence_status_message = U("essence status healthy");
        auto expected_essence_status_transition_counter = 1;

        BST_REQUIRE(nmos::nc::set_sender_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_transmission_status(resources, monitor_oid, transmission_status, transmission_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(nmos::nc::set_sender_monitor_essence_status(resources, monitor_oid, essence_status, essence_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status, actual_transmission_status.as_integer());
        auto actual_transmission_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status_message, actual_transmission_status_message.as_string());
        auto actual_transmission_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_transmission_status_transition_counter, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_essence_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status, actual_essence_status.as_integer());
        auto actual_essence_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status_message, actual_essence_status_message.as_string());
        auto actual_essence_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_essence_status_transition_counter, actual_essence_status_transition_counter.as_integer());

        auto overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::unhealthy, overall_status.as_integer());
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testActivateDeactivateSenderMonitor)
{
    nmos::resources resources;

    bool reset_monitor_called = false;

    nmos::reset_monitor_handler reset_monitor = [&reset_monitor_called]()
    {
        // check that the property changed handler gets called
        reset_monitor_called = true;

        return nmos::nc::details::make_method_result({ nmos::nc_method_status::ok });
    };

    nmos::experimental::control_protocol_state control_protocol_state(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, reset_monitor);
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor = nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_method_descriptor_handler get_control_protocol_method_descriptor = nmos::make_get_control_protocol_method_descriptor_handler(control_protocol_state);

    boost::iostreams::stream< boost::iostreams::null_sink > null_ostream((boost::iostreams::null_sink()));
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

    // Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto root_block_oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager_oid = ++root_block_oid;
    auto class_manager = nmos::make_class_manager(class_manager_oid, control_protocol_state);
    auto monitor_oid = ++class_manager_oid;

    auto monitor = nmos::make_sender_monitor(monitor_oid, true, root_block_oid, U("monitor"), U("monitor"), U("monitor"), web::json::value::null());

    nmos::nc::push_back(root_block, class_manager);
    nmos::nc::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        // autoResetCounterAndMessages will reset all counters and messages on activate, including calling back into application code
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_auto_reset_monitor_property_id, web::json::value::boolean(true), get_control_protocol_class_descriptor, gate);

        uint32_t transition_count = 10;
        // set transition counters
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::nc::activate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

        // Check statuses
        auto actual_essence_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::healthy, actual_essence_status.as_integer());
        auto actual_transmission_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::healthy, actual_transmission_status.as_integer());
        auto actual_overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, actual_overall_status.as_integer());

        // Check transition counters
        auto actual_essence_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_essence_status_transition_counter.as_integer());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_external_synchronization_status_transition_counter.as_integer());

        // Check that reset_monitor handler was invoked
        BST_CHECK(reset_monitor_called);
    }
    {
        // Do deactivation
        nmos::nc::deactivate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_essence_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::inactive, actual_essence_status.as_integer());
        auto actual_transmission_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::inactive, actual_transmission_status.as_integer());
        auto actual_overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, actual_overall_status.as_integer());
    }
    {
        reset_monitor_called = false;

        // disable autoResetCounterAndMessages
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_auto_reset_monitor_property_id, web::json::value::boolean(false), get_control_protocol_class_descriptor, gate);

        int32_t transition_count = 10;
        // set transition counters
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::nc::set_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::nc::activate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

        // Check statuses
        auto actual_essence_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::healthy, actual_essence_status.as_integer());
        auto actual_transmission_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::healthy, actual_transmission_status.as_integer());
        auto actual_overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, actual_overall_status.as_integer());

        // Check transition counters
        auto actual_essence_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_essence_status_transition_counter.as_integer());
        auto actual_link_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status_transition_counter = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_external_synchronization_status_transition_counter.as_integer());

        // Check that reset_monitor handler was NOT invoked
        BST_CHECK(!reset_monitor_called);
    }
    {
        // Do deactivation
        nmos::nc::deactivate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_essence_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::inactive, actual_essence_status.as_integer());
        auto actual_transmission_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::inactive, actual_transmission_status.as_integer());
        auto actual_overall_status = nmos::nc::get_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, actual_overall_status.as_integer());
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSetMonitorStatusWithDelay)
{
    nmos::resources resources;

    bool reset_monitor_called = false;
    bool monitor_status_pending_called = false;

    nmos::reset_monitor_handler reset_monitor = [&reset_monitor_called]()
    {
        // check that the property changed handler gets called
        reset_monitor_called = true;

        return nmos::nc::details::make_method_result({nmos::nc_method_status::ok});
    };

    nmos::monitor_status_pending_handler monitor_status_pending = [&]()
    {
        monitor_status_pending_called = true;
    };

    nmos::experimental::control_protocol_state control_protocol_state(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, reset_monitor);
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);

    boost::iostreams::stream< boost::iostreams::null_sink > null_ostream((boost::iostreams::null_sink()));
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

    // Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto root_block_oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager_oid = ++root_block_oid;
    auto class_manager = nmos::make_class_manager(class_manager_oid, control_protocol_state);
    auto monitor_oid = ++class_manager_oid;

    auto monitor = nmos::make_receiver_monitor(monitor_oid, true, root_block_oid, U("monitor"), U("monitor"), U("monitor"), web::json::value::null());

    nmos::nc::push_back(root_block, class_manager);
    nmos::nc::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        // Status should change with no delay to Inactive
        // Initial stream status of healthy
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status, 1, gate);

        // Set stream stream status to inactive at t=0
        bool success = nmos::nc::details::set_monitor_status_with_delay(resources, monitor_oid, 0, U(""),
                nmos::nc_receiver_monitor_stream_status_property_id,
                nmos::nc_receiver_monitor_stream_status_message_property_id,
                nmos::nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending,
                nmos::fields::nc::stream_status_message_pending,
                nmos::fields::nc::stream_status_pending_received_time,
                0,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        BST_REQUIRE(success);

        const auto actual_value = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to change with no delay to inactive
        BST_CHECK_EQUAL(0, actual_value.as_integer());
    }
    {
        // Status should change to Inactive with no delay

        // Initial stream status of healthy and monitor activation time to t=0
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status, 1, gate);

        long long current_time = 1;
        // Set stream stream status to inactive at t=1 - within initial status_reporting_delay period
        bool success = nmos::nc::details::set_monitor_status_with_delay(resources, monitor_oid, 0, U(""),
                nmos::nc_receiver_monitor_stream_status_property_id,
                nmos::nc_receiver_monitor_stream_status_message_property_id,
                nmos::nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending,
                nmos::fields::nc::stream_status_message_pending,
                nmos::fields::nc::stream_status_pending_received_time,
                current_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        BST_REQUIRE(success);

        // Expected status to change with no delay to inactive
        const auto actual_value = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        BST_CHECK_EQUAL(0, actual_value.as_integer());

        // Expected status pending received time to be 0 i.e. not pending
        const auto actual_time = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(0, actual_time.as_integer());
    }
    {
        // Within initial status reporting period status should remain healthy
        // Any unhealthy status updates should be pending
        //
        // Initial stream status of healthy and monitor activation time to t=0
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status, 1, gate);

        long long current_time = 1;
        int expected_status = 3;
        // Set stream stream status to unhealthy at t=1 - within initial status_reporting_delay period
        bool success = nmos::nc::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
                nmos::nc_receiver_monitor_stream_status_property_id,
                nmos::nc_receiver_monitor_stream_status_message_property_id,
                nmos::nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending,
                nmos::fields::nc::stream_status_message_pending,
                nmos::fields::nc::stream_status_pending_received_time,
                current_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        BST_REQUIRE(success);

        const auto actual_value = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to remain healthy
        BST_CHECK_EQUAL(1, actual_value.as_integer());
        // Expected status pending received time to be current time i.e. pending
        const auto actual_time = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(current_time, actual_time.as_integer());
        // Expected status pending to be expected status i.e. pending
        const auto actual_status = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending, gate);
        BST_CHECK_EQUAL(expected_status, actual_status.as_integer());
    }
    {
        // Status updates from healthy to unhealthy after initial reporting delay should happen with no delay
        //
        // Initial stream status of healthy and monitor activation time to t=0
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status, 1, gate);

        long long current_time = 5;
        int expected_status = 3;
        // Set stream status to unhealthy at t=5 - after initial status_reporting_delay period
        bool success = nmos::nc::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
                nmos::nc_receiver_monitor_stream_status_property_id,
                nmos::nc_receiver_monitor_stream_status_message_property_id,
                nmos::nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending,
                nmos::fields::nc::stream_status_message_pending,
                nmos::fields::nc::stream_status_pending_received_time,
                current_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        BST_REQUIRE(success);

        const auto actual_value = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to be unhealthy
        BST_CHECK_EQUAL(expected_status, actual_value.as_integer());
        // Expected status pending received time to be 0 i.e. not pending
        const auto actual_time = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(0, actual_time.as_integer());
    }
    {
        // Status updates from partailly unhealthy to unhealthy after initial reporting delay should happen with no delay
        //
        // Initial stream status of partially unhealthy and monitor activation time to t=0
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status, 2, gate);

        long long current_time = 5;
        int expected_status = 3;
        // Set stream stream status to unhealthy at t=5 - after initial status_reporting_delay period
        bool success = nmos::nc::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
                nmos::nc_receiver_monitor_stream_status_property_id,
                nmos::nc_receiver_monitor_stream_status_message_property_id,
                nmos::nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending,
                nmos::fields::nc::stream_status_message_pending,
                nmos::fields::nc::stream_status_pending_received_time,
                current_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        BST_REQUIRE(success);

        const auto actual_value = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to go to unhealthy without delay
        BST_CHECK_EQUAL(expected_status, actual_value.as_integer());
        // Expected status pending received time to be 0 i.e. not pending
        const auto actual_time = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(0, actual_time.as_integer());
    }
    {
        // Status update from partially unhealthy to healthy should be pending and not update status immediately
        //
        // Initial stream status of partially unhealthy and monitor activation time to t=0
        int initial_status = 2;
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status, initial_status, gate);

        long long current_time = 5;
        int expected_status = 1;
        // Set stream stream status to healthy at t=5 - after initial status_reporting_delay period
        bool success = nmos::nc::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
                nmos::nc_receiver_monitor_stream_status_property_id,
                nmos::nc_receiver_monitor_stream_status_message_property_id,
                nmos::nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending,
                nmos::fields::nc::stream_status_message_pending,
                nmos::fields::nc::stream_status_pending_received_time,
                current_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        BST_REQUIRE(success);

        const auto actual_value = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to stay partially unhealthy
        BST_CHECK_EQUAL(initial_status, actual_value.as_integer());
        // Expected status pending received time to be current time i.e. pending
        const auto actual_time = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(current_time, actual_time.as_integer());
        // Expected status pending to be expected status i.e. healthy
        const auto actual_status = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending, gate);
        BST_CHECK_EQUAL(expected_status, actual_status.as_integer());
    }
    {
        // If status gets healthier update which update already pending, don't updated the pending received time
        //
        // Initial stream status of partially unhealthy and monitor activation time to t=0
        int initial_status = 2;
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status, initial_status, gate);

        long long current_time = 9;
        int expected_status = 1;
        long long received_time = 8;
        // Status already pending with healthy state at t=8
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending, expected_status, gate);
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, received_time, gate);

        // Set stream stream status to healthy at t=9 - healthy status already pending
        bool success = nmos::nc::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
                nmos::nc_receiver_monitor_stream_status_property_id,
                nmos::nc_receiver_monitor_stream_status_message_property_id,
                nmos::nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending,
                nmos::fields::nc::stream_status_message_pending,
                nmos::fields::nc::stream_status_pending_received_time,
                current_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        BST_REQUIRE(success);

        const auto actual_value = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to stay unhealthy
        BST_CHECK_EQUAL(initial_status, actual_value.as_integer());
        // Expected status pending received time to be the original pending time
        const auto actual_time = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(received_time, actual_time.as_integer());
        // Expected status pending to be expected status i.e. healthy
        const auto actual_status = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending, gate);
        BST_CHECK_EQUAL(expected_status, actual_status.as_integer());
    }
    {
        // If the status is updated to the same value but the status message changes, update the status message
        //
        // Initial stream status of healthy and monitor activation time to t=0
        int initial_status = 1;
        utility::string_t initial_status_message = U("initial status message");
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status, initial_status, gate);
        nmos::nc::set_property(resources, monitor_oid, nmos::fields::nc::stream_status_message, web::json::value::string(initial_status_message), gate);

        long long current_time = 9;
        utility::string_t updated_status_message = U("updated status message");

        // Maintain healthy status but change the status message
        bool success = nmos::nc::details::set_monitor_status_with_delay(resources, monitor_oid, initial_status, updated_status_message,
                nmos::nc_receiver_monitor_stream_status_property_id,
                nmos::nc_receiver_monitor_stream_status_message_property_id,
                nmos::nc_receiver_monitor_stream_status_transition_counter_property_id,
                nmos::fields::nc::stream_status_pending,
                nmos::fields::nc::stream_status_message_pending,
                nmos::fields::nc::stream_status_pending_received_time,
                current_time,
                monitor_status_pending,
                get_control_protocol_class_descriptor,
                gate);
        BST_REQUIRE(success);

        const auto actual_value = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to stay healthy
        BST_CHECK_EQUAL(initial_status, actual_value.as_integer());
        const auto actual_status_message = nmos::nc::get_property(resources, monitor_oid, nmos::fields::nc::stream_status_message, gate);
        // Expected message to have been updated
        BST_CHECK_EQUAL(updated_status_message, actual_status_message.as_string());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testFindTouchpointResources)
{
    using web::json::value_of;
    using web::json::value;

    const auto touchpoint1_id = U("1000");
    auto touchpoint1_data = value_of({
                { nmos::fields::id, touchpoint1_id },
                { nmos::fields::version, nmos::make_version() },
                { nmos::fields::label, U("touchpoint1") },
                { nmos::fields::description, U("touchpoint1") },
                { nmos::fields::tags, value::null() }
        });
    nmos::resource touchpoint1 = { nmos::is04_versions::v1_3, nmos::types::node, std::move(touchpoint1_data), false };

    const auto touchpoint2_id = U("1001");
    auto touchpoint2_data = value_of({
                { nmos::fields::id, touchpoint2_id },
                { nmos::fields::version, nmos::make_version() },
                { nmos::fields::label, U("touchpoint2") },
                { nmos::fields::description, U("touchpoint2") },
                { nmos::fields::tags, value::null() }
        });
    nmos::resource touchpoint2 = { nmos::is04_versions::v1_3, nmos::types::node, std::move(touchpoint2_data), false };

    const auto non_existant_id = U("1002");

    // Create Device Model
    auto oid = nmos::root_block_oid;
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, nmos::root_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, touchpoint1_id})} }));
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, nmos::root_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, touchpoint2_id})} }));
    auto monitor3 = nmos::make_receiver_monitor(++oid, true, nmos::root_block_oid, U("mon3"), U("monitor 3"), U("monitor 3"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, non_existant_id})} }));

    nmos::resources resources;
    // Insert dummy NMOS resources
    insert_resource(resources, std::move(touchpoint1));
    insert_resource(resources, std::move(touchpoint2));

    {
        const auto& touchpoint = nmos::nc::find_touchpoint_resource(resources, monitor1);

        BST_CHECK_EQUAL(touchpoint1_id, nmos::fields::id(touchpoint->data));
        BST_CHECK_EQUAL(U("touchpoint1"), nmos::fields::label(touchpoint->data));
        BST_CHECK_EQUAL(U("touchpoint1"), nmos::fields::description(touchpoint->data));
    }
    {
        const auto& touchpoint = nmos::nc::find_touchpoint_resource(resources, monitor2);

        BST_CHECK_EQUAL(touchpoint2_id, nmos::fields::id(touchpoint->data));
        BST_CHECK_EQUAL(U("touchpoint2"), nmos::fields::label(touchpoint->data));
        BST_CHECK_EQUAL(U("touchpoint2"), nmos::fields::description(touchpoint->data));
    }
    {
        const auto& touchpoint = nmos::nc::find_touchpoint_resource(resources, monitor3);

        BST_CHECK_EQUAL(touchpoint, resources.end());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testFindMembersByClassId)
{
    using web::json::value_of;
    using web::json::value;

    auto make_example_block_resource = [](const nmos::nc_class_id& class_id, nmos::nc_oid oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description)
        {
            auto data = nmos::nc::details::make_block(class_id, oid, true, owner, role, value::string(user_label), description, value::null(), value::null(), true, value::array());

            return nmos::control_protocol_resource{ nmos::is12_versions::v1_0, nmos::types::nc_worker, std::move(data), true };
        };

    auto make_example_worker_resource = [](const nmos::nc_class_id& class_id, nmos::nc_oid oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description)
        {
            auto data = nmos::nc::details::make_worker(class_id, oid, true, owner, role, value::string(user_label), description, value::null(), value::null(), true);

            return nmos::control_protocol_resource{ nmos::is12_versions::v1_0, nmos::types::nc_worker, std::move(data), true };
        };

    nmos::nc_oid oid = nmos::root_block_oid;

    auto root_block = nmos::make_root_block();

    // { 1, 1, 0, 1 }
    const auto example_class_id_1 = nmos::nc::make_class_id(nmos::nc_block_class_id, 0, { 1 });
    // { 1, 2, 0, 1, 1 }
    const auto example_class_id_2 = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 1, 1 });

    auto example_1 = make_example_block_resource(example_class_id_1, ++oid, nmos::root_block_oid, U("example block 1"), U("Example Block 1"), U("Example Block 1"));
    auto example_2 = make_example_worker_resource(example_class_id_2, ++oid, nmos::root_block_oid, U("example block 2"), U("Example Block 2"), U("Example Block 2"));

    nmos::nc::push_back(root_block, example_1);
    nmos::nc::push_back(root_block, example_2);

    nmos::resources resources;

    // insert root block and all sub control protocol resources to resource list
    nmos::nc::insert_root(resources, root_block);

    // get root block
    const auto found_root_block = nmos::find_resource_if(resources, nmos::types::nc_block, [&](const nmos::resource& resource)
        {
            return nmos::root_block_oid == nmos::fields::nc::oid(resource.data);
        });

    // search unknown class_id members
    {
        auto members = value::array();
        nmos::nc::find_members_by_class_id(resources, *found_root_block, { 1, 0 }, true, true, members.as_array());
        BST_REQUIRE_EQUAL(0, members.size());
    }

    // search all NcObject members
    {
        auto members = value::array();
        nmos::nc::find_members_by_class_id(resources, *found_root_block, nmos::nc_object_class_id, true, true, members.as_array());
        BST_REQUIRE_EQUAL(2, members.size());
    }

    // search all NcBlock members
    {
        auto members = value::array();
        nmos::nc::find_members_by_class_id(resources, *found_root_block, nmos::nc_block_class_id, true, true, members.as_array());
        BST_REQUIRE_EQUAL(1, members.size());
        BST_REQUIRE_EQUAL(example_class_id_1, nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(members.at(0))));
    }

    // search NcWorker members
    {
        auto members = value::array();
        nmos::nc::find_members_by_class_id(resources, *found_root_block, nmos::nc_worker_class_id, true, true, members.as_array());
        BST_REQUIRE_EQUAL(1, members.size());
        BST_REQUIRE_EQUAL(example_class_id_2, nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(members.at(0))));
    }
}

BST_TEST_CASE(testValidateResourceProperties)
{
    using web::json::value_of;
    using web::json::value;

    nmos::experimental::control_protocol_state control_protocol_state;
    auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);

    auto root_block = nmos::make_root_block();
    auto oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager = nmos::make_class_manager(++oid, control_protocol_state);
    auto receiver_block_oid = ++oid;
    // root, receivers
    auto receivers = nmos::make_block(receiver_block_oid, nmos::root_block_oid, U("receivers"), U("Receivers"), U("Receivers block"));

    // root, receivers, mon1
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));

    auto block_class_descriptor = get_control_protocol_class_descriptor(nmos::nc_block_class_id);

    BST_CHECK_NO_THROW(nmos::nc::validate_resource(root_block, block_class_descriptor));
    BST_CHECK_NO_THROW(nmos::nc::validate_resource(receivers, block_class_descriptor));

    auto class_manager_class_descriptor = get_control_protocol_class_descriptor(nmos::nc_class_manager_class_id);

    BST_CHECK_NO_THROW(nmos::nc::validate_resource(class_manager, class_manager_class_descriptor));

    auto receiver_monitor_class_descriptor = get_control_protocol_class_descriptor(nmos::nc_receiver_monitor_class_id);

    BST_CHECK_NO_THROW(nmos::nc::validate_resource(monitor1, receiver_monitor_class_descriptor));

    // Negative tests
    BST_CHECK_THROW(nmos::nc::validate_resource(root_block, receiver_monitor_class_descriptor), nmos::control_protocol_exception);
    BST_CHECK_THROW(nmos::nc::validate_resource(receivers, class_manager_class_descriptor), nmos::control_protocol_exception);
    BST_CHECK_THROW(nmos::nc::validate_resource(class_manager, block_class_descriptor), nmos::control_protocol_exception);
    BST_CHECK_THROW(nmos::nc::validate_resource(monitor1, block_class_descriptor), nmos::control_protocol_exception);
}

BST_TEST_CASE(testValidateResourceMethods)
{
    using web::json::value_of;
    using web::json::value;

    nmos::experimental::control_protocol_state control_protocol_state;
    auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);

    auto root_block = nmos::make_root_block();

    auto example_method_with_no_args = [](nmos::resources& resources, const nmos::resource& resource, const web::json::value& arguments, bool is_deprecated, slog::base_gate& gate)
        {
            return nmos::nc::details::make_method_result({ is_deprecated ? nmos::nc_method_status::method_deprecated : nmos::nc_method_status::ok });
        };

    auto control_class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 2 });
    // Define class descriptor with method and associated method - should succeed on validate
    {
        std::vector<web::json::value> property_descriptors = {};

        std::vector<nmos::experimental::method> method_descriptors =
        {
            { nmos::experimental::make_control_class_method_descriptor(U("Example method with no arguments"), { 3, 1 }, U("MethodNoArgs"), U("NcMethodResult"), {}, false, example_method_with_no_args) }
        };

        auto control_class_descriptor = nmos::experimental::make_control_class_descriptor(U(""), nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 2 }), U("ControlClass"), property_descriptors, method_descriptors, {});

        auto make_control_object = [&](nmos::nc_oid oid)
            {
                auto data = nmos::nc::details::make_worker(control_class_id, oid, true, 1, U("role"), value::string(U("")), U(""), value::null(), value::null(), true);
                return nmos::control_protocol_resource{ nmos::is12_versions::v1_0, nmos::types::nc_worker, std::move(data), true };
            };

        auto control_object = make_control_object(3);

        BST_CHECK_NO_THROW(nmos::nc::validate_resource(control_object, control_class_descriptor));
    }
    // Define class descriptor with method with no associated method - should throw on validate
    {
        std::vector<web::json::value> property_descriptors = {};

        std::vector<nmos::experimental::method> method_descriptors =
        {
            { nmos::experimental::make_control_class_method_descriptor(U("Example method with no arguments"), { 3, 1 }, U("MethodNoArgs"), U("NcMethodResult"), {}, false, nullptr) }
        };

        auto control_class_descriptor = nmos::experimental::make_control_class_descriptor(U(""), nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 2 }), U("ControlClass"), property_descriptors, method_descriptors, {});

        auto make_control_object = [&](nmos::nc_oid oid)
            {
                auto data = nmos::nc::details::make_worker(control_class_id, oid, true, 1, U("role"), value::string(U("")), U(""), value::null(), value::null(), true);
                return nmos::control_protocol_resource{ nmos::is12_versions::v1_0, nmos::types::nc_worker, std::move(data), true };
            };

        auto control_object = make_control_object(3);

        BST_CHECK_THROW(nmos::nc::validate_resource(control_object, control_class_descriptor), nmos::control_protocol_exception);
    }
}

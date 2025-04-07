// The first "test" is of course whether the header compiles standalone
#include "boost/iostreams/stream.hpp"
#include "boost/iostreams/device/null.hpp"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/log_gate.h"
#include "nmos/slog.h"

#include "nmos/is04_versions.h"

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
    nmos::push_back(root_block, class_manager);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));

	auto oid = nmos::get_control_protocol_property(resources, class_manager_oid, nmos::nc_object_oid_property_id, get_control_protocol_class_descriptor, gate);

	BST_CHECK_EQUAL(class_manager_oid, static_cast<uint32_t>(oid.as_integer()));

	auto role = get_control_protocol_property(resources, class_manager_oid, nmos::nc_object_role_property_id, get_control_protocol_class_descriptor, gate);

	BST_CHECK_EQUAL(U("ClassManager"), role.as_string());

    auto test_label = U("ThisIsATest");

	bool result = set_control_protocol_property(resources, class_manager_oid, nmos::nc_object_user_label_property_id, web::json::value::string(test_label), get_control_protocol_class_descriptor, gate);

	BST_REQUIRE(result);

	auto label = get_control_protocol_property(resources, class_manager_oid, nmos::nc_object_user_label_property_id, get_control_protocol_class_descriptor, gate);

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

    nmos::push_back(root_block, class_manager);
    nmos::push_back(root_block, monitor);
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

        BST_REQUIRE(set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status, actual_connection_status.as_integer());
        auto actual_connection_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status_message, actual_connection_status_message.as_string());
        auto actual_connection_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_connection_status_transition_counter, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_stream_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status, actual_stream_status.as_integer());
        auto actual_stream_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status_message, actual_stream_status_message.as_string());
        auto actual_stream_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_stream_status_transition_counter, actual_stream_status_transition_counter.as_integer());

        auto overall_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

        BST_REQUIRE(set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status, actual_connection_status.as_integer());
        auto actual_connection_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status_message, actual_connection_status_message.as_string());
        auto actual_connection_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_connection_status_transition_counter, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_stream_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status, actual_stream_status.as_integer());
        auto actual_stream_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status_message, actual_stream_status_message.as_string());
        auto actual_stream_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_stream_status_transition_counter, actual_stream_status_transition_counter.as_integer());

        auto overall_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

        BST_REQUIRE(set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status, actual_connection_status.as_integer());
        auto actual_connection_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status_message, actual_connection_status_message.as_string());
        auto actual_connection_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_connection_status_transition_counter, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_stream_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status, actual_stream_status.as_integer());
        auto actual_stream_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status_message, actual_stream_status_message.as_string());
        auto actual_stream_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_stream_status_transition_counter, actual_stream_status_transition_counter.as_integer());

        auto overall_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

        BST_REQUIRE(set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, control_protocol_state, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status, actual_connection_status.as_integer());
        auto actual_connection_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(connection_status_message, actual_connection_status_message.as_string());
        auto actual_connection_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_connection_status_transition_counter, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_stream_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status, actual_stream_status.as_integer());
        auto actual_stream_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(stream_status_message, actual_stream_status_message.as_string());
        auto actual_stream_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_stream_status_transition_counter, actual_stream_status_transition_counter.as_integer());

        auto overall_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

    nmos::push_back(root_block, class_manager);
    nmos::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        utility::string_t sync_source_id = U("SYNCID");
        nmos::set_receiver_monitor_synchronization_source_id(resources, monitor_oid, sync_source_id, get_control_protocol_class_descriptor, gate);
        auto actual_sync_source_id = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_synchronization_source_id_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(sync_source_id, actual_sync_source_id.as_string());
    }
    {
        utility::string_t sync_source_id = U("");
        nmos::set_receiver_monitor_synchronization_source_id(resources, monitor_oid, sync_source_id, get_control_protocol_class_descriptor, gate);
        auto actual_sync_source_id = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_synchronization_source_id_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK(actual_sync_source_id.is_null());
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testActivateDeactivateReceiverMonitor)
{
    nmos::resources resources;

    bool reset_counters_called = false;

    nmos::reset_counters_handler reset_counters = [&reset_counters_called]()
    {
        // check that the property changed handler gets called
        reset_counters_called = true;

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok });
    };

    nmos::experimental::control_protocol_state control_protocol_state(nullptr, nullptr, reset_counters, nullptr);
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

    nmos::push_back(root_block, class_manager);
    nmos::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        // autoResetCounter will reset all counters on activate, including calling back into application code
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_auto_reset_counters_property_id, web::json::value::boolean(true), get_control_protocol_class_descriptor, gate);

        uint32_t transition_count = 10;
        // set transition counters
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::activate_receiver_monitor(resources, monitor_oid, control_protocol_state, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::healthy, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::healthy, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, actual_overall_status.as_integer());

        // Check transition counters
        auto actual_stream_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_stream_status_transition_counter.as_integer());
        auto actual_link_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_external_synchronization_status_transition_counter.as_integer());

        // Check that reset_counters handler was invoked
        BST_CHECK(reset_counters_called);
    }
    {
        // Do deactivation
        nmos::deactivate_receiver_monitor(resources, monitor_oid, control_protocol_state, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, actual_overall_status.as_integer());
    }
    {
        reset_counters_called = false;

        // disable autoResetCounter
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_auto_reset_counters_property_id, web::json::value::boolean(false), get_control_protocol_class_descriptor, gate);

        int32_t transition_count = 10;
        // set transition counters
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::activate_receiver_monitor(resources, monitor_oid, control_protocol_state, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::healthy, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::healthy, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, actual_overall_status.as_integer());

        // Check transition counters
        auto actual_stream_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_stream_status_transition_counter.as_integer());
        auto actual_link_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_link_status_transition_counter.as_integer());
        auto actual_connection_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_connection_status_transition_counter.as_integer());
        auto actual_external_synchronization_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_external_synchronization_status_transition_counter.as_integer());

        // Check that reset_counters handler was NOT invoked
        BST_CHECK(!reset_counters_called);
    }
    {
        // Do deactivation
        nmos::deactivate_receiver_monitor(resources, monitor_oid, control_protocol_state, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, actual_overall_status.as_integer());
    }
}

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

	bool result = set_control_protocol_property_and_notify(resources, class_manager_oid, nmos::nc_object_user_label_property_id, web::json::value::string(test_label), get_control_protocol_class_descriptor, gate);

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

        BST_REQUIRE(set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, get_control_protocol_class_descriptor, gate));

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

        BST_REQUIRE(set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, get_control_protocol_class_descriptor, gate));

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

        BST_REQUIRE(set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, get_control_protocol_class_descriptor, gate));

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

        BST_REQUIRE(set_receiver_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_connection_status(resources, monitor_oid, connection_status, connection_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_receiver_monitor_stream_status(resources, monitor_oid, stream_status, stream_status_message, get_control_protocol_class_descriptor, gate));

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
        nmos::set_monitor_synchronization_source_id(resources, monitor_oid, sync_source_id, get_control_protocol_class_descriptor, gate);
        auto actual_sync_source_id = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_synchronization_source_id_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(sync_source_id, actual_sync_source_id.as_string());
    }
    {
        nmos::set_monitor_synchronization_source_id(resources, monitor_oid, {}, get_control_protocol_class_descriptor, gate);
        auto actual_sync_source_id = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_synchronization_source_id_property_id, get_control_protocol_class_descriptor, gate);
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

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok });
    };

    nmos::experimental::control_protocol_state control_protocol_state(nullptr, nullptr, reset_monitor, nullptr);
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
        // autoResetCounterAndMessages will reset all counters and messages on activate, including calling back into application code
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_auto_reset_monitor_property_id, web::json::value::boolean(true), get_control_protocol_class_descriptor, gate);

        uint32_t transition_count = 10;
        // set transition counters
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::activate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

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

        // Check that reset_monitor handler was invoked
        BST_CHECK(reset_monitor_called);
    }
    {
        // Do deactivation
        nmos::deactivate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, actual_overall_status.as_integer());
    }
    {
        reset_monitor_called = false;

        // disable autoResetCounterAndMessages
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_auto_reset_monitor_property_id, web::json::value::boolean(false), get_control_protocol_class_descriptor, gate);

        int32_t transition_count = 10;
        // set transition counters
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::activate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

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

        // Check that reset_monitor handler was NOT invoked
        BST_CHECK(!reset_monitor_called);
    }
    {
        // Do deactivation
        nmos::deactivate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_stream_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_stream_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_stream_status.as_integer());
        auto actual_connection_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_receiver_monitor_connection_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_stream_status::status::inactive, actual_connection_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

    nmos::push_back(root_block, class_manager);
    nmos::push_back(root_block, monitor);
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

        BST_REQUIRE(set_sender_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_transmission_status(resources, monitor_oid, transmission_status, transmission_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_essence_status(resources, monitor_oid, essence_status, essence_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status, actual_transmission_status.as_integer());
        auto actual_transmission_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status_message, actual_transmission_status_message.as_string());
        auto actual_transmission_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_transmission_status_transition_counter, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_essence_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status, actual_essence_status.as_integer());
        auto actual_essence_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status_message, actual_essence_status_message.as_string());
        auto actual_essence_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_essence_status_transition_counter, actual_essence_status_transition_counter.as_integer());

        auto overall_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

        BST_REQUIRE(set_sender_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_transmission_status(resources, monitor_oid, transmission_status, transmission_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_essence_status(resources, monitor_oid, essence_status, essence_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status, actual_transmission_status.as_integer());
        auto actual_transmission_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status_message, actual_transmission_status_message.as_string());
        auto actual_transmission_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_transmission_status_transition_counter, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_essence_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status, actual_essence_status.as_integer());
        auto actual_essence_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status_message, actual_essence_status_message.as_string());
        auto actual_essence_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_essence_status_transition_counter, actual_essence_status_transition_counter.as_integer());

        auto overall_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

        BST_REQUIRE(set_sender_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_transmission_status(resources, monitor_oid, transmission_status, transmission_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_essence_status(resources, monitor_oid, essence_status, essence_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status, actual_transmission_status.as_integer());
        auto actual_transmission_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status_message, actual_transmission_status_message.as_string());
        auto actual_transmission_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_transmission_status_transition_counter, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_essence_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status, actual_essence_status.as_integer());
        auto actual_essence_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status_message, actual_essence_status_message.as_string());
        auto actual_essence_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_essence_status_transition_counter, actual_essence_status_transition_counter.as_integer());

        auto overall_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

        BST_REQUIRE(set_sender_monitor_link_status(resources, monitor_oid, link_status, link_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_transmission_status(resources, monitor_oid, transmission_status, transmission_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_external_synchronization_status(resources, monitor_oid, external_synchronization_status, external_synchronization_status_message, get_control_protocol_class_descriptor, gate));
        BST_REQUIRE(set_sender_monitor_essence_status(resources, monitor_oid, essence_status, essence_status_message, get_control_protocol_class_descriptor, gate));

        auto actual_link_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status, actual_link_status.as_integer());
        auto actual_link_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(link_status_message, actual_link_status_message.as_string());
        auto actual_link_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_link_status_transition_counter, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status, actual_transmission_status.as_integer());
        auto actual_transmission_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transmission_status_message, actual_transmission_status_message.as_string());
        auto actual_transmission_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_transmission_status_transition_counter, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status, actual_external_synchronization_status.as_integer());
        auto actual_external_synchronization_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(external_synchronization_status_message, actual_external_synchronization_status_message.as_string());
        auto actual_external_synchronization_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_external_synchronization_status_transition_counter, actual_external_synchronization_status_transition_counter.as_integer());
        auto actual_essence_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status, actual_essence_status.as_integer());
        auto actual_essence_status_message = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_message_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(essence_status_message, actual_essence_status_message.as_string());
        auto actual_essence_status_transition_counter = get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(expected_essence_status_transition_counter, actual_essence_status_transition_counter.as_integer());

        auto overall_status = get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

        return nmos::details::make_nc_method_result({ nmos::nc_method_status::ok });
    };

    nmos::experimental::control_protocol_state control_protocol_state(nullptr, nullptr, reset_monitor, nullptr);
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

    nmos::push_back(root_block, class_manager);
    nmos::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        // autoResetCounterAndMessages will reset all counters and messages on activate, including calling back into application code
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_auto_reset_monitor_property_id, web::json::value::boolean(true), get_control_protocol_class_descriptor, gate);

        uint32_t transition_count = 10;
        // set transition counters
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::activate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

        // Check statuses
        auto actual_essence_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::healthy, actual_essence_status.as_integer());
        auto actual_transmission_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::healthy, actual_transmission_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, actual_overall_status.as_integer());

        // Check transition counters
        auto actual_essence_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_essence_status_transition_counter.as_integer());
        auto actual_link_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(0, actual_external_synchronization_status_transition_counter.as_integer());

        // Check that reset_monitor handler was invoked
        BST_CHECK(reset_monitor_called);
    }
    {
        // Do deactivation
        nmos::deactivate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_essence_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::inactive, actual_essence_status.as_integer());
        auto actual_transmission_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::inactive, actual_transmission_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::inactive, actual_overall_status.as_integer());
    }
    {
        reset_monitor_called = false;

        // disable autoResetCounterAndMessages
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_auto_reset_monitor_property_id, web::json::value::boolean(false), get_control_protocol_class_descriptor, gate);

        int32_t transition_count = 10;
        // set transition counters
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);
        nmos::set_control_protocol_property_and_notify(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, web::json::value::number(transition_count), get_control_protocol_class_descriptor, gate);

        // Do activatation
        nmos::activate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, get_control_protocol_method_descriptor, gate);

        // Check statuses
        auto actual_essence_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::healthy, actual_essence_status.as_integer());
        auto actual_transmission_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::healthy, actual_transmission_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_overall_status::status::healthy, actual_overall_status.as_integer());

        // Check transition counters
        auto actual_essence_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_essence_status_transition_counter.as_integer());
        auto actual_link_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_link_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_link_status_transition_counter.as_integer());
        auto actual_transmission_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_transmission_status_transition_counter.as_integer());
        auto actual_external_synchronization_status_transition_counter = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_external_synchronization_status_transition_counter_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(transition_count, actual_external_synchronization_status_transition_counter.as_integer());

        // Check that reset_monitor handler was NOT invoked
        BST_CHECK(!reset_monitor_called);
    }
    {
        // Do deactivation
        nmos::deactivate_monitor(resources, monitor_oid, get_control_protocol_class_descriptor, gate);

        // Check statuses
        auto actual_essence_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_essence_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::inactive, actual_essence_status.as_integer());
        auto actual_transmission_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_sender_monitor_transmission_status_property_id, get_control_protocol_class_descriptor, gate);
        BST_CHECK_EQUAL(nmos::nc_essence_status::status::inactive, actual_transmission_status.as_integer());
        auto actual_overall_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::nc_status_monitor_overall_status_property_id, get_control_protocol_class_descriptor, gate);
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

        return nmos::details::make_nc_method_result({nmos::nc_method_status::ok});
    };

    nmos::monitor_status_pending_handler monitor_status_pending = [&]()
    {
        monitor_status_pending_called = true;
    };

    nmos::experimental::control_protocol_state control_protocol_state(nullptr, nullptr, reset_monitor, nullptr);
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

    nmos::push_back(root_block, class_manager);
    nmos::push_back(root_block, monitor);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(monitor));

    {
        // Status should change with no delay to Inactive
        // Initial stream status of healthy
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, 1, gate);

        // Set stream stream status to inactive at t=0
        bool success = nmos::details::set_monitor_status_with_delay(resources, monitor_oid, 0, U(""),
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

        const auto actual_value = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to change with no delay to inactive
        BST_CHECK_EQUAL(0, actual_value.as_integer());
    }
    {
        // Status should change to Inactive with no delay

        // Initial stream status of healthy and monitor activation time to t=0
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, 1, gate);

        long long current_time = 1;
        // Set stream stream status to inactive at t=1 - within initial status_reporting_delay period
        bool success = nmos::details::set_monitor_status_with_delay(resources, monitor_oid, 0, U(""),
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
        const auto actual_value = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        BST_CHECK_EQUAL(0, actual_value.as_integer());

        // Expected status pending received time to be 0 i.e. not pending
        const auto actual_time = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(0, actual_time.as_integer());
    }
    {
        // Within initial status reporting period status should remain healthy
        // Any unhealthy status updates should be pending
        //
        // Initial stream status of healthy and monitor activation time to t=0
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, 1, gate);

        long long current_time = 1;
        int expected_status = 3;
        // Set stream stream status to unhealthy at t=1 - within initial status_reporting_delay period
        bool success = nmos::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
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

        const auto actual_value = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to remain healthy
        BST_CHECK_EQUAL(1, actual_value.as_integer());
        // Expected status pending received time to be current time i.e. pending
        const auto actual_time = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(current_time, actual_time.as_integer());
        // Expected status pending to be expected status i.e. pending
        const auto actual_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending, gate);
        BST_CHECK_EQUAL(expected_status, actual_status.as_integer());
    }
    {
        // Status updates from healthy to unhealthy after initial reporting delay should happen with no delay
        //
        // Initial stream status of healthy and monitor activation time to t=0
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, 1, gate);

        long long current_time = 5;
        int expected_status = 3;
        // Set stream status to unhealthy at t=5 - after initial status_reporting_delay period
        bool success = nmos::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
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

        const auto actual_value = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to be unhealthy
        BST_CHECK_EQUAL(expected_status, actual_value.as_integer());
        // Expected status pending received time to be 0 i.e. not pending
        const auto actual_time = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(0, actual_time.as_integer());
    }
    {
        // Status updates from partailly unhealthy to unhealthy after initial reporting delay should happen with no delay
        //
        // Initial stream status of partially unhealthy and monitor activation time to t=0
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, 2, gate);

        long long current_time = 5;
        int expected_status = 3;
        // Set stream stream status to unhealthy at t=5 - after initial status_reporting_delay period
        bool success = nmos::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
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

        const auto actual_value = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to go to unhealthy without delay
        BST_CHECK_EQUAL(expected_status, actual_value.as_integer());
        // Expected status pending received time to be 0 i.e. not pending
        const auto actual_time = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(0, actual_time.as_integer());
    }
    {
        // Status update from partially unhealthy to healthy should be pending and not update status immediately
        //
        // Initial stream status of partially unhealthy and monitor activation time to t=0
        int initial_status = 2;
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, initial_status, gate);

        long long current_time = 5;
        int expected_status = 1;
        // Set stream stream status to healthy at t=5 - after initial status_reporting_delay period
        bool success = nmos::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
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

        const auto actual_value = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to stay partially unhealthy
        BST_CHECK_EQUAL(initial_status, actual_value.as_integer());
        // Expected status pending received time to be current time i.e. pending
        const auto actual_time = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(current_time, actual_time.as_integer());
        // Expected status pending to be expected status i.e. healthy
        const auto actual_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending, gate);
        BST_CHECK_EQUAL(expected_status, actual_status.as_integer());
    }
    {
        // If status gets healthier update which update already pending, don't updated the pending received time
        //
        // Initial stream status of partially unhealthy and monitor activation time to t=0
        int initial_status = 2;
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, initial_status, gate);

        long long current_time = 9;
        int expected_status = 1;
        long long received_time = 8;
        // Status already pending with healthy state at t=8
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending, expected_status, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, received_time, gate);

        // Set stream stream status to healthy at t=9 - healthy status already pending
        bool success = nmos::details::set_monitor_status_with_delay(resources, monitor_oid, expected_status, U(""),
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

        const auto actual_value = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to stay unhealthy
        BST_CHECK_EQUAL(initial_status, actual_value.as_integer());
        // Expected status pending received time to be the original pending time
        const auto actual_time = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending_received_time, gate);
        BST_CHECK_EQUAL(received_time, actual_time.as_integer());
        // Expected status pending to be expected status i.e. healthy
        const auto actual_status = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_pending, gate);
        BST_CHECK_EQUAL(expected_status, actual_status.as_integer());
    }
    {
        // If the status is updated to the same value but the status message changes, update the status message
        //
        // Initial stream status of healthy and monitor activation time to t=0
        int initial_status = 1;
        utility::string_t initial_status_message = U("initial status message");
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::monitor_activation_time, 0, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, initial_status, gate);
        nmos::set_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_message, web::json::value::string(initial_status_message), gate);

        long long current_time = 9;
        utility::string_t updated_status_message = U("updated status message");

        // Maintain healthy status but change the status message
        bool success = nmos::details::set_monitor_status_with_delay(resources, monitor_oid, initial_status, updated_status_message,
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

        const auto actual_value = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status, gate);
        // Expected status to stay healthy
        BST_CHECK_EQUAL(initial_status, actual_value.as_integer());
        const auto actual_status_message = nmos::get_control_protocol_property(resources, monitor_oid, nmos::fields::nc::stream_status_message, gate);
        // Expected message to have been updated
        BST_CHECK_EQUAL(updated_status_message, actual_status_message.as_string());
    }
}
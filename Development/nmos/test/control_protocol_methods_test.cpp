// The first "test" is of course whether the header compiles standalone
#include "boost/iostreams/stream.hpp"
#include "boost/iostreams/device/null.hpp"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_methods.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/is12_versions.h"
#include "nmos/json_fields.h"
#include "nmos/log_gate.h"
#include "nmos/slog.h"
#include "bst/test/test.h"


////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testRemoveSequenceItem)
{
    using web::json::value_of;
    using web::json::value;

    bool property_changed_called = false;

    boost::iostreams::stream< boost::iostreams::null_sink > null_ostream((boost::iostreams::null_sink()));

    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

    nmos::resources resources;
    nmos::control_protocol_property_changed_handler property_changed = [&property_changed_called](const nmos::resource& resource, const utility::string_t& property_name, int index)
    {
        // check that the property changed handler gets called
        property_changed_called = true;
    };
    nmos::experimental::control_protocol_state control_protocol_state(property_changed);
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);


    // Create simple non-standard class with writable sequence property
    const auto writable_sequence_class_id = nmos::make_nc_class_id(nmos::nc_worker_class_id, -1234, { 1000 });
    const web::json::field_as_array writable_value{ U("writableValue") };
    {
        // Writable sequence_class property descriptors
        std::vector<web::json::value> writable_sequence_property_descriptors = { nmos::experimental::make_control_class_property_descriptor(U("Writable sequence"), { 3, 1 }, writable_value, U("NcInt16"), false, false, true, false, web::json::value::null()) };

        // create writable_sequence class descriptor
        auto writable_sequence_class_descriptor = nmos::experimental::make_control_class_descriptor(U("Writable sequence class descriptor"), writable_sequence_class_id, U("WritableSequence"), writable_sequence_property_descriptors);

        // insert writable_sequence class descriptor to global state, which will be used by the control_protocol_ws_message_handler to process incoming ws message
        control_protocol_state.insert(writable_sequence_class_descriptor);
    }
    // helper function to create writable_sequence object
    auto make_writable_sequence = [&writable_value, &writable_sequence_class_id](nmos::nc_oid oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description)
    {
        auto data = nmos::details::make_nc_worker(writable_sequence_class_id, oid, true, owner, role, value::string(user_label), description, web::json::value::null(), web::json::value::null(), true);
        auto values = value::array();
        web::json::push_back(values, value::number(10));
        web::json::push_back(values, value::number(9));
        web::json::push_back(values, value::number(8));
        data[writable_value] = values;

        return nmos::control_protocol_resource{ nmos::is12_versions::v1_0, nmos::types::nc_worker, std::move(data), true };
    };

    // Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager = nmos::make_class_manager(++oid, control_protocol_state);
    auto receiver_block_oid = ++oid;
    // root, receivers
    auto receivers = nmos::make_block(receiver_block_oid, nmos::root_block_oid, U("receivers"), U("Receivers"), U("Receivers block"));
    auto receivers_id = receivers.id;

    // root, receivers, mon1
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value::null());
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value::null());

    auto writable_sequence = make_writable_sequence(++oid, nmos::root_block_oid, U("writableSequence"), U("writable sequence"), U("writable sequence"));
    auto writable_sequence_id = writable_sequence.id;

    nmos::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::push_back(root_block, receivers);
    // add class-manager to root-block
    nmos::push_back(root_block, class_manager);
    // add writable sequence to root block
    nmos::push_back(root_block, writable_sequence);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(receivers));
    insert_resource(resources, std::move(monitor1));
    insert_resource(resources, std::move(monitor2));
    insert_resource(resources, std::move(writable_sequence));

    // Attempt to remove a member from a block - read only error expected
    {
        property_changed_called = false;

        auto block_members_property_id = value_of({
            { U("level"), nmos::nc_block_members_property_id.level },
            { U("index"), nmos::nc_block_members_property_id.index},
            });

        auto arguments = value_of({
            { nmos::fields::nc::id, block_members_property_id },
            { nmos::fields::nc::index, 0}
            });

        auto resource = nmos::find_resource(resources, receivers_id);
        BST_CHECK_NE(resources.end(), resource);
        auto result = nmos::remove_sequence_item(resources, *resource, arguments, false, get_control_protocol_class_descriptor, property_changed, gate);

        // Expect read only error, and for property changed not to be called
        BST_CHECK_EQUAL(false, property_changed_called);
        BST_CHECK_EQUAL(nmos::nc_method_status::read_only, nmos::fields::nc::status(result));
    }

    // Remove writable sequence item - success and property_changed event expected
    {
        property_changed_called = false;

        auto writable_sequence_property_id = value_of({
            { U("level"), 3 },
            { U("index"), 1},
            });

        auto arguments = value_of({
            { nmos::fields::nc::id, writable_sequence_property_id },
            { nmos::fields::nc::index, 1}
            });

        auto resource = nmos::find_resource(resources, writable_sequence_id);
        BST_CHECK_NE(resources.end(), resource);
        auto result = nmos::remove_sequence_item(resources, *resource, arguments, false, get_control_protocol_class_descriptor, property_changed, gate);

        // Expect success, and property changed event
        BST_CHECK_EQUAL(true, property_changed_called);
        BST_CHECK_EQUAL(nmos::nc_method_status::ok, nmos::fields::nc::status(result));
    }
}

// The first "test" is of course whether the header compiles standalone
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/configuration_resources.h"
#include "nmos/configuration_methods.h"
#include "nmos/configuration_utils.h"

#include "bst/test/test.h"


////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testGetPropertiesByPath)
{
    using web::json::value_of;
    using web::json::value;

    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor = nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

    bool create_validation_fingerprint_called = false;
    // callback stubs
    nmos::create_validation_fingerprint_handler create_validation_fingerprint = [&](const nmos::resources& resources, const nmos::resource& resource)
    {
        create_validation_fingerprint_called = true;
        return U("test fingerprint");
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
    nmos::make_rebuildable(receivers);
    // root, receivers, mon1
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({{nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})}}));
    // make monitor1 rebuildable
    nmos::make_rebuildable(monitor1);

    auto monitor_class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    nmos::nc::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::nc::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::nc::push_back(root_block, receivers);
    // add class-manager to root-block
    nmos::nc::push_back(root_block, class_manager);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(receivers));
    insert_resource(resources, std::move(monitor1));
    insert_resource(resources, std::move(monitor2));

    {
        create_validation_fingerprint_called = false;
        const auto target_role_path = value_of({ U("root") });
        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        auto method_result = get_properties_by_path(resources, *resource, true, true, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, create_validation_fingerprint);

        BST_REQUIRE_EQUAL(nmos::nc_method_status::ok, nmos::fields::nc::status(method_result));

        const auto& bulk_properties_holder = nmos::fields::nc::value(method_result);
        const auto& object_properties_holders = nmos::fields::nc::values(bulk_properties_holder);

        BST_REQUIRE_EQUAL(5, object_properties_holders.size());

        BST_CHECK(create_validation_fingerprint_called);
    }
    {
        create_validation_fingerprint_called = false;
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        auto method_result = get_properties_by_path(resources, *resource, true, true, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, create_validation_fingerprint);

        BST_REQUIRE_EQUAL(nmos::nc_method_status::ok, nmos::fields::nc::status(method_result));

        const auto& bulk_properties_holder = nmos::fields::nc::value(method_result);
        const auto& object_properties_holders = nmos::fields::nc::values(bulk_properties_holder);

        BST_REQUIRE_EQUAL(3, object_properties_holders.size());

        BST_CHECK(create_validation_fingerprint_called);
    }
    {
        create_validation_fingerprint_called = false;
        const auto target_role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        auto method_result = get_properties_by_path(resources, *resource, true, true, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, create_validation_fingerprint);

        BST_REQUIRE_EQUAL(nmos::nc_method_status::ok, nmos::fields::nc::status(method_result));

        const auto& bulk_properties_holder = nmos::fields::nc::value(method_result);
        const auto& object_properties_holders = nmos::fields::nc::values(bulk_properties_holder);

        BST_REQUIRE_EQUAL(1, object_properties_holders.size());

        BST_CHECK(create_validation_fingerprint_called);
    }
}

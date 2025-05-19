// The first "test" is of course whether the header compiles standalone
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/configuration_resources.h"
#include "nmos/configuration_utils.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testIsRolePathRoot)
{
    {
        auto role_path = web::json::value_of({ U("root"), U("path1") }).as_array();
        auto role_path_root = web::json::value_of({ U("root"), U("path1") }).as_array();

        BST_REQUIRE(nmos::is_role_path_root(role_path_root, role_path));
    }
    {
        auto role_path = web::json::value_of({ U("root"), U("path1"), U("path2"), U("path3")}).as_array();
        auto role_path_root = web::json::value_of({ U("root"), U("path1"), U("path2") }).as_array();

        BST_REQUIRE(nmos::is_role_path_root(role_path_root, role_path));
    }
    {
        auto role_path = web::json::value_of({ U("root"), U("path1") }).as_array();
        auto role_path_root = web::json::value_of({ U("root"), U("path1"), U("path2") }).as_array();

        BST_REQUIRE(!nmos::is_role_path_root(role_path_root, role_path));
    }
    {
        auto role_path = web::json::value_of({ U("root"), U("path3"), U("path4") }).as_array();
        auto role_path_root = web::json::value_of({ U("root"), U("path1"), U("path2") }).as_array();

        BST_REQUIRE(!nmos::is_role_path_root(role_path_root, role_path));
    }
    {
        auto role_path = web::json::value_of({ U("path3"), U("path4") }).as_array();
        auto role_path_root = web::json::value_of({ U("root"), U("path1"), U("path2") }).as_array();

        BST_REQUIRE(!nmos::is_role_path_root(role_path_root, role_path));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testIsBlockModified)
{
    using web::json::value_of;
    using web::json::value;

    // Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto oid = nmos::root_block_oid;
    // root, receivers
    auto receivers = nmos::make_block(++oid, nmos::root_block_oid, U("receivers"), U("Receivers"), U("Receivers block"));
    auto receiver_block_oid = oid;
    // root, receivers, mon1
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));
    auto monitor_1_oid = oid;
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    auto monitor_2_oid = oid;
    nmos::nc::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::nc::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::nc::push_back(root_block, receivers);

    // Create Object Properties Holder
    auto role_path = value::array();
    push_back(role_path, U("root"));
    push_back(role_path, U("receivers"));

    // Members unchanged
    {
        auto property_value_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const nmos::nc_property_id property_id(2, 2); // block members
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(!nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed number of members
    {
        auto property_value_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
        const auto block_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("monitor 1"), monitor_1_oid, true, class_id, U("label"), receiver_block_oid);
        push_back(members, block_descriptor);
        const nmos::nc_property_id property_id(2, 2); // block members
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed oids
    {
        auto property_value_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), 10, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), 20, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const nmos::nc_property_id property_id(2, 2); // block members
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed roles
    {
        auto property_value_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon3"), monitor_1_oid, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon4"), monitor_2_oid, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const nmos::nc_property_id property_id(2, 2); // block members
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed class id
    {
        auto property_value_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 1 });
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const nmos::nc_property_id property_id(2, 2); // block members
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed owner oid
    {
        auto property_value_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 1 });
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, class_id, U("monitor 1"), 20);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, class_id, U("monitor 2"), 20);
            push_back(members, block_member_descriptor);
        }
        const nmos::nc_property_id property_id(2, 2); // block members
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed constant oid
    {
        auto property_value_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 1 });
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, false, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, false, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const nmos::nc_property_id property_id(2, 2); // block members
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testGetObjectPropertiesHolder)
{
    using web::json::value_of;
    using web::json::value;

    // Create Object Properties Holder
    auto object_properties_holders = value::array();

    {
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, value::boolean(false));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
    }
    {
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        auto property_value_holders = value::array();
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, value::boolean(false));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
    }
    {
        const auto role_path = value_of({ U("root"), U("senders"), U("mon1") });
        auto property_value_holders = value::array();
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, value::boolean(false));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
    }

    {
        const auto target_role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        web::json::array object_property_holder = nmos::get_object_properties_holder(object_properties_holders.as_array(), target_role_path.as_array());
        BST_REQUIRE_EQUAL(1, object_property_holder.size());
        BST_CHECK_EQUAL(target_role_path.as_array(), nmos::fields::nc::path(*object_property_holder.begin()));
    }
    {
        const auto target_role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        web::json::array object_property_holder = nmos::get_object_properties_holder(object_properties_holders.as_array(), target_role_path.as_array());
        BST_REQUIRE_EQUAL(1, object_property_holder.size());
        BST_CHECK_EQUAL(target_role_path.as_array(), nmos::fields::nc::path(*object_property_holder.begin()));
    }
    {
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        web::json::array object_property_holder = nmos::get_object_properties_holder(object_properties_holders.as_array(), target_role_path.as_array());
        BST_REQUIRE_EQUAL(0, object_property_holder.size());
    }
    {
        const auto target_role_path = value_of({ U("root"), U("does_not_exist") });
        web::json::array object_property_holder = nmos::get_object_properties_holder(object_properties_holders.as_array(), target_role_path.as_array());
        BST_REQUIRE_EQUAL(0, object_property_holder.size());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testGetChildObjectPropertiesHolders)
{
    using web::json::value_of;
    using web::json::value;

    // Create Object Properties Holder
    auto object_properties_holders = value::array();

    {
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, value::boolean(false));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
    }
    {
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        auto property_value_holders = value::array();
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, value::boolean(false));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
    }
    {
        const auto role_path = value_of({ U("root"), U("senders"), U("mon1") });
        auto property_value_holders = value::array();
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, value::boolean(false));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
    }

    {
        const auto target_role_path = value_of({ U("root"), U("receivers") });

        const auto child_object_properties_holders = nmos::get_child_object_properties_holders(object_properties_holders.as_array(), target_role_path.as_array());

        BST_REQUIRE_EQUAL(2, child_object_properties_holders.size());

        const auto& object_properties_holder1 = nmos::get_object_properties_holder(child_object_properties_holders, value_of({ U("root"), U("receivers"), U("mon1") }).as_array());
        BST_CHECK_EQUAL(1, object_properties_holder1.size());

        const auto& object_properties_holder2 = nmos::get_object_properties_holder(child_object_properties_holders, value_of({ U("root"), U("receivers"), U("mon2") }).as_array());
        BST_CHECK_EQUAL(1, object_properties_holder2.size());
    }
    {
        const auto target_role_path = value_of({ U("root"), U("receivers"), U("mon1")});

        const auto child_object_properties_holders = nmos::get_child_object_properties_holders(object_properties_holders.as_array(), target_role_path.as_array());

        BST_REQUIRE_EQUAL(1, child_object_properties_holders.size());

        const auto& object_properties_holder1 = nmos::get_object_properties_holder(child_object_properties_holders, value_of({ U("root"), U("receivers"), U("mon1") }).as_array());
        BST_CHECK_EQUAL(1, object_properties_holder1.size());
    }
    {
        const auto target_role_path = value_of({ U("root"), U("does_not_exist") });

        const auto child_object_properties_holders = nmos::get_child_object_properties_holders(object_properties_holders.as_array(), target_role_path.as_array());

        BST_REQUIRE_EQUAL(0, child_object_properties_holders.size());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testGetRolePath)
{
    // Create Fake Device Model
    using web::json::value_of;
    using web::json::value;

    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::resources resources;

    // root
    auto root_block = nmos::make_root_block();
    auto oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager = nmos::make_class_manager(++oid, control_protocol_state);
    nmos::nc_oid receiver_block_oid = ++oid;
    // root, receivers
    auto receivers = nmos::make_block(receiver_block_oid, nmos::root_block_oid, U("receivers"), U("Receivers"), U("Receivers block"));
    nmos::make_rebuildable(receivers);
    // root, receivers, mon1
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));
    nmos::nc_class_id monitor_class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
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

    auto expected_role_paths = value::array();
    push_back(expected_role_paths, value_of({ U("root") }));
    push_back(expected_role_paths, value_of({ U("root"), U("ClassManager")}));
    push_back(expected_role_paths, value_of({ U("root"), U("receivers") }));
    push_back(expected_role_paths, value_of({ U("root"), U("receivers"), U("mon1") }));
    push_back(expected_role_paths, value_of({ U("root"), U("receivers"), U("mon2") }));

    for (const auto& expected_role_path : expected_role_paths.as_array())
    {
        const auto& resource = nmos::nc::find_resource_by_role_path(resources, expected_role_path.as_array());
        const auto actual_role_path = nmos::get_role_path(resources, *resource);
        BST_CHECK_EQUAL(expected_role_path.as_array(), actual_role_path);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testApplyBackupDataSet)
{
    using web::json::value_of;
    using web::json::value;

    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);

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

    auto monitor_1_oid = oid;
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

    bool filter_property_value_holders_called = false;
    bool modify_rebuildable_block_called = false;

    // callback stubs
    nmos::filter_property_value_holders_handler filter_property_value_holders = [&](const nmos::resource& resource, const web::json::array& target_role_path, const web::json::array& property_values, bool recurse, bool validate, web::json::array& property_restore_notices, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
        {
            filter_property_value_holders_called = true;
            auto modifiable_property_value_holders = value::array();

            for (const auto& property_value : property_values)
            {
                web::json::push_back(modifiable_property_value_holders, property_value);
            }
            return modifiable_property_value_holders.as_array();
        };
    nmos::modify_rebuildable_block_handler modify_rebuildable_block = [&](const nmos::resource& resource, const web::json::array& target_role_path, const web::json::array& object_properties_holders, bool recurse, bool validate, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor)
        {
            modify_rebuildable_block_called = true;
            auto out = value::array();
            const auto& object_properties_set_validation = nmos::make_object_properties_set_validation(target_role_path, nmos::nc_restore_validation_status::ok, U("OK"));
            web::json::push_back(out, object_properties_set_validation);
            return out;
        };

    {
        // Check the successful modification of the "enabled" flag of mon1's worker base class in Modify mode
        //
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, value::boolean(false));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        const auto target_role_path = value_of({ U("root"), U("receivers")});
        bool recurse = true;
        bool validate = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::modify };

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        auto object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());

        // not expecting callbacks to be invoked as no read only properties, or rebuildable blocks modified
        BST_CHECK(!filter_property_value_holders_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Check filter_property_value_holders_handler is called when changing a read only property of rebuildable object in Rebuild mode
        //
        filter_property_value_holders_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        const nmos::nc_property_id property_id(2, 1);
        // This is a read only property
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("connectionStatusMessage"), U("NcString"), false, value("change this value"));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };
        bool validate = true;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        // expecting callback to filter_property_value_holders_called
        // but not to modify_rebuildable_block_called
        BST_CHECK(filter_property_value_holders_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Check error generated when attempting to change a read only property of non-rebuidable object in Rebuild mode
        //
        filter_property_value_holders_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        auto property_value_holders = value::array();
        const nmos::nc_property_id property_id(2, 1);
        // This is a read only property
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("connectionStatusMessage"), U("NcString"), false, value("change this value"));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };
        bool validate = true;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        const auto& property_restore_notices = nmos::fields::nc::notices(object_properties_set_validation);
        // expectation a single notice for the read only property that couldn't be changed
        BST_REQUIRE_EQUAL(1, property_restore_notices.size());

        const auto& notice = *property_restore_notices.begin();
        BST_CHECK_EQUAL(nmos::nc_property_id(3, 2), nmos::details::parse_nc_property_id(nmos::fields::nc::id(notice)));
        BST_CHECK_EQUAL(U("connectionStatusMessage"), nmos::fields::nc::name(notice));
        BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::error, nmos::fields::nc::notice_type(notice));
        BST_CHECK_NE(U(""), nmos::fields::nc::notice_message(notice));

        // expecting callback to filter_property_value_holders_called
        // but not to modify_rebuildable_block_called
        BST_CHECK(!filter_property_value_holders_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Check an error is caused by trying to modify a read only property in Modify mode
        //
        filter_property_value_holders_called = false;
        modify_rebuildable_block_called = false;

        // Change a read only property in Rebuild mode
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        const nmos::nc_property_id property_id(2, 1);
        // This is a read only property
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("connectionStatusMessage"), U("NcString"), true, value("change this value")));
        // This is a writable property
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, false));
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::modify };
        bool validate = true;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        // expect overall status for object to be OK as although the read only property change should fail
        // the writable property should succeed
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        const auto& property_restore_notices = nmos::fields::nc::notices(object_properties_set_validation);
        // expectation a single notice for the read only property that couldn't be changed
        BST_REQUIRE_EQUAL(1, property_restore_notices.size());

        const auto& notice = *property_restore_notices.begin();
        BST_CHECK_EQUAL(nmos::nc_property_id(3, 2), nmos::details::parse_nc_property_id(nmos::fields::nc::id(notice)));
        BST_CHECK_EQUAL(U("connectionStatusMessage"), nmos::fields::nc::name(notice));
        BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::error, nmos::fields::nc::notice_type(notice));
        BST_CHECK_NE(U(""), nmos::fields::nc::notice_message(notice));

        BST_CHECK(!filter_property_value_holders_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Check modify_rebuildable_block_handler is called when trying to modify a rebuildable block
        //
        filter_property_value_holders_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers")});
        auto property_value_holders = value::array();
        auto members = value::array();

        push_back(members, nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("monitor 1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 2), U("members"), U("NcBlockMemberDescriptor"), true, members));
        push_back(object_properties_holders, nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        const auto object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());

        BST_CHECK(!filter_property_value_holders_called);
        BST_CHECK(modify_rebuildable_block_called);
    }
    {
        // Check that role paths outside of the scope of the target role path are errored
        //
        filter_property_value_holders_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("other_receivers") });
        auto property_value_holders = value::array();
        auto members = value::array();

        push_back(members, nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("monitor 1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 2), U("members"), U("NcBlockMemberDescriptor"), true, members));
        push_back(object_properties_holders, nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        const auto object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::not_found, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());

        BST_CHECK(!filter_property_value_holders_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Mixture of filter_property_value_holders_handler and errors in Rebuild mode
        //
        filter_property_value_holders_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        const nmos::nc_property_id property_id(2, 1);
        // This is a read only property
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("connectionStatusMessage"), U("NcString"), false, value("change this value"))); //read only
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcString"), false, false)); // error in data type
        push_back(object_properties_holders, nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };
        bool validate = true;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        const auto& property_restore_notices = nmos::fields::nc::notices(object_properties_set_validation);
        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        BST_REQUIRE_EQUAL(1, property_restore_notices.size());

        const auto notice = *property_restore_notices.begin();
        BST_CHECK_EQUAL(nmos::nc_property_id(2, 1), nmos::details::parse_nc_property_id(nmos::fields::nc::id(notice)));
        BST_CHECK_EQUAL(U("enabled"), nmos::fields::nc::name(notice));
        BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::error, nmos::fields::nc::notice_type(notice));
        BST_CHECK_NE(U(""), nmos::fields::nc::notice_message(notice));

        // expecting callback to filter_property_value_holders_called
        // but not to modify_rebuildable_block_called
        BST_CHECK(filter_property_value_holders_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Incorrect property name in property value holders
        //
        filter_property_value_holders_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        // This is a read only property
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("wrong_property_name"), U("NcString"), false, value("change this value"))); //read only
        push_back(object_properties_holders, nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };
        bool validate = true;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        const auto& property_restore_notices = nmos::fields::nc::notices(object_properties_set_validation);
        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        BST_REQUIRE_EQUAL(1, property_restore_notices.size());

        const auto notice = *property_restore_notices.begin();
        BST_CHECK_EQUAL(nmos::nc_property_id(3, 2), nmos::details::parse_nc_property_id(nmos::fields::nc::id(notice)));
        BST_CHECK_EQUAL(U("wrong_property_name"), nmos::fields::nc::name(notice));
        BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::error, nmos::fields::nc::notice_type(notice));
        BST_CHECK_NE(U(""), nmos::fields::nc::notice_message(notice));

        // expecting callback to filter_property_value_holders_called
        // but not to modify_rebuildable_block_called
        BST_CHECK(!filter_property_value_holders_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Incorrect property type in property value holders
        //
        filter_property_value_holders_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        // This is a read only property
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("connectionStatusMessage"), U("wrong_data_type"), false, value("change this value"))); //read only
        push_back(object_properties_holders, nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };
        bool validate = true;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        const auto& property_restore_notices = nmos::fields::nc::notices(object_properties_set_validation);
        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        BST_REQUIRE_EQUAL(1, property_restore_notices.size());

        const auto notice = *property_restore_notices.begin();
        BST_CHECK_EQUAL(nmos::nc_property_id(3, 2), nmos::details::parse_nc_property_id(nmos::fields::nc::id(notice)));
        BST_CHECK_EQUAL(U("connectionStatusMessage"), nmos::fields::nc::name(notice));
        BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::error, nmos::fields::nc::notice_type(notice));
        BST_CHECK_NE(U(""), nmos::fields::nc::notice_message(notice));

        // expecting callback to filter_property_value_holders_called
        // but not to modify_rebuildable_block_called
        BST_CHECK(!filter_property_value_holders_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    // ensure an error if trying to invoke rebuildable block when in Modify mode
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testApplyBackupDataSet_WithoutCallbacks)
{
    using web::json::value_of;
    using web::json::value;

    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);

    // Create Device Model
    // root
    auto root_block = nmos::make_root_block();
    auto oid = nmos::root_block_oid;
    // root, ClassManager
    auto class_manager = nmos::make_class_manager(++oid, control_protocol_state);
    const auto receiver_block_oid = ++oid;
    // root, receivers
    auto receivers = nmos::make_block(receiver_block_oid, nmos::root_block_oid, U("receivers"), U("Receivers"), U("Receivers block"));
    nmos::make_rebuildable(receivers);
    // root, receivers, mon1
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));
    nmos::make_rebuildable(monitor1);
    auto monitor_1_oid = oid;
    const auto monitor_class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
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

    // undefined callback stubs
    nmos::filter_property_value_holders_handler filter_property_value_holders;
    nmos::modify_rebuildable_block_handler modify_rebuildable_block;

    {
        // Check that Modify mode is unaffected by undefined Rebuild mode callbacks
        //
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, false);
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::modify };

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        const auto object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
    }
    {
        // Check that Rebuild mode is unaffected by undefined Rebuild mode callbacks when no objects are being rebuilt
        //
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, false);
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        const auto object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
    }
    {
        // Check undefined filter_property_value_holders_handler causes an unsupported mode error when attempting to modify a read only property in Rebuild mode
        //
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_value_holders = value::array();
        const nmos::nc_property_id property_id(2, 1);
        // This is a read only property
        const auto property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("connectionStatusMessage"), U("NcString"), false, value("change this value"));
        push_back(property_value_holders, property_value_holder);
        const auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };
        bool validate = true;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
    }
    {
        // Check undefined modify_rebuildable_block_handler causes an unsupported error when attempting to modify a rebuildable block
        //
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        auto property_value_holders = value::array();
        auto members = value::array();

        push_back(members, nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("monitor 1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 2), U("members"), U("NcBlockMemberDescriptor"), true, members));
        push_back(object_properties_holders, nmos::details::make_nc_object_properties_holder(role_path.as_array(), property_value_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, filter_property_value_holders, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        const auto object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
    }
}
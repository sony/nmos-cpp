// The first "test" is of course whether the header compiles standalone
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/configuration_methods.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testIsRolePathRoot)
{
    {
        web::json::value role_path = web::json::value_of({ U("root"), U("path1")});
        web::json::value role_path_root = web::json::value_of({ U("root"), U("path1")});

        BST_REQUIRE(nmos::is_role_path_root(role_path_root, role_path));
    }
    {
        web::json::value role_path = web::json::value_of({ U("root"), U("path1"), U("path2"), U("path3")});
        web::json::value role_path_root = web::json::value_of({ U("root"), U("path1"), U("path2") });

        BST_REQUIRE(nmos::is_role_path_root(role_path_root, role_path));
    }
    {
        web::json::value role_path = web::json::value_of({ U("root"), U("path1")});
        web::json::value role_path_root = web::json::value_of({ U("root"), U("path1"), U("path2") });

        BST_REQUIRE(!nmos::is_role_path_root(role_path_root, role_path));
    }
    {
        web::json::value role_path = web::json::value_of({ U("root"), U("path3"), U("path4") });
        web::json::value role_path_root = web::json::value_of({ U("root"), U("path1"), U("path2") });

        BST_REQUIRE(!nmos::is_role_path_root(role_path_root, role_path));
    }
    {
        web::json::value role_path = web::json::value_of({ U("path3"), U("path4") });
        web::json::value role_path_root = web::json::value_of({ U("root"), U("path1"), U("path2") });

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
    nmos::nc_oid oid = nmos::root_block_oid;
    // root, receivers
    auto receivers = nmos::make_block(++oid, nmos::root_block_oid, U("receivers"), U("Receivers"), U("Receivers block"));
    nmos::nc_oid receiver_block_oid = oid;
    // root, receivers, mon1
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));
    nmos::nc_oid monitor_1_oid = oid;
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    nmos::nc_oid monitor_2_oid = oid;
    nmos::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::push_back(root_block, receivers);

    // Create Object Properties Holder
    value role_path = value::array();
    push_back(role_path, U("root"));
    push_back(role_path, U("receivers"));

    // Members unchanged
    {
        value property_value_holders = value::array();

        value members = value::array();
        nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        nmos::nc_property_id property_id(2, 2); // block members
        web::json::value property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);

        BST_CHECK(!nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed number of members
    {
        value property_value_holders = value::array();

        value members = value::array();
        nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
        value block_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("monitor 1"), monitor_1_oid, true, class_id, U("label"), receiver_block_oid);
        push_back(members, block_descriptor);
        nmos::nc_property_id property_id(2, 2); // block members
        web::json::value property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed oids
    {
        value property_value_holders = value::array();

        value members = value::array();
        nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), 10, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), 20, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        nmos::nc_property_id property_id(2, 2); // block members
        web::json::value property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed roles
    {
        value property_value_holders = value::array();

        value members = value::array();
        nmos::nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon3"), monitor_1_oid, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon4"), monitor_2_oid, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        nmos::nc_property_id property_id(2, 2); // block members
        web::json::value property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed class id
    {
        value property_value_holders = value::array();

        value members = value::array();
        nmos::nc_class_id class_id = nmos::make_nc_class_id(nmos::nc_worker_class_id, 0, { 1 });
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        nmos::nc_property_id property_id(2, 2); // block members
        web::json::value property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed owner oid
    {
        value property_value_holders = value::array();

        value members = value::array();
        nmos::nc_class_id class_id = nmos::make_nc_class_id(nmos::nc_worker_class_id, 0, { 1 });
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, class_id, U("monitor 1"), 20);
            push_back(members, block_member_descriptor);
        }
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, class_id, U("monitor 2"), 20);
            push_back(members, block_member_descriptor);
        }
        nmos::nc_property_id property_id(2, 2); // block members
        web::json::value property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed constant oid
    {
        value property_value_holders = value::array();

        value members = value::array();
        nmos::nc_class_id class_id = nmos::make_nc_class_id(nmos::nc_worker_class_id, 0, { 1 });
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, false, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            value block_member_descriptor = nmos::details::make_nc_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, false, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        nmos::nc_property_id property_id(2, 2); // block members
        web::json::value property_value_holder = nmos::details::make_nc_property_value_holder(property_id, U("members"), U("NcBlockMemberDescriptor"), false, members);
        web::json::push_back(property_value_holders, property_value_holder);
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
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
    nmos::nc_oid oid = nmos::root_block_oid;
    // root, Class<anager
    auto class_manager = nmos::make_class_manager(++oid, control_protocol_state);
    nmos::nc_oid receiver_block_oid = ++oid;
    // root, receivers
    auto receivers = nmos::make_block(++oid, nmos::root_block_oid, U("receivers"), U("Receivers"), U("Receivers block"), web::json::value::null(), web::json::value::null(), web::json::value::array(), true);
    // root, receivers, mon1
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({{nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})}}));
    nmos::nc_oid monitor_1_oid = oid;
    nmos::nc_class_id monitor_class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(monitor1.data));
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    nmos::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::push_back(root_block, receivers);
    // add class-manager to root-block
    nmos::push_back(root_block, class_manager);
    insert_resource(resources, std::move(root_block));
    insert_resource(resources, std::move(class_manager));
    insert_resource(resources, std::move(receivers));
    insert_resource(resources, std::move(monitor1));
    insert_resource(resources, std::move(monitor2));

    bool modify_read_only_config_properties_called = false;
    bool modify_rebuildable_block_called = false;
    
    // callback stubs
    nmos::modify_read_only_config_properties_handler modify_read_only_config_properties = [&](nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const web::json::value& target_role_path, const web::json::value& object_properties_holders, bool recurse, const web::json::value& restore_mode, bool validate)
        {
            modify_read_only_config_properties_called = true;
            return nmos::details::make_nc_object_properties_set_validation(target_role_path, nmos::nc_restore_validation_status::ok, value::array(), U("OK"));
        };
    nmos::modify_rebuildable_block_handler modify_rebuildable_block = [&](nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, const web::json::value& target_role_path, const web::json::value& property_values, bool recurse, const web::json::value& restore_mode, bool validate)
        {
            modify_rebuildable_block_called = true;
            value out = value::array();
            const auto& object_properties_set_validation = nmos::details::make_nc_object_properties_set_validation(target_role_path, nmos::nc_restore_validation_status::ok, value::array(), U("OK"));
            web::json::push_back(out, object_properties_set_validation);
            return out;
        };

    {
        // Check the successful modification of the "enabled" flag of mon1's worker base class in Modify mode
        //
        // Create Object Properties Holder
        value object_properties_holders = value::array();
        value role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        value property_value_holders = value::array();
        value property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, false);
        push_back(property_value_holders, property_value_holder);
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);
        push_back(object_properties_holders, object_properties_holder);
        value target_role_path = value_of({ U("root"), U("receivers")});
        bool recurse = true;
        bool validate = true;
        web::json::value restore_mode = nmos::nc_restore_mode::restore_mode::modify;

        const auto& resource = find_control_protocol_resource_by_role_path(resources, target_role_path);
        value output = nmos::apply_backup_data_set(resources, get_control_protocol_class_descriptor, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, modify_read_only_config_properties, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        value object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path, nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());

        // not expecting callbacks to be invoked as no read only properties, or rebuildable blocks modified
        BST_CHECK(!modify_read_only_config_properties_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Check modify_read_only_config_properties_handler is called when changing a read only property in Rebuild mode
        // 
        modify_read_only_config_properties_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        value object_properties_holders = value::array();
        value role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        value property_value_holders = value::array();
        nmos::nc_property_id property_id(2, 1);
        // This is a read only property
        value property_value_holder = nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("connectionStatusMessage"), U("NcString"), false, value("change this value"));
        push_back(property_value_holders, property_value_holder);
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        value target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        web::json::value restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;
        bool validate = true;

        const auto& resource = find_control_protocol_resource_by_role_path(resources, target_role_path);
        value output = nmos::apply_backup_data_set(resources, get_control_protocol_class_descriptor, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, modify_read_only_config_properties, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        value object_properties_set_validation = output.as_array().at(0);

        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        // expecting callback to modify_read_only_config_properties_called 
        // but not to modify_rebuildable_block_called
        BST_CHECK(modify_read_only_config_properties_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Check an error is caused by trying to modify a read only property in Modify mode
        //
        modify_read_only_config_properties_called = false;
        modify_rebuildable_block_called = false;

        // Change a read only property in Rebuild mode
        // Create Object Properties Holder
        value object_properties_holders = value::array();
        value role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        value property_value_holders = value::array();
        nmos::nc_property_id property_id(2, 1);
        // This is a read only property
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("connectionStatusMessage"), U("NcString"), true, value("change this value")));
        // This is a writable property
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcBoolean"), false, false));
        auto object_properties_holder = nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        value target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        web::json::value restore_mode = nmos::nc_restore_mode::restore_mode::modify;
        bool validate = true;

        const auto& resource = find_control_protocol_resource_by_role_path(resources, target_role_path);
        value output = nmos::apply_backup_data_set(resources, get_control_protocol_class_descriptor, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, modify_read_only_config_properties, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        value object_properties_set_validation = output.as_array().at(0);

        // expect overall status for object to be OK as although the read only property change should fail
        // the writable property should succeed
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        const auto& property_restore_notices = nmos::fields::nc::notices(object_properties_set_validation);
        // expectation a single notice for the read only property that couldn't be changed
        BST_REQUIRE_EQUAL(1, property_restore_notices.size());

        const auto notice = *property_restore_notices.begin();
        BST_CHECK_EQUAL(nmos::nc_property_id(3, 2), nmos::details::parse_nc_property_id(nmos::fields::nc::id(notice)));
        BST_CHECK_EQUAL(U("connectionStatusMessage"), nmos::fields::nc::name(notice));
        BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::error, nmos::fields::nc::notice_type(notice));
        BST_CHECK_NE(U(""), nmos::fields::nc::notice_message(notice));

        BST_CHECK(!modify_read_only_config_properties_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Check modify_rebuildable_block_handler is called when trying to modify a rebuildable block
        //
        modify_read_only_config_properties_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        value object_properties_holders = value::array();
        value role_path = value_of({ U("root"), U("receivers")});
        value property_value_holders = value::array();
        value members = value::array();
        
        push_back(members, nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("monitor 1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 2), U("members"), U("NcBlockMemberDescriptor"), true, members));
        push_back(object_properties_holders, nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false));
        value target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        web::json::value restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = find_control_protocol_resource_by_role_path(resources, target_role_path);
        value output = nmos::apply_backup_data_set(resources, get_control_protocol_class_descriptor, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, modify_read_only_config_properties, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        value object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path, nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());

        BST_CHECK(!modify_read_only_config_properties_called);
        BST_CHECK(modify_rebuildable_block_called);
    }
    {
        // Check that role paths outside of the scope of the target role path are errored
        //
        modify_read_only_config_properties_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        value object_properties_holders = value::array();
        value role_path = value_of({ U("root"), U("other_receivers") });
        value property_value_holders = value::array();
        value members = value::array();

        push_back(members, nmos::details::make_nc_block_member_descriptor(U("monitor 1"), U("monitor 1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 2), U("members"), U("NcBlockMemberDescriptor"), true, members));
        push_back(object_properties_holders, nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false));
        value target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        web::json::value restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = find_control_protocol_resource_by_role_path(resources, target_role_path);
        value output = nmos::apply_backup_data_set(resources, get_control_protocol_class_descriptor, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, modify_read_only_config_properties, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        value object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path, nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::not_found, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());

        BST_CHECK(!modify_read_only_config_properties_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }
    {
        // Mixture of modify_read_only_config_properties_handler and errors in Rebuild mode
        // 
        modify_read_only_config_properties_called = false;
        modify_rebuildable_block_called = false;

        // Create Object Properties Holder
        value object_properties_holders = value::array();
        value role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        value property_value_holders = value::array();
        nmos::nc_property_id property_id(2, 1);
        // This is a read only property
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(3, 2), U("connectionStatusMessage"), U("NcString"), false, value("change this value"))); //read only
        push_back(property_value_holders, nmos::details::make_nc_property_value_holder(nmos::nc_property_id(2, 1), U("enabled"), U("NcString"), false, false)); // error in data type
        push_back(object_properties_holders, nmos::details::make_nc_object_properties_holder(role_path, property_value_holders, false));
        // must be a more efficient way of initializing these role paths
        value target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        web::json::value restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;
        bool validate = true;

        const auto& resource = find_control_protocol_resource_by_role_path(resources, target_role_path);
        value output = nmos::apply_backup_data_set(resources, get_control_protocol_class_descriptor, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, modify_read_only_config_properties, modify_rebuildable_block);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        value object_properties_set_validation = output.as_array().at(0);

        const auto& property_restore_notices = nmos::fields::nc::notices(object_properties_set_validation);
        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        BST_REQUIRE_EQUAL(1, property_restore_notices.size());

        const auto notice = *property_restore_notices.begin();
        BST_CHECK_EQUAL(nmos::nc_property_id(2, 1), nmos::details::parse_nc_property_id(nmos::fields::nc::id(notice)));
        BST_CHECK_EQUAL(U("enabled"), nmos::fields::nc::name(notice));
        BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::error, nmos::fields::nc::notice_type(notice));
        BST_CHECK_NE(U(""), nmos::fields::nc::notice_message(notice));

        // expecting callback to modify_read_only_config_properties_called 
        // but not to modify_rebuildable_block_called
        BST_CHECK(modify_read_only_config_properties_called);
        BST_CHECK(!modify_rebuildable_block_called);
    }

    // ensure an error if trying to invoke rebuildable block when in Modify mode
}
// The first "test" is of course whether the header compiles standalone
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/configuration_handlers.h"
#include "nmos/configuration_methods.h"
#include "nmos/configuration_resources.h"
#include "nmos/configuration_utils.h"
#include "nmos/is12_versions.h"

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
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));
    auto monitor_1_oid = oid;
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
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

    const auto block_members_property_descriptor = nmos::nc::details::make_property_descriptor(U("members"), nmos::nc_block_members_property_id, nmos::fields::nc::members, U("NcBlockMemberDescriptor"), true, false, true, false, web::json::value::null());

    // Members unchanged
    {
        auto property_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members);
        web::json::push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(!nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed number of members
    {
        auto property_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
        const auto block_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("monitor 1"), monitor_1_oid, true, class_id, U("label"), receiver_block_oid);
        push_back(members, block_descriptor);
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members);
        web::json::push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed oids
    {
        auto property_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), 10, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), 20, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members);
        web::json::push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed roles
    {
        auto property_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon3"), monitor_1_oid, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon4"), monitor_2_oid, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members);
        web::json::push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed class id
    {
        auto property_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 1 });
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members);
        web::json::push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed owner oid
    {
        auto property_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 1 });
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, class_id, U("monitor 1"), 20);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, class_id, U("monitor 2"), 20);
            push_back(members, block_member_descriptor);
        }
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members);
        web::json::push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }

    // Changed constant oid
    {
        auto property_holders = value::array();

        auto members = value::array();
        const auto class_id = nmos::nc::make_class_id(nmos::nc_worker_class_id, 0, { 1 });
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, false, class_id, U("monitor 1"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        {
            const auto block_member_descriptor = nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, false, class_id, U("monitor 2"), receiver_block_oid);
            push_back(members, block_member_descriptor);
        }
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members);
        web::json::push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);

        BST_CHECK(nmos::is_block_modified(receivers, object_properties_holder));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testGetObjectPropertiesHolder)
{
    using web::json::value_of;
    using web::json::value;

    const auto enabled_property_descriptor = nmos::nc::details::make_property_descriptor(U("enabled"), nmos::nc_worker_enabled_property_id, nmos::fields::nc::enabled, U("NcBoolean"), false, false, false, false, web::json::value::null());

    // Create Object Properties Holder
    auto object_properties_holders = value::array();

    {
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_holders = value::array();
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_worker_enabled_property_id, enabled_property_descriptor, value::boolean(false));
        push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
    }
    {
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        auto property_holders = value::array();
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_worker_enabled_property_id, enabled_property_descriptor, value::boolean(false));
        push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
    }
    {
        const auto role_path = value_of({ U("root"), U("senders"), U("mon1") });
        auto property_holders = value::array();
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_worker_enabled_property_id, enabled_property_descriptor, value::boolean(false));
        push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
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
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));
    nmos::nc_class_id monitor_class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    nmos::nc::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::nc::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::nc::push_back(root_block, receivers);
    // add class-manager to root-block
    nmos::nc::push_back(root_block, class_manager);

    // insert root block and all sub control protocol resources to resource list
    nmos::nc::insert_root(resources, root_block);

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
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor= nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

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
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({{nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})}}));
    // make monitor1 rebuildable
    nmos::make_rebuildable(monitor1);

    auto monitor_1_oid = oid;
    auto monitor_class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    auto monitor_2_oid = oid;
    nmos::nc::push_back(receivers, monitor1);
    nmos::nc::push_back(receivers, monitor2);
    nmos::nc::push_back(root_block, receivers);
    // add class-manager to root-block
    nmos::nc::push_back(root_block, class_manager);

    // insert root block and all sub control protocol resources to resource list
    nmos::nc::insert_root(resources, root_block);

    bool get_read_only_modification_allow_list_called = false;
    bool remove_device_model_object_called = false;
    bool create_device_model_object_called = false;
    bool property_changed_called = false;

    // callback stubs
    nmos::control_protocol_property_changed_handler property_changed = [&](const nmos::resource& resource, const utility::string_t& property_name, int index)
        {
            property_changed_called = true;
        };

    nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list = [&](const nmos::resources& resources, const nmos::resource& resource, const std::vector<utility::string_t>& target_role_path, const std::vector<nmos::nc_property_id>& property_ids)
    {
        get_read_only_modification_allow_list_called = true;
        return property_ids;
    };

    nmos::remove_device_model_object_handler remove_device_model_object = [&](const nmos::resource& resource, const std::vector<utility::string_t>& target_role_path, bool validate)
    {
        remove_device_model_object_called = true;

        return true;
    };

    nmos::create_device_model_object_handler create_device_model_object = [&](const nmos::nc_class_id& class_id, nmos::nc_oid oid, bool constant_oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, bool validate, const std::map<nmos::nc_property_id, web::json::value>& property_values)
    {
        using web::json::value;

        create_device_model_object_called = true;

        auto data = nmos::nc::details::make_object(nmos::nc_receiver_monitor_class_id, oid, true, owner, role, web::json::value::string(user_label), U(""), web::json::value::null(), web::json::value::null());

        return nmos::control_protocol_resource({ nmos::is12_versions::v1_0, nmos::types::nc_block, std::move(data), true });
    };

    // Capture initial state of device model so we can reset after each check
    const auto root_resource = nmos::nc::find_resource_by_role_path(resources, value_of({ U("root") }).as_array());
    auto initial_backup_dataset = nmos::get_properties_by_path(resources, *root_resource, true, false, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, nullptr);
    auto& initial_object_properties_holders = nmos::fields::nc::values(nmos::fields::nc::value(initial_backup_dataset));

    const auto enabled_property_descriptor = nmos::nc::details::make_property_descriptor(U("enabled"), nmos::nc_worker_enabled_property_id, nmos::fields::nc::enabled, U("NcBoolean"), false, false, false, false, web::json::value::null());
    const auto class_id_property_descriptor = nmos::nc::details::make_property_descriptor(U("classId"), nmos::nc_object_class_id_property_id, nmos::fields::nc::class_id, U("NcClassId"), true, false, false, false, web::json::value::null());
    {
        // Check the successful modification of the "enabled" flag of mon1's worker base class in Modify mode
        //
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_holders = value::array();
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_worker_enabled_property_id, enabled_property_descriptor, value::boolean(false));
        push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        const auto target_role_path = value_of({ U("root"), U("receivers")});
        bool recurse = true;
        bool validate = false;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::modify;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        auto object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());

        // not expecting callbacks to be invoked as no read only properties, or rebuildable blocks modified
        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(!remove_device_model_object_called);
        BST_CHECK(!create_device_model_object_called);
        BST_CHECK(property_changed_called);

        // reset device model to initial state
        nmos::apply_backup_data_set(resources, *resource, initial_object_properties_holders, recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);
    }
    const auto connection_status_property_descriptor = nmos::nc::details::make_property_descriptor(U("connectionStatusMessage"), nmos::nc_receiver_monitor_connection_status_message_property_id, nmos::fields::nc::connection_status_message, U("NcString"), true, false, false, false, web::json::value::null());
    {
        // Check get_read_only_modification_allow_list_handler is called when changing a read only property of rebuildable object in Rebuild mode
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_holders = value::array();
        // This is a read only property
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_receiver_monitor_connection_status_message_property_id, connection_status_property_descriptor, value(U("change this value")));
        push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;
        bool validate = false;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));

        // expecting callback to get_read_only_modification_allow_list_called
        // but not to modify_rebuildable_block_called
        BST_CHECK(get_read_only_modification_allow_list_called);
        BST_CHECK(!remove_device_model_object_called);
        BST_CHECK(!create_device_model_object_called);
        BST_CHECK(property_changed_called);

        // reset device model to initial state
        nmos::apply_backup_data_set(resources, *resource, initial_object_properties_holders, recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);
    }
    {
        // Check error generated when attempting to change a read only property of non-rebuidable object in Rebuild mode
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        auto property_holders = value::array();
        // This is a read only property
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_receiver_monitor_connection_status_message_property_id, connection_status_property_descriptor, value(U("change this value")));
        push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;
        bool validate = true;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));

        const auto& property_restore_notices = nmos::fields::nc::notices(object_properties_set_validation);
        // expectation a single notice for the read only property that couldn't be changed
        BST_REQUIRE_EQUAL(1, property_restore_notices.size());

        const auto& notice = *property_restore_notices.begin();
        BST_CHECK_EQUAL(nmos::nc_receiver_monitor_connection_status_message_property_id, nmos::nc::details::parse_property_id(nmos::fields::nc::id(notice)));
        BST_CHECK_EQUAL(nmos::fields::nc::connection_status_message.key, nmos::fields::nc::name(notice));
        BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::error, nmos::fields::nc::notice_type(notice));
        BST_CHECK_NE(U(""), nmos::fields::nc::notice_message(notice));

        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(!remove_device_model_object_called);
        BST_CHECK(!create_device_model_object_called);
        BST_CHECK(!property_changed_called);
    }
    {
        // Check an error is caused by trying to modify a read only property in Modify mode
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        // Change a read only property in Rebuild mode
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_holders = value::array();
        // This is a read only property
        push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_receiver_monitor_connection_status_message_property_id, connection_status_property_descriptor, nmos::nc_connection_status::partially_healthy));
        // This is a writable property
        push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_worker_enabled_property_id, enabled_property_descriptor, value::boolean(false)));
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::modify;
        bool validate = false;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        // expect overall status for object to be failed as although the writable property should succeed,
        // the read only property change should fail with an error notice
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));

        const auto& property_restore_notices = nmos::fields::nc::notices(object_properties_set_validation);
        // expectation a single notice for the read only property that couldn't be changed
        BST_REQUIRE_EQUAL(1, property_restore_notices.size());

        const auto& notice = *property_restore_notices.begin();
        BST_CHECK_EQUAL(nmos::nc_receiver_monitor_connection_status_message_property_id, nmos::nc::details::parse_property_id(nmos::fields::nc::id(notice)));
        BST_CHECK_EQUAL(nmos::fields::nc::connection_status_message.key, nmos::fields::nc::name(notice));
        BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::error, nmos::fields::nc::notice_type(notice));
        BST_CHECK_NE(U(""), nmos::fields::nc::notice_message(notice));

        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(!remove_device_model_object_called);
        BST_CHECK(!create_device_model_object_called);
        BST_CHECK(property_changed_called);

        // reset device model to initial state
        nmos::apply_backup_data_set(resources, *resource, initial_object_properties_holders, recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);
    }
    const auto block_members_property_descriptor = nmos::nc::details::make_property_descriptor(U("members"), nmos::nc_block_members_property_id, nmos::fields::nc::members, U("NcBlockMemberDescriptor"), true, false, true, false, web::json::value::null());
    {
        // Check remove_device_model_object_called is called when trying to modify a rebuildable block
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        auto property_holders = value::array();
        auto members = value::array();

        push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
        push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = false;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        const auto object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());

        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(remove_device_model_object_called);
        BST_CHECK(!create_device_model_object_called);
        BST_CHECK(property_changed_called);

        // reset device model to initial state
        nmos::apply_backup_data_set(resources, *resource, initial_object_properties_holders, recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);
    }
    const auto oid_property_descriptor = nmos::nc::details::make_property_descriptor(U("oid"), nmos::nc_object_oid_property_id, nmos::fields::nc::oid, U("NcOid"), true, false, false, false, web::json::value::null());
    {
        // Check create_device_model_object_called is called when trying to modify a rebuildable block
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        auto monitor_3_oid = 999;
        // Create Object Properties Holder for Block, with a Property Holder for the block members
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        {
            auto property_holders = value::array();
            auto members = value::array();
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 3"), U("mon3"), monitor_3_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        // Create Object Properties Holder for new monitor
        const auto monitor_2_role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_2_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_2_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto monitor_3_role_path = value_of({ U("root"), U("receivers"), U("mon3") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_3_oid));
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_class_id_property_id, class_id_property_descriptor, nmos::nc::details::make_class_id(nmos::nc_receiver_monitor_class_id)));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_3_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = false;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one for the block and one each for the monitors
        BST_REQUIRE_EQUAL(3, output.as_array().size());

        // Check the correct object properties holders have been returned
        const auto& block_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), role_path.as_array());
        BST_REQUIRE_EQUAL(block_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = block_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }
        const auto& monitor_2_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_2_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_2_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_2_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }
        const auto& monitor_3_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_3_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_3_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_3_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }

        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(!remove_device_model_object_called);
        BST_CHECK(create_device_model_object_called);
        BST_CHECK(property_changed_called);
    }
    {
        // Check that role paths outside of the scope of the target role path are errored
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("other_receivers") });
        auto property_holders = value::array();
        auto members = value::array();

        push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
        push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = false;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation no object_properties_holders as not in the restore scope
        BST_REQUIRE_EQUAL(0, output.as_array().size());

        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(!remove_device_model_object_called);
        BST_CHECK(!create_device_model_object_called);
        BST_CHECK(!property_changed_called);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testApplyBackupDataSet_WithoutCallbacks)
{
    using web::json::value_of;
    using web::json::value;

    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor= nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

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
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));
    nmos::make_rebuildable(monitor1);
    auto monitor_1_oid = oid;
    const auto monitor_class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    nmos::nc::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::nc::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::nc::push_back(root_block, receivers);
    // add class-manager to root-block
    nmos::nc::push_back(root_block, class_manager);

    // insert root block and all sub control protocol resources to resource list
    nmos::nc::insert_root(resources, root_block);

    // undefined callback stubs
    nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list;
    nmos::remove_device_model_object_handler remove_device_model_object;
    nmos::create_device_model_object_handler create_device_model_object;
    nmos::control_protocol_property_changed_handler property_changed;

    const auto enabled_property_descriptor = nmos::nc::details::make_property_descriptor(U("enabled"), nmos::nc_worker_enabled_property_id, nmos::fields::nc::enabled, U("NcBoolean"), false, false, false, false, web::json::value::null());
    {
        // Check that Modify mode is unaffected by undefined Rebuild mode callbacks
        //
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_holders = value::array();
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_worker_enabled_property_id, enabled_property_descriptor, value::boolean(false));
        push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::modify;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

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
        auto property_holders = value::array();
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_worker_enabled_property_id, enabled_property_descriptor, value::boolean(false));
        push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        const auto object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
    }
    {
        // Check undefined get_read_only_modification_allow_list_handler causes an unsupported mode error when attempting to modify a read only property in Rebuild mode
        //
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        auto property_holders = value::array();
        // This is a read only property
        const auto property_descriptor = nmos::nc::details::make_property_descriptor(U("connectionStatusMessage"), nmos::nc_receiver_monitor_connection_status_message_property_id, nmos::fields::nc::connection_status_message, U("NcString"), true, false, false, false, web::json::value::null());
        const auto property_holder = nmos::nc::details::make_property_holder(nmos::nc_receiver_monitor_connection_status_message_property_id, property_descriptor, value(U("change this value")));
        push_back(property_holders, property_holder);
        const auto object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, object_properties_holder);
        // must be a more efficient way of initializing these role paths
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;
        bool validate = true;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());

        const auto object_properties_set_validation = output.as_array().at(0);

        // make sure the validation status propagates from the callback
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
    }
    {
        const auto block_members_property_descriptor = nmos::nc::details::make_property_descriptor(U("members"), nmos::nc_block_members_property_id, nmos::fields::nc::members, U("NcBlockMemberDescriptor"), true, false, true, false, web::json::value::null());
        const auto oid_property_descriptor = nmos::nc::details::make_property_descriptor(U("oid"), nmos::nc_object_oid_property_id, nmos::fields::nc::oid, U("NcOid"), true, false, false, false, web::json::value::null());
        // Check undefined create_device_model_object and remove_device_model_object causes an unsupported error when attempting to modify a rebuildable block
        //
        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        auto property_holders = value::array();
        auto members = value::array();

        push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
        push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        const auto monitor_1_role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_1_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_1_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holder
        BST_CHECK_EQUAL(2, output.as_array().size());
        const auto& block_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), role_path.as_array());
        BST_REQUIRE_EQUAL(block_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = block_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
            BST_REQUIRE_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }
        const auto& monitor_1_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_1_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_1_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_1_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testApplyBackupDataSet_AddDeviceModelObject)
{
    using web::json::value_of;
    using web::json::value;

    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor= nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

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
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({{nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})}}));
    // make monitor1 rebuildable
    nmos::make_rebuildable(monitor1);

    auto monitor_1_oid = oid;
    auto monitor_class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    auto monitor_2_oid = oid;
    nmos::nc::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::nc::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::nc::push_back(root_block, receivers);
    // add class-manager to root-block
    nmos::nc::push_back(root_block, class_manager);

    // insert root block and all sub control protocol resources to resource list
    nmos::nc::insert_root(resources, root_block);

    bool get_read_only_modification_allow_list_called = false;
    bool remove_device_model_object_called = false;
    bool create_device_model_object_called = false;
    bool property_changed_called = false;

    // callback stubs
    nmos::control_protocol_property_changed_handler property_changed = [&](const nmos::resource& resource, const utility::string_t& property_name, int index)
        {
            property_changed_called = true;
        };

    nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list = [&](const nmos::resources& resources, const nmos::resource& resource, const std::vector<utility::string_t>& target_role_path, const std::vector<nmos::nc_property_id>& property_ids)
    {
        get_read_only_modification_allow_list_called = true;
        return property_ids;
    };

    nmos::remove_device_model_object_handler remove_device_model_object = [&](const nmos::resource& resource, const std::vector<utility::string_t>& target_role_path, bool validate)
    {
        remove_device_model_object_called = true;

        return true;
    };

    nmos::create_device_model_object_handler create_device_model_object = [&](const nmos::nc_class_id& class_id, nmos::nc_oid oid, bool constant_oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, bool validate, const std::map<nmos::nc_property_id, web::json::value>& property_values)
    {
        using web::json::value;

        create_device_model_object_called = true;

        auto data = nmos::nc::details::make_object(nmos::nc_receiver_monitor_class_id, oid, true, owner, role, web::json::value::string(user_label), U(""), web::json::value::null(), web::json::value::null());

        return nmos::control_protocol_resource({ nmos::is12_versions::v1_0, nmos::types::nc_block, std::move(data), true });
    };

    const auto block_members_property_descriptor = nmos::nc::details::make_property_descriptor(U("members"), nmos::nc_block_members_property_id, nmos::fields::nc::members, U("NcBlockMemberDescriptor"), true, false, true, false, web::json::value::null());
    const auto oid_property_descriptor = nmos::nc::details::make_property_descriptor(U("oid"), nmos::nc_object_oid_property_id, nmos::fields::nc::oid, U("NcOid"), true, false, false, false, web::json::value::null());
    const auto class_id_property_descriptor = nmos::nc::details::make_property_descriptor(U("classId"), nmos::nc_object_class_id_property_id, nmos::fields::nc::class_id, U("NcClassId"), true, false, false, false, web::json::value::null());
    {
        // Handle constant oid clash
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        // Create Object Properties Holder for Block, with a Property Holder for the block members
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        {
            auto property_holders = value::array();
            auto members = value::array();
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 3"), U("mon3"), monitor_2_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto monitor_2_role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_2_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_2_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        // Create Object Properties Holder for new monitor
        const auto monitor_3_role_path = value_of({ U("root"), U("receivers"), U("mon3") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_2_oid));
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_class_id_property_id, class_id_property_descriptor, nmos::nc::details::make_class_id(nmos::nc_receiver_monitor_class_id)));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_3_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one for the block and one each for the monitors
        BST_REQUIRE_EQUAL(3, output.as_array().size());

        // Check the correct object properties holders have been returned
        const auto& block_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), role_path.as_array());
        BST_REQUIRE_EQUAL(block_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = block_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
        }
        const auto& monitor_2_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_2_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_2_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_2_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        }
        const auto& monitor_3_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_3_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_3_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_3_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::device_error, nmos::fields::nc::status(object_properties_set_validation));
        }

        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(!remove_device_model_object_called);
        BST_CHECK(!create_device_model_object_called);
        BST_CHECK(!property_changed_called);
    }
    {
        // Check new oid is generated for new device model object
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        auto monitor_3_oid = 999;
        // Create Object Properties Holder for Block, with a Property Holder for the block members
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        {
            auto property_holders = value::array();
            auto members = value::array();
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 3"), U("mon3"), monitor_3_oid, false, monitor_class_id, U("label"), receiver_block_oid));
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        // Create Object Properties Holder for new monitor
        const auto monitor_2_role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_2_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_2_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto monitor_3_role_path = value_of({ U("root"), U("receivers"), U("mon3") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_3_oid));
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_class_id_property_id, class_id_property_descriptor, nmos::nc::details::make_class_id(nmos::nc_receiver_monitor_class_id)));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_3_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one for the block and one each for the monitors
        BST_REQUIRE_EQUAL(3, output.as_array().size());

        // Check the correct object properties holders have been returned
        const auto& block_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), role_path.as_array());
        BST_REQUIRE_EQUAL(block_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = block_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            // Expect a warning that the oid for mon3 has changed
            BST_REQUIRE_EQUAL(1, nmos::fields::nc::notices(object_properties_set_validation).size());
            const auto& notice = nmos::fields::nc::notices(object_properties_set_validation).at(0);
            BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::warning, nmos::fields::nc::notice_type(notice));
            const auto& property_id = nmos::fields::nc::id(notice);
            BST_CHECK_EQUAL(nmos::fields::nc::level(property_id), nmos::nc_block_members_property_id.level);
            BST_CHECK_EQUAL(nmos::fields::nc::index(property_id), nmos::nc_block_members_property_id.index);
        }
        const auto& monitor_2_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_2_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_2_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_2_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }
        const auto& monitor_3_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_3_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_3_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_3_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(1, nmos::fields::nc::notices(object_properties_set_validation).size());
            const auto& notice = nmos::fields::nc::notices(object_properties_set_validation).at(0);
            BST_CHECK_EQUAL(nmos::nc_property_restore_notice_type::warning, nmos::fields::nc::notice_type(notice));
            const auto& property_id = nmos::fields::nc::id(notice);
            BST_CHECK_EQUAL(nmos::fields::nc::level(property_id), nmos::nc_object_oid_property_id.level);
            BST_CHECK_EQUAL(nmos::fields::nc::index(property_id), nmos::nc_object_oid_property_id.index);
        }

        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(!remove_device_model_object_called);
        BST_CHECK(create_device_model_object_called);
        BST_CHECK(!property_changed_called);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testApplyBackupDataSet_NegativeTests)
{
    using web::json::value_of;
    using web::json::value;

    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor= nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

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
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));
    // make monitor1 rebuildable
    nmos::make_rebuildable(monitor1);

    auto monitor_1_oid = oid;
    auto monitor_class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    auto monitor_2_oid = oid;
    nmos::nc::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::nc::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::nc::push_back(root_block, receivers);
    // add class-manager to root-block
    nmos::nc::push_back(root_block, class_manager);

    // insert root block and all sub control protocol resources to resource list
    nmos::nc::insert_root(resources, root_block);

    bool get_read_only_modification_allow_list_called = false;
    bool remove_device_model_object_called = false;
    bool create_device_model_object_called = false;
    bool property_changed_called = true;

    // callback stubs
    nmos::control_protocol_property_changed_handler property_changed = [&](const nmos::resource& resource, const utility::string_t& property_name, int index)
        {
            property_changed_called = true;
        };

    nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list = [&](const nmos::resources& resources, const nmos::resource& resource, const std::vector<utility::string_t>& target_role_path, const std::vector<nmos::nc_property_id>& property_ids)
    {
        get_read_only_modification_allow_list_called = true;
        return property_ids;
    };

    nmos::remove_device_model_object_handler remove_device_model_object = [&](const nmos::resource& resource, const std::vector<utility::string_t>& target_role_path, bool validate)
    {
        remove_device_model_object_called = true;

        // Simulate error on removing object from device model
        return false;
    };

    nmos::create_device_model_object_handler create_device_model_object = [&](const nmos::nc_class_id& class_id, nmos::nc_oid oid, bool constant_oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, bool validate, const std::map<nmos::nc_property_id, web::json::value>& property_values)
    {
        create_device_model_object_called = true;
        // Simulate error on adding object to device model
        return nmos::control_protocol_resource();
    };
    const auto block_members_property_descriptor = nmos::nc::details::make_property_descriptor(U("members"), nmos::nc_block_members_property_id, nmos::fields::nc::members, U("NcBlockMemberDescriptor"), true, false, true, false, web::json::value::null());
    const auto oid_property_descriptor = nmos::nc::details::make_property_descriptor(U("oid"), nmos::nc_object_oid_property_id, nmos::fields::nc::oid, U("NcOid"), true, false, false, false, web::json::value::null());
    {
        // Check remove_device_model_object_called error is handled when attempting to modify a rebuildable block
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        auto property_holders = value::array();
        auto members = value::array();

        push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
        push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);
        // expectation is there will be a result for each of the object_properties_holders i.e. one
        BST_REQUIRE_EQUAL(1, output.as_array().size());
        const auto& object_properties_set_validation = output.as_array().at(0);

        BST_CHECK_EQUAL(role_path.as_array(), nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
        BST_REQUIRE_EQUAL(1, nmos::fields::nc::notices(object_properties_set_validation).size());
        const auto& notice = nmos::fields::nc::notices(object_properties_set_validation).at(0);
        BST_CHECK_EQUAL(nmos::fields::nc::notice_type(notice), nmos::nc_property_restore_notice_type::error);
        const auto& property_id = nmos::fields::nc::id(notice);
        BST_CHECK_EQUAL(nmos::fields::nc::level(property_id), nmos::nc_block_members_property_id.level);
        BST_CHECK_EQUAL(nmos::fields::nc::index(property_id), nmos::nc_block_members_property_id.index);

        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(remove_device_model_object_called);
        BST_CHECK(!create_device_model_object_called);
        BST_CHECK(!property_changed_called);
    }
    {
        // Check on remove_device_model_object_called error all other object properties holders are processed
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        auto property_holders = value::array();
        auto members = value::array();

        push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
        push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        const auto monitor_1_role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_1_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_1_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }

        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders
        BST_CHECK_EQUAL(2, output.as_array().size());

        // Check the correct object properties holders have been returned
        const auto& block_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), role_path.as_array());
        BST_REQUIRE_EQUAL(block_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = block_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(1, nmos::fields::nc::notices(object_properties_set_validation).size());
            const auto& notice = nmos::fields::nc::notices(object_properties_set_validation).at(0);
            BST_CHECK_EQUAL(nmos::fields::nc::notice_type(notice), nmos::nc_property_restore_notice_type::error);
            const auto& property_id = nmos::fields::nc::id(notice);
            BST_CHECK_EQUAL(nmos::fields::nc::level(property_id), nmos::nc_block_members_property_id.level);
            BST_CHECK_EQUAL(nmos::fields::nc::index(property_id), nmos::nc_block_members_property_id.index);
        }
        const auto& monitor_1_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_1_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_1_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_1_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }
    }
    const auto class_id_property_descriptor = nmos::nc::details::make_property_descriptor(U("classId"), nmos::nc_object_class_id_property_id, U("classId"), U("NcClassId"), true, false, false, false, web::json::value::null());
    {
        // Check create_device_model_object_called error is handled when trying to modify a rebuildable block
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        auto monitor_3_oid = 999;
        // Create Object Properties Holder for Block, with a Property Holder for the block members
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        {
            auto property_holders = value::array();
            auto members = value::array();
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 3"), U("mon3"), monitor_3_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        // Create Object Properties Holder for new monitor
        const auto monitor_1_role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_1_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_1_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto monitor_2_role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_2_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_2_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto monitor_3_role_path = value_of({ U("root"), U("receivers"), U("mon3") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_3_oid));
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_class_id_property_id, class_id_property_descriptor, nmos::nc::details::make_class_id(nmos::nc_receiver_monitor_class_id)));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_3_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders i.e. one for the block and one each for the monitors
        BST_CHECK_EQUAL(4, output.as_array().size());

        // Check the correct object properties holders have been returned
        const auto& block_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), role_path.as_array());
        BST_REQUIRE_EQUAL(block_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = block_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }
        const auto& monitor_1_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_1_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_1_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_1_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }
        const auto& monitor_2_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_2_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_2_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_2_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }
        const auto& monitor_3_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_3_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_3_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_3_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::device_error, nmos::fields::nc::status(object_properties_set_validation));
            BST_CHECK_EQUAL(0, nmos::fields::nc::notices(object_properties_set_validation).size());
        }

        BST_CHECK(!get_read_only_modification_allow_list_called);
        BST_CHECK(!remove_device_model_object_called);
        BST_CHECK(create_device_model_object_called);
        BST_CHECK(!property_changed_called);
    }
    {
        // Check duplicate block object properties holders are handled
        //
        get_read_only_modification_allow_list_called = false;
        remove_device_model_object_called = false;
        create_device_model_object_called = false;
        property_changed_called = false;

        // Create Object Properties Holder
        auto object_properties_holders = value::array();
        const auto role_path = value_of({ U("root"), U("receivers") });
        auto property_holders1 = value::array();
        auto members1 = value::array();
        auto property_holders2 = value::array();
        auto members2 = value::array();

        push_back(members1, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_holders1, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members1));
        push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders1.as_array(), value::array().as_array(), value::array().as_array(), false));
        // duplicate
        push_back(members2, nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
        push_back(property_holders2, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members2));
        push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path.as_array(), property_holders2.as_array(), value::array().as_array(), value::array().as_array(), false));
        const auto monitor_1_role_path = value_of({ U("root"), U("receivers"), U("mon1") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_1_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_1_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }

        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool recurse = true;
        bool validate = true;
        const auto restore_mode = nmos::nc_restore_mode::restore_mode::rebuild;

        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        const auto output = nmos::apply_backup_data_set(resources, *resource, object_properties_holders.as_array(), recurse, restore_mode, validate, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);

        // expectation is there will be a result for each of the object_properties_holders
        BST_CHECK_EQUAL(3, output.as_array().size());

        // Check the correct object properties holders have been returned
        const auto& block_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), role_path.as_array());
        BST_REQUIRE_EQUAL(block_object_properties_holder.size(), 2);  // expect a duplicate
        {
            const auto& object_properties_set_validation0 = block_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation0));
            const auto& object_properties_set_validation1 = block_object_properties_holder.at(1);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation1));
        }
        const auto& monitor_1_object_properties_holder = nmos::get_object_properties_holder(output.as_array(), monitor_1_role_path.as_array());
        BST_REQUIRE_EQUAL(monitor_1_object_properties_holder.size(), 1);
        {
            const auto& object_properties_set_validation = monitor_1_object_properties_holder.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testModifyRebuildableBlock)
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
    nmos::set_block_allowed_member_classes(receivers, {nmos::nc_receiver_monitor_class_id});
    // root, receivers, mon1
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})} }));
    // make monitor1 rebuildable
    nmos::make_rebuildable(monitor1);

    auto monitor_1_oid = oid;
    auto monitor_class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
    // root, receivers, mon2
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_2")})} }));
    auto monitor_2_oid = oid;
    nmos::nc::push_back(receivers, monitor1);
    // add example-control to root-block
    nmos::nc::push_back(receivers, monitor2);
    // add stereo-gain to root-block
    nmos::nc::push_back(root_block, receivers);
    // add class-manager to root-block
    nmos::nc::push_back(root_block, class_manager);

    // insert root block and all sub control protocol resources to resource list
    nmos::nc::insert_root(resources, root_block);

    bool get_read_only_modification_allow_list_called = false;
    bool remove_device_model_object_called = false;
    bool create_device_model_object_called = false;
    bool property_changed_called = true;

    // callback stubs
    nmos::control_protocol_property_changed_handler property_changed = [&](const nmos::resource& resource, const utility::string_t& property_name, int index)
        {
            property_changed_called = true;
        };

    nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list = [&](const nmos::resources& resources, const nmos::resource& resource, const std::vector<utility::string_t>& target_role_path, const std::vector<nmos::nc_property_id>& property_ids)
    {
        get_read_only_modification_allow_list_called = true;
        return property_ids;
    };

    nmos::remove_device_model_object_handler remove_device_model_object = [&](const nmos::resource& resource, const std::vector<utility::string_t>& target_role_path, bool validate)
    {
        remove_device_model_object_called = true;

        return true;
    };

    nmos::create_device_model_object_handler create_device_model_object = [&](const nmos::nc_class_id& class_id, nmos::nc_oid oid, bool constant_oid, nmos::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, bool validate, const std::map<nmos::nc_property_id, web::json::value>& property_values)
    {
        using web::json::value;

        create_device_model_object_called = true;

        auto data = nmos::nc::details::make_object(nmos::nc_receiver_monitor_class_id, oid, true, owner, role, web::json::value::string(user_label), U(""), web::json::value::null(), web::json::value::null());

        return nmos::control_protocol_resource({ nmos::is12_versions::v1_0, nmos::types::nc_block, std::move(data), true });
    };
    const auto block_members_property_descriptor = nmos::nc::details::make_property_descriptor(U("members"), nmos::nc_block_members_property_id, nmos::fields::nc::members, U("NcBlockMemberDescriptor"), true, false, true, false, web::json::value::null());
    const auto oid_property_descriptor = nmos::nc::details::make_property_descriptor(U("oid"), nmos::nc_object_oid_property_id, nmos::fields::nc::oid, U("NcOid"), true, false, false, false, web::json::value::null());
    const auto class_id_property_descriptor = nmos::nc::details::make_property_descriptor(U("classId"), nmos::nc_object_class_id_property_id, nmos::fields::nc::class_id, U("NcClassId"), true, false, false, false, web::json::value::null());

    // No class id specified in the object properties holder for new monitor causes an error
    {
        auto monitor_3_oid = 999;
        // Create Object Properties Holder for Block, with a Property Holder for the block members
        auto object_properties_holders = value::array();
        auto block_property_holders = value::array();
        property_changed_called = false;
        const auto role_path = value_of({ U("root"), U("receivers") });
        {
            auto members = value::array();
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 3"), U("mon3"), monitor_3_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(block_property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
        }
        const auto block_object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), block_property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, block_object_properties_holder);
        const auto monitor_2_role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_2_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_2_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        // Create Object Properties Holder for new monitor
        auto monitor3_property_holders = value::array();
        const auto monitor_3_role_path = value_of({ U("root"), U("receivers"), U("mon3") });
        {
            // No property holders, including no class id
            push_back(monitor3_property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_3_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_3_role_path.as_array(), monitor3_property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool validate = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };

        nmos::object_properties_map object_properties_holder_map;

        for (const auto& object_properties_holder: object_properties_holders.as_array())
        {
            const auto& role_path = nmos::fields::nc::path(object_properties_holder);
            object_properties_holder_map.insert({ role_path, object_properties_holder });
        }

        const auto resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());

        // allowed member classes specified for block but no class_id property holder in the new monitor object properties holder
        const auto object_set_validations = nmos::details::modify_rebuildable_block(resources, object_properties_holder_map, *resource, target_role_path.as_array(), block_object_properties_holder, validate, get_control_protocol_class_descriptor, remove_device_model_object, create_device_model_object, property_changed);

        BST_REQUIRE_EQUAL(object_set_validations.size(), 2);
        {
            const auto& object_properties_set_validation = object_set_validations.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
        }
        {
            // warnings perhaps?
            const auto& object_properties_set_validation = object_set_validations.at(1);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
        }
        BST_CHECK(!property_changed_called);
    }

    // add class id to the property holders, but use a disallowed class id
    {
        auto monitor_3_oid = 999;
        // Create Object Properties Holder for Block, with a Property Holder for the block members
        auto object_properties_holders = value::array();
        auto block_property_holders = value::array();
        property_changed_called = false;
        const auto role_path = value_of({ U("root"), U("receivers") });
        {
            auto members = value::array();
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 3"), U("mon3"), monitor_3_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(block_property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
        }
        const auto block_object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), block_property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, block_object_properties_holder);
        // Create Object Properties Holder for new monitor
        const auto monitor_2_role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_2_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_2_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        auto monitor3_property_holders = value::array();
        const auto monitor_3_role_path = value_of({ U("root"), U("receivers"), U("mon3") });
        {
            //auto property_holders = value::array();
            push_back(monitor3_property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_3_oid));
            push_back(monitor3_property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_class_id_property_id, class_id_property_descriptor, nmos::nc::details::make_class_id(nmos::nc_block_class_id))); // disallowed class id
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_3_role_path.as_array(), monitor3_property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool validate = true;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };

        nmos::object_properties_map object_properties_holder_map;

        for (const auto& object_properties_holder: object_properties_holders.as_array())
        {
            const auto& role_path = nmos::fields::nc::path(object_properties_holder);
            object_properties_holder_map.insert({ role_path, object_properties_holder });
        }

        const auto resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        // allowed member classes specified for block but class_id property holder has disallowed class_id
        const auto object_set_validations = nmos::details::modify_rebuildable_block(resources, object_properties_holder_map, *resource, target_role_path.as_array(), block_object_properties_holder, validate, get_control_protocol_class_descriptor, remove_device_model_object, create_device_model_object, property_changed);

        BST_REQUIRE_EQUAL(object_set_validations.size(), 2);
        {
            const auto& object_properties_set_validation = object_set_validations.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
        }
        {
            // warnings perhaps?
            const auto& object_properties_set_validation = object_set_validations.at(1);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::failed, nmos::fields::nc::status(object_properties_set_validation));
        }
        BST_CHECK(!property_changed_called);
    }

    // add class id to the property holders, and use an allowed class id
    {
        auto monitor_3_oid = 999;
        // Create Object Properties Holder for Block, with a Property Holder for the block members
        auto object_properties_holders = value::array();
        auto block_property_holders = value::array();
        property_changed_called = false;
        const auto role_path = value_of({ U("root"), U("receivers") });
        {
            auto members = value::array();
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 1"), U("mon1"), monitor_1_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 2"), U("mon2"), monitor_2_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(members, nmos::nc::details::make_block_member_descriptor(U("monitor 3"), U("mon3"), monitor_3_oid, true, monitor_class_id, U("label"), receiver_block_oid));
            push_back(block_property_holders, nmos::nc::details::make_property_holder(nmos::nc_block_members_property_id, block_members_property_descriptor, members));
        }
        const auto block_object_properties_holder = nmos::nc::details::make_object_properties_holder(role_path.as_array(), block_property_holders.as_array(), value::array().as_array(), value::array().as_array(), false);
        push_back(object_properties_holders, block_object_properties_holder);
        // Create Object Properties Holder for new monitor
        const auto monitor_2_role_path = value_of({ U("root"), U("receivers"), U("mon2") });
        {
            auto property_holders = value::array();
            push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_2_oid));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_2_role_path.as_array(), property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        auto monitor3_property_holders = value::array();
        const auto monitor_3_role_path = value_of({ U("root"), U("receivers"), U("mon3") });
        {
            //auto property_holders = value::array();
            push_back(monitor3_property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_oid_property_id, oid_property_descriptor, monitor_3_oid));
            push_back(monitor3_property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_class_id_property_id, class_id_property_descriptor, nmos::nc::details::make_class_id(nmos::nc_receiver_monitor_class_id)));
            push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(monitor_3_role_path.as_array(), monitor3_property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));
        }
        const auto target_role_path = value_of({ U("root"), U("receivers") });
        bool validate = false;
        const value restore_mode{ nmos::nc_restore_mode::restore_mode::rebuild };

        nmos::object_properties_map object_properties_holder_map;

        for (const auto& object_properties_holder: object_properties_holders.as_array())
        {
            const auto& role_path = nmos::fields::nc::path(object_properties_holder);
            object_properties_holder_map.insert({ role_path, object_properties_holder });
        }

        const auto resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        // allowed member classes specified for block and class_id property holder is allowed class_id
        const auto object_set_validations = nmos::details::modify_rebuildable_block(resources, object_properties_holder_map, *resource, target_role_path.as_array(), block_object_properties_holder, validate, get_control_protocol_class_descriptor, remove_device_model_object, create_device_model_object, property_changed);

        BST_REQUIRE_EQUAL(object_set_validations.size(), 2);
        {
            const auto& object_properties_set_validation = object_set_validations.at(0);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        }
        {
            // warnings perhaps?
            const auto& object_properties_set_validation = object_set_validations.at(1);
            BST_CHECK_EQUAL(nmos::nc_restore_validation_status::ok, nmos::fields::nc::status(object_properties_set_validation));
        }
        BST_CHECK(property_changed_called);
    }
}

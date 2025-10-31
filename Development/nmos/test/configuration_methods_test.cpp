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
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, receiver_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({{nmos::nc::details::make_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, U("id_1")})}}));
    // make monitor1 rebuildable
    nmos::make_rebuildable(monitor1);

    auto monitor_class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
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

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSetPropertiesByPath)
{
    using web::json::value_of;
    using web::json::value;

    nmos::resources resources;
    nmos::experimental::control_protocol_state control_protocol_state;
    nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);
    nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor = nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state);

    bool validate_validation_fingerprint_called = false;
    bool property_changed_called = false;
    // callback stubs
    nmos::validate_validation_fingerprint_handler validate_validation_fingerprint = [&](const nmos::resources& resources, const nmos::resource& resource, const utility::string_t& validation_fingerprint)
        {
            validate_validation_fingerprint_called = true;
            return true;
        };

    nmos::get_read_only_modification_allow_list_handler get_read_only_modification_allow_list = [&](const nmos::resources& resources, const nmos::resource& resource, const std::vector<utility::string_t>& role_path, const std::vector<nmos::nc_property_id>& property_ids)
        {
            return property_ids;
        };

    nmos::control_protocol_property_changed_handler property_changed = [&](const nmos::resource& resource, const utility::string_t& property_name, int index)
        {
            property_changed_called = true;
        };

    nmos::remove_device_model_object_handler remove_device_model_object = nullptr;
    nmos::create_device_model_object_handler create_device_model_object = nullptr;

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

    auto monitor_class_id = nmos::nc::details::parse_class_id(nmos::fields::nc::class_id(monitor1.data));
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

    {
        // create a backup dataset
        value object_properties_holders = value::array();
        value property_holders = value::array();
        web::json::push_back(property_holders, nmos::nc::details::make_property_holder(nmos::nc_object_user_label_property_id, false, value::string(U("test string"))));
        const auto role_path = get_role_path(resources, monitor1);
        web::json::push_back(object_properties_holders, nmos::nc::details::make_object_properties_holder(role_path, property_holders.as_array(), value::array().as_array(), value::array().as_array(), false));

        auto bulk_properties_holder = nmos::nc::details::make_bulk_properties_holder(U(""), object_properties_holders);

        validate_validation_fingerprint_called = false;
        property_changed_called = false;

        const auto target_role_path = value_of({ U("root") });
        const auto& resource = nmos::nc::find_resource_by_role_path(resources, target_role_path.as_array());
        auto method_result = set_properties_by_path(resources, *resource, bulk_properties_holder, true, nmos::nc_restore_mode::modify, get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor, validate_validation_fingerprint, get_read_only_modification_allow_list, remove_device_model_object, create_device_model_object, property_changed);
        BST_REQUIRE_EQUAL(nmos::nc_method_status::ok, nmos::fields::nc::status(method_result));

        const auto& object_property_set_validations = nmos::fields::nc::value(method_result);

        BST_REQUIRE_EQUAL(1, object_property_set_validations.size());

        BST_CHECK(validate_validation_fingerprint_called);
        BST_CHECK(property_changed_called);
    }
}


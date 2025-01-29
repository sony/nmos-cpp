// The first "test" is of course whether the header compiles standalone
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"

#include "nmos/is04_versions.h"

#include "bst/test/test.h"

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
    auto monitor1 = nmos::make_receiver_monitor(++oid, true, nmos::root_block_oid, U("mon1"), U("monitor 1"), U("monitor 1"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, touchpoint1_id})} }));
    auto monitor2 = nmos::make_receiver_monitor(++oid, true, nmos::root_block_oid, U("mon2"), U("monitor 2"), U("monitor 2"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, touchpoint2_id})} }));
    auto monitor3 = nmos::make_receiver_monitor(++oid, true, nmos::root_block_oid, U("mon3"), U("monitor 3"), U("monitor 3"), value_of({ {nmos::details::make_nc_touchpoint_nmos({nmos::ncp_touchpoint_resource_types::receiver, non_existant_id})} }));

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

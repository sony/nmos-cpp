// The first "test" is of course whether the header compiles standalone
#include "nmos/resources.h"

#include "bst/test/test.h"
#include "nmos/is04_versions.h"

namespace
{
    nmos::resource make_test_node(const nmos::id& id)
    {
        using web::json::value_of;

        return{ nmos::is04_versions::v1_3, nmos::types::node, value_of({
            { U("id"), id }
        }), false };
    }

    nmos::resource make_test_device(const nmos::id& id, const nmos::id& node_id)
    {
        using web::json::value_of;

        return{ nmos::is04_versions::v1_3, nmos::types::device, value_of({
            { U("id"), id },
            { U("node_id"), node_id }
        }), false };
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// A resource id is unique in the model, so a device that reuses an existing node id cannot
// be inserted. Previously, insert_resource still recorded that id in the node's sub_resources
// before the insert was rejected, leaving a self-reference that made set_resource_health /
// erase_resource recurse without bound.
// See https://github.com/sony/nmos-cpp/issues/403
BST_TEST_CASE(testInsertResourceRejectsDuplicateIdWithoutCorruptingSubResources)
{
    const nmos::id id{ U("11111111-1111-1111-1111-111111111111") };

    nmos::resources resources;
    BST_REQUIRE(nmos::insert_resource(resources, make_test_node(id)).second);

    // same id as the node, and node_id also equal to that id (the mistake from issue #403)
    BST_REQUIRE(!nmos::insert_resource(resources, make_test_device(id, id)).second);

    BST_REQUIRE_EQUAL(1u, resources.size());
    const auto node = nmos::find_resource(resources, { id, nmos::types::node });
    BST_REQUIRE(resources.end() != node);
    BST_REQUIRE(node->sub_resources.empty());

    // would previously stack-overflow / crash
    nmos::set_resource_health(resources, id);
    BST_REQUIRE_EQUAL(1u, nmos::erase_resource(resources, id));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testInsertResourceRecordsSubResources)
{
    const nmos::id node_id{ U("22222222-2222-2222-2222-222222222222") };
    const nmos::id device_id{ U("33333333-3333-3333-3333-333333333333") };

    nmos::resources resources;
    BST_REQUIRE(nmos::insert_resource(resources, make_test_node(node_id)).second);
    BST_REQUIRE(nmos::insert_resource(resources, make_test_device(device_id, node_id)).second);

    const auto node = nmos::find_resource(resources, { node_id, nmos::types::node });
    BST_REQUIRE(resources.end() != node);
    BST_REQUIRE_EQUAL(1u, node->sub_resources.count(device_id));

    const auto device = nmos::find_resource(resources, { device_id, nmos::types::device });
    BST_REQUIRE(resources.end() != device);
    BST_REQUIRE_EQUAL(node->health.load(), device->health.load());

    BST_REQUIRE_EQUAL(2u, nmos::erase_resource(resources, node_id));
}

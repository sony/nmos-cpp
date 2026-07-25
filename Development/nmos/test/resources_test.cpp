// The first "test" is of course whether the header compiles standalone
#include "nmos/resources.h"

#include "bst/test/test.h"
#include "nmos/is04_versions.h"

namespace
{
    nmos::resource make_test_node(const nmos::id& id)
    {
        using web::json::value_of;

        auto data = value_of({
            { U("id"), id }
        });
        return{ nmos::is04_versions::v1_3, nmos::types::node, std::move(data), id, false };
    }

    nmos::resource make_test_device(const nmos::id& id, const nmos::id& node_id)
    {
        using web::json::value_of;

        auto data = value_of({
            { U("id"), id },
            { U("node_id"), node_id }
        });
        return{ nmos::is04_versions::v1_3, nmos::types::device, std::move(data), id, false };
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testEraseResourceWithCyclicSubResources)
{
    // a resource listed as a sub-resource of itself must not cause unbounded recursion
    // see https://github.com/sony/nmos-cpp/issues/403
    {
        nmos::resources resources;
        auto self = make_test_node(U("self"));
        self.sub_resources.insert(U("self"));
        nmos::insert_resource(resources, std::move(self));

        BST_REQUIRE_EQUAL(1u, nmos::erase_resource(resources, U("self")));
        BST_REQUIRE(resources.empty());
    }

    // resources indirectly listed as sub-resources of themselves must not either
    {
        nmos::resources resources;
        auto a = make_test_node(U("a"));
        a.sub_resources.insert(U("b"));
        auto b = make_test_device(U("b"), U("a"));
        b.sub_resources.insert(U("a"));
        nmos::insert_resource(resources, std::move(a));
        nmos::insert_resource(resources, std::move(b));

        BST_REQUIRE_EQUAL(2u, nmos::erase_resource(resources, U("a")));
        BST_REQUIRE(resources.empty());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testEraseResourceWithSubResources)
{
    // erasing a resource also erases its sub-resources, and only those
    nmos::resources resources;
    auto node = make_test_node(U("node"));
    node.sub_resources.insert(U("device"));
    auto device = make_test_device(U("device"), U("node"));
    auto other = make_test_node(U("other"));
    nmos::insert_resource(resources, std::move(node));
    nmos::insert_resource(resources, std::move(device));
    nmos::insert_resource(resources, std::move(other));

    BST_REQUIRE_EQUAL(2u, nmos::erase_resource(resources, U("node")));
    BST_REQUIRE(resources.end() == resources.find(U("node")));
    BST_REQUIRE(resources.end() == resources.find(U("device")));
    BST_REQUIRE(resources.end() != resources.find(U("other")));
}

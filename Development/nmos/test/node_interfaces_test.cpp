// The first "test" is of course whether the header compiles standalone
#include "nmos/node_interfaces.h"

#include "bst/test/test.h"
#include "nmos/json_fields.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeParseNodeInterface)
{
    using web::json::value_of;

    const nmos::node_interface iface{
        U("aa-bb-cc-dd-ee-01"),
        U("aa-bb-cc-dd-ee-ff"),
        U("eth0"),
        U(""),
        U("")
    };

    const auto json = nmos::make_node_interface(iface);
    BST_REQUIRE(!json.at(nmos::fields::chassis_id).is_null());
    BST_REQUIRE_EQUAL(U("aa-bb-cc-dd-ee-01"), nmos::fields::chassis_id(json));
    BST_REQUIRE_EQUAL(U("aa-bb-cc-dd-ee-ff"), nmos::fields::port_id(json));
    BST_REQUIRE_EQUAL(U("eth0"), nmos::fields::name(json));
    BST_REQUIRE(!json.has_field(nmos::fields::attached_network_device));

    BST_REQUIRE(iface == nmos::parse_node_interface(json));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeParseNodeInterfaceNullChassisId)
{
    const nmos::node_interface iface{
        U(""),
        U("aa-bb-cc-dd-ee-ff"),
        U("eth0"),
        U(""),
        U("")
    };

    const auto json = nmos::make_node_interface(iface);
    BST_REQUIRE(json.at(nmos::fields::chassis_id).is_null());
    BST_REQUIRE(!json.has_field(nmos::fields::attached_network_device));

    BST_REQUIRE(iface == nmos::parse_node_interface(json));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeParseNodeInterfaceAttachedNetworkDevice)
{
    const nmos::node_interface iface{
        U(""),
        U("aa-bb-cc-dd-ee-ff"),
        U("eth0"),
        U("11-22-33-44-55-66"),
        U("77-88-99-aa-bb-cc")
    };

    const auto json = nmos::make_node_interface(iface);
    BST_REQUIRE(json.has_field(nmos::fields::attached_network_device));

    const auto& attached = nmos::fields::attached_network_device(json);
    BST_REQUIRE_EQUAL(U("11-22-33-44-55-66"), nmos::fields::chassis_id(attached));
    BST_REQUIRE_EQUAL(U("77-88-99-aa-bb-cc"), nmos::fields::port_id(attached));

    BST_REQUIRE(iface == nmos::parse_node_interface(json));
}

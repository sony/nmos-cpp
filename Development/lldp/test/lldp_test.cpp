// The first "test" is of course whether the header compiles standalone
#include "lldp/lldp.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMacAddress)
{
    std::vector<uint8_t> mac{ 0xD4, 0xC9, 0xEF, 0xED, 0xC1, 0x09 };
    BST_REQUIRE_EQUAL("d4-c9-ef-ed-c1-09", lldp::parse_mac_address(mac));
    BST_REQUIRE_EQUAL(mac, lldp::make_mac_address("d4-c9-ef-ed-c1-09"));

    BST_REQUIRE_EQUAL(mac, lldp::make_mac_address("D4:C9:EF:ED:C1:09"));

    std::vector<uint8_t> not_mac{ 0xD4, 0xC9, 0xEF };
    BST_REQUIRE(lldp::parse_mac_address(not_mac).empty());
    BST_REQUIRE(lldp::make_mac_address("D4-C9-EF").empty());
}

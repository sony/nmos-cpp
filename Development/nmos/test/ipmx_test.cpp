// The first "test" is of course whether the header compiles standalone
#include "nmos/ipmx.h"

#include <stdexcept>
#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testIpmxDefaultMulticastAddress)
{
    // example from VSF TR-10-9:2025 Section 17.1
    BST_REQUIRE_EQUAL(U("239.1.123.45"), nmos::ipmx::make_default_multicast_address_v4(U("192.168.123.45"), 1));
    BST_REQUIRE_EQUAL(U("239.2.123.45"), nmos::ipmx::make_default_multicast_address_v4(U("192.168.123.45"), 2));

    BST_REQUIRE_EQUAL(U("239.127.0.1"), nmos::ipmx::make_default_multicast_address_v4(U("10.0.0.1"), 127));

    BST_REQUIRE_THROW(nmos::ipmx::make_default_multicast_address_v4(U("192.168.123.45"), 0), std::invalid_argument);
    BST_REQUIRE_THROW(nmos::ipmx::make_default_multicast_address_v4(U("192.168.123.45"), 128), std::invalid_argument);
    BST_REQUIRE_THROW(nmos::ipmx::make_default_multicast_address_v4(U("2001:db8::1"), 1), std::exception);
}

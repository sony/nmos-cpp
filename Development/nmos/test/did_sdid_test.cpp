// The first "test" is of course whether the header compiles standalone
#include "nmos/did_sdid.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testDidSdid)
{
    BST_REQUIRE_EQUAL(U("0xAB"), nmos::make_did_or_sdid(0xAB));
    BST_REQUIRE_EQUAL(0xAB, nmos::parse_did_or_sdid(U("0xAB")));

    BST_REQUIRE_EQUAL(0xAB, nmos::parse_did_or_sdid(U("0xab")));

    BST_REQUIRE_EQUAL(U("{0xAB,0xCD}"), nmos::make_fmtp_did_sdid({ 0xAB, 0xCD }));
    BST_REQUIRE_EQUAL(nmos::did_sdid(0xAB, 0xCD), nmos::parse_fmtp_did_sdid(U("{0xAB,0xCD}")));

    // hmm, no test spec for fewer/more than two digits, or other invalid formats like "{0xAB,0x"
}

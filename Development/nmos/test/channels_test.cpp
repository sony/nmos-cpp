// The first "test" is of course whether the header compiles standalone
#include "nmos/channels.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeFmtpChannelOrder)
{
    using namespace nmos::channel_symbols;

    // two simple examples

    const std::vector<nmos::channel_symbol> stereo{ L, R };
    BST_REQUIRE_EQUAL(U("SMPTE2110.(ST)"), nmos::make_fmtp_channel_order(stereo));

    const std::vector<nmos::channel_symbol> dual_mono{ M1, M2 };
    BST_REQUIRE_EQUAL(U("SMPTE2110.(DM)"), nmos::make_fmtp_channel_order(dual_mono));

    // two examples from ST 2110-30:2017 Section 6.2.2 Channel Order Convention

    const std::vector<nmos::channel_symbol> example_1{ L, R, C, LFE, Ls, Rs, L, R };
    BST_REQUIRE_EQUAL(U("SMPTE2110.(51,ST)"), nmos::make_fmtp_channel_order(example_1));

    const std::vector<nmos::channel_symbol> example_2{ M1, M1, M1, M1, L, R, Undefined(37), Undefined(42) };
    BST_REQUIRE_EQUAL(U("SMPTE2110.(M,M,M,M,ST,U02)"), nmos::make_fmtp_channel_order(example_2));

    // same result as above, because there's only a partial match to the 5.1 Surround grouping (but a complete match for Standard Stereo)
    const std::vector<nmos::channel_symbol> example_3{ M1, M1, M1, M1, L, R, C, LFE };
    BST_REQUIRE_EQUAL(U("SMPTE2110.(M,M,M,M,ST,U02)"), nmos::make_fmtp_channel_order(example_3));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testParseFmtpChannelOrder)
{
    using namespace nmos::channel_symbols;

    // two simple examples

    const std::vector<nmos::channel_symbol> stereo{ L, R };
    BST_REQUIRE_EQUAL(stereo, nmos::parse_fmtp_channel_order(U("SMPTE2110.(ST)")));

    const std::vector<nmos::channel_symbol> dual_mono{ M1, M2 };
    BST_REQUIRE_EQUAL(dual_mono, nmos::parse_fmtp_channel_order(U("SMPTE2110.(DM)")));

    // two examples from ST 2110-30:2017 Section 6.2.2 Channel Order Convention

    const std::vector<nmos::channel_symbol> example_1{ L, R, C, LFE, Ls, Rs, L, R };
    BST_REQUIRE_EQUAL(example_1, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,ST)")));

    //const std::vector<nmos::channel_symbol> example_2{ M1, M1, M1, M1, L, R, Undefined(1), Undefined(2) };
    //BST_REQUIRE_EQUAL(example_2, nmos::parse_fmtp_channel_order(U("SMPTE2110.(M,M,M,M,ST,U02)")));

    // bad examples

    const std::vector<nmos::channel_symbol> empty;
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,ST)BAD")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,ST,)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,,ST)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,BAD)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,ST")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("BAD")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("")));
}

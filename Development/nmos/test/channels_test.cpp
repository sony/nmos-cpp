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

    const std::vector<nmos::channel_symbol> example_2{ M1, M1, M1, M1, L, R, Undefined(1), Undefined(2) };
    BST_REQUIRE_EQUAL(example_2, nmos::parse_fmtp_channel_order(U("SMPTE2110.(M,M,M,M,ST,U02)")));

    // groups that have no equivalent sequence of channel symbols

    const std::vector<nmos::channel_symbol> surround_222{
        L, R, C, LFE, Lrs, Rrs, Lc, Rc, Cs, Undefined(10), Lss, Rss,
        Undefined(13), Undefined(14), Undefined(15), Undefined(16), Undefined(17), Undefined(18),
        Undefined(19), Undefined(20), Undefined(21), Undefined(22), Undefined(23), Undefined(24)
    };
    BST_REQUIRE_EQUAL(surround_222, nmos::parse_fmtp_channel_order(U("SMPTE2110.(222)")));

    const std::vector<nmos::channel_symbol> sdi_groups{
        Undefined(1), Undefined(2), Undefined(3), Undefined(4),
        Undefined(1), Undefined(2), Undefined(3), Undefined(4)
    };
    BST_REQUIRE_EQUAL(sdi_groups, nmos::parse_fmtp_channel_order(U("SMPTE2110.(SGRP,SGRP)")));

    const auto undefined_64 = nmos::parse_fmtp_channel_order(U("SMPTE2110.(U64)"));
    BST_REQUIRE_EQUAL(64u, undefined_64.size());
    BST_REQUIRE_EQUAL(Undefined(64), undefined_64.back());

    // make_fmtp_channel_order does not produce 'SGRP'
    BST_REQUIRE_EQUAL(U("SMPTE2110.(U04)"), nmos::make_fmtp_channel_order(nmos::parse_fmtp_channel_order(U("SMPTE2110.(SGRP)"))));

    // bad examples

    const std::vector<nmos::channel_symbol> empty;
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,ST)BAD")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,ST,)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,,ST)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,BAD)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(U00)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(U65)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(U2)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(U002)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(U0A)")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,ST")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51,")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(51")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.(")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("SMPTE2110.")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("BAD")));
    BST_REQUIRE_EQUAL(empty, nmos::parse_fmtp_channel_order(U("")));
}

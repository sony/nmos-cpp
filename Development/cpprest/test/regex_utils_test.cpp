// The first "test" is of course whether the header compiles standalone
#include "cpprest/regex_utils.h"

#include "bst/test/test.h"
#include "cpprest/basic_utils.h" // for utility::us2s, utility::s2us

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeNamedSubMatch)
{
    BST_REQUIRE_STRING_EQUAL("(?<iso_date>[0-9]{4}-[0-9]{2}-[0-9]{2})", utility::us2s(utility::make_named_sub_match(U("iso_date"), U("[0-9]{4}-[0-9]{2}-[0-9]{2}"))));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testParseRegexNamedSubMatches)
{
    const auto iso_date = utility::make_named_sub_match(U("iso_date"), U("[0-9]{4}-[0-9]{2}-[0-9]{2}"));
    const auto iso_time = utility::make_named_sub_match(U("iso_time"), U("[0-9]{2}:[0-9]{2}:[0-9]{2}"));
    const auto iso_datetime = utility::make_named_sub_match(U("iso_datetime"), iso_date + U('T') + iso_time);

    const auto actual = utility::parse_regex_named_sub_matches(iso_datetime);

    BST_REQUIRE_STRING_EQUAL("(([0-9]{4}-[0-9]{2}-[0-9]{2})T([0-9]{2}:[0-9]{2}:[0-9]{2}))", utility::us2s(actual.first));
    BST_REQUIRE_EQUAL(3, actual.second.size());
    BST_REQUIRE_EQUAL(1, actual.second.at(U("iso_datetime")));
    BST_REQUIRE_EQUAL(2, actual.second.at(U("iso_date")));
    BST_REQUIRE_EQUAL(3, actual.second.at(U("iso_time")));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testParseRegexNamedSubMatchesWithoutNamedSubMatches)
{
    const auto pattern = "(([0-9]{4}-[0-9]{2}-[0-9]{2})T([0-9]{2}:[0-9]{2}:[0-9]{2}))";

    const auto actual = utility::parse_regex_named_sub_matches(utility::s2us(pattern));

    BST_REQUIRE_STRING_EQUAL(pattern, utility::us2s(actual.first));
    BST_REQUIRE(actual.second.empty());
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testParseRegexNamedSubMatchesWithTrickiness)
{
    const auto pattern = R"((\\\((?<foo>(?:foo)?)bar(?<baz>baz))";
    const auto expected = R"((\\\(((?:foo)?)bar(baz))";

    const auto actual = utility::parse_regex_named_sub_matches(utility::s2us(pattern));

    BST_REQUIRE_STRING_EQUAL(expected, utility::us2s(actual.first));
    BST_REQUIRE_EQUAL(2, actual.second.size());
    BST_REQUIRE_EQUAL(2, actual.second.at(U("foo")));
    BST_REQUIRE_EQUAL(3, actual.second.at(U("baz")));
}

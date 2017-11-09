// The first "test" is of course whether the header compiles standalone
#include "mdns/core.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testParseTxtRecordsExample)
{
    mdns::txt_records textRecords
    {
        "api_proto=http",
        "api_ver=v1.0,v1.1,v1.2",
        "pri=100"
    };

    auto structuredRecords = mdns::parse_txt_records(textRecords);

    BST_REQUIRE_EQUAL(3, structuredRecords.size());
    BST_REQUIRE_EQUAL("api_proto", structuredRecords.front().first);
    BST_REQUIRE_EQUAL("http", structuredRecords.front().second);
    BST_REQUIRE_EQUAL("pri", structuredRecords.back().first);
    BST_REQUIRE_EQUAL("100", structuredRecords.back().second);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testParseTxtRecordsEdgeCases)
{
    mdns::txt_records textRecords
    {
        "key=value",
        "empty-attribute=",
        "=illegal-attribute",
        "boolean-attribute"
    };

    auto structuredRecords = mdns::parse_txt_records(textRecords);

    // DNS-SD "boolean attributes" are not currently supported

    BST_REQUIRE_EQUAL(2, structuredRecords.size());
    BST_REQUIRE_EQUAL("key", structuredRecords.front().first);
    BST_REQUIRE_EQUAL("value", structuredRecords.front().second);
    BST_REQUIRE_EQUAL("empty-attribute", structuredRecords.back().first);
    BST_REQUIRE_EQUAL("", structuredRecords.back().second);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeTxtRecords)
{
    mdns::structured_txt_records structuredRecords
    {
        { "key", "value" },
        { "empty-attribute", "" },
        { "", "illegal-attribute" }
    };

    auto textRecords = mdns::make_txt_records(structuredRecords);

    // DNS-SD "boolean attributes" are not currently supported

    BST_REQUIRE_EQUAL(2, textRecords.size());
    BST_REQUIRE_EQUAL("key=value", textRecords.front());
    BST_REQUIRE_EQUAL("empty-attribute=", textRecords.back());
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testParseTxtRecord)
{
    mdns::structured_txt_records structuredRecords
    {
        { "foo", "fizz" },
        { "bar", "42" },
        { "baz", "buzz" }
    };

    auto parse = [](const std::string& v){ int p; std::istringstream is(v); is >> p; return is.fail() ? -1 : p; };

    // find and parse successfully
    BST_REQUIRE_EQUAL(42, mdns::parse_txt_record<int>(structuredRecords, "bar", parse));
    // find but fail to parse
    BST_REQUIRE_EQUAL(-1, mdns::parse_txt_record<int>(structuredRecords, "baz", parse));
    // fail to find, return default-constructed value
    BST_REQUIRE_EQUAL(0, mdns::parse_txt_record<int>(structuredRecords, "qux", parse));
    // fail to find, return specified value
    BST_REQUIRE_EQUAL(57, mdns::parse_txt_record(structuredRecords, "qux", parse, 57));
}

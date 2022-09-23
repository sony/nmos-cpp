// The first "test" is of course whether the header compiles standalone
#include "rql/rql.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testRqlParseQuery)
{
    {
        const utility::string_t query_rql = U("eq(foo.bar.baz.qux,meow)");
        const auto rql_query = rql::parse_query(query_rql);
        const auto& key_path = rql_query.at(U("args")).at(0).as_string();
        BST_REQUIRE_STRING_EQUAL(U("foo.bar.baz.qux"), key_path);
    }

    {
        const utility::string_t query_rql = U("eq((foo,bar,baz.qux),purr)");
        const auto rql_query = rql::parse_query(query_rql);
        const auto& key_path = rql_query.at(U("args")).at(0).as_array();
        BST_REQUIRE_STRING_EQUAL(U("baz.qux"), key_path.rbegin()->as_string());
    }

    {
        const utility::string_t query_rql = U("eq((foo,bar,baz%2Equx),purr)");
        const auto rql_query = rql::parse_query(query_rql);
        const auto& key_path = rql_query.at(U("args")).at(0).as_array();
        BST_REQUIRE_STRING_EQUAL(U("baz.qux"), key_path.rbegin()->as_string());
    }

    {
        const utility::string_t query_rql = U("eq((foo,bar,baz%252Equx),hiss)");
        const auto rql_query = rql::parse_query(query_rql);
        const auto& key_path = rql_query.at(U("args")).at(0).as_array();
        BST_REQUIRE_STRING_EQUAL(U("baz%2Equx"), key_path.rbegin()->as_string());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testRqlValidateQuery)
{
    const rql::operators operators{
        { U("foo"), {} },
        { U("bar"), {} },
        { U("baz"), {} }
    };

    // no call-operator
    {
        const utility::string_t query_rql = U("meow");
        const auto rql_query = rql::parse_query(query_rql);
        BST_REQUIRE_NO_THROW(rql::validate_query(rql_query, operators));
    }

    // only valid call-operators
    {
        const utility::string_t query_rql = U("foo(meow,bar(purr,baz(),hiss,(qux,yowl)))");
        const auto rql_query = rql::parse_query(query_rql);
        BST_REQUIRE_NO_THROW(rql::validate_query(rql_query, operators));
    }

    // invalid call-operator
    {
        const utility::string_t query_rql = U("meow()");
        const auto rql_query = rql::parse_query(query_rql);
        BST_REQUIRE_THROW(rql::validate_query(rql_query, operators), std::runtime_error);
    }

    // invalid call-operator within an array arg nested in valid call-operators
    {
        const utility::string_t query_rql = U("foo(meow,bar(purr,baz(),hiss,(qux(),yowl)))");
        const auto rql_query = rql::parse_query(query_rql);
        BST_REQUIRE_THROW(rql::validate_query(rql_query, operators), std::runtime_error);
    }
}

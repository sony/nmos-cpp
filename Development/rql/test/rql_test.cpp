// The first "test" is of course whether the header compiles standalone
#include "rql/rql.h"

#include "bst/test/test.h"
#include "cpprest/json_utils.h"
#include "nmos/query_utils.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testRqlParse)
{
    const auto test_value = web::json::value_of({
        { U("foo"), web::json::value_of({
            { U("bar"), web::json::value_of({
                { U("baz.qux"), U("quux") },
                { U("baz%2Equx"), U("quuux") },
                { U("foo"), U("quuuux") }
            }) }
        }) }
    });

    {
        const utility::string_t query_rql = U("matches(foo.bar.baz%2Equx,quux)");
        const auto rql_query = rql::parse_query(query_rql, false);

        const auto key_path = nmos::split_decode_key_path(rql_query.at(U("args")).at(0).as_string());

        web::json::value results;
        BST_REQUIRE_EQUAL(true, web::json::extract(test_value.as_object(), results, key_path));
        BST_REQUIRE_EQUAL(U("quux"), results.as_string());
    }

    {
        const utility::string_t query_rql = U("matches(foo.bar.baz%252Equx,quux)");
        const auto rql_query = rql::parse_query(query_rql, false);

        const auto key_path = nmos::split_decode_key_path(rql_query.at(U("args")).at(0).as_string());

        web::json::value results;
        BST_REQUIRE_EQUAL(true, web::json::extract(test_value.as_object(), results, key_path));
        BST_REQUIRE_EQUAL(U("quuux"), results.as_string());
    }

    {
        const utility::string_t query_rql = U("matches(foo.bar.foo,quuuux)");
        const auto rql_query = rql::parse_query(query_rql, false);

        const auto key_path = nmos::split_decode_key_path(rql_query.at(U("args")).at(0).as_string());

        web::json::value results;
        BST_REQUIRE_EQUAL(true, web::json::extract(test_value.as_object(), results, key_path));
        BST_REQUIRE_EQUAL(U("quuuux"), results.as_string());
    }
}

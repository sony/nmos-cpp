// The first "test" is of course whether the header compiles standalone
#include "rql/rql.h"

#include "bst/test/test.h"
#include "cpprest/json_utils.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testRqlParse)
{
    const auto test_value = web::json::value_of({
        { U("foo"), web::json::value_of({
            { U("bar"), web::json::value_of({
                { U("baz.qux"), U("quux")}
            }) }
        }) }
    });

    const utility::string_t query_rql = U("matches(foo.bar.baz%2Equx,quux)");
    const auto rql_query = rql::parse_query(query_rql);

    const auto args = rql_query.at(U("args"));
    const auto key_path = args.as_array().at(0).as_string();
    web::json::value results;

    BST_REQUIRE_EQUAL(true, web::json::extract(test_value.as_object(), results, key_path));
    BST_REQUIRE_EQUAL(U("quux"), results.as_string());
}

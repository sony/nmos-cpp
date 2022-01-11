// The first "test" is of course whether the header compiles standalone
#include "rql/rql.h"

#include <boost/range/adaptor/transformed.hpp>
#include "bst/test/test.h"
#include "cpprest/json_utils.h"

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

    auto get_key_path = [](const web::json::array& key)
    {
        return boost::copy_range<std::vector<utility::string_t>>(key | boost::adaptors::transformed([](const web::json::value& key_path)
        {
            return key_path.as_string();
        }));
    };

    {
        const utility::string_t query_rql = U("eq((foo,bar,baz.qux),quux)");
        const auto rql_query = rql::parse_query(query_rql);
        const auto key_path = get_key_path(rql_query.at(U("args")).at(0).as_array());

        web::json::value results;
        BST_REQUIRE_EQUAL(true, web::json::extract(test_value.as_object(), results, key_path));
        BST_REQUIRE_EQUAL(U("quux"), results.as_string());
    }

    {
        const utility::string_t query_rql = U("eq((foo,bar,baz%252Equx),quuux)");
        const auto rql_query = rql::parse_query(query_rql);
        const auto key_path = get_key_path(rql_query.at(U("args")).at(0).as_array());

        web::json::value results;
        BST_REQUIRE_EQUAL(true, web::json::extract(test_value.as_object(), results, key_path));
        BST_REQUIRE_EQUAL(U("quuux"), results.as_string());
    }

    {
        const utility::string_t query_rql = U("eq((foo,bar,foo),quuuux)");
        const auto rql_query = rql::parse_query(query_rql);
        const auto key_path = get_key_path(rql_query.at(U("args")).at(0).as_array());

        web::json::value results;
        BST_REQUIRE_EQUAL(true, web::json::extract(test_value.as_object(), results, key_path));
        BST_REQUIRE_EQUAL(U("quuuux"), results.as_string());
    }
}

// The first "test" is of course whether the header compiles standalone
#include "ssl/ssl_utils.h"

#include "bst/test/test.h"
#include "sdp/json.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSslParseErrors)
{
    ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);

    struct test
    {
        std::string field;
        std::string value;
    };

    const test tests[] =
    {
        { "C", "UK"},
        { "O", "Disorganized Organization"},
        { "CN", "Joe Bloggs"}
    };

    // set up X509_NAME for the test
    for (const auto& test : tests)
    {
        X509_NAME_add_entry_by_txt(x509_name.get(), test.field.c_str(), MBSTRING_ASC, (const unsigned char*)test.value.c_str(), -1, -1, 0);
    }

    // do test
    for (const auto& test : tests)
    {
        BST_REQUIRE_EQUAL(test.value, ssl::experimental::details::get_attribute_value(x509_name.get(), test.field));
    }
    BST_REQUIRE_EQUAL("", ssl::experimental::details::get_attribute_value(x509_name.get(), " CN"));
    BST_REQUIRE_EQUAL("", ssl::experimental::details::get_attribute_value(x509_name.get(), "CN "));
    BST_REQUIRE_EQUAL("", ssl::experimental::details::get_attribute_value(x509_name.get(), "cn"));
}

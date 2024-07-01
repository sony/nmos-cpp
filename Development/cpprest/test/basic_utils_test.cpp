// The first "test" is of course whether the header compiles standalone
#include "cpprest/basic_utils.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testBase64Url)
{
    // See https://tools.ietf.org/html/rfc4648#section-10
    const std::pair<std::string, std::string> tests[] = {
        { "", "" },
        { "f", "Zg" },
        { "fo", "Zm8" },
        { "foo", "Zm9v" },
        { "foob", "Zm9vYg" },
        { "fooba", "Zm9vYmE" },
        { "foobar", "Zm9vYmFy" },
        { "???~~~", "Pz8_fn5-" }
    };

    for (const auto& test : tests)
    {
        const std::vector<unsigned char> data(test.first.begin(), test.first.end());
        const utility::string_t str(test.second.begin(), test.second.end());
        BST_REQUIRE_STRING_EQUAL(str, utility::conversions::to_base64url(data));
        BST_REQUIRE_EQUAL(data, utility::conversions::from_base64url(str));
    }
}

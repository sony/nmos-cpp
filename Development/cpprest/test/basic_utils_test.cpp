// The first "test" is of course whether the header compiles standalone
#include "cpprest/basic_utils.h"

#include "bst/test/test.h"
#ifdef _WIN32
#include <windows.h> // for GetACP
#endif

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

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testS2usLenientValidUtf8)
{
    // valid UTF-8 must take the fast path, i.e. give exactly what s2us gives, and round-trip
    const std::string tests[] = {
        "",
        "hello",
        "\xC3\xBC",             // U+00FC latin small letter u with diaeresis
        "M\xC3\xBCller",
        "\xE2\x82\xAC",         // U+20AC euro sign
        "\xF0\x9F\x8E\xA5"      // U+1F3A5 movie camera
    };

    for (const auto& test : tests)
    {
        BST_REQUIRE_EQUAL(utility::s2us(test), utility::s2us_lenient(test));
        BST_REQUIRE_EQUAL(test, utility::us2s(utility::s2us_lenient(test)));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testS2usLenientInvalidUtf8)
{
    // these are the byte sequences that make s2us throw std::range_error on a wide build,
    // e.g. on Windows, where they would escape the async logging worker thread
    const std::string tests[] = {
        "\xFC",                 // u with diaeresis in the Windows Western code page, or Latin-1
        "M\xFCller",
        "\x80",                 // continuation byte with no leading byte
        "\xC3",                 // truncated two byte sequence
        "\xFF\xFE"
    };

    for (const auto& test : tests)
    {
        // to_utf16string is the strict decoder that s2us uses on a wide build, on every
        // platform, so this asserts the Windows failure mode without needing a wide build
        BST_REQUIRE_THROW(utility::conversions::to_utf16string(test), std::range_error);

        utility::string_t converted;
        BST_REQUIRE_NO_THROW(converted = utility::s2us_lenient(test));

        // the message must be preserved, not dropped
        BST_REQUIRE(!converted.empty());

        // and whatever it decoded to must be well-formed, so that it can go into a JSON
        // log event and be converted back without throwing
        BST_REQUIRE_NO_THROW(utility::conversions::to_utf16string(utility::us2s(converted)));
    }

    // empty input is not an error
    BST_REQUIRE(utility::s2us_lenient("").empty());

    // note that the decoder is not a strict UTF-8 validator - it accepts UTF-16 surrogates,
    // code points above U+10FFFF and overlong encodings - so these do not take the fallback
    // that is deliberate: s2us_lenient differs from s2us only where s2us would have thrown,
    // and passes everything else through unchanged
    const std::string tolerated[] = {
        "\xED\xA0\x80",         // a UTF-16 surrogate
        "\xF5\x80\x80\x80",     // above U+10FFFF
        "\xC0\xAF"              // overlong encoding of '/'
    };

    for (const auto& test : tolerated)
    {
        BST_REQUIRE_NO_THROW(utility::conversions::to_utf16string(test));
        BST_REQUIRE_EQUAL(utility::s2us(test), utility::s2us_lenient(test));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testS2usLenientAnsiCodePage)
{
#ifdef _WIN32
    // the ANSI code page is a machine setting, so this is only asserted for the Windows Western
    // code page (1252), in which 0xFC is u with diaeresis
    if (1252 != ::GetACP()) return;

    BST_REQUIRE_EQUAL(utility::s2us("\xC3\xBC"), utility::s2us_lenient("\xFC"));
    BST_REQUIRE_EQUAL(utility::s2us("M\xC3\xBCller"), utility::s2us_lenient("M\xFCller"));
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testS2usLenientReplacement)
{
#ifdef _WIN32
    // the ANSI code page is used instead unless it is UTF-8
    if (CP_UTF8 != ::GetACP()) return;
#endif

    // each invalid byte is replaced with U+FFFD, and the valid text around it is unchanged
    const std::pair<std::string, std::string> tests[] = {
        { "\xFC", "\xEF\xBF\xBD" },
        { "M\xFCller", "M\xEF\xBF\xBDller" },
        // valid UTF-8 before the invalid byte
        { "R\xC3\xA9sum\xC3\xA9 \xFC", "R\xC3\xA9sum\xC3\xA9 \xEF\xBF\xBD" },
        // 0xE9 starts a three byte sequence, which the space cuts short
        { "caf\xE9 au lait", "caf\xEF\xBF\xBD au lait" },
        // a truncated sequence is replaced byte by byte
        { "\xE2\x82" "A", "\xEF\xBF\xBD\xEF\xBF\xBD" "A" }
    };

    for (const auto& test : tests)
    {
        BST_REQUIRE_EQUAL(utility::s2us(test.second), utility::s2us_lenient(test.first));
    }
}

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
BST_TEST_CASE(testS2usLenientLatin1Fallback)
{
#ifndef _WIN32
    // where there is no ANSI code page, the fallback is Latin-1, which maps each byte to the
    // code point of the same value; the Windows Western code page (1252) agrees over 0xA0-0xFF,
    // so the accented letters come out the same on either
    // this is not asserted on Windows, where the ANSI code page is a machine setting and a
    // double byte code page, e.g. 932, treats 0xFC as a leading byte instead
    BST_REQUIRE_EQUAL(utility::s2us("\xC3\xBC"), utility::s2us_lenient("\xFC"));
    BST_REQUIRE_EQUAL(utility::s2us("M\xC3\xBCller"), utility::s2us_lenient("M\xFCller"));
#endif
}

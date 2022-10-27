// The first "test" is of course whether the header compiles standalone
#include "ssl/ssl_utils.h"

#include "bst/test/test.h"
#include "sdp/json.h"

////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    struct x509_name_add_entry_by_txt_param
    {
        std::string field;
        std::string value;
        int loc;
        int set;
        int type;
        x509_name_add_entry_by_txt_param(std::string field, std::string value, int loc = -1, int set = 0, int type = MBSTRING_ASC)
            : field(std::move(field))
            , value(std::move(value))
            , loc(loc)
            , set(set)
            , type(type)
        {}
    };

    template <typename Entries>
    ssl::experimental::X509_NAME_ptr make_x509_name(const Entries& entries)
    {
        ssl::experimental::X509_NAME_ptr result(X509_NAME_new(), &X509_NAME_free);
        for (const auto& entry : entries)
        {
            int err = X509_NAME_add_entry_by_txt(result.get(), entry.field.c_str(), entry.type, (const unsigned char*)entry.value.data(), (int)entry.value.size(), entry.loc, entry.set);
            BST_REQUIRE_EQUAL(1, err);
        }
        return result;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSslGetAttributeValue)
{
    // test against RFC 2253 examples
    // see https://www.rfc-editor.org/rfc/rfc2253#section-5

    {
        // "three relative distinguished names(RDNs): CN=Steve Kille,O=Isode Limited,C=GB"
        const x509_name_add_entry_by_txt_param entries[] =
        {
            { "CN", "Steve Kille" },
            { "O", "Isode Limited" },
            { "C", "GB" }
        };
        const auto x509_name = make_x509_name(entries);
        for (const auto& entry : entries)
        {
            BST_REQUIRE_EQUAL(entry.value, ssl::experimental::details::get_attribute_value(x509_name.get(), entry.field));
        }
    }

    {
        // "three RDNs, in which the first RDN is multi-valued: OU=Sales+CN=J.Smith,O=Widget Inc.,C=US"
        const x509_name_add_entry_by_txt_param entries[] =
        {
            { "OU", "Sales" },
            { "CN", "J.Smith", 0, -1 },
            { "O", "Widget Inc." },
            { "C", "US" }
        };
        const auto x509_name = make_x509_name(entries);
        for (const auto& entry : entries)
        {
            BST_REQUIRE_EQUAL(entry.value, ssl::experimental::details::get_attribute_value(x509_name.get(), entry.field));
        }
    }

    {
        // "quoting of a comma in an organization name: CN=L. Eagle,O=Sue\, Grabbit and Runn,C=GB"
        const x509_name_add_entry_by_txt_param entries[] =
        {
            { "CN", "L. Eagle" },
            { "O", "Sue, Grabbit and Runn" },
            { "C", "GB" }
        };
        const auto x509_name = make_x509_name(entries);
        for (const auto& entry : entries)
        {
            BST_REQUIRE_EQUAL(entry.value, ssl::experimental::details::get_attribute_value(x509_name.get(), entry.field));
        }
    }

    {
        // "a value contains a carriage return character: CN=Before\0DAfter,O=Test,C=GB"
        const x509_name_add_entry_by_txt_param entries[] =
        {
            { "CN", "Before\rAfter" },
            { "O", "Test" },
            { "C", "GB" }
        };
        const auto x509_name = make_x509_name(entries);
        for (const auto& entry : entries)
        {
            BST_REQUIRE_EQUAL(entry.value, ssl::experimental::details::get_attribute_value(x509_name.get(), entry.field));
        }
    }

// hmm, not quite sure how to test OCTET STRING, as X509_NAME_add_entry_by_txt doesn't seem to support V_ASN1_OCTET_STRING
// and if it did, the result would currently still be the BER encoded value, not the original bytes
#if 0
    {
        // "an RDN of an unrecognized type [and] the value is the BER encoding of an OCTET STRING
        // containing two bytes 0x48 and 0x69: 1.3.6.1.4.1.1466.0=#04024869,O=Test,C=GB"
        const x509_name_add_entry_by_txt_param entries[] =
        {
            { "1.3.6.1.4.1.1466.0", "\x48\x69", -1, 0, V_ASN1_OCTET_STRING }, // "Hi"
            { "O", "Test" },
            { "C", "GB" }
        };
        const auto x509_name = make_x509_name(entries);
        for (const auto& entry : entries)
        {
            BST_REQUIRE_EQUAL(entry.value, ssl::experimental::details::get_attribute_value(x509_name.get(), entry.field));
        }
    }
#endif

    {
        // "an RDN surname value consisting of 5 letters: SN=Lu\C4\8Di\C4\87"
        const x509_name_add_entry_by_txt_param entries[] =
        {
            { "SN", "Lu\xC4\x8Di\xC4\x87", -1, 0, MBSTRING_UTF8 }, // Lučić
            { "O", "Test" },
            { "C", "GB" }
        };
        const auto x509_name = make_x509_name(entries);
        for (const auto& entry : entries)
        {
            BST_REQUIRE_EQUAL(entry.value, ssl::experimental::details::get_attribute_value(x509_name.get(), entry.field));
        }
    }

    {
        // no CN
        const x509_name_add_entry_by_txt_param entries[] =
        {
            { "O", "Test" },
            { "C", "GB" }
        };
        const auto x509_name = make_x509_name(entries);
        BST_REQUIRE_EQUAL("", ssl::experimental::details::get_attribute_value(x509_name.get(), "CN"));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSslParseDistinguishedName)
{
    // test against RFC 2253 examples
    // see https://www.rfc-editor.org/rfc/rfc2253#section-5

    {
        std::vector<std::pair<std::string, ssl::experimental::details::distinguished_name>> tests
        {
            { "CN=Steve Kille,O=Isode Limited,C=GB", { { { "CN", "Steve Kille" } }, { { "O", "Isode Limited" } }, { { "C", "GB" } } } },
            { "OU=Sales+CN=J.Smith,O=Widget Inc.,C=US", { { { "OU", "Sales" }, { "CN", "J.Smith" } }, { { "O", "Widget Inc." } }, { { "C", "US" } } } },
            { "CN=L. Eagle,O=Sue\\, Grabbit and Runn,C=GB", { { { "CN", "L. Eagle" } }, { { "O", "Sue, Grabbit and Runn" } }, { { "C", "GB" } } } },
            { "CN=Before\\0DAfter,O=Test,C=GB", { { { "CN", "Before\rAfter" } }, { { "O", "Test" } }, { { "C", "GB" } } } },
            { "1.3.6.1.4.1.1466.0=#04024869,O=Test,C=GB", { { { "1.3.6.1.4.1.1466.0", "\x04\x02\x48\x69" } }, { { "O", "Test" } }, { { "C", "GB" } } } },
            { "SN=Lu\\C4\\8Di\\C4\\87", { { { "SN", "Lu\xC4\x8Di\xC4\x87" } } } }
        };

        for (const auto& test : tests)
        {
            const auto& expected = test.second;
            const auto actual = ssl::experimental::details::parse_distinguished_name(test.first);
            BST_REQUIRE_EQUAL(expected, actual);
        }
    }

    {
        std::vector<std::pair<std::string, ssl::experimental::details::distinguished_name>> tests
        {
            { "", {} },
            { "FOO=", { { { "FOO", "" } } } },
            { "FOO=+BAR=\"meow+\\\"purr\\\"\",BAZ=\\,+QUX=\\#hiss\\=", { { {"FOO", ""}, {"BAR", "meow+\"purr\""} }, { { "BAZ", "," }, { "QUX", "#hiss=" } } } }
        };

        for (const auto& test : tests)
        {
            const auto& expected = test.second;
            const auto actual = ssl::experimental::details::parse_distinguished_name(test.first);
            BST_REQUIRE_EQUAL(expected, actual);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSslParseDistinguishedNameSyntaxErrors)
{
    std::vector<std::string> bad_tests
    {
        "=",
        "FOO=+",
        "FOO=#",
        "FOO=#0",
        "FOO=#0!",
        "FOO=\"",
        "FOO=\"\\\"",
        "FOO=\\",
        "0.=",
        "0..=",
        "."
    };

    for (const auto& bad : bad_tests)
    {
        BST_REQUIRE_THROW(ssl::experimental::details::parse_distinguished_name(bad), std::invalid_argument);
    }
}

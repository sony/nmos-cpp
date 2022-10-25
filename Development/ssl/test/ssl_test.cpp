// The first "test" is of course whether the header compiles standalone
#include "ssl/ssl_utils.h"

#include "bst/test/test.h"
#include "sdp/json.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSslParseErrors)
{
    struct x509_name_add_entry_by_txt_param
    {
        std::string field;
        std::string value;
        int loc;
        int set;
    };

    // test against RFC 2253 examples
    // see https://www.rfc-editor.org/rfc/rfc2253#section-5
    {
        // three relative distinguished names(RDNs): CN=Steve Kille,O=Isode Limited,C=GB
        const x509_name_add_entry_by_txt_param x509_name_add_entry_by_txt_params[] =
        {
            { "CN", "Steve Kille", -1, 0 },
            { "O", "Isode Limited", -1, 0 },
            { "C", "GB", -1, 0 }
        };
        // set up X509 name to `CN=Steve Kille,O=Isode Limited,C=GB`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        for (const auto& param : x509_name_add_entry_by_txt_params)
        {
            X509_NAME_add_entry_by_txt(x509_name.get(), param.field.c_str(), MBSTRING_ASC, (const unsigned char*)param.value.c_str(), -1, param.loc, param.set);
        }
        // run tests
        for (const auto& param : x509_name_add_entry_by_txt_params)
        {
            BST_REQUIRE_EQUAL(param.value, ssl::experimental::details::get_attribute_value(x509_name.get(), param.field));
        }
    }

    {
        // three RDNs, in which the first RDN is multi-valued: OU=Sales+CN=J.Smith,O=Widget Inc.,C=US
        const x509_name_add_entry_by_txt_param x509_name_add_entry_by_txt_params[] =
        {
            { "OU", "Sales", -1, 0 },
            { "CN", "J.Smith", 0, 1 },
            { "O", "Widget Inc.", -1, 0 },
            { "C", "US", -1, 0 }
        };
        // set up X509 name to `OU=Sales+CN=J.Smith,O=Widget Inc.,C=US`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        for (const auto& param : x509_name_add_entry_by_txt_params)
        {
            X509_NAME_add_entry_by_txt(x509_name.get(), param.field.c_str(), MBSTRING_ASC, (const unsigned char*)param.value.c_str(), -1, param.loc, param.set);
        }
        // run tests
        for (const auto& param : x509_name_add_entry_by_txt_params)
        {
            BST_REQUIRE_EQUAL(param.value, ssl::experimental::details::get_attribute_value(x509_name.get(), param.field));
        }
    }

    {
        // three RDNs, in which the second RDN has comma: CN=L. Eagle,O=Sue\, Grabbit and Runn,C=GB
        const x509_name_add_entry_by_txt_param x509_name_add_entry_by_txt_params[] =
        {
            { "CN", "L. Eagle", -1, 0 },
            { "O", "Sue, Grabbit and Runn", -1, 0 },
            { "C", "GB", -1, 0 }
        };
        // set up X509 name to `CN=L. Eagle,O=Sue\, Grabbit and Runn,C=GB`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        for (const auto& param : x509_name_add_entry_by_txt_params)
        {
            X509_NAME_add_entry_by_txt(x509_name.get(), param.field.c_str(), MBSTRING_ASC, (const unsigned char*)param.value.c_str(), -1, param.loc, param.set);
        }
        // run tests
        for (const auto& param : x509_name_add_entry_by_txt_params)
        {
            BST_REQUIRE_EQUAL(param.value, ssl::experimental::details::get_attribute_value(x509_name.get(), param.field));
        }
    }

    {
        // two RDNs, missing CN RDN: OU=Sales,O=Widget Inc.,C=US
        const x509_name_add_entry_by_txt_param x509_name_add_entry_by_txt_params[] =
        {
            { "OU", "Sales", -1, 0 },
            { "O", "Widget Inc.", -1, 0 },
            { "C", "US", -1, 0 }
        };

        // set up X509 name to `OU=Sales,O=Widget Inc.,C=US`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        for (const auto& param : x509_name_add_entry_by_txt_params)
        {
            X509_NAME_add_entry_by_txt(x509_name.get(), param.field.c_str(), MBSTRING_ASC, (const unsigned char*)param.value.c_str(), -1, param.loc, param.set);
        }
        // do tests
        BST_REQUIRE_EQUAL("", ssl::experimental::details::get_attribute_value(x509_name.get(), "CN"));
    }
}

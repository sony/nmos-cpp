// The first "test" is of course whether the header compiles standalone
#include "ssl/ssl_utils.h"

#include  <iterator>
#include "bst/test/test.h"
#include "sdp/json.h"

namespace
{
    // create oid dot string from oid vector
    std::string make_oid_dot_string(const std::vector<int>& oid)
    {
        if (oid.empty()) { throw std::runtime_error("empty oid string"); }

        std::stringstream ss;
        std::copy(oid.begin(), oid.end() - 1, std::ostream_iterator<int>(ss, "."));
        ss << oid.back();
        return ss.str();
    }

    // convert to hex UTF8String
    // see https://en.wikipedia.org/wiki/X.690
    // see https://docs.oracle.com/cd/E19476-01/821-0510/def-basic-encoding-rules.html#:~:text=The%20Basic%20Encoding%20Rules%20(BER,encoded%20in%20a%20binary%20form.
    std::string make_hex_string(const std::string& raw)
    {
        std::stringstream ss;

        // hexstring starts with '#'
        ss << "#";

        // BER type (UTF8String)
        ss << "0C";

        // BER length
        ss << std::setfill('0') << std::setw(2) << std::hex;
        auto len = raw.length();

        auto bytes_count = [](size_t value)
        {
            int count{ 0 };
            while(value)
            {
                value >>= 8;
                count++;
            }
            return count;
        };

        // is long length
        if (len > 127)
        {
            const auto bytes = bytes_count(len);
            ss << (0x80 | bytes);

            int idx = bytes;
            while(idx)
            {
                int value = (0xFF) & (len >> (8 * --idx));
                ss << std::setw(2) << (int)value;
            }
        }
        else
        {
            ss << len;
        }

        // BER value
        for (const auto c : raw)
        {
            ss << std::setw(2) << (int)c;
        }
        return ss.str();
    }

    struct x509_name_add_entry_param
    {
        std::string sn; // short name
        std::vector<int> oid; // object identifier
        std::string value;
        int loc;
        int set;
    };

    // set up X509 NAME with the given set of x509_name_add_entry_param
    void set_up_x509_name(X509_NAME* x509_name, const x509_name_add_entry_param* params, size_t params_size)
    {
        for (size_t idx = 0; idx < params_size; idx++)
        {
            const auto& param = params[idx];

            if (!param.sn.empty())
            {
                X509_NAME_add_entry_by_txt(x509_name, param.sn.c_str(), MBSTRING_UTF8, (const unsigned char*)param.value.c_str(), -1, param.loc, param.set);
            }

            if (!param.oid.empty())
            {
                ssl::experimental::ASN1_OBJECT_ptr ans1_object(OBJ_txt2obj(make_oid_dot_string(param.oid).c_str(), 1), &ASN1_OBJECT_free);
                X509_NAME_add_entry_by_OBJ(x509_name, ans1_object.get(), MBSTRING_UTF8, (unsigned char*)param.value.c_str(), -1, param.loc, param.set);
            }
        }
    }

    // run test on each x509_name_add_entry_param
    void run_tests(X509_NAME* x509_name, const x509_name_add_entry_param* params, size_t params_size)
    {
        for (size_t idx = 0; idx < params_size; idx++)
        {
            const auto& param = params[idx];

            if (!param.sn.empty())
            {
                BST_REQUIRE_EQUAL(param.value, ssl::experimental::details::get_attribute_value(x509_name, param.sn));
            }

            if (!param.oid.empty())
            {
                BST_REQUIRE_EQUAL(make_hex_string(param.value), ssl::experimental::details::get_attribute_value(x509_name, make_oid_dot_string(param.oid)));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSslParseErrors)
{
    // test against RFC 2253 examples
    // see https://www.rfc-editor.org/rfc/rfc2253#section-5
    {
        // three relative distinguished names(RDNs): CN=Steve Kille,O=Isode Limited,C=GB
        const x509_name_add_entry_param x509_name_add_entry_params[] =
        {
            { "CN", {}, "Steve Kille", -1, 0 },
            { "O", {}, "Isode Limited", -1, 0 },
            { "C", {}, "GB", -1, 0 }
        };

        // set up X509 name to `CN=Steve Kille,O=Isode Limited,C=GB`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        const auto params_size = sizeof(x509_name_add_entry_params) / sizeof(x509_name_add_entry_param);
        set_up_x509_name(x509_name.get(), x509_name_add_entry_params, params_size);

        // do tests
        run_tests(x509_name.get(), x509_name_add_entry_params, params_size);
    }

    {
        // three RDNs, in which the first RDN is multi-valued: OU=Sales+CN=J.Smith,O=Widget Inc.,C=US
        const x509_name_add_entry_param x509_name_add_entry_params[] =
        {
            { "OU", {}, "Sales", -1, 0 },
            { "CN", {}, "J.Smith", 0, 1 },
            { "O", {}, "Widget Inc.", -1, 0 },
            { "C", {}, "US", -1, 0 }
        };

        // set up X509 name to `OU=Sales+CN=J.Smith,O=Widget Inc.,C=US`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        const auto params_size = sizeof(x509_name_add_entry_params) / sizeof(x509_name_add_entry_param);
        set_up_x509_name(x509_name.get(), x509_name_add_entry_params, params_size);

        // do tests
        run_tests(x509_name.get(), x509_name_add_entry_params, params_size);
    }

    {
        // three RDNs, in which the second RDN has comma: CN=L. Eagle,O=Sue\, Grabbit and Runn,C=GB
        const x509_name_add_entry_param x509_name_add_entry_params[] =
        {
            { "CN", {}, "L. Eagle", -1, 0 },
            { "O", {}, "Sue, Grabbit and Runn", -1, 0 },
            { "C", {}, "GB", -1, 0 }
        };

        // set up X509 name to `CN=L. Eagle,O=Sue\, Grabbit and Runn,C=GB`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        const auto params_size = sizeof(x509_name_add_entry_params) / sizeof(x509_name_add_entry_param);
        set_up_x509_name(x509_name.get(), x509_name_add_entry_params, params_size);

        // do tests
        run_tests(x509_name.get(), x509_name_add_entry_params, params_size);
    }

    {
        // An example name in which a value contains a carriage return character

        // three relative distinguished names(RDNs): CN=Before\0DAfter,O=Test,C=GB
        const x509_name_add_entry_param x509_name_add_entry_params[] =
        {
            { "CN", {}, "Before\rAfter", -1, 0 },
            { "O", {}, "Test", -1, 0 },
            { "C", {}, "GB", -1, 0 }
        };

        // set up X509 name to `CN=Before\0DAfter,O=Test,C=GB`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        const auto params_size = sizeof(x509_name_add_entry_params) / sizeof(x509_name_add_entry_param);
        set_up_x509_name(x509_name.get(), x509_name_add_entry_params, params_size);

        // do tests
        run_tests(x509_name.get(), x509_name_add_entry_params, params_size);
    }

    {
        // An example name in which an RDN was of an unrecognized type.  The
        // value is the BER encoding of an OCTET STRING containing two bytes
        // 0x48 and 0x69.

        // three RDNs, 1.3.6.1.4.1.1466.0=#0C024869,O=Test,C=GB
        const x509_name_add_entry_param x509_name_add_entry_params[] =
        {
            { {}, {1,3,6,1,4,1,1466,0}, "Hi", -1, 0 },
            { "O", {}, "Test", -1, 0 },
            { "C", {}, "GB", -1, 0 }
        };

        // set up X509 name to 1.3.6.1.4.1.1466.0=#0C024869,O=Test,C=GB
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        const auto params_size = sizeof(x509_name_add_entry_params) / sizeof(x509_name_add_entry_param);
        set_up_x509_name(x509_name.get(), x509_name_add_entry_params, params_size);

        // do tests
        run_tests(x509_name.get(), x509_name_add_entry_params, params_size);
    }

    {
        // one RDN, SN=Lu\C4\8Di\C4\87
        const x509_name_add_entry_param x509_name_add_entry_params[] =
        {
            { "SN", {}, "Lu\xC4\x8Di\xC4\x87", -1, 0 }
        };

        // set up X509 name to `SN=Lu\C4\8Di\C4\87`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        const auto params_size = sizeof(x509_name_add_entry_params) / sizeof(x509_name_add_entry_param);
        set_up_x509_name(x509_name.get(), x509_name_add_entry_params, params_size);

        // do test
        run_tests(x509_name.get(), x509_name_add_entry_params, params_size);
    }

    {
        // two RDNs, missing CN RDN: O=Isode Limited,C=GB
        const x509_name_add_entry_param x509_name_add_entry_params[] =
        {
            { "O", {}, "Isode Limited", -1, 0 },
            { "C", {}, "GB", -1, 0 }
        };

        // set up X509 name to `O=Isode Limited,C=GB`
        ssl::experimental::X509_NAME_ptr x509_name(X509_NAME_new(), &X509_NAME_free);
        const auto params_size = sizeof(x509_name_add_entry_params) / sizeof(x509_name_add_entry_param);
        set_up_x509_name(x509_name.get(), x509_name_add_entry_params, params_size);

        // do test
        BST_REQUIRE_EQUAL("", ssl::experimental::details::get_attribute_value(x509_name.get(), "CN"));
    }
}

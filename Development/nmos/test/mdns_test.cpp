// The first "test" is of course whether the header compiles standalone
#include "nmos/mdns.h"

#include "bst/test/test.h"
#include "cpprest/basic_utils.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameFormat)
{
    using web::json::value_of;

    // with default settings, service_name should be "<prefix>_<api>_<host>_<port>"
    // with dots replaced by dashes
    const nmos::settings settings;

    BST_CHECK_EQUAL("nmos-cpp_node_127-0-0-1_3212", nmos::experimental::service_name(nmos::service_types::node, settings));
    BST_CHECK_EQUAL("nmos-cpp_query_127-0-0-1_3211", nmos::experimental::service_name(nmos::service_types::query, settings));
    BST_CHECK_EQUAL("nmos-cpp_registration_127-0-0-1_3210", nmos::experimental::service_name(nmos::service_types::registration, settings));
    BST_CHECK_EQUAL("nmos-cpp_system_127-0-0-1_10641", nmos::experimental::service_name(nmos::service_types::system, settings));
    BST_CHECK_EQUAL("nmos-cpp_auth_127-0-0-1_443", nmos::experimental::service_name(nmos::service_types::authorization, settings));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameCustomPrefix)
{
    using web::json::value_of;

    // dots in prefix should be replaced with dashes
    const nmos::settings settings = value_of({
        { nmos::fields::service_name_prefix, U("my.prefix") }
    });

    BST_CHECK_EQUAL("my-prefix_node_127-0-0-1_3212", nmos::experimental::service_name(nmos::service_types::node, settings));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameDotReplacement)
{
    using web::json::value_of;

    // dots in host name should be replaced with dashes
    const nmos::settings settings = value_of({
        { nmos::fields::host_name, U("my-server.example.com") },
        { nmos::experimental::fields::href_mode, 1 }
    });

    BST_CHECK_EQUAL("nmos-cpp_node_my-server-example-com_3212", nmos::experimental::service_name(nmos::service_types::node, settings));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameCustomPort)
{
    using web::json::value_of;

    const nmos::settings settings = value_of({
        { nmos::fields::node_port, 8080 }
    });

    BST_CHECK_EQUAL("nmos-cpp_node_127-0-0-1_8080", nmos::experimental::service_name(nmos::service_types::node, settings));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameMaxLength)
{
    using web::json::value_of;

    // RFC 6763 Section 4.1.1 specifies instance names must not exceed 63 bytes
    const size_t max_length = 63;

    // a name that fits within 63 bytes should pass through unchanged
    {
        const nmos::settings settings;
        const auto name = nmos::experimental::service_name(nmos::service_types::node, settings);
        BST_REQUIRE(name.size() <= max_length);
        // no hash suffix
        BST_CHECK_EQUAL("nmos-cpp_node_127-0-0-1_3212", name);
    }

    // a name that would exceed 63 bytes should be truncated with a hash suffix
    {
        const nmos::settings settings = value_of({
            { nmos::fields::host_name, U("a-host-name-that-is-itself-valid-but-already-63-characters-long.example.com") },
            { nmos::experimental::fields::href_mode, 1 }
        });

        const auto name = nmos::experimental::service_name(nmos::service_types::registration, settings);
        BST_REQUIRE(name.size() <= max_length);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameTruncationStable)
{
    using web::json::value_of;

    // the truncated name should be stable (consistent) even across runs of the same program
    // but that's hard to test (and actually, std::hash is only required to produce the same
    // result for the same input within a single execution of a program; this allows salted
    // hashes that prevent collision denial-of-service attacks, which would be unfortunate
    // for this use case, but common standard library implementations don't do that...)

    const nmos::settings settings = value_of({
        { nmos::fields::host_name, U("a-host-name-that-is-itself-valid-but-already-63-characters-long.example.com") },
        { nmos::experimental::fields::href_mode, 1 }
    });

    const auto name1 = nmos::experimental::service_name(nmos::service_types::registration, settings);
    const auto name2 = nmos::experimental::service_name(nmos::service_types::registration, settings);
    BST_CHECK_EQUAL(name1, name2);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameTruncationSuffix)
{
    using web::json::value_of;

    const size_t max_length = 63;

    // the truncated name should end with a dash followed by a 5-character alphanumeric hash
    const nmos::settings settings = value_of({
        { nmos::fields::host_name, U("a-host-name-that-is-itself-valid-but-already-63-characters-long.example.com") },
        { nmos::experimental::fields::href_mode, 1 }
    });

    const auto name = nmos::experimental::service_name(nmos::service_types::registration, settings);
    BST_REQUIRE(name.size() <= max_length);

    const auto last_dash = name.rfind('-');
    BST_REQUIRE(std::string::npos != last_dash);

    const auto suffix = name.substr(last_dash + 1);
    BST_CHECK_EQUAL(5, suffix.size());
    BST_CHECK(suffix.end() == std::find_if(suffix.begin(), suffix.end(), [](char c)
    {
        return !std::isdigit(static_cast<unsigned char>(c)) && !std::islower(static_cast<unsigned char>(c));
    }));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameExactly63Bytes)
{
    using web::json::value_of;

    // a name that is exactly 63 bytes should not be truncated
    // "nmos-cpp_node_" = 14 chars, "_3212" = 5 chars, so host needs to be 63 - 14 - 5 = 44 chars
    // after dot replacement: e.g. a host name with no dots that is 44 chars
    const std::string host_44(44, 'x');
    const nmos::settings settings = value_of({
        { nmos::fields::host_name, utility::s2us(host_44) },
        { nmos::experimental::fields::href_mode, 1 }
    });

    const auto name = nmos::experimental::service_name(nmos::service_types::node, settings);
    BST_CHECK_EQUAL(63, name.size());
    BST_CHECK_EQUAL("nmos-cpp_node_" + host_44 + "_3212", name);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameOneOver63Bytes)
{
    using web::json::value_of;

    // a name that is 64 bytes should be truncated
    const std::string host_45(45, 'x');
    const nmos::settings settings = value_of({
        { nmos::fields::host_name, utility::s2us(host_45) },
        { nmos::experimental::fields::href_mode, 1 }
    });

    const auto name = nmos::experimental::service_name(nmos::service_types::node, settings);
    BST_REQUIRE(name.size() <= 63);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameTruncationDistinct)
{
    using web::json::value_of;

    // settings that produce distinct over-long names before truncation usually result in
    // distinct names because the hash suffix is based on the full untruncated name
    const auto make_settings = [](int port)
    {
        return value_of({
            { nmos::fields::host_name, U("a-host-name-that-is-itself-valid-but-already-63-characters-long.example.com") },
            { nmos::experimental::fields::href_mode, 1 },
            { nmos::fields::node_port, port }
        });
    };

    const auto name1 = nmos::experimental::service_name(nmos::service_types::node, make_settings(3212));
    const auto name2 = nmos::experimental::service_name(nmos::service_types::node, make_settings(3213));

    BST_CHECK(name1 != name2);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testServiceNameTruncationExample)
{
    using web::json::value_of;

    const nmos::settings settings = value_of({
        { nmos::fields::host_name, U("a-host-name-that-is-itself-valid-but-already-63-characters-long.xz6zx.example.com") },
        { nmos::experimental::fields::href_mode, 1 }
    });

    const auto name = nmos::experimental::service_name(nmos::service_types::node, settings);
    BST_REQUIRE_EQUAL("nmos-cpp_node_a-host-name-that-is-itself-valid-but-alread-", name.substr(0, name.size() - 5));
#ifdef __GLIBCXX__
    BST_CHECK_EQUAL("cr4ck", name.substr(name.size() - 5));
    // ...e4y8q.example.com produces the same hash suffix with this std::hash implementation
    // but anything could happen on another platform...
#endif
}

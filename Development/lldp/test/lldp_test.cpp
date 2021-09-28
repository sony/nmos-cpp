// The first "test" is of course whether the header compiles standalone
#include "lldp/lldp.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMacAddress)
{
    std::vector<uint8_t> mac{ 0xD4, 0xC9, 0xEF, 0xED, 0xC1, 0x09 };
    BST_REQUIRE_EQUAL("d4-c9-ef-ed-c1-09", lldp::parse_mac_address(mac));
    BST_REQUIRE_EQUAL(mac, lldp::make_mac_address("d4-c9-ef-ed-c1-09"));

    BST_REQUIRE_EQUAL(mac, lldp::make_mac_address("D4:C9:EF:ED:C1:09"));

    std::vector<uint8_t> not_mac{ 0xD4, 0xC9, 0xEF };
    BST_REQUIRE(lldp::parse_mac_address(not_mac).empty());
    BST_REQUIRE(lldp::make_mac_address("D4-C9-EF").empty());
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testNetworkAddress)
{
    // example IPv4 address
    // see https://www.iana.org/assignments/iana-ipv4-special-registry/iana-ipv4-special-registry.xhtml
    // and https://tools.ietf.org/html/rfc5737
    const auto ipv4 = lldp::make_network_address("192.0.2.1");
    BST_REQUIRE_EQUAL(lldp::network_address_family_numbers::ipv4, ipv4.front());
    BST_REQUIRE_STRING_EQUAL("192.0.2.1", lldp::parse_network_address(ipv4));
    const std::vector<uint8_t> bad_ipv4{ ipv4.begin(), std::prev(ipv4.end()) };
    BST_REQUIRE(lldp::parse_network_address(bad_ipv4).empty());

    // example IPv6 address
    // see https://www.iana.org/assignments/iana-ipv6-special-registry/iana-ipv6-special-registry.xhtml
    // and https://tools.ietf.org/html/rfc3849
    const auto ipv6 = lldp::make_network_address("2001:0db8::0001");
    BST_REQUIRE_EQUAL(lldp::network_address_family_numbers::ipv6, ipv6.front());
    BST_REQUIRE_STRING_EQUAL("2001:db8::1", lldp::parse_network_address(ipv6));
    const std::vector<uint8_t> bad_ipv6{ ipv6.begin(), std::prev(ipv6.end()) };
    BST_REQUIRE(lldp::parse_network_address(bad_ipv6).empty());

    // example MAC address
    // see https://www.iana.org/assignments/ethernet-numbers/ethernet-numbers.xhtml#ethernet-numbers-2
    // and https://tools.ietf.org/html/rfc7042
    const auto mac = lldp::make_network_address("00-00-5E-00-53-01");
    BST_REQUIRE_EQUAL(lldp::network_address_family_numbers::mac, mac.front());
    BST_REQUIRE_STRING_EQUAL("00-00-5e-00-53-01", lldp::parse_network_address(mac));
    const std::vector<uint8_t> bad_mac{ mac.begin(), std::prev(mac.end()) };
    BST_REQUIRE(lldp::parse_network_address(bad_mac).empty());

    // example DNS name
    // see https://www.iana.org/domains/reserved
    // and https://tools.ietf.org/html/rfc6761
    const auto dns = lldp::make_network_address("api.example.com");
    BST_REQUIRE_EQUAL(lldp::network_address_family_numbers::dns, dns.front());
    BST_REQUIRE_STRING_EQUAL("api.example.com", lldp::parse_network_address(dns));

    // example fully-qualified DNS name
    BST_REQUIRE_EQUAL(lldp::network_address_family_numbers::dns, lldp::make_network_address("api.example.com.").front());

    // example unqualified DNS name
    BST_REQUIRE_EQUAL(lldp::network_address_family_numbers::dns, lldp::make_network_address("api").front());

    // invalid network address
    BST_REQUIRE(lldp::make_mac_address("42").empty());
}

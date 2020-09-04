// The first "test" is of course whether the headers compile standalone
#include "mdns/service_advertiser.h"
#include "mdns/service_discovery.h"

#include <chrono>
#include <mutex>
#include <thread>

#include "bst/test/test.h"
#include "cpprest/basic_utils.h" // for utility::us2s, utility::s2us
#include "cpprest/host_utils.h" // for host_name
#include "slog/all_in_one.h"
#include "mdns/service_advertiser_impl.h"
#include "mdns/service_discovery_impl.h"

static uint16_t testPort1 = 49999;
static uint16_t testPort2 = 49998;
static uint16_t testPort3 = 49997;

namespace
{
    class test_gate : public slog::base_gate
    {
    public:
        test_gate() {}
        virtual ~test_gate() {}

        virtual bool pertinent(slog::severity level) const
        {
            return true;
        }

        virtual void log(const slog::log_message& message) const
        {
            std::lock_guard<std::mutex> lock(mutex);
            messages.push_back(message.str());
        }

        bool hasLogMessage(const char* str) const
        {
            std::lock_guard<std::mutex> lock(mutex);
            return messages.end() != std::find_if(messages.begin(), messages.end(), [&](const std::string& message) { return message.find(str) != std::string::npos; });
        }

        void clearLogMessages()
        {
            std::lock_guard<std::mutex> lock(mutex);
            messages.clear();
        }

    private:
        mutable std::vector<std::string> messages;
        mutable std::mutex mutex;
    };
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsAdvertiserInitOpenClose)
{
    test_gate gate;

    mdns::service_advertiser advertiser(gate);

    advertiser.open().wait();

    advertiser.close().wait();
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsDiscoveryInit)
{
    test_gate gate;

    mdns::service_discovery browser(gate);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsAdvertiseAddress)
{
    test_gate gate;

    mdns::service_advertiser advertiser(gate);

    // Advertise our APIs
    advertiser.open().wait();

    // hmmm, the Avahi compatibility layer implementations of DNSServiceCreateConnection and DNSServiceRegisterRecord
    // just return kDNSServiceErr_Unsupported, so this will fail with the Avahi-based implementation right now
    BST_CHECK(advertiser.register_address("test-mdns-advertise-address", "127.0.0.1", {}).get());

    BST_REQUIRE(gate.hasLogMessage("Registered address: 127.0.0.1 for hostname: test-mdns-advertise-address"));

    advertiser.close().wait();
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsAdvertiseAPIs)
{
    test_gate gate;

    mdns::service_advertiser advertiser(gate);

    mdns::txt_records textRecords
    {
        "api_proto=http",
        "api_ver=v1.0,v1.1,v1.2",
        "pri=100"
    };

    // Advertise our APIs
    advertiser.open().wait();

    BST_CHECK(advertiser.register_service("test-mdns-advertise-1", "_sea-lion-test1._tcp", testPort1, {}, {}, textRecords).get());
    BST_CHECK(advertiser.register_service("test-mdns-advertise-2", "_sea-lion-test1._tcp", testPort2, {}, {}, textRecords).get());
    BST_CHECK(advertiser.register_service("test-mdns-advertise-3", "_sea-lion-test2._tcp", testPort3, {}, {}, textRecords).get());

    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-advertise-1._sea-lion-test1._tcp"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-advertise-2._sea-lion-test1._tcp"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-advertise-3._sea-lion-test2._tcp"));

    advertiser.close().wait();
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsBrowseAPIs)
{
    test_gate gate;

    mdns::service_advertiser advertiser(gate);

    mdns::txt_records textRecords
    {
        "api_proto=http",
        "api_ver=v1.0,v1.1,v1.2",
        "pri=100"
    };

    // Advertise our APIs
    advertiser.open().wait();

    BST_CHECK(advertiser.register_service("test-mdns-browse-1", "_sea-lion-test1._tcp", testPort1, {}, {}, textRecords).get());
    BST_CHECK(advertiser.register_service("test-mdns-browse-2", "_sea-lion-test1._tcp", testPort2, {}, {}, textRecords).get());
    BST_CHECK(advertiser.register_service("test-mdns-browse-3", "_sea-lion-test2._tcp", testPort3, {}, {}, textRecords).get());

    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-browse-1._sea-lion-test1._tcp"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-browse-2._sea-lion-test1._tcp"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-browse-3._sea-lion-test2._tcp"));
    gate.clearLogMessages();

    // Now discover the APIs
    mdns::service_discovery browser(gate);
    std::vector<mdns::browse_result> browsed;
    auto browse_count = [&](const std::string& name)
    {
        return std::count_if(browsed.begin(), browsed.end(), [&](const mdns::browse_result& br)
        {
            return br.name == name;
        });
    };

    int retries = 0;
    do
    {
        browsed = browser.browse("_sea-lion-test1._tcp").get();
    }
    while (retries++ < 3 && browse_count("test-mdns-browse-1") == 0 && browse_count("test-mdns-browse-2") == 0);

    BST_CHECK(retries <= 3);

    BST_CHECK(browse_count("test-mdns-browse-1") >= 1);

    BST_CHECK(browse_count("test-mdns-browse-2") >= 1);

    browsed = browser.browse("_sea-lion-test2._tcp").get();

    BST_CHECK(browse_count("test-mdns-browse-3") >= 1);

    gate.clearLogMessages();

    // stop all the advertising
    advertiser.close().wait();

    BST_REQUIRE(gate.hasLogMessage("Advertisement stopped for: test-mdns-browse-1"));
    BST_REQUIRE(gate.hasLogMessage("Advertisement stopped for: test-mdns-browse-2"));
    BST_REQUIRE(gate.hasLogMessage("Advertisement stopped for: test-mdns-browse-3"));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsBrowseCancellation)
{
    test_gate gate;

    mdns::service_discovery browser(gate);
    std::vector<mdns::browse_result> browsed;

    auto browse = browser.browse("_sea-lion-test0._tcp", {}, 0, std::chrono::seconds(1));

    BST_REQUIRE_EQUAL(pplx::task_status::completed, browse.wait());
    browsed = browse.get();
    BST_REQUIRE(browsed.empty());

    pplx::cancellation_token_source cts;
    browse = browser.browse("_sea-lion-test0._tcp", {}, 0, std::chrono::seconds(60), cts.get_token());

    std::this_thread::sleep_for(std::chrono::seconds(1));
    cts.cancel();

    BST_REQUIRE_EQUAL(pplx::task_status::canceled, browse.wait());
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsResolveAPIs)
{
    test_gate gate;

    mdns::service_advertiser advertiser(gate);

    mdns::txt_records textRecords;

    textRecords.push_back("api_proto=http");
    textRecords.push_back("api_ver=v1.0,v1.1,v1.2");
    textRecords.push_back("pri=100");

    // get the ip addresses
    std::set<std::string> ipAddresses;
    for (const auto& i : web::hosts::experimental::host_interfaces())
    {
        for (const auto& a : i.addresses)
        {
            ipAddresses.insert(utility::us2s(a));
        }
    }
    // hmm, mdns::service_discovery::resolve can also return the loopback address on some
    // platforms (macOS) and in some circumstances (no real network interfaces)
    ipAddresses.insert("127.0.0.1");

    // Advertise our APIs
    advertiser.open().wait();

    BST_CHECK(advertiser.register_service("test-mdns-resolve-1", "_sea-lion-test1._tcp", testPort1, {}, {}, textRecords).get());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-resolve-1._sea-lion-test1._tcp"));
    gate.clearLogMessages();

    // Now discover an API
    mdns::service_discovery resolver(gate);
    std::vector<mdns::browse_result> browsed;

    browsed = resolver.browse("_sea-lion-test1._tcp").get();

    BST_REQUIRE(!browsed.empty());
    gate.clearLogMessages();

    // Now resolve all the ip addresses and port
    std::vector<mdns::resolve_result> resolved;
    for (auto& resolving : browsed)
    {
        if (resolving.name == "test-mdns-resolve-1")
        {
            std::vector<mdns::resolve_result> r;
            r = resolver.resolve(resolving.name, resolving.type, resolving.domain, resolving.interface_id, std::chrono::seconds(2)).get();
            BST_REQUIRE(!r.empty());
            resolved.insert(resolved.end(), r.begin(), r.end());
        }
    }

    BST_REQUIRE(!resolved.empty());

    std::set<std::string> ip_addresses;
    for (auto& result : resolved)
    {
        ip_addresses.insert(result.ip_addresses.begin(), result.ip_addresses.end());

        BST_REQUIRE(result.port == testPort1);

        BST_REQUIRE(result.txt_records.size() == textRecords.size());
        size_t count = 0;
        for (size_t i = 0; i < textRecords.size(); i++)
        {
            if (result.txt_records.end() != std::find(result.txt_records.begin(), result.txt_records.end(), textRecords[i]))
            {
                count++;
            }
        }
        BST_REQUIRE(count == textRecords.size());
    }

    // hmmm, ideally, all addresses would be returned by mdns::service_discovery::resolve
    // but that isn't the case with the Avahi-based implementation which doesn't provide
    // DNSServiceGetAddrInfo and only returns one address from 'mdns4' or 'mdns4_minimal'
    // for the name-service-switch to return from getaddrinfo
    BST_REQUIRE(!ip_addresses.empty());
    for (auto& address : ip_addresses)
    {
        BST_CHECK(ipAddresses.find(address) != ipAddresses.end());
    }

    // now update the txt records
    textRecords.pop_back();
    textRecords.push_back("pri=1");

    BST_REQUIRE(advertiser.update_record("test-mdns-resolve-1", "_sea-lion-test1._tcp", {}, textRecords).get());

    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Now resolve again and check the txt records
    auto& resolving = browsed[0];
    resolved = resolver.resolve(resolving.name, resolving.type, resolving.domain, resolving.interface_id, std::chrono::seconds(2)).get();

    BST_REQUIRE(!resolved.empty());
    for (auto& result : resolved)
    {
        BST_REQUIRE(result.txt_records.size() == textRecords.size());
        size_t count = 0;
        for (size_t i = 0; i < textRecords.size(); i++)
        {
            if (result.txt_records.end() != std::find(result.txt_records.begin(), result.txt_records.end(), textRecords[i]))
            {
                count++;
            }
        }
        BST_REQUIRE(count == textRecords.size());
    }

    // stop all the advertising
    advertiser.close().wait();

    BST_REQUIRE(gate.hasLogMessage("Advertisement stopped for: test-mdns-resolve-1"));
}

////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    struct test_advertiser_impl : public mdns::details::service_advertiser_impl
    {
        explicit test_advertiser_impl(slog::base_gate& gate)
            : gate(gate)
        {}

        pplx::task<void> open() override
        {
            return pplx::create_task([&]
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Open";
            });
        }

        pplx::task<void> close() override
        {
            return pplx::create_task([&]
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Close";
            });
        }

        pplx::task<bool> register_address(const std::string& host_name, const std::string& ip_address, const std::string& domain, std::uint32_t interface_id) override
        {
            return pplx::task_from_result(true);
        }

        pplx::task<bool> register_service(const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain, const std::string& host_name, const mdns::txt_records& txt_records) override
        {
            return pplx::task_from_result(true);
        }

        pplx::task<bool> update_record(const std::string& name, const std::string& type, const std::string& domain, const mdns::txt_records& txt_records) override
        {
            return pplx::task_from_result(false);
        }

        slog::base_gate& gate;
    };

    class test_discovery_impl : public mdns::details::service_discovery_impl
    {
    public:
        explicit test_discovery_impl(slog::base_gate& gate)
            : gate(gate)
        {}

        pplx::task<bool> browse(const mdns::browse_handler& handler, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token) override
        {
            return pplx::create_task([&]
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Browsed";
                return true;
            });
        }
        pplx::task<bool> resolve(const mdns::resolve_handler& handler, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token) override
        {
            return pplx::task_from_result(false);
        }

        slog::base_gate& gate;
    };
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsImpl)
{
    test_gate gate;

    mdns::service_advertiser advertiser(std::unique_ptr<mdns::details::service_advertiser_impl>(new test_advertiser_impl(gate)));
    mdns::service_discovery browser(std::unique_ptr<mdns::details::service_discovery_impl>(new test_discovery_impl(gate)));

    advertiser.open().wait();
    BST_REQUIRE(gate.hasLogMessage("Open"));

    auto browsed = browser.browse("_sea-lion-test1._tcp").get();
    BST_REQUIRE(gate.hasLogMessage("Browsed"));
    BST_REQUIRE(browsed.empty());

    advertiser.close().wait();
    BST_REQUIRE(gate.hasLogMessage("Close"));
}

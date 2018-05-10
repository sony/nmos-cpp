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

static unsigned int sleepSeconds = 2;

static uint16_t testPort1 = 49999;
static uint16_t testPort2 = 49998;
static uint16_t testPort3 = 49997;

namespace
{
    class test_gate : public slog::base_gate
    {
    public:
        test_gate() : service({ messages, mutex }) {}
        virtual ~test_gate() {}

        virtual bool pertinent(slog::severity level) const { return true; }
        virtual void log(const slog::log_message& message) const { service(message); }

        bool hasLogMessage(const char* str) const
        {
            return messages.end() != std::find_if(messages.begin(), messages.end(), [&](const std::string& message) { return message.find(str) != std::string::npos; });
        }

        void clearLogMessages()
        {
            messages.clear();
        }

    private:
        struct service_function
        {
            std::vector<std::string>& messages;
            std::mutex& mutex;

            typedef const slog::async_log_message& argument_type;
            void operator()(argument_type message) const
            {
                std::lock_guard<std::mutex> lock(mutex);
                messages.push_back(message.str());
            }
        };

        std::vector<std::string> messages;
        std::mutex mutex;
        mutable slog::async_log_service<service_function> service;
    };
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsInit)
{
    test_gate gate;

    std::unique_ptr<mdns::service_advertiser> advertiser = mdns::make_advertiser(gate);

    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Discovery/advertiser instance constructed"));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsAdvertiseStart)
{
    test_gate gate;

    std::unique_ptr<mdns::service_advertiser> advertiser = mdns::make_advertiser(gate);

    advertiser->start();

    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Discovery/advertiser instance constructed"));
    BST_REQUIRE(gate.hasLogMessage("Advertisement started for 0 service(s)"));

    advertiser->stop();
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsAdvertiseAPIs)
{
    test_gate gate;

    std::unique_ptr<mdns::service_advertiser> advertiser = mdns::make_advertiser(gate);

    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Discovery/advertiser instance constructed"));
    gate.clearLogMessages();

    mdns::txt_records textRecords
    {
        "api_proto=http",
        "api_ver=v1.0,v1.1,v1.2",
        "pri=100"
    };

    advertiser->register_service("test-mdns-advertise-1", "_sea-lion-test1._tcp", testPort1, {}, {}, textRecords);
    advertiser->register_service("test-mdns-advertise-2", "_sea-lion-test1._tcp", testPort2, {}, {}, textRecords);
    advertiser->register_service("test-mdns-advertise-3", "_sea-lion-test2._tcp", testPort3, {}, {}, textRecords);

    // Advertise our APIs
    advertiser->start();
    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Advertisement started for 3 service(s)"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-advertise-1._sea-lion-test1._tcp"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-advertise-2._sea-lion-test1._tcp"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-advertise-3._sea-lion-test2._tcp"));

    advertiser->stop();
}


////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsBrowseAPIs)
{
    test_gate gate;

    std::unique_ptr<mdns::service_advertiser> advertiser = mdns::make_advertiser(gate);

    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Discovery/advertiser instance constructed"));
    gate.clearLogMessages();

    mdns::txt_records textRecords
    {
        "api_proto=http",
        "api_ver=v1.0,v1.1,v1.2",
        "pri=100"
    };

    advertiser->register_service("test-mdns-browse-1", "_sea-lion-test1._tcp", testPort1, {}, {}, textRecords);
    advertiser->register_service("test-mdns-browse-2", "_sea-lion-test1._tcp", testPort2, {}, {}, textRecords);
    advertiser->register_service("test-mdns-browse-3", "_sea-lion-test2._tcp", testPort3, {}, {}, textRecords);

    // Advertise our APIs
    advertiser->start();
    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Advertisement started for 3 service(s)"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-browse-1._sea-lion-test1._tcp"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-browse-2._sea-lion-test1._tcp"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-browse-3._sea-lion-test2._tcp"));
    gate.clearLogMessages();

    // Now discover the APIs
    std::unique_ptr<mdns::service_discovery> browser = mdns::make_discovery(gate);
    std::vector<mdns::service_discovery::browse_result> browsed;

    browser->browse(browsed, "_sea-lion-test1._tcp");

    auto browseResult = std::count_if(browsed.begin(), browsed.end(), [](const mdns::service_discovery::browse_result& br)
    {
        return br.name == "test-mdns-browse-2";
    });
    BST_REQUIRE(browseResult >= 1);

    browser->browse(browsed, "_sea-lion-test2._tcp");

    browseResult = std::count_if(browsed.begin(), browsed.end(), [](const mdns::service_discovery::browse_result& br)
    {
        return br.name == "test-mdns-browse-3";
    });
    BST_REQUIRE(browseResult >= 1);

    gate.clearLogMessages();

    // stop all the advertising
    advertiser->stop();
    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Advertisement stopped for: test-mdns-browse-1"));
    BST_REQUIRE(gate.hasLogMessage("Advertisement stopped for: test-mdns-browse-2"));
    BST_REQUIRE(gate.hasLogMessage("Advertisement stopped for: test-mdns-browse-3"));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMdnsResolveAPIs)
{
    test_gate gate;

    std::unique_ptr< mdns::service_advertiser > advertiser = mdns::make_advertiser(gate);

    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Discovery/advertiser instance constructed"));
    gate.clearLogMessages();

    mdns::txt_records textRecords;

    textRecords.push_back("api_proto=http");
    textRecords.push_back("api_ver=v1.0,v1.1,v1.2");
    textRecords.push_back("pri=100");

    // get the ip addresses
    std::string hostname = utility::us2s(web::http::experimental::host_name());
    std::vector<std::string> ipAddresses;
    for (const auto& a : web::http::experimental::host_addresses(utility::s2us(hostname)))
    {
        ipAddresses.push_back(utility::us2s(a));
    }
    BST_REQUIRE(!ipAddresses.empty());

    advertiser->register_service("test-mdns-resolve-1", "_sea-lion-test1._tcp", testPort1, {}, {}, textRecords);

    // Advertise our APIs
    advertiser->start();
    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Advertisement started for 1 service(s)"));
    BST_REQUIRE(gate.hasLogMessage("Registered advertisement for: test-mdns-resolve-1._sea-lion-test1._tcp"));
    gate.clearLogMessages();

    // Now discover an API
    std::unique_ptr<mdns::service_discovery> resolver = mdns::make_discovery(gate);
    std::vector<mdns::service_discovery::browse_result> browsed;
    mdns::service_discovery::browse_result testQuery;

    resolver->browse(browsed, "_sea-lion-test1._tcp");

    for (size_t i = 0; i < browsed.size(); i++)
    {
        if (browsed[i].name == "test-mdns-resolve-1")
        {
            testQuery = browsed[i];
            break;
        }
    }

    BST_REQUIRE(!testQuery.name.empty());

    // Now resolve the ip addresses and port
    std::vector<mdns::service_discovery::resolve_result> resolved;

    gate.clearLogMessages();

    BST_REQUIRE(resolver->resolve(resolved, testQuery.name, testQuery.type, testQuery.domain, testQuery.interface_id, 2));

    BST_REQUIRE(!resolved.empty());
    for (auto& result : resolved)
    {
        for (size_t i = 0; i < ipAddresses.size(); i++)
        {
            BST_CHECK(result.ip_addresses.end() != std::find(result.ip_addresses.begin(), result.ip_addresses.end(), ipAddresses[i]));
        }

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

    // now update the txt records
    textRecords.pop_back();
    textRecords.push_back("pri=1");

    BST_REQUIRE(advertiser->update_record("test-mdns-resolve-1", "_sea-lion-test1._tcp", {}, textRecords));

    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    // Now resolve again and check the txt records
    BST_REQUIRE(resolver->resolve(resolved, testQuery.name, testQuery.type, testQuery.domain, testQuery.interface_id, 2));

    BST_REQUIRE(!resolved.empty());
    for (auto& result : resolved)
    {
        for (size_t i = 0; i < ipAddresses.size(); i++)
        {
            BST_CHECK(result.ip_addresses.end() != std::find(result.ip_addresses.begin(), result.ip_addresses.end(), ipAddresses[i]));
        }

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

    // stop all the advertising
    advertiser->stop();
    std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));

    BST_REQUIRE(gate.hasLogMessage("Advertisement stopped for: test-mdns-resolve-1"));
}

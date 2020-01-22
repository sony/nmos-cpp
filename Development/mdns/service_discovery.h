#ifndef MDNS_SERVICE_DISCOVERY_H
#define MDNS_SERVICE_DISCOVERY_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include "pplx/pplxtasks.h"
#include "mdns/core.h"

namespace slog
{
    class base_gate;
}

// An interface for straightforward DNS Service Discovery (DNS-SD) browsing
// via unicast DNS, or multicast DNS (mDNS)
namespace mdns
{
    // service discovery implementation
    namespace details
    {
        class service_discovery_impl;
    }

    struct browse_result
    {
        browse_result() : interface_id(0) {}
        browse_result(const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id = 0) : name(name), type(type), domain(domain), interface_id(interface_id) {}

        std::string name;
        std::string type;
        std::string domain;
        std::uint32_t interface_id;
    };

    // return true from the browse result callback if the operation should be ended before its specified timeout once no more results are "imminent"
    // the callback must not throw
    typedef std::function<bool(const browse_result&)> browse_handler;

    struct resolve_result
    {
        resolve_result() {}
        resolve_result(const std::string& host_name, std::uint16_t port, const mdns::txt_records& txt_records, std::uint32_t interface_id = 0) : host_name(host_name), port(port), txt_records(txt_records), interface_id(interface_id) {}

        std::string host_name;
        std::uint16_t port;
        mdns::txt_records txt_records;
        std::uint32_t interface_id;

        std::vector<std::string> ip_addresses;
    };

    // return true from the resolve result callback if the operation should be ended before its specified timeout once no more results are "imminent"
    // the callback must not throw
    typedef std::function<bool(const resolve_result&)> resolve_handler;

    class service_discovery
    {
    public:
        explicit service_discovery(slog::base_gate& gate); // or web::logging::experimental::log_handler to avoid the dependency on slog?
        ~service_discovery(); // do not destroy this object with outstanding tasks!

        pplx::task<bool> browse(const browse_handler& handler, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token = pplx::cancellation_token::none());
        pplx::task<bool> resolve(const resolve_handler& handler, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token = pplx::cancellation_token::none());

        template <typename Rep = std::chrono::seconds::rep, typename Period = std::chrono::seconds::period>
        pplx::task<bool> browse(const browse_handler& handler, const std::string& type, const std::string& domain = {}, std::uint32_t interface_id = 0, const std::chrono::duration<Rep, Period>& timeout = std::chrono::seconds(default_timeout_seconds), const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            return browse(handler, type, domain, interface_id, std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout), token);
        }
        template <typename Rep = std::chrono::seconds::rep, typename Period = std::chrono::seconds::period>
        pplx::task<bool> resolve(const resolve_handler& handler, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id = 0, const std::chrono::duration<Rep, Period>& timeout = std::chrono::seconds(default_timeout_seconds), const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            return resolve(handler, name, type, domain, interface_id, std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout), token);
        }

        template <typename Rep = std::chrono::seconds::rep, typename Period = std::chrono::seconds::period>
        pplx::task<std::vector<browse_result>> browse(const std::string& type, const std::string& domain = {}, std::uint32_t interface_id = 0, const std::chrono::duration<Rep, Period>& timeout = std::chrono::seconds(default_timeout_seconds), const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            std::shared_ptr<std::vector<browse_result>> results(new std::vector<browse_result>());
            return browse([results](const browse_result& result) { results->push_back(result); return true; }, type, domain, interface_id, std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout), token)
                .then([results](bool) { return std::move(*results); });
        }
        template <typename Rep = std::chrono::seconds::rep, typename Period = std::chrono::seconds::period>
        pplx::task<std::vector<resolve_result>> resolve(const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id = 0, const std::chrono::duration<Rep, Period>& timeout = std::chrono::seconds(default_timeout_seconds), const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            std::shared_ptr<std::vector<resolve_result>> results(new std::vector<resolve_result>());
            return resolve([results](const resolve_result& result) { results->push_back(result); return true; }, name, type, domain, interface_id, std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout), token)
                .then([results](bool) { return std::move(*results); });
        }

        service_discovery(service_discovery&& other);
        service_discovery& operator=(service_discovery&& other);

        service_discovery(std::unique_ptr<details::service_discovery_impl> impl);

    private:
        service_discovery(const service_discovery& other);
        service_discovery& operator=(const service_discovery& other);

        std::unique_ptr<details::service_discovery_impl> impl;
    };
}

#endif

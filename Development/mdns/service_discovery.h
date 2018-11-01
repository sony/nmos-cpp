#ifndef MDNS_SERVICE_DISCOVERY_H
#define MDNS_SERVICE_DISCOVERY_H

#include <chrono>
#include <cstdint>
#include <memory>
#include "mdns/core.h"

namespace slog
{
    class base_gate;
}

// An interface for straightforward mDNS Service Discovery browsing
namespace mdns
{
    class service_discovery
    {
    public:
        virtual ~service_discovery() {}

        struct browse_result
        {
            browse_result() : interface_id(0) {}
            browse_result(const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id) : name(name), type(type), domain(domain), interface_id(interface_id) {}

            std::string name;
            std::string type;
            std::string domain;
            std::uint32_t interface_id;
        };

        struct resolve_result
        {
            resolve_result() {}
            resolve_result(const std::string& host_name, std::uint16_t port, const mdns::txt_records& txt_records) : host_name(host_name), port(port), txt_records(txt_records) {}

            std::string host_name;
            std::uint16_t port;
            mdns::txt_records txt_records;

            std::vector<std::string> ip_addresses;
        };

        virtual bool browse(std::vector<browse_result>& results, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& latest_timeout, const std::chrono::steady_clock::duration& earliest_timeout) = 0;
        virtual bool resolve(std::vector<resolve_result>& results, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& latest_timeout, const std::chrono::steady_clock::duration& earliest_timeout) = 0;

        template <typename Rep1 = std::chrono::seconds::rep, typename Period1 = std::chrono::seconds::period, typename Rep2 = std::chrono::seconds::rep, typename Period2 = std::chrono::seconds::period>
        bool browse(std::vector<browse_result>& results, const std::string& type, const std::string& domain = {}, std::uint32_t interface_id = 0, const std::chrono::duration<Rep1, Period1>& latest_timeout = std::chrono::seconds(default_latest_timeout_seconds), const std::chrono::duration<Rep2, Period2>& earliest_timeout = std::chrono::seconds(default_earliest_timeout_seconds))
        {
            return browse(results, type, domain, interface_id, std::chrono::duration_cast<std::chrono::steady_clock::duration>(latest_timeout), std::chrono::duration_cast<std::chrono::steady_clock::duration>(earliest_timeout));
        }
        template <typename Rep1 = std::chrono::seconds::rep, typename Period1 = std::chrono::seconds::period, typename Rep2 = std::chrono::seconds::rep, typename Period2 = std::chrono::seconds::period>
        bool resolve(std::vector<resolve_result>& results, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id = 0, const std::chrono::duration<Rep1, Period1>& latest_timeout = std::chrono::seconds(default_latest_timeout_seconds), const std::chrono::duration<Rep2, Period2>& earliest_timeout = std::chrono::seconds(default_earliest_timeout_seconds))
        {
            return resolve(results, name, type, domain, interface_id, std::chrono::duration_cast<std::chrono::steady_clock::duration>(latest_timeout), std::chrono::duration_cast<std::chrono::steady_clock::duration>(earliest_timeout));
        }
    };

    // make a default implementation of the mDNS Service Discovery browsing interface
    std::unique_ptr<service_discovery> make_discovery(slog::base_gate& gate);
}

#endif

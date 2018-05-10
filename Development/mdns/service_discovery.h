#ifndef MDNS_SERVICE_DISCOVERY_H
#define MDNS_SERVICE_DISCOVERY_H

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

        virtual bool browse(std::vector<browse_result>& results, const std::string& type, const std::string& domain = {}, std::uint32_t interface_id = 0, unsigned int latest_timeout_seconds = default_latest_timeout_seconds, unsigned int earliest_timeout_seconds = default_earliest_timeout_seconds) = 0;
        virtual bool resolve(std::vector<resolve_result>& results, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id = 0, unsigned int latest_timeout_seconds = default_latest_timeout_seconds, unsigned int earliest_timeout_seconds = default_earliest_timeout_seconds) = 0;
    };

    // make a default implementation of the mDNS Service Discovery browsing interface
    std::unique_ptr<service_discovery> make_discovery(slog::base_gate& gate);
}

#endif

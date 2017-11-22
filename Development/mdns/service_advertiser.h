#ifndef MDNS_SERVICE_ADVERTISER_H
#define MDNS_SERVICE_ADVERTISER_H

#include <cstdint>
#include <memory>
#include "mdns/core.h"

namespace slog
{
    class base_gate;
}

// An interface for straightforward mDNS Service Discovery advertisement
namespace mdns
{
    class service_advertiser
    {
    public:
        virtual ~service_advertiser() {}

        virtual bool register_service(const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain = {}, const std::string& host_name = {}, const txt_records& txt_records = {}) = 0;
        virtual bool update_record(const std::string& name, const std::string& type, const std::string& domain = {}, const txt_records& txt_records = {}) = 0;

        virtual void stop() = 0;
        virtual void start() = 0;
    };

    // RAII helper for service advertisement sessions
    struct service_advertiser_guard
    {
        service_advertiser_guard(service_advertiser& advertiser) : advertiser(advertiser) { advertiser.start(); }
        ~service_advertiser_guard() { advertiser.stop(); }
        service_advertiser& advertiser;
    };

    // make a default implementation of the mDNS Service Discovery advertisement interface 
    std::unique_ptr<service_advertiser> make_advertiser(slog::base_gate& gate);
}

#endif

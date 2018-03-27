#ifndef MDNS_BONJOUR_DNS_IMPL_H
#define MDNS_BONJOUR_DNS_IMPL_H

#include "mdns/service_advertiser.h"
#include "mdns/service_discovery.h"

namespace mdns
{
    class bonjour_dns_impl : public mdns::service_advertiser, public mdns::service_discovery
    {
    public:
        bonjour_dns_impl(slog::base_gate& gate);
        virtual ~bonjour_dns_impl();

        // Advertisement
        virtual bool register_service(const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain, const std::string& host_name, const txt_records& txt_records);
        virtual bool update_record(const std::string& name, const std::string& type, const std::string& domain, const txt_records& txt_records);

        virtual void stop();
        virtual void start();

        // Discovery - not thread-safe!
        virtual bool browse(std::vector<browse_result>& found, const std::string& type, const std::string& domain, unsigned int latest_timeout_seconds, unsigned int earliest_timeout_seconds);
        virtual bool resolve(resolve_result& resolved, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, unsigned int timeout_seconds);

        struct service
        {
            service(const std::string& name, const std::string& type, const std::string& domain, void* sdRef)
                : name(name)
                , type(type)
                , domain(domain)
                , sdRef(sdRef)
            {}

            std::string name;
            std::string type;
            std::string domain;

            void* sdRef;
        };

        std::vector<service> m_services;
        slog::base_gate& m_gate;

        // browse/resolve in-flight state
        std::vector<browse_result>* m_found;
        bool m_more_coming;
        resolve_result* m_resolved;
    };
}

#endif

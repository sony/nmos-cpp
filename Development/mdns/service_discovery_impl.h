#ifndef MDNS_SERVICE_DISCOVERY_IMPL_H
#define MDNS_SERVICE_DISCOVERY_IMPL_H

#include "mdns/dns_sd_impl.h"
#include "mdns/service_discovery.h"

namespace mdns_details
{
    struct address_result
    {
        address_result(const std::string& host_name, const std::string& ip_address, std::uint32_t ttl = 0, std::uint32_t interface_id = 0) : host_name(host_name), ip_address(ip_address), ttl(ttl), interface_id(interface_id) {}

        std::string host_name;
        std::string ip_address;
        std::uint32_t ttl;
        std::uint32_t interface_id;
    };

    // return true from the address result callback if the operation should be ended before its specified timeout once no more results are "imminent"
    typedef std::function<bool(const address_result&)> address_handler;

    bool getaddrinfo(const address_handler& handler, const std::string& host_name, std::uint32_t interface_id, const std::chrono::steady_clock::duration& latest_timeout, DNSServiceCancellationToken cancel, slog::base_gate& gate);
}

namespace mdns
{
    namespace details
    {
        struct cancellation_guard
        {
            cancellation_guard(const pplx::cancellation_token& source)
                : source(source)
                , target(nullptr)
            {
                if (source.is_cancelable())
                {
                    DNSServiceCreateCancellationToken(&target);
                    reg = source.register_callback([this] { DNSServiceCancel(target); });
                }
            }

            ~cancellation_guard()
            {
                if (pplx::cancellation_token_registration{} != reg) source.deregister_callback(reg);
                DNSServiceCancellationTokenDeallocate(target);
            }

            pplx::cancellation_token source;
            DNSServiceCancellationToken target;
            pplx::cancellation_token_registration reg;
        };

        class service_discovery_impl
        {
        public:
            virtual ~service_discovery_impl() {}

            virtual pplx::task<bool> browse(const browse_handler& handler, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token) = 0;
            virtual pplx::task<bool> resolve(const resolve_handler& handler, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token) = 0;
        };
    }
}

#endif

#ifndef MDNS_SERVICE_DISCOVERY_IMPL_H
#define MDNS_SERVICE_DISCOVERY_IMPL_H

#include "mdns/service_discovery.h"

namespace mdns
{
    namespace details
    {
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

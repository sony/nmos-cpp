#ifndef MDNS_SERVICE_ADVERTISER_IMPL_H
#define MDNS_SERVICE_ADVERTISER_IMPL_H

#include "mdns/service_advertiser.h"

namespace mdns
{
    namespace details
    {
        class service_advertiser_impl
        {
        public:
            virtual ~service_advertiser_impl() {}

            virtual pplx::task<void> open() = 0;
            virtual pplx::task<void> close() = 0;
            virtual pplx::task<bool> register_address(const std::string& host_name, const std::string& ip_address, const std::string& domain, std::uint32_t interface_id) = 0;
            virtual pplx::task<bool> register_service(const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain, const std::string& host_name, const txt_records& txt_records) = 0;
            virtual pplx::task<bool> update_record(const std::string& name, const std::string& type, const std::string& domain, const txt_records& txt_records) = 0;
        };
    }
}

#endif

#ifndef MDNS_SERVICE_ADVERTISER_H
#define MDNS_SERVICE_ADVERTISER_H

#include <cstdint>
#include <memory>
#include "pplx/pplx_utils.h"
#include "mdns/core.h"

namespace slog
{
    class base_gate;
}

// An interface for straightforward DNS Service Discovery (DNS-SD) advertisement
// typically via multicast DNS (mDNS)
namespace mdns
{
    // service advertiser implementation
    namespace details
    {
        class service_advertiser_impl;
    }

    class service_advertiser
    {
    public:
        explicit service_advertiser(slog::base_gate& gate); // or web::logging::experimental::log_handler to avoid the dependency on slog?
        ~service_advertiser(); // do not destroy this object with outstanding tasks!

        pplx::task<void> open();
        pplx::task<void> close();
        pplx::task<bool> register_address(const std::string& host_name, const std::string& ip_address, const std::string& domain = {}, std::uint32_t interface_id = {});
        pplx::task<bool> register_service(const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain = {}, const std::string& host_name = {}, const txt_records& txt_records = {});
        pplx::task<bool> update_record(const std::string& name, const std::string& type, const std::string& domain = {}, const txt_records& txt_records = {});

        service_advertiser(service_advertiser&& other);
        service_advertiser& operator=(service_advertiser&& other);

        service_advertiser(std::unique_ptr<details::service_advertiser_impl> impl);

    private:
        service_advertiser(const service_advertiser& other);
        service_advertiser& operator=(const service_advertiser& other);

        std::unique_ptr<details::service_advertiser_impl> impl;
    };

    // RAII helper for service advertisement sessions
    typedef pplx::open_close_guard<service_advertiser> service_advertiser_guard;
}

#endif

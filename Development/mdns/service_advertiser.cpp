#include "mdns/service_advertiser.h"
#include "mdns/bonjour_dns_impl.h"

namespace mdns
{
    std::unique_ptr<service_advertiser> make_advertiser(slog::base_gate& gate)
    {
        return std::unique_ptr<service_advertiser>(new mdns::bonjour_dns_impl(gate));
    }
}

#include "mdns/service_discovery.h"
#include "mdns/bonjour_dns_impl.h"

namespace mdns
{
    std::unique_ptr<service_discovery> make_discovery(slog::base_gate& gate)
    {
        return std::unique_ptr<service_discovery>(new mdns::bonjour_dns_impl(gate));
    }
}

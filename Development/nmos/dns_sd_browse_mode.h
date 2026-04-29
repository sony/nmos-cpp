#ifndef NMOS_DNS_SD_BROWSE_MODE_H
#define NMOS_DNS_SD_BROWSE_MODE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // DNS-SD browse method per TR-10-9 Section 15
    // Selected via the dns_sd_browse_mode setting in nmos::fields
    DEFINE_STRING_ENUM(dns_sd_browse_mode)
    namespace dns_sd_browse_modes
    {
        const dns_sd_browse_mode both{ U("both") };       // unicast DNS first, mDNS fallback if unsuccessful
        const dns_sd_browse_mode unicast{ U("unicast") }; // unicast DNS only
        const dns_sd_browse_mode mdns{ U("mdns") };       // mDNS only
    }
}

#endif

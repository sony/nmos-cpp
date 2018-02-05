#ifndef NMOS_MDNS_API_H
#define NMOS_MDNS_API_H

#include "nmos/api_utils.h" // for web::http::experimental::listener::api_router and nmos::route_pattern
#include "nmos/slog.h" // for slog::base_gate and slog::severity, etc.

// This is an experimental extension to expose mDNS Service Discovery via a REST API
namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_mdns_api(slog::base_gate& gate);

        namespace patterns
        {
            // A service type combines a registered service identifier (name) and the protocol, each prepended by an underscore ('_') and separated by a dot ('.')
            // For example, the IS-04 service types are "_nmos-query._tcp", "_nmos-registration._tcp" and "_nmos-node._tcp".
            // SRVNAME = *(1*DIGIT [HYPHEN]) ALPHA *([HYPHEN] ALNUM)
            // See https://tools.ietf.org/html/rfc6335#section-5.1
            // "The transport protocol(s) [...] is currently limited to one or more of TCP, UDP, SCTP, and DCCP."
            // See https://tools.ietf.org/html/rfc6335#section-8.1.1
            const nmos::route_pattern mdnsServiceType = make_route_pattern(U("mdnsServiceType"), U("_([0-9]{1,}-?)*[A-Za-z](-?[A-Za-z0-9])*\\._(tcp|udp|sctp|dccp)"));

            // A service (instance) name (regex could be better!)
            const nmos::route_pattern mdnsServiceName = make_route_pattern(U("mdnsServiceName"), U(".{1,63}"));
        }
    }
}

#endif

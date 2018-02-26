#ifndef NMOS_MDNS_H
#define NMOS_MDNS_H

#include "mdns/core.h" // for mdns::structured_txt_records
#include "nmos/api_version.h"
#include "nmos/settings.h" // just a forward declaration of nmos::settings required for nmos::experimental functions

namespace mdns
{
    class service_advertiser;
    class service_discovery;
}

namespace nmos
{
    // "APIs MUST produce an mDNS advertisement [...] accompanied by DNS TXT records"
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/RegistrationAPI.raml#L17 etc.

    typedef std::string service_type;

    namespace service_types
    {
        const service_type node{ "_nmos-node._tcp" };
        const service_type query{ "_nmos-query._tcp" };
        const service_type registration{ "_nmos-registration._tcp" };
    }

    // "The DNS-SD advertisement MUST be accompanied by a TXT record of name 'api_proto' with a value
    // of either 'http' or 'https' dependent on the protocol in use by the [...] server."
    // (However, v1.0 did not include this record, so when it is omitted, "http" should be assumed.)

    typedef std::string service_protocol;

    namespace service_protocols
    {
        const service_protocol http{ "http" };
        const service_protocol https{ "https" };
    }

    // find and parse the 'api_proto' TXT record (or return the default)
    service_protocol parse_api_proto_record(const mdns::structured_txt_records& records);

    // "The DNS-SD advertisement MUST be accompanied by a TXT record of name 'api_ver'.
    // The value of this TXT record is a comma separated list of API versions supported by the server.
    // There should be no whitespace between commas, and versions should be listed in ascending order."
    // (However, v1.0 did not include this record, so when it is omitted, "v1.0" should be assumed.)

    namespace is04_versions
    {
        const std::vector<api_version> all{ nmos::is04_versions::v1_0, nmos::is04_versions::v1_1, nmos::is04_versions::v1_2 };
        const std::vector<api_version> unspecified{ nmos::is04_versions::v1_0 };
    }

    // find and parse the 'api_ver' TXT record (or return the default)
    std::vector<api_version> parse_api_ver_record(const mdns::structured_txt_records& records);

    // "The DNS-SD advertisement MUST include a TXT record with key 'pri' and an integer value.
    // Values 0 to 99 correspond to an active NMOS Registration API (zero being the highest priority).
    // Values 100+ are reserved for development work to avoid colliding with a live system."

    typedef int service_priority;

    namespace service_priorities
    {
        const service_priority highest_active_priority = 0;
        const service_priority lowest_active_priority = 99;

        const service_priority highest_development_priority = 100;
        const service_priority no_priority = (std::numeric_limits<int>::max)();
    }

    // find and parse the 'pri' TXT record
    service_priority parse_pri_record(const mdns::structured_txt_records& records);

    // make all three required TXT records from the specified values (or sensible default values)
    mdns::structured_txt_records make_txt_records(service_priority pri = service_priorities::highest_development_priority, const std::vector<api_version>& api_ver = is04_versions::all, const service_protocol& api_proto = service_protocols::http);

    namespace experimental
    {
        // helper function for registering the specified service (API)
        void register_service(mdns::service_advertiser& advertiser, const nmos::service_type& service, const nmos::settings& settings, const mdns::structured_txt_records& records = make_txt_records());

        // helper function for resolving instances of the specified service (API)
        // with the highest priority instances at the front, and (by default) services with the same priority ordered randomly
        std::multimap<service_priority, web::uri> resolve_service(mdns::service_discovery& discovery, const nmos::service_type& service, const std::vector<nmos::api_version>& api_ver = nmos::is04_versions::all, bool randomize = true);
    }
}

#endif

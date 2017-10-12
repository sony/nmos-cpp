#ifndef NMOS_MDNS_H
#define NMOS_MDNS_H

#include "mdns/core.h" // for mdns::structured_txt_records
#include "nmos/api_version.h"
#include "nmos/settings.h" // just a forward declaration of nmos::settings required for nmos::mdns::experimental functions

namespace mdns
{
    class service_advertiser;
    class service_discovery;
}

namespace nmos
{
    namespace mdns
    {
        // "APIs MUST produce an mDNS advertisement [...] accompanied by DNS TXT records"
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2-dev/APIs/RegistrationAPI.raml#L17 etc.

        typedef std::string service;

        namespace services
        {
            const service node{ "_nmos-node._tcp" };
            const service query{ "_nmos-query._tcp" };
            const service registration{ "_nmos-registration._tcp" };
        }
        
        // "The DNS-SD advertisement MUST be accompanied by a TXT record of name 'api_proto' with a value
        // of either 'http' or 'https' dependent on the protocol in use by the [...] server."
        // (However, v1.0 did not include this record, so when it is omitted, "http" should be assumed.)

        const std::string http_protocol{ "http" };
        const std::string https_protocol{ "https" };

        // find and parse the 'api_proto' TXT record (or return the default)
        std::string parse_api_proto_record(const ::mdns::structured_txt_records& records);

        // "The DNS-SD advertisement MUST be accompanied by a TXT record of name 'api_ver'.
        // The value of this TXT record is a comma separated list of API versions supported by the server.
        // There should be no whitespace between commas, and versions should be listed in ascending order."
        // (However, v1.0 did not include this record, so when it is omitted, "v1.0" should be assumed.)

        const std::vector<api_version> is04_versions{ nmos::is04_versions::v1_0, nmos::is04_versions::v1_1, nmos::is04_versions::v1_2 };
        const std::vector<api_version> is04_versions_unspecified{ nmos::is04_versions::v1_0 };

        // find and parse the 'api_ver' TXT record (or return the default)
        std::vector<api_version> parse_api_ver_record(const ::mdns::structured_txt_records& records);

        // "The DNS-SD advertisement MUST include a TXT record with key 'pri' and an integer value.
        // Values 0 to 99 correspond to an active NMOS Registration API (zero being the highest priority).
        // Values 100+ are reserved for development work to avoid colliding with a live system."

        const int highest_active_priority = 0;
        const int lowest_active_priority = 99;

        const int highest_development_priority = 100;
        const int no_priority = (std::numeric_limits<int>::max)();

        // find and parse the 'pri' TXT record
        int parse_pri_record(const ::mdns::structured_txt_records& records);

        // make all three required TXT records from the specified values (or sensible default values)
        ::mdns::structured_txt_records make_txt_records(int pri = highest_development_priority, const std::vector<api_version>& api_ver = is04_versions, const std::string& api_proto = http_protocol);

        namespace experimental
        {
            // helper function for registering the specified service (API)
            void register_service(::mdns::service_advertiser& advertiser, const nmos::mdns::service& service, const nmos::settings& settings, const ::mdns::structured_txt_records& records = make_txt_records());

            // helper function for resolving the highest priority instance of the specified service (API)
            web::uri resolve_service(::mdns::service_discovery& discovery, const nmos::mdns::service& service, const std::vector<nmos::api_version>& api_ver = nmos::mdns::is04_versions);
        }
    }
}

#endif

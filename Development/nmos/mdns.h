#ifndef NMOS_MDNS_H
#define NMOS_MDNS_H

#include <list>
#include "cpprest/base_uri.h"
#include "mdns/core.h" // for mdns::structured_txt_records
#include "nmos/is04_versions.h"
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
        // IS-04 Node API
        const service_type node{ "_nmos-node._tcp" };

        // IS-04 Query API
        const service_type query{ "_nmos-query._tcp" };

        // IS-04 Registration API
        // "RFC6763 Section 7.2 specifies that the maximum service name length for an mDNS advertisement
        // is 16 characters when including the leading underscore, but "_nmos-registration" is 18 characters."
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2.1/APIs/RegistrationAPI.raml#L19
        // This is to be addressed in v1.3, by specifying a shorter service type, "_nmos-register._tcp".
        // See https://github.com/AMWA-TV/nmos-discovery-registration/pull/71
        const service_type registration{ "_nmos-registration._tcp" };

        // IS-09 System API (originally specified in JT-NM TR-1001-1:2018 Annex A)
        const service_type system{ "_nmos-system._tcp" };

        // IS-10 Authorization API
        const service_type authorization{ "_nmos-auth._tcp" };
    }

    // "The DNS-SD advertisement MUST be accompanied by a TXT record of name 'api_proto' with a value
    // of either 'http' or 'https' dependent on the protocol in use by the [...] server."
    // (However, v1.0 did not include this record, so when it is omitted, "http" should be assumed.)

    typedef std::string service_protocol;

    namespace service_protocols
    {
        const service_protocol http{ "http" };
        const service_protocol https{ "https" };

        const std::set<service_protocol> all{ nmos::service_protocols::http, nmos::service_protocols::https };
    }

    // returns "http" or "https" depending on settings
    service_protocol get_service_protocol(const nmos::settings& settings);

    // find and parse the 'api_proto' TXT record (or return the default)
    service_protocol parse_api_proto_record(const mdns::structured_txt_records& records);

    // "The DNS-SD advertisement MUST be accompanied by a TXT record of name 'api_ver'.
    // The value of this TXT record is a comma separated list of API versions supported by the server.
    // There should be no whitespace between commas, and versions should be listed in ascending order."
    // (However, v1.0 did not include this record, so when it is omitted, "v1.0" should be assumed.)

    namespace is04_versions
    {
        const std::set<api_version> unspecified{ nmos::is04_versions::v1_0 };
    }

    // find and parse the 'api_ver' TXT record (or return the default)
    std::set<api_version> parse_api_ver_record(const mdns::structured_txt_records& records);

    // "The DNS-SD advertisement MUST be accompanied by a TXT record of name 'api_auth'
    // with a value of either 'true' or 'false' dependent on whether authorization is required
    // in order to interact with the API or not."
    // (This record is added in v1.3, so when it is omitted, "false" should be assumed.)

    // returns true or false depending on settings
    bool get_service_authorization(const nmos::settings& settings);

    // find and parse the 'api_auth' TXT record (or return the default)
    bool parse_api_auth_record(const mdns::structured_txt_records& records);

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

    // make the required TXT records from the specified values (or sensible default values)
    mdns::structured_txt_records make_txt_records(const nmos::service_type& service, service_priority pri = service_priorities::highest_development_priority, const std::set<api_version>& api_ver = is04_versions::all, const service_protocol& api_proto = service_protocols::http, bool api_auth = false);

    // "The value of each of the ['ver_' TXT records] should be an unsigned 8-bit integer initialised
    // to '0'. This integer MUST be incremented and mDNS TXT record updated whenever a change is made
    // to the corresponding HTTP API resource on the Node. The integer must wrap back to a value of
    // '0' after reaching a maximum value of '255' (MAX_UINT8_T)."
    typedef unsigned char api_resource_version;

    struct api_resource_versions
    {
        api_resource_version self = 0;
        api_resource_version devices = 0;
        api_resource_version sources = 0;
        api_resource_version flows = 0;
        api_resource_version senders = 0;
        api_resource_version receivers = 0;
    };

    // find and parse the Node 'ver_' TXT records
    api_resource_versions parse_ver_records(const mdns::structured_txt_records& records);

    // make the Node 'ver_' TXT records from the specified values
    mdns::structured_txt_records make_ver_records(const api_resource_versions& ver);

    namespace experimental
    {
        // helper function for registering addresses when the host name is explicitly configured
        void register_addresses(mdns::service_advertiser& advertiser, const nmos::settings& settings);

        // helper function for registering the specified service (API)
        void register_service(mdns::service_advertiser& advertiser, const nmos::service_type& service, const nmos::settings& settings);

        // helper function for updating the specified service (API) TXT records
        void update_service(mdns::service_advertiser& advertiser, const nmos::service_type& service, const nmos::settings& settings, mdns::structured_txt_records add_records = {});

        // helper function for resolving instances of the specified service (API)
        // with the highest version, highest priority instances at the front, and (by default) services with the same priority ordered randomly
        pplx::task<std::list<web::uri>> resolve_service(mdns::service_discovery& discovery, const nmos::service_type& service, const std::string& browse_domain, const std::set<nmos::api_version>& api_ver, const std::pair<nmos::service_priority, nmos::service_priority>& priorities, const std::set<nmos::service_protocol>& api_proto, const std::set<bool>& api_auth, bool randomize, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token = pplx::cancellation_token::none());

        template <typename Rep = std::chrono::seconds::rep, typename Period = std::chrono::seconds::period>
        inline pplx::task<std::list<web::uri>> resolve_service(mdns::service_discovery& discovery, const nmos::service_type& service, const std::string& browse_domain = {}, const std::set<nmos::api_version>& api_ver = nmos::is04_versions::all, const std::pair<nmos::service_priority, nmos::service_priority>& priorities = { service_priorities::highest_active_priority, service_priorities::no_priority }, const std::set<nmos::service_protocol>& api_proto = nmos::service_protocols::all, const std::set<bool>& api_auth = { false, true }, bool randomize = true, const std::chrono::duration<Rep, Period>& timeout = std::chrono::seconds(mdns::default_timeout_seconds), const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            return resolve_service(discovery, service, browse_domain, api_ver, api_proto, api_auth, randomize, std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout), token);
        }
    }
}

#endif

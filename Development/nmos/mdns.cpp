#include "nmos/mdns.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/basic_utils.h"
#include "cpprest/uri_builder.h"
#include "mdns/service_advertiser.h"
#include "mdns/service_discovery.h"

namespace nmos
{
    namespace mdns
    {
        // "APIs MUST produce an mDNS advertisement [...] accompanied by DNS TXT records"
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2-dev/APIs/RegistrationAPI.raml#L17 etc.

        // For now, the TXT record keys and the functions to make/parse the values are kept as implementation details

        namespace keys
        {
            const std::string api_proto{ "api_proto" };
            const std::string api_ver{ "api_ver" };
            const std::string pri{ "pri" };
        }

        inline std::string make_api_proto_value(const std::string& api_proto = http_protocol)
        {
            return api_proto;
        }

        inline std::string parse_api_proto_value(const std::string& api_proto)
        {
            return api_proto;
        }

        std::string parse_api_proto_record(const ::mdns::structured_txt_records& records)
        {
            return ::mdns::parse_txt_record(records, keys::api_proto, parse_api_proto_value, http_protocol);
        }

        inline std::string make_api_ver_value(const std::vector<api_version>& api_ver = is04_versions)
        {
            return boost::algorithm::join(api_ver | boost::adaptors::transformed(make_api_version) | boost::adaptors::transformed(utility::us2s), ",");
        }

        inline std::vector<api_version> parse_api_ver_value(const std::string& api_ver)
        {
            std::vector<std::string> api_vers;
            boost::algorithm::split(api_vers, api_ver, [](char c){ return ',' == c; });
            return boost::copy_range<std::vector<api_version>>(api_vers | boost::adaptors::transformed(utility::s2us) | boost::adaptors::transformed(parse_api_version));
        }

        std::vector<api_version> parse_api_ver_record(const ::mdns::structured_txt_records& records)
        {
            return ::mdns::parse_txt_record(records, keys::api_ver, parse_api_ver_value, is04_versions_unspecified);
        }

        inline std::string make_pri_value(int pri = highest_development_priority)
        {
            return utility::us2s(utility::ostringstreamed(pri));
        }

        inline int parse_pri_value(const std::string& pri)
        {
            return utility::istringstreamed<int>(utility::s2us(pri), no_priority);
        }

        int parse_pri_record(const ::mdns::structured_txt_records& records)
        {
            return ::mdns::parse_txt_record(records, keys::pri, parse_pri_value, no_priority);
        }

        ::mdns::structured_txt_records make_txt_records(int pri, const std::vector<api_version>& api_ver, const std::string& api_proto)
        {
            return
            {
                { keys::api_proto, make_api_proto_value(api_proto) },
                { keys::api_ver, make_api_ver_value(api_ver) },
                { keys::pri, make_pri_value(pri) }
            };
        }

        namespace experimental
        {
            inline int service_port(const nmos::mdns::service& service, const nmos::settings& settings)
            {
                if (nmos::mdns::services::node == service) return nmos::fields::node_port(settings);
                if (nmos::mdns::services::query == service) return nmos::fields::query_port(settings);
                if (nmos::mdns::services::registration == service) return nmos::fields::registration_port(settings);
                return 0;
            }

            inline std::string service_api(const nmos::mdns::service& service)
            {
                if (nmos::mdns::services::node == service) return "node";
                if (nmos::mdns::services::query == service) return "query";
                if (nmos::mdns::services::registration == service) return "registration";
                return{};
            }

            inline std::string service_base_name(const nmos::mdns::service& service)
            {
                return "nmos-cpp_" + service_api(service);
            }

            std::string service_name(const nmos::mdns::service& service, const nmos::settings& settings)
            {
                // this just serves as an example of a possible service naming strategy
                return service_base_name(service) + "_" + utility::us2s(nmos::fields::host_address(settings)) + ":" + utility::us2s(utility::ostringstreamed(service_port(service, settings)));
            }

            // helper function for registering the specified service (API)
            void register_service(::mdns::service_advertiser& advertiser, const nmos::mdns::service& service, const nmos::settings& settings, const ::mdns::structured_txt_records& records)
            {
                advertiser.register_service(service_name(service, settings), service, (uint16_t)service_port(service, settings), {}, ::mdns::make_txt_records(records));
            }

            // helper function for resolving the highest priority instance of the specified service (API)
            web::uri resolve_service(::mdns::service_discovery& discovery, const nmos::mdns::service& service, const std::vector<nmos::api_version>& api_ver)
            {
                std::vector<::mdns::service_discovery::browse_result> browsed;
                discovery.browse(browsed, service);

                // "Given multiple returned Registration APIs, the Node orders these based on their advertised priority (TXT pri),
                // filtering out any APIs which do not support its required API version and protocol (TXT api_ver and api_proto)."
                // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2-dev/docs/3.1.%20Discovery%20-%20Registered%20Operation.md#registration
                std::multimap<int, web::uri> by_priority;
                for (auto& resolving : browsed)
                {
                    ::mdns::service_discovery::resolve_result resolved;
                    if (discovery.resolve(resolved, resolving.name, resolving.type, resolving.domain, resolving.interface_id))
                    {
                        // parse into structured TXT records
                        auto records = ::mdns::parse_txt_records(resolved.txt_records);

                        // 'pri' must not be omitted
                        auto pri = nmos::mdns::parse_pri_record(records);
                        if (nmos::mdns::no_priority == pri) continue;

                        // for now, HTTP only
                        auto api_proto = nmos::mdns::parse_api_proto_record(records);
                        if (nmos::mdns::http_protocol != api_proto) continue;

                        // check the advertisement includes a version we support
                        auto api_vers = nmos::mdns::parse_api_ver_record(records);
                        auto api_ver = std::find_first_of(api_vers.rbegin(), api_vers.rend(), nmos::mdns::is04_versions.begin(), nmos::mdns::is04_versions.end());
                        if (api_vers.rend() == api_ver) continue;

                        by_priority.insert({pri, web::uri_builder()
                            .set_scheme(utility::s2us(api_proto))
                            .set_host(utility::s2us(resolved.ip_address))
                            .set_port(resolved.port)
                            .set_path(U("/x-nmos/") + utility::s2us(service_api(service)) + U("/") + make_api_version(*api_ver) + U("/"))
                            .to_uri()
                        });
                    }
                }

                // "The Node selects a Registration API to use based on the priority, and a random selection if multiple Registration APIs
                // with the same priority are identified."
                // For now, return the first discovered when multiple instances with the same priority are identified...
                return by_priority.empty() ? web::uri{} : by_priority.begin()->second;
            }
        }
    }
}

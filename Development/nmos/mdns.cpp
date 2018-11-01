#include "nmos/mdns.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/basic_utils.h"
#include "cpprest/uri_builder.h"
#include "mdns/service_advertiser.h"
#include "mdns/service_discovery.h"
#include "nmos/random.h"

namespace nmos
{
    // "APIs MUST produce an mDNS advertisement [...] accompanied by DNS TXT records"
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/RegistrationAPI.raml#L17
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/QueryAPI.raml#L122
    // and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/NodeAPI.raml#L37

    // For now, the TXT record keys and the functions to make/parse the values are kept as implementation details

    namespace txt_record_keys
    {
        const std::string api_proto{ "api_proto" };
        const std::string api_ver{ "api_ver" };
        const std::string pri{ "pri" };
        const std::string ver_slf{ "ver_slf" };
        const std::string ver_src{ "ver_src" };
        const std::string ver_flw{ "ver_flw" };
        const std::string ver_dvc{ "ver_dvc" };
        const std::string ver_snd{ "ver_snd" };
        const std::string ver_rcv{ "ver_rcv" };
    }

    namespace details
    {
        inline std::string make_api_proto_value(const service_protocol& api_proto = service_protocols::http)
        {
            return api_proto;
        }

        inline service_protocol parse_api_proto_value(const std::string& api_proto)
        {
            return api_proto;
        }
    }

    // find and parse the 'api_proto' TXT record (or return the default)
    service_protocol parse_api_proto_record(const mdns::structured_txt_records& records)
    {
        return mdns::parse_txt_record(records, txt_record_keys::api_proto, details::parse_api_proto_value, service_protocols::http);
    }

    namespace details
    {
        inline std::string make_api_ver_value(const std::vector<api_version>& api_ver = is04_versions::all)
        {
            return boost::algorithm::join(api_ver | boost::adaptors::transformed(make_api_version) | boost::adaptors::transformed(utility::us2s), ",");
        }

        inline std::vector<api_version> parse_api_ver_value(const std::string& api_ver)
        {
            std::vector<std::string> api_vers;
            boost::algorithm::split(api_vers, api_ver, [](char c){ return ',' == c; });
            return boost::copy_range<std::vector<api_version>>(api_vers | boost::adaptors::transformed(utility::s2us) | boost::adaptors::transformed(parse_api_version));
        }
    }

    // find and parse the 'api_ver' TXT record (or return the default)
    std::vector<api_version> parse_api_ver_record(const mdns::structured_txt_records& records)
    {
        return mdns::parse_txt_record(records, txt_record_keys::api_ver, details::parse_api_ver_value, is04_versions::unspecified);
    }

    namespace details
    {
        template <typename T>
        inline std::string ostringstreamed(const T& value)
        {
            std::ostringstream os; os << value; return os.str();
        }

        template <typename T>
        inline T istringstreamed(const std::string& value, const T& default_value = {})
        {
            T result{ default_value }; std::istringstream is(value); is >> result; return result;
        }

        inline std::string make_pri_value(service_priority pri = service_priorities::highest_development_priority)
        {
            return ostringstreamed(pri);
        }

        inline service_priority parse_pri_value(const std::string& pri)
        {
            return istringstreamed(pri, service_priorities::no_priority);
        }

        inline std::string make_ver_value(api_resource_version ver)
        {
            return ostringstreamed<int>(ver);
        }

        inline api_resource_version parse_ver_value(const std::string& ver)
        {
            return (api_resource_version)istringstreamed<int>(ver, 0);
        }
    }

    // find and parse the 'pri' TXT record
    service_priority parse_pri_record(const mdns::structured_txt_records& records)
    {
        return mdns::parse_txt_record(records, txt_record_keys::pri, details::parse_pri_value, service_priorities::no_priority);
    }

    // make the required TXT records from the specified values (or sensible default values)
    mdns::structured_txt_records make_txt_records(const nmos::service_type& service, service_priority pri, const std::vector<api_version>& api_ver, const service_protocol& api_proto)
    {
        if (service == nmos::service_types::node)
        {
            return
            {
                { txt_record_keys::api_proto, details::make_api_proto_value(api_proto) },
                { txt_record_keys::api_ver, details::make_api_ver_value(api_ver) }
            };
        }
        else
        {
            return
            {
                { txt_record_keys::api_proto, details::make_api_proto_value(api_proto) },
                { txt_record_keys::api_ver, details::make_api_ver_value(api_ver) },
                { txt_record_keys::pri, details::make_pri_value(pri) }
            };
        }
    }

    // find and parse the Node 'ver_' TXT records
    api_resource_versions parse_ver_records(const mdns::structured_txt_records& records)
    {
        const api_resource_version no_ver{ 0 };
        api_resource_versions ver;
        ver.self = mdns::parse_txt_record(records, txt_record_keys::ver_slf, details::parse_ver_value, no_ver);
        ver.sources = mdns::parse_txt_record(records, txt_record_keys::ver_src, details::parse_ver_value, no_ver);
        ver.flows = mdns::parse_txt_record(records, txt_record_keys::ver_flw, details::parse_ver_value, no_ver);
        ver.devices = mdns::parse_txt_record(records, txt_record_keys::ver_dvc, details::parse_ver_value, no_ver);
        ver.senders = mdns::parse_txt_record(records, txt_record_keys::ver_snd, details::parse_ver_value, no_ver);
        ver.receivers = mdns::parse_txt_record(records, txt_record_keys::ver_rcv, details::parse_ver_value, no_ver);
        return ver;
    }

    // make the Node 'ver_' TXT records from the specified values
    mdns::structured_txt_records make_ver_records(const api_resource_versions& ver)
    {
        return
        {
            { txt_record_keys::ver_slf, details::make_ver_value(ver.self) },
            { txt_record_keys::ver_src, details::make_ver_value(ver.sources) },
            { txt_record_keys::ver_flw, details::make_ver_value(ver.flows) },
            { txt_record_keys::ver_dvc, details::make_ver_value(ver.devices) },
            { txt_record_keys::ver_snd, details::make_ver_value(ver.senders) },
            { txt_record_keys::ver_rcv, details::make_ver_value(ver.receivers) }
        };
    }

    namespace experimental
    {
        namespace details
        {
            inline int service_port(const nmos::service_type& service, const nmos::settings& settings)
            {
                if (nmos::service_types::node == service) return nmos::fields::node_port(settings);
                if (nmos::service_types::query == service) return nmos::fields::query_port(settings);
                if (nmos::service_types::registration == service) return nmos::fields::registration_port(settings);
                return 0;
            }

            inline std::string service_api(const nmos::service_type& service)
            {
                if (nmos::service_types::node == service) return "node";
                if (nmos::service_types::query == service) return "query";
                if (nmos::service_types::registration == service) return "registration";
                return{};
            }

            inline std::string service_base_name(const nmos::service_type& service)
            {
                return "nmos-cpp_" + service_api(service);
            }
        }

        std::string service_name(const nmos::service_type& service, const nmos::settings& settings)
        {
            // this just serves as an example of a possible service naming strategy
            // replacing '.' with '-', since although '.' is legal in service names, some DNS-SD implementations just don't like it
            return boost::algorithm::replace_all_copy(details::service_base_name(service) + "_" + utility::us2s(nmos::fields::host_address(settings)) + ":" + utility::us2s(utility::ostringstreamed(details::service_port(service, settings))), ".", "-");
        }

        // helper function for registering the specified service (API)
        void register_service(mdns::service_advertiser& advertiser, const nmos::service_type& service, const nmos::settings& settings, const mdns::structured_txt_records& records)
        {
            // if a host_name has been explicitly specified, attempt to register it in the specified domain
            const auto host_name = utility::us2s(nmos::fields::host_name(settings));
            const auto domain = utility::us2s(nmos::fields::domain(settings));
            if (!host_name.empty())
            {
                const auto at_least_one_host_address = web::json::value_of({ web::json::value::string(nmos::fields::host_address(settings)) });
                const auto& host_addresses = settings.has_field(nmos::fields::host_addresses) ? nmos::fields::host_addresses(settings) : at_least_one_host_address.as_array();
                for (const auto& host_address : host_addresses)
                {
                    advertiser.register_address(host_name, utility::us2s(host_address.as_string()), domain);
                }
            }

            advertiser.register_service(service_name(service, settings), service, (uint16_t)details::service_port(service, settings), domain, host_name, mdns::make_txt_records(records));
        }

        // helper function for updating the specified service (API) TXT records
        void update_service(mdns::service_advertiser& advertiser, const nmos::service_type& service, const nmos::settings& settings, const mdns::structured_txt_records& records)
        {
            const auto domain = utility::us2s(nmos::fields::domain(settings));
            advertiser.update_record(service_name(service, settings), service, domain, mdns::make_txt_records(records));
        }

        // helper function for resolving instances of the specified service (API)
        // with the highest priority instances at the front, and (by default) services with the same priority ordered randomly
        std::multimap<service_priority, web::uri> resolve_service(mdns::service_discovery& discovery, const nmos::service_type& service, const std::string& browse_domain, const std::vector<nmos::api_version>& api_ver, bool randomize, const std::chrono::steady_clock::duration& timeout)
        {
            const auto absolute_timeout = std::chrono::steady_clock::now() + timeout;

            std::vector<mdns::service_discovery::browse_result> browsed;

            discovery.browse(browsed, service, browse_domain, 0, absolute_timeout - std::chrono::steady_clock::now());

            // "Given multiple returned Registration APIs, the Node orders these based on their advertised priority (TXT pri),
            // filtering out any APIs which do not support its required API version and protocol (TXT api_ver and api_proto)."
            // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/3.1.%20Discovery%20-%20Registered%20Operation.md#registration

            if (randomize)
            {
                // "The Node selects a Registration API to use based on the priority, and a random selection if multiple Registration APIs
                // with the same priority are identified."
                // Therefore shuffle the browse results before inserting any into the resulting priority queue...
                nmos::details::seed_generator seeder;
                std::shuffle(browsed.begin(), browsed.end(), std::default_random_engine(seeder));
            }

            std::multimap<service_priority, web::uri> by_priority;
            for (auto& resolving : browsed)
            {
                std::vector<mdns::service_discovery::resolve_result> resolved;

                if (discovery.resolve(resolved, resolving.name, resolving.type, resolving.domain, resolving.interface_id, absolute_timeout - std::chrono::steady_clock::now()))
                {
                    // note, since we specified the interface_id, we expect only one result...

                    // parse into structured TXT records
                    auto records = mdns::parse_txt_records(resolved.front().txt_records);

                    // 'pri' must not be omitted for Registration API and Query API (see nmos::make_txt_records)
                    auto resolved_pri = nmos::parse_pri_record(records);
                    if (service != nmos::service_types::node && nmos::service_priorities::no_priority == resolved_pri) continue;

                    // for now, HTTP only
                    auto resolved_proto = nmos::parse_api_proto_record(records);
                    if (nmos::service_protocols::http != resolved_proto) continue;

                    // check the advertisement includes a version we support
                    auto resolved_vers = nmos::parse_api_ver_record(records);
                    auto resolved_ver = std::find_first_of(resolved_vers.rbegin(), resolved_vers.rend(), api_ver.begin(), api_ver.end());
                    if (resolved_vers.rend() == resolved_ver) continue;

                    for (const auto& ip_address : resolved.front().ip_addresses)
                    {
                        by_priority.insert({ resolved_pri, web::uri_builder()
                            .set_scheme(utility::s2us(resolved_proto))
                            .set_host(utility::s2us(ip_address))
                            .set_port(resolved.front().port)
                            .set_path(U("/x-nmos/") + utility::s2us(details::service_api(service)) + U("/") + make_api_version(*resolved_ver))
                            .to_uri()
                        });
                    }
                }

                // even if absolute_timeout has now passed, continue to try to resolve all browse results
            }

            return by_priority;
        }
    }
}

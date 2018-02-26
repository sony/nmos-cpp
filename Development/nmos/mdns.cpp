#include "nmos/mdns.h"

#include <random>
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
    // "APIs MUST produce an mDNS advertisement [...] accompanied by DNS TXT records"
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/RegistrationAPI.raml#L17 etc.

    // For now, the TXT record keys and the functions to make/parse the values are kept as implementation details

    namespace txt_record_keys
    {
        const std::string api_proto{ "api_proto" };
        const std::string api_ver{ "api_ver" };
        const std::string pri{ "pri" };
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

    std::vector<api_version> parse_api_ver_record(const mdns::structured_txt_records& records)
    {
        return mdns::parse_txt_record(records, txt_record_keys::api_ver, details::parse_api_ver_value, is04_versions::unspecified);
    }

    namespace details
    {
        inline std::string make_pri_value(service_priority pri = service_priorities::highest_development_priority)
        {
            return utility::us2s(utility::ostringstreamed(pri));
        }

        inline service_priority parse_pri_value(const std::string& pri)
        {
            return utility::istringstreamed(utility::s2us(pri), service_priorities::no_priority);
        }
    }

    service_priority parse_pri_record(const mdns::structured_txt_records& records)
    {
        return mdns::parse_txt_record(records, txt_record_keys::pri, details::parse_pri_value, service_priorities::no_priority);
    }

    mdns::structured_txt_records make_txt_records(service_priority pri, const std::vector<api_version>& api_ver, const service_protocol& api_proto)
    {
        return
        {
            { txt_record_keys::api_proto, details::make_api_proto_value(api_proto) },
            { txt_record_keys::api_ver, details::make_api_ver_value(api_ver) },
            { txt_record_keys::pri, details::make_pri_value(pri) }
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
            return details::service_base_name(service) + "_" + utility::us2s(nmos::fields::host_address(settings)) + ":" + utility::us2s(utility::ostringstreamed(details::service_port(service, settings)));
        }

        std::string qualified_host_name(const std::string& host_name)
        {
            // if an explicit host_hame has been specified, and is only a single label, generate the appropriate multicast .local domain name
            return !host_name.empty() && std::string::npos == host_name.find('.') ? host_name + ".local" : host_name;
        }

        // helper function for registering the specified service (API)
        void register_service(mdns::service_advertiser& advertiser, const nmos::service_type& service, const nmos::settings& settings, const mdns::structured_txt_records& records)
        {
            // explicitly specify a host_name to enable running on a Mininet host
            advertiser.register_service(service_name(service, settings), service, (uint16_t)details::service_port(service, settings), {}, qualified_host_name(utility::us2s(nmos::fields::host_name(settings))), mdns::make_txt_records(records));
        }

        namespace details
        {
            // this isn't a proper SeedSequence because two instances of random_device aren't going to produce the same values
            // so the constructors, and size and param member functions are provided only to meet the syntactic requirements
            class seed_generator
            {
            public:
                seed_generator() {}
                template <typename InputIterator> seed_generator(InputIterator first, InputIterator last) {}
                template <typename T> seed_generator(const std::initializer_list<T>&) {}

                template <typename RandomAccessorIterator>
                void generate(const RandomAccessorIterator first, const RandomAccessorIterator last)
                {
                    std::uniform_int_distribution<std::uint32_t> uint32s;
                    std::generate(first, last, [&] { return uint32s(device); });
                }

                size_t size() const { return 0; }
                template <typename OutputIterator> void param(OutputIterator) {}

            private:
                std::random_device device;
            };
        }

        // helper function for resolving instances of the specified service (API)
        // with the highest priority instances at the front, and (by default) services with the same priority ordered randomly
        std::multimap<service_priority, web::uri> resolve_service(mdns::service_discovery& discovery, const nmos::service_type& service, const std::vector<nmos::api_version>& api_ver, bool randomize)
        {
            std::vector<mdns::service_discovery::browse_result> browsed;
            discovery.browse(browsed, service);

            // "Given multiple returned Registration APIs, the Node orders these based on their advertised priority (TXT pri),
            // filtering out any APIs which do not support its required API version and protocol (TXT api_ver and api_proto)."
            // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/3.1.%20Discovery%20-%20Registered%20Operation.md#registration

            if (randomize)
            {
                // "The Node selects a Registration API to use based on the priority, and a random selection if multiple Registration APIs
                // with the same priority are identified."
                // Therefore shuffle the browse results before inserting any into the resulting priority queue... 
                details::seed_generator seeder;
                std::shuffle(browsed.begin(), browsed.end(), std::default_random_engine(seeder));
            }

            std::multimap<service_priority, web::uri> by_priority;
            for (auto& resolving : browsed)
            {
                mdns::service_discovery::resolve_result resolved;
                if (discovery.resolve(resolved, resolving.name, resolving.type, resolving.domain, resolving.interface_id))
                {
                    // parse into structured TXT records
                    auto records = mdns::parse_txt_records(resolved.txt_records);

                    // 'pri' must not be omitted
                    auto pri = nmos::parse_pri_record(records);
                    if (nmos::service_priorities::no_priority == pri) continue;

                    // for now, HTTP only
                    auto api_proto = nmos::parse_api_proto_record(records);
                    if (nmos::service_protocols::http != api_proto) continue;

                    // check the advertisement includes a version we support
                    auto api_vers = nmos::parse_api_ver_record(records);
                    auto api_ver = std::find_first_of(api_vers.rbegin(), api_vers.rend(), nmos::is04_versions::all.begin(), nmos::is04_versions::all.end());
                    if (api_vers.rend() == api_ver) continue;

                    by_priority.insert({pri, web::uri_builder()
                        .set_scheme(utility::s2us(api_proto))
                        .set_host(utility::s2us(resolved.ip_address))
                        .set_port(resolved.port)
                        .set_path(U("/x-nmos/") + utility::s2us(details::service_api(service)) + U("/") + make_api_version(*api_ver) + U("/"))
                        .to_uri()
                    });
                }
            }

            return by_priority;
        }
    }
}

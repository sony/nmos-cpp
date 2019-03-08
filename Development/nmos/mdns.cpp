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
        inline std::string make_api_ver_value(const std::set<api_version>& api_ver = is04_versions::all)
        {
            return boost::algorithm::join(api_ver | boost::adaptors::transformed(make_api_version) | boost::adaptors::transformed(utility::us2s), ",");
        }

        inline std::set<api_version> parse_api_ver_value(const std::string& api_ver)
        {
            // "The value of this TXT record is a comma separated list of API versions supported by the server. For example: 'v1.0,v1.1,v2.0'.
            // There should be no whitespace between commas, and versions should be listed in ascending order."
            // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/RegistrationAPI.raml#L33
            std::vector<std::string> api_vers;
            boost::algorithm::split(api_vers, api_ver, [](char c){ return ',' == c; });
            // Since ascending order is recommended, not required, convert straight to an ordered set without checking that.
            return boost::copy_range<std::set<api_version>>(api_vers | boost::adaptors::transformed(utility::s2us) | boost::adaptors::transformed(parse_api_version));
        }
    }

    // find and parse the 'api_ver' TXT record (or return the default)
    std::set<api_version> parse_api_ver_record(const mdns::structured_txt_records& records)
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
    mdns::structured_txt_records make_txt_records(const nmos::service_type& service, service_priority pri, const std::set<api_version>& api_ver, const service_protocol& api_proto)
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

    namespace service_types
    {
        // see nmos::service_types::registration, nmos::experimental::register_service, etc.
        const service_type register_{ "_nmos-register._tcp" };
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
                if (nmos::service_types::register_ == service) return nmos::fields::registration_port(settings);
                if (nmos::service_types::system == service) return nmos::fields::system_port(settings);
                return 0;
            }

            inline std::string service_api(const nmos::service_type& service)
            {
                if (nmos::service_types::node == service) return "node";
                if (nmos::service_types::query == service) return "query";
                if (nmos::service_types::registration == service) return "registration";
                if (nmos::service_types::register_ == service) return "registration";
                if (nmos::service_types::system == service) return "system";
                return{};
            }

            inline std::string service_base_name(const nmos::service_type& service)
            {
                return "nmos-cpp_" + service_api(service);
            }

            inline std::set<nmos::api_version> service_versions(const nmos::service_type& service, const nmos::settings& settings)
            {
                // the System API is defined by TR-1001-1:2018
                if (nmos::service_types::system == service) return{ { 1, 0 } };
                // all the other APIs are defined by IS-04, and should advertise consistent versions
                return nmos::is04_versions::from_settings(settings);
            }
        }

        std::string service_name(const nmos::service_type& service, const nmos::settings& settings)
        {
            // this just serves as an example of a possible service naming strategy
            // replacing '.' with '-', since although '.' is legal in service names, some DNS-SD implementations just don't like it
            return boost::algorithm::replace_all_copy(details::service_base_name(service) + "_" + utility::us2s(nmos::get_host(settings)) + ":" + utility::us2s(utility::ostringstreamed(details::service_port(service, settings))), ".", "-");
        }

        // helper function for registering the specified service (API)
        void register_service(mdns::service_advertiser& advertiser, const nmos::service_type& service, const nmos::settings& settings)
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
                    advertiser.register_address(host_name, utility::us2s(host_address.as_string()), domain).wait();
                }
            }

            const auto instance_name = service_name(service, settings);
            const auto instance_port_or_disabled = details::service_port(service, settings);
            if (0 > instance_port_or_disabled) return;
            const auto instance_port = (uint16_t)instance_port_or_disabled;
            const auto api_ver = details::service_versions(service, settings);
            const auto records = nmos::make_txt_records(service, nmos::fields::pri(settings), api_ver);
            const auto txt_records = mdns::make_txt_records(records);

            // also advertise "_nmos-register._tcp" for v1.3
            if (nmos::service_types::registration == service)
            {
                if (*api_ver.begin() < nmos::is04_versions::v1_3)
                {
                    advertiser.register_service(instance_name, service, instance_port, domain, host_name, txt_records).wait();
                }
                advertiser.register_service(instance_name, nmos::service_types::register_, instance_port, domain, host_name, txt_records).wait();
            }
            else
            {
                advertiser.register_service(instance_name, service, instance_port, domain, host_name, txt_records).wait();
            }
        }

        // helper function for updating the specified service (API) TXT records
        void update_service(mdns::service_advertiser& advertiser, const nmos::service_type& service, const nmos::settings& settings, mdns::structured_txt_records add_records)
        {
            const auto domain = utility::us2s(nmos::fields::domain(settings));
            const auto instance_name = service_name(service, settings);
            auto records = nmos::make_txt_records(service, nmos::fields::pri(settings), nmos::is04_versions::from_settings(settings));
            records.insert(records.end(), std::make_move_iterator(add_records.begin()), std::make_move_iterator(add_records.end()));
            const auto txt_records = mdns::make_txt_records(records);

            advertiser.update_record(instance_name, service, domain, txt_records).wait();
        }

        namespace details
        {
            typedef std::pair<api_version, service_priority> api_ver_pri;
            typedef std::pair<api_ver_pri, web::uri> resolved_service;
            typedef std::vector<resolved_service> resolved_services;

            pplx::task<bool> resolve_service(std::shared_ptr<resolved_services> results, mdns::service_discovery& discovery, const nmos::service_type& service, const std::string& browse_domain, const std::set<nmos::api_version>& api_ver, const std::pair<nmos::service_priority, nmos::service_priority>& priorities, const std::chrono::steady_clock::time_point& timeout, const pplx::cancellation_token& token)
            {
                return discovery.browse([=, &discovery](const mdns::browse_result& resolving)
                {
                    const bool cancel = pplx::canceled == discovery.resolve([=](const mdns::resolve_result& resolved)
                    {
                        // "The Node [filters] out any APIs which do not support its required API version and protocol (TXT api_ver and api_proto)."
                        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/3.1.%20Discovery%20-%20Registered%20Operation.md#registration

                        // note, since we specified the interface_id, we expect only one result...

                        // parse into structured TXT records
                        auto records = mdns::parse_txt_records(resolved.txt_records);

                        // 'pri' must not be omitted for Registration API and Query API (see nmos::make_txt_records)
                        auto resolved_pri = nmos::parse_pri_record(records);
                        if (service != nmos::service_types::node)
                        {
                            // ignore results with unsuitable priorities (too high or too low) to avoid development and live systems colliding
                            // only services between priorities.first and priorities.second (inclusive) should be returned
                            if (resolved_pri < priorities.first) return true;
                            if (priorities.second < resolved_pri) return true;
                        }

                        // for now, HTTP only
                        auto resolved_proto = nmos::parse_api_proto_record(records);
                        if (nmos::service_protocols::http != resolved_proto) return true;

                        // check the advertisement includes a version we support
                        auto resolved_vers = nmos::parse_api_ver_record(records);
                        auto resolved_ver = std::find_first_of(resolved_vers.rbegin(), resolved_vers.rend(), api_ver.rbegin(), api_ver.rend());
                        if (resolved_vers.rend() == resolved_ver) return true;

                        for (const auto& ip_address : resolved.ip_addresses)
                        {
                            results->push_back({ { *resolved_ver, resolved_pri }, web::uri_builder()
                                .set_scheme(utility::s2us(resolved_proto))
                                .set_host(utility::s2us(ip_address))
                                .set_port(resolved.port)
                                .set_path(U("/x-nmos/") + utility::s2us(details::service_api(service)))
                                .to_uri()
                            });
                        }

                        return true;
                    }, resolving.name, resolving.type, resolving.domain, resolving.interface_id, timeout - std::chrono::steady_clock::now(), token).wait();
                    return cancel || !results->empty();
                }, service, browse_domain, 0, timeout - std::chrono::steady_clock::now(), token);
            }

            // "Given multiple returned Registration APIs, the Node orders these based on their advertised priority (TXT pri)"
            bool less_api_ver_pri(const api_ver_pri& lhs, const api_ver_pri& rhs)
            {
                // the higher version is preferred; for the same version, the 'higher' priority is preferred
                return lhs.first > rhs.first || (lhs.first == rhs.first && lhs.second < rhs.second);
            }
        }

        // helper function for resolving instances of the specified service (API)
        // with the highest version, highest priority instances at the front, and (by default) services with the same priority ordered randomly
        pplx::task<std::list<web::uri>> resolve_service(mdns::service_discovery& discovery, const nmos::service_type& service, const std::string& browse_domain, const std::set<nmos::api_version>& api_ver, const std::pair<nmos::service_priority, nmos::service_priority>& priorities, bool randomize, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token)
        {
            const auto absolute_timeout = std::chrono::steady_clock::now() + timeout;

            std::shared_ptr<details::resolved_services> results(new details::resolved_services);
            pplx::task<bool> resolve_task;

            // also browse for "_nmos-register._tcp" for v1.3
            if (nmos::service_types::registration == service)
            {
                // hmm, this technique is depressingly verbose
                if (*api_ver.begin() < nmos::is04_versions::v1_3)
                {
                    // pplx::cancellation_token_source::create_linked_source is rubbish
                    // it should take its argument by const& but doesn't
                    // and it doesn't ever deregister the callback it uses
                    // see https://github.com/Microsoft/cpprestsdk/issues/589
                    pplx::cancellation_token_source linked_source;
                    pplx::cancellation_token_registration linked_registration;
                    if (token.is_cancelable()) linked_registration = token.register_callback([linked_source] { linked_source.cancel(); });
                    auto linked_token = linked_source.get_token();

                    const std::vector<std::shared_ptr<details::resolved_services>> both_results{
                        std::shared_ptr<details::resolved_services>(new details::resolved_services),
                        std::shared_ptr<details::resolved_services>(new details::resolved_services)
                    };

                    const std::vector<pplx::task<bool>> both_tasks{
                        details::resolve_service(both_results[0], discovery, nmos::service_types::register_, browse_domain, api_ver, priorities, absolute_timeout, linked_token),
                        details::resolve_service(both_results[1], discovery, service, browse_domain, api_ver, priorities, absolute_timeout, linked_token)
                    };

                    // when either task is completed, cancel and wait for the other to be completed
                    // and then merge the two sets of results
                    resolve_task = pplx::when_any(both_tasks.begin(), both_tasks.end()).then([both_results, linked_source, both_tasks](std::pair<bool, size_t> first_result)
                    {
                        if (!both_results[first_result.second]->empty()) linked_source.cancel();

                        return both_tasks[0 == first_result.second ? 1 : 0];
                    }).then([results, both_results](pplx::task<bool> finally)
                    {
                        finally.wait();

                        results->insert(results->end(), std::make_move_iterator(both_results[0]->begin()), std::make_move_iterator(both_results[0]->end()));
                        results->insert(results->end(), std::make_move_iterator(both_results[1]->begin()), std::make_move_iterator(both_results[1]->end()));
                        return !results->empty();
                    }, token);

                    // pplx::cancellation_token_source::create_linked_source is rubbish
                    // see above
                    if (token.is_cancelable()) resolve_task.then([token, linked_registration](pplx::task<bool>)
                    {
                        token.deregister_callback(linked_registration);
                    });
                }
                else
                {
                    resolve_task = details::resolve_service(results, discovery, nmos::service_types::register_, browse_domain, api_ver, priorities, absolute_timeout, token);
                }
            }
            else
            {
                resolve_task = details::resolve_service(results, discovery, service, browse_domain, api_ver, priorities, absolute_timeout, token);
            }

            return resolve_task.then([results, randomize](bool)
            {
                // since each advertisement may be discovered via multiple interfaces and, in the case of the Registration API, via two service types
                // remove duplicate uris, after sorting to ensure the highest advertised priority is kept for each
                std::stable_sort(results->begin(), results->end(), [](const details::resolved_service& lhs, const details::resolved_service& rhs)
                {
                    return lhs.second < rhs.second || (lhs.second == rhs.second && details::less_api_ver_pri(lhs.first, rhs.first));
                });
                results->erase(std::unique(results->begin(), results->end(), [](const details::resolved_service& lhs, const details::resolved_service& rhs)
                {
                    return lhs.second == rhs.second;
                }), results->end());

                if (randomize)
                {
                    // "The Node selects a Registration API to use based on the priority, and a random selection if multiple Registration APIs
                    // with the same priority are identified."
                    // Therefore shuffle the results before inserting any into the resulting priority queue...
                    nmos::details::seed_generator seeder;
                    std::shuffle(results->begin(), results->end(), std::default_random_engine(seeder));
                }

                // "Given multiple returned Registration APIs, the Node orders these based on their advertised priority (TXT pri)"
                std::stable_sort(results->begin(), results->end(), [](const details::resolved_service& lhs, const details::resolved_service& rhs)
                {
                    return details::less_api_ver_pri(lhs.first, rhs.first);
                });

                // add the version to each uri
                return boost::copy_range<std::list<web::uri>>(*results | boost::adaptors::transformed([](const details::resolved_service& s)
                {
                    return web::uri_builder(s.second).append_path(U("/") + make_api_version(s.first.first)).to_uri();
                }));
            });
        }
    }
}

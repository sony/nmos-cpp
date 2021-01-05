#include "nmos/mdns_api.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "mdns/service_discovery.h"
#include "nmos/api_utils.h"
#include "nmos/mdns.h"
#include "nmos/mdns_versions.h"
#include "nmos/model.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_unmounted_mdns_api(nmos::base_model& model, slog::base_gate& gate);

        web::http::experimental::listener::api_router make_mdns_api(nmos::base_model& model, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router mdns_api;

            mdns_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-dns-sd/") }, req, res));
                return pplx::task_from_result(true);
            });

            mdns_api.support(U("/") + nmos::experimental::patterns::mdns_api.pattern + U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(mdns_versions::all), req, res));
                return pplx::task_from_result(true);
            });

            mdns_api.mount(U("/") + nmos::experimental::patterns::mdns_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_mdns_api(model, gate));

            return mdns_api;
        }

        web::json::value make_mdns_result(const nmos::api_version& version, const ::mdns::browse_result& browsed, const ::mdns::resolve_result& resolved)
        {
            using web::json::value;

            value result;

            result[U("name")] = value::string(utility::s2us(browsed.name));
            // from v1.1, the service domain is included in the response
            if (nmos::experimental::mdns_versions::v1_1 <= version)
            {
                result[U("domain")] = value::string(utility::s2us(browsed.domain));
            }
            result[U("host_target")] = value::string(utility::s2us(resolved.host_name));
            value& addresses = result[U("addresses")] = value::array();
            for (const auto& ip_address : resolved.ip_addresses)
            {
                web::json::push_back(addresses, value::string(utility::s2us(ip_address)));
            }
            result[U("port")] = resolved.port;

            value& txt = result[U("txt")];

            auto records = ::mdns::parse_txt_records(resolved.txt_records);
            for (const auto& record : records)
            {
                txt[utility::s2us(record.first)] = value::string(utility::s2us(record.second));
            }

            return result;
        }

        namespace fields
        {
            const web::json::field_as_string_or query_domain{ U("query.domain"), {} };
        }

        namespace details
        {
            unsigned int get_request_timeout(const web::http::http_headers& headers)
            {
                // hm, ought to use the "wait" preference instead really
                // see https://tools.ietf.org/html/rfc7240#section-4.3
                auto header = headers.find(U("Request-Timeout"));
                return headers.end() != header
                    ? utility::conversions::details::scan_string(header->second, mdns::default_timeout_seconds)
                    : mdns::default_timeout_seconds;
            }

            // from v1.1, the browse domain is specified in the path
            // in v1.0, the browse domain may be specified as a query parameter
            utility::string_t get_domain(const web::uri& request_uri, const web::http::experimental::listener::route_parameters& parameters)
            {
                const auto mdnsBrowseDomain = parameters.find(nmos::experimental::patterns::mdnsBrowseDomain.name);
                if (parameters.end() != mdnsBrowseDomain)
                {
                    return web::uri::decode(mdnsBrowseDomain->second);
                }
                else
                {
                    auto flat_query_params = web::json::value_from_query(request_uri.query());
                    nmos::details::decode_elements(flat_query_params);
                    return nmos::experimental::fields::query_domain(flat_query_params);
                }
            }
        }

        web::http::experimental::listener::api_router make_unmounted_mdns_domain_api(nmos::base_model& model, slog::base_gate& gate);

        web::http::experimental::listener::api_router make_unmounted_mdns_api(nmos::base_model& model, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router mdns_api;

            // check for supported API version
            mdns_api.support(U(".*"), nmos::details::make_api_version_handler(nmos::experimental::mdns_versions::all, gate));

            // from v1.1, the top level provides a list of available browse domains
            mdns_api.support(U("/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

                if (nmos::experimental::mdns_versions::v1_1 > version)
                {
                    return pplx::task_from_result(true);
                }

                // hm, should use DNSServiceEnumerateDomains here
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("local./") }, req, res));

                throw nmos::details::to_api_finally_handler{}; // in order to skip other route handlers and then send the response
            });

            // from v1.1, the browse domain is specified in the path (and the next level provides the list of available service types)
            mdns_api.mount(U("/") + nmos::experimental::patterns::mdnsBrowseDomain.pattern, make_unmounted_mdns_domain_api(model, gate));

            // in v1.0, the browse domain may be specified as a query parameter (and the top level provides the list of available service types)
            // from v1.1, this is deprecated
            mdns_api.mount({}, make_unmounted_mdns_domain_api(model, gate));

            return mdns_api;
        }

        web::http::experimental::listener::api_router make_unmounted_mdns_domain_api(nmos::base_model& model, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router mdns_api;

            mdns_api.support(U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                // hmmm, fragile; make shared, and capture into continuation below, in order to extend lifetime until after discovery
                std::shared_ptr<nmos::api_gate> gate(new nmos::api_gate(gate_, req, parameters));

                const std::chrono::seconds timeout(details::get_request_timeout(req.headers()));

                // get the browse domain from the path, query parameters or settings
                const auto query_domain = details::get_domain(req.request_uri(), parameters);
                const auto browse_domain = utility::us2s(!query_domain.empty() ? query_domain : with_read_lock(model.mutex, [&] { return nmos::get_domain(model.settings); }));

                std::shared_ptr<::mdns::service_discovery> discovery(new ::mdns::service_discovery(*gate));

                // note, only the list of available service types that are explicitly being advertised is returned by "_services._dns-sd._udp"
                // see https://tools.ietf.org/html/rfc6763#section-9
                return discovery->browse("_services._dns-sd._udp", browse_domain, 0, timeout).then([req, res, gate](std::vector<::mdns::browse_result> browsed) mutable
                {
                    auto results = boost::copy_range<std::set<utility::string_t>>(browsed | boost::adaptors::transformed([](const ::mdns::browse_result& br)
                    {
                        // results for this query seem to be e.g. name = "_nmos-query", type = "_tcp.local."
                        return utility::s2us(br.name + '.' + br.type.substr(0, br.type.find('.')) + '/');
                    }));
                    res.headers().add(U("X-Total-Count"), results.size());
                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body(std::move(results), req, res));
                    return true;
                });
            });

            mdns_api.support(U("/") + nmos::experimental::patterns::mdnsServiceType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                // hmmm, fragile; make shared, and capture into continuation below, in order to extend lifetime until after discovery
                std::shared_ptr<nmos::api_gate> gate(new nmos::api_gate(gate_, req, parameters));

                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
                // hmm, something to think about... the regex patterns are presumably being used on encoded paths?
                const std::string serviceType = utility::us2s(web::uri::decode(parameters.at(nmos::experimental::patterns::mdnsServiceType.name)));

                const std::chrono::seconds timeout(details::get_request_timeout(req.headers()));

                // get the browse domain from the path, query parameters or settings
                const auto query_domain = details::get_domain(req.request_uri(), parameters);
                const auto browse_domain = utility::us2s(!query_domain.empty() ? query_domain : with_read_lock(model.mutex, [&] { return nmos::get_domain(model.settings); }));

                std::shared_ptr<::mdns::service_discovery> discovery(new ::mdns::service_discovery(*gate));

                // hmm, how to add cancellation on shutdown to the browse and resolve operations?
                return discovery->browse(serviceType, browse_domain, 0, timeout).then([res, version, discovery, timeout, gate](std::vector<::mdns::browse_result> browsed) mutable
                {
                    if (!browsed.empty())
                    {
                        typedef std::pair<::mdns::browse_result, std::vector<::mdns::resolve_result>> mdns_result;
                        std::vector<pplx::task<mdns_result>> tasks;
                        for (const auto& browsed1 : browsed)
                        {
                            tasks.push_back(discovery->resolve(browsed1.name, browsed1.type, browsed1.domain, browsed1.interface_id, timeout)
                                .then([browsed1, gate](std::vector<::mdns::resolve_result> resolved)
                            {
                                return mdns_result(browsed1, resolved);
                            }));
                        }
                        return pplx::when_all(tasks.begin(), tasks.end()).then([res, version, tasks](pplx::task<std::vector<mdns_result>> finally) mutable
                        {
                            // to ensure an exception from one doesn't leave other tasks' exceptions unobserved
                            for (auto& task : tasks)
                            {
                                try { task.wait(); } catch (...) {}
                            }

                            // merge results that have the same host_target, port and txt records
                            // and only differ in the resolved addresses

                            std::map<value, std::set<std::string>> results;
                            for (auto& result : finally.get())
                            {
                                auto& browsed1 = result.first;
                                for (auto& resolved1 : result.second)
                                {
                                    auto ip_addresses = resolved1.ip_addresses;
                                    resolved1.ip_addresses.clear();
                                    auto instance = make_mdns_result(version, browsed1, resolved1);
                                    results[instance].insert(ip_addresses.begin(), ip_addresses.end());
                                }
                            }

                            set_reply(res, status_codes::OK,
                                web::json::serialize_array(results | boost::adaptors::transformed([](const std::map<value, std::set<std::string>>::value_type& result)
                                {
                                    auto instance = result.first;
                                    for (const auto& address : result.second)
                                    {
                                        web::json::push_back(instance[U("addresses")], web::json::value::string(utility::s2us(address)));
                                    }
                                    return instance;
                                })),
                                web::http::details::mime_types::application_json);
                            res.headers().add(U("X-Total-Count"), results.size());
                            return true;
                        });
                    }
                    else
                    {
                        return pplx::create_task([] {}).then([res]() mutable
                        {
                            set_reply(res, status_codes::OK, value::array());
                            res.headers().add(U("X-Total-Count"), 0);
                            return true;
                        });
                    }
                });
            });

            mdns_api.support(U("/") + nmos::experimental::patterns::mdnsServiceType.pattern + U("/") + nmos::experimental::patterns::mdnsServiceName.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                // hmmm, fragile; make shared, and capture into continuation below, in order to extend lifetime until after discovery
                std::shared_ptr<nmos::api_gate> gate(new nmos::api_gate(gate_, req, parameters));

                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));
                const std::string serviceType = utility::us2s(web::uri::decode(parameters.at(nmos::experimental::patterns::mdnsServiceType.name)));
                const std::string serviceName = utility::us2s(web::uri::decode(parameters.at(nmos::experimental::patterns::mdnsServiceName.name)));

                const std::chrono::seconds timeout(details::get_request_timeout(req.headers()));

                // get the browse domain from the path, query parameters or settings
                const auto query_domain = details::get_domain(req.request_uri(), parameters);
                const auto service_domain = utility::us2s(!query_domain.empty() ? query_domain : with_read_lock(model.mutex, [&] { return nmos::get_domain(model.settings); }));

                std::shared_ptr<::mdns::service_discovery> discovery(new ::mdns::service_discovery(*gate));

                // When browsing, we resolve using the browse results' domain and interface
                // so this can give different results...
                const ::mdns::browse_result browsed1(serviceName, serviceType, service_domain);
                return discovery->resolve(browsed1.name, browsed1.type, browsed1.domain, browsed1.interface_id, timeout)
                    .then([res, version, browsed1, gate](std::vector<::mdns::resolve_result> resolved) mutable
                {
                    // merge results that have the same host_target, port and txt records
                    // and only differ in the resolved addresses

                    std::map<value, std::set<std::string>> results;
                    for (auto& resolved1 : resolved)
                    {
                        auto ip_addresses = resolved1.ip_addresses;
                        resolved1.ip_addresses.clear();
                        auto instance = make_mdns_result(version, browsed1, resolved1);
                        results[instance].insert(ip_addresses.begin(), ip_addresses.end());
                    }

                    if (!results.empty())
                    {
                        // for now, pick one of the merged results, even though there conceivably may be one per interface
                        const auto& result = *results.begin();

                        auto instance = result.first;
                        for (const auto& address : result.second)
                        {
                            web::json::push_back(instance[U("addresses")], web::json::value::string(utility::s2us(address)));
                        }
                        set_reply(res, status_codes::OK, instance);
                    }
                    else
                    {
                        set_reply(res, status_codes::NotFound);
                    }

                    return true;
                });
            });

            return mdns_api;
        }
    }
}

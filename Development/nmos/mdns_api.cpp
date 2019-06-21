#include "nmos/mdns_api.h"

#include <boost/range/adaptor/transformed.hpp>
#include "mdns/service_discovery.h"
#include "nmos/api_utils.h"
#include "nmos/mdns.h"
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

            mdns_api.support(U("/x-dns-sd/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("v1.0/") }, req, res));
                return pplx::task_from_result(true);
            });

            mdns_api.mount(U("/x-dns-sd/v1.0"), make_unmounted_mdns_api(model, gate));

            return mdns_api;
        }

        web::json::value make_mdns_result(const std::string& name, const ::mdns::resolve_result& resolved)
        {
            using web::json::value;

            value result;

            result[U("name")] = value::string(utility::s2us(name));
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

        web::http::experimental::listener::api_router make_unmounted_mdns_api(nmos::base_model& model, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router mdns_api;

            mdns_api.support(U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                // hmmm, fragile; make shared, and capture into continuation below, in order to extend lifetime until after discovery
                std::shared_ptr<nmos::api_gate> gate(new nmos::api_gate(gate_, req, parameters));

                // get the browse domain from the query parameters or settings

                auto flat_query_params = web::json::value_from_query(req.request_uri().query());
                nmos::details::decode_elements(flat_query_params);

                const auto settings_domain = with_read_lock(model.mutex, [&] { return nmos::fields::domain(model.settings); });
                const auto browse_domain = utility::us2s(web::json::field_as_string_or{ { nmos::fields::domain }, settings_domain }(flat_query_params));

                std::shared_ptr<::mdns::service_discovery> discovery(new ::mdns::service_discovery(*gate));

                // note, only the list of available service types that are explicitly being advertised is returned by "_services._dns-sd._udp"
                // see https://tools.ietf.org/html/rfc6763#section-9
                return discovery->browse("_services._dns-sd._udp", browse_domain).then([req, res, gate](std::vector<::mdns::browse_result> browsed) mutable
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

                // hmm, something to think about... the regex patterns are presumably being used on encoded paths?
                const std::string serviceType = utility::us2s(web::uri::decode(parameters.at(nmos::experimental::patterns::mdnsServiceType.name)));

                // get the browse domain from the query parameters or settings

                auto flat_query_params = web::json::value_from_query(req.request_uri().query());
                nmos::details::decode_elements(flat_query_params);

                const auto settings_domain = with_read_lock(model.mutex, [&] { return nmos::fields::domain(model.settings); });
                const auto browse_domain = utility::us2s(web::json::field_as_string_or{ { nmos::fields::domain }, settings_domain }(flat_query_params));

                std::shared_ptr<::mdns::service_discovery> discovery(new ::mdns::service_discovery(*gate));

                // hmm, how to add cancellation on shutdown to the browse and resolve operations?
                return discovery->browse(serviceType, browse_domain).then([res, discovery, gate](std::vector<::mdns::browse_result> browsed) mutable
                {
                    if (!browsed.empty())
                    {
                        std::shared_ptr<std::map<std::string, value>> results(new std::map<std::string, value>());

                        // add a continuation for each discovered service to resolve it
                        // but, to simplify inserting these into the priority map, the continuations are sequential
                        // rather than parallelised using pplx::when_all
                        auto resolve_all = pplx::task_from_result();
                        for (const auto& resolving : browsed)
                        {
                            resolve_all = resolve_all.then([discovery, results, resolving, gate]
                            {
                                const auto& serviceName = resolving.name;

                                // check if we already have resolved this service
                                if (results->end() != results->find(serviceName)) return pplx::task_from_result();

                                // if not, resolve and add a result for it
                                return discovery->resolve(resolving.name, resolving.type, resolving.domain, resolving.interface_id)
                                    .then([results, serviceName, gate](std::vector<::mdns::resolve_result> resolved)
                                {
                                    if (!resolved.empty())
                                    {
                                        (*results)[serviceName] = make_mdns_result(serviceName, resolved.front());
                                    }
                                });
                            });
                        }
                        return resolve_all.then([res, results]() mutable
                        {
                            if (!results->empty())
                            {
                                set_reply(res, status_codes::OK,
                                    web::json::serialize(*results, [](const std::map<std::string, value>::value_type& kv) { return kv.second; }),
                                    web::http::details::mime_types::application_json);
                                res.headers().add(U("X-Total-Count"), results->size());
                            }
                            else
                            {
                                set_reply(res, status_codes::NotFound);
                            }
                            return true;
                        });
                    }
                    else
                    {
                        return pplx::create_task([] {}).then([res]() mutable
                        {
                            set_reply(res, status_codes::NotFound);
                            return true;
                        });
                    }
                });
            });

            mdns_api.support(U("/") + nmos::experimental::patterns::mdnsServiceType.pattern + U("/") + nmos::experimental::patterns::mdnsServiceName.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                // hmmm, fragile; make shared, and capture into continuation below, in order to extend lifetime until after discovery
                std::shared_ptr<nmos::api_gate> gate(new nmos::api_gate(gate_, req, parameters));

                const std::string serviceType = utility::us2s(web::uri::decode(parameters.at(nmos::experimental::patterns::mdnsServiceType.name)));
                const std::string serviceName = utility::us2s(web::uri::decode(parameters.at(nmos::experimental::patterns::mdnsServiceName.name)));

                // get the service domain from the query parameters or settings

                auto flat_query_params = web::json::value_from_query(req.request_uri().query());
                nmos::details::decode_elements(flat_query_params);

                const auto settings_domain = with_read_lock(model.mutex, [&] { return nmos::fields::domain(model.settings); });
                const auto service_domain = utility::us2s(web::json::field_as_string_or{ { nmos::fields::domain }, settings_domain }(flat_query_params));

                std::shared_ptr<::mdns::service_discovery> discovery(new ::mdns::service_discovery(*gate));

                // When browsing, we resolve using the browse results' domain and interface
                // so this can give different results...
                return discovery->resolve(serviceName, serviceType, service_domain.empty() ? "local." : service_domain)
                    .then([res, serviceName, gate](std::vector<::mdns::resolve_result> resolved) mutable
                {
                    if (!resolved.empty())
                    {
                        // for now, pick one result, even though there may be one per interface
                        value result = make_mdns_result(serviceName, resolved.front());

                        set_reply(res, status_codes::OK, result);
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

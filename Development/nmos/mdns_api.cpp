#include "nmos/mdns_api.h"

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

            mdns_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ value(U("x-dns-sd/")) }));
                return pplx::task_from_result(true);
            });

            mdns_api.support(U("/x-dns-sd/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ value(U("v1.0/")) }));
                return pplx::task_from_result(true);
            });

            mdns_api.mount(U("/x-dns-sd/v1.0"), make_unmounted_mdns_api(model, gate));

            nmos::add_api_finally_handler(mdns_api, gate);

            return mdns_api;
        }

        web::json::value make_mdns_result(const std::string& name, const ::mdns::service_discovery::resolve_result& resolved)
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

        web::http::experimental::listener::api_router make_unmounted_mdns_api(nmos::base_model& model, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router mdns_api;

            mdns_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                // the list of available service types cannot easily be enumerated so it might be better to respond with status_codes::NoContent
                // rather than return a misleading list?
                set_reply(res, status_codes::OK, value_of(
                {
                    value::string(utility::s2us(nmos::service_types::query) + U("/")),
                    value::string(utility::s2us(nmos::service_types::registration) + U("/")),
                    value::string(utility::s2us(nmos::service_types::node) + U("/"))
                }));
                return pplx::task_from_result(true);
            });

            mdns_api.support(U("/") + nmos::experimental::patterns::mdnsServiceType.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                return pplx::create_task([]{}).then([&, req, res, parameters]() mutable
                {
                    // hmm, something to think about... the regex patterns are presumably being used on encoded paths?
                    const std::string serviceType = utility::us2s(web::uri::decode(parameters.at(nmos::experimental::patterns::mdnsServiceType.name)));

                    // get the browse domain from the query parameters or settings

                    auto flat_query_params = web::json::value_from_query(req.request_uri().query());
                    nmos::details::decode_elements(flat_query_params);

                    const auto settings_domain = with_read_lock(model.mutex, [&] { return nmos::fields::domain(model.settings); });
                    const auto browse_domain = utility::us2s(web::json::field_as_string_or{ { nmos::fields::domain }, settings_domain }(flat_query_params));

                    std::unique_ptr<::mdns::service_discovery> discovery = ::mdns::make_discovery(gate);
                    std::vector<::mdns::service_discovery::browse_result> found;

                    if (discovery->browse(found, serviceType, browse_domain))
                    {
                        std::map<std::string, value> results;

                        for (const auto& f : found)
                        {
                            // check if we already have resolved this service
                            if (results.end() != results.find(f.name)) continue;

                            std::vector<::mdns::service_discovery::resolve_result> resolved;

                            if (discovery->resolve(resolved, f.name, f.type, f.domain, f.interface_id))
                            {
                                results[f.name] = make_mdns_result(f.name, resolved.front());
                            }
                        }

                        if (!results.empty())
                        {
                            set_reply(res, status_codes::OK,
                                web::json::serialize(results, [](const std::map<std::string, value>::value_type& kv) { return kv.second; }),
                                U("application/json"));
                            res.headers().add(U("X-Total-Count"), results.size());
                        }
                        else
                        {
                            set_reply(res, status_codes::NotFound);
                        }
                    }
                    else
                    {
                        set_reply(res, status_codes::NotFound);
                    }

                    return true;
                });
            });

            mdns_api.support(U("/") + nmos::experimental::patterns::mdnsServiceType.pattern + U("/") + nmos::experimental::patterns::mdnsServiceName.pattern + U("/?"), methods::GET, [&model, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                return pplx::create_task([]{}).then([&, req, res, parameters]() mutable
                {
                    const std::string serviceType = utility::us2s(web::uri::decode(parameters.at(nmos::experimental::patterns::mdnsServiceType.name)));
                    const std::string serviceName = utility::us2s(web::uri::decode(parameters.at(nmos::experimental::patterns::mdnsServiceName.name)));

                    // get the service domain from the query parameters or settings

                    auto flat_query_params = web::json::value_from_query(req.request_uri().query());
                    nmos::details::decode_elements(flat_query_params);

                    const auto settings_domain = with_read_lock(model.mutex, [&] { return nmos::fields::domain(model.settings); });
                    const auto service_domain = utility::us2s(web::json::field_as_string_or{ { nmos::fields::domain }, settings_domain }(flat_query_params));

                    std::unique_ptr<::mdns::service_discovery> discovery = ::mdns::make_discovery(gate);
                    std::vector<::mdns::service_discovery::resolve_result> resolved;

                    // When browsing, we resolve using the browse results' domain and interface
                    // so this can give different results...
                    if (discovery->resolve(resolved, serviceName, serviceType, service_domain.empty() ? "local." : service_domain))
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

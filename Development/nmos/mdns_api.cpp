#include "nmos/mdns_api.h"

#include "mdns/service_discovery.h"
#include "nmos/api_utils.h"
#include "nmos/mdns.h"

namespace nmos
{
    namespace experimental
    {
        web::json::value make_mdns_result(const std::string& name, const ::mdns::service_discovery::resolve_result& resolved)
        {
            using web::json::value;

            value result;

            result[U("name")] = value::string(utility::s2us(name));
            result[U("host_target")] = value::string(utility::s2us(resolved.host_name));
            result[U("address")] = !resolved.ip_address.empty() ? value::string(utility::s2us(resolved.ip_address)) : value::null();
            result[U("port")] = resolved.port;

            value& txt = result[U("txt")];

            auto records = ::mdns::parse_txt_records(resolved.txt_records);
            for (const auto& record : records)
            {
                txt[utility::s2us(record.first)] = value::string(utility::s2us(record.second));
            }

            return result;
        }

        web::http::experimental::listener::api_router make_mdns_api(slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router mdns_api;

            mdns_api.support(U("/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ JU("x-mdns/") }));
                return pplx::task_from_result(true);
            });

            mdns_api.support(U("/x-mdns/?"), methods::GET, [](http_request, http_response res, const string_t&, const route_parameters&)
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

            mdns_api.support(U("/x-mdns/") + nmos::experimental::patterns::mdnsServiceType.pattern + U("/?"), methods::GET, [&gate](http_request, http_response res, const string_t&, const route_parameters& parameters)
            {
                return pplx::create_task([]{}).then([&, res, parameters]() mutable
                {
                    const std::string serviceType = utility::us2s(parameters.at(nmos::experimental::patterns::mdnsServiceType.name));

                    std::unique_ptr<::mdns::service_discovery> discovery = ::mdns::make_discovery(gate);
                    std::vector<::mdns::service_discovery::browse_result> found;

                    if (discovery->browse(found, serviceType))
                    {
                        std::map<std::string, value> results;

                        for (const auto& f : found)
                        {
                            // check if we already have resolved this service
                            if (results.end() != results.find(f.name)) continue;

                            ::mdns::service_discovery::resolve_result resolved;

                            if (discovery->resolve(resolved, f.name, f.type, f.domain, f.interface_id))
                            {
                                results[f.name] = make_mdns_result(f.name, resolved);
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

            mdns_api.support(U("/x-mdns/") + nmos::experimental::patterns::mdnsServiceType.pattern + U("/") + nmos::experimental::patterns::mdnsServiceName.pattern + U("/?"), methods::GET, [&gate](http_request, http_response res, const string_t&, const route_parameters& parameters)
            {
                return pplx::create_task([]{}).then([&, res, parameters]() mutable
                {
                    const std::string serviceType = utility::us2s(parameters.at(nmos::experimental::patterns::mdnsServiceType.name));
                    const std::string serviceName = utility::us2s(parameters.at(nmos::experimental::patterns::mdnsServiceName.name));

                    std::unique_ptr<::mdns::service_discovery> discovery = ::mdns::make_discovery(gate);
                    ::mdns::service_discovery::resolve_result resolved;

                    // When browsing, we use the default domain and then use the browse results' domain (and interface)
                    // but here, I'm not sure what we should specify?
                    if (discovery->resolve(resolved, serviceName, serviceType, "local."))
                    {
                        value result = make_mdns_result(serviceName, resolved);

                        set_reply(res, status_codes::OK, result);
                    }
                    else
                    {
                        set_reply(res, status_codes::NotFound);
                    }

                    return true;
                });
            });

            nmos::add_api_finally_handler(mdns_api, gate);

            return mdns_api;
        }
    }
}

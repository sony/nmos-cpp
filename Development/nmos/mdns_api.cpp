#include "nmos/mdns_api.h"

#include "nmos/api_utils.h"
#include "mdns/service_discovery.h"

namespace nmos
{
    namespace experimental
    {
        web::json::value make_mdns_result(const std::string& name, const mdns::service_discovery::resolve_result& resolved)
        {
            using web::json::value;

            value result;

            result[U("name")] = value::string(utility::s2us(name));
            result[U("address")] = !resolved.ip_address.empty() ? value::string(utility::s2us(resolved.ip_address)) : value::null();
            result[U("port")] = resolved.port;

            value& txt = result[U("txt")];

            for (const auto& record : resolved.txt_records)
            {
                // "During lookups, TXT records that do not contain an unquoted "=" are
                // ignored.  TXT records that seem to contain a null attribute name,
                // that is, the TXT-DATA starts with the character "=", are also
                // ignored."
                // Ought to handle backslash (\) and backquote (`) escaping too...
                // See https://tools.ietf.org/html/rfc1464#section-2
                const auto eq = record.find_first_of('=');
                if (0 == eq || std::string::npos == eq) continue;

                const auto key = record.substr(0, eq);
                const auto value = record.substr(std::string::npos == eq ? eq : eq + 1);

                txt[utility::s2us(key)] = value::string(utility::s2us(value));
            }

            return result;
        }

        web::http::experimental::listener::api_router make_mdns_api(std::mutex& mutex, std::atomic<slog::severity>& logging_level, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router mdns_api;

            mdns_api.support(U("/?"), methods::GET, [](const http_request&, http_response& res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ JU("x-mdns/") }));
                return true;
            });

            mdns_api.support(U("/x-mdns/?"), methods::GET, [](const http_request&, http_response& res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, value_of({ JU("_nmos-query._tcp"), JU("_nmos-registration._tcp"), JU("_nmos-node._tcp") }));
                return true;
            });

            mdns_api.support(U("/x-mdns/") + nmos::experimental::patterns::mdnsServiceType.pattern + U("/?"), methods::GET, [&mutex, &gate](const http_request&, http_response& res, const string_t&, const route_parameters& parameters)
            {
                std::lock_guard<std::mutex> lock(mutex);

                const std::string serviceType = utility::us2s(parameters.at(nmos::experimental::patterns::mdnsServiceType.name));

                std::unique_ptr<mdns::service_discovery> browser = mdns::make_discovery(gate);
                std::vector<mdns::service_discovery::browse_result> found;

                if (browser->browse(found, serviceType))
                {
                    std::map<std::string, value> results;

                    for (const auto& f : found)
                    {
                        // check if we already have resolved this service
                        if (results.end() != results.find(f.name)) continue;

                        mdns::service_discovery::resolve_result resolved;

                        if (browser->resolve(resolved, f.name, f.type, f.domain, f.interface_id))
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

            mdns_api.support(U("/x-mdns/") + nmos::experimental::patterns::mdnsServiceType.pattern + U("/") + nmos::experimental::patterns::mdnsServiceName.pattern + U("/?"), methods::GET, [&mutex, &gate](const http_request&, http_response& res, const string_t&, const route_parameters& parameters)
            {
                std::lock_guard<std::mutex> lock(mutex);

                const std::string serviceType = utility::us2s(parameters.at(nmos::experimental::patterns::mdnsServiceType.name));
                const std::string serviceName = utility::us2s(parameters.at(nmos::experimental::patterns::mdnsServiceName.name));

                std::unique_ptr<mdns::service_discovery> browser = mdns::make_discovery(gate);
                mdns::service_discovery::resolve_result resolved;

                // When browsing, we use the default domain and then use the browse results' domain (and interface)
                // but here, I'm not sure what we should specify?
                if (browser->resolve(resolved, serviceName, serviceType, "local."))
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

            nmos::add_api_finally_handler(mdns_api, gate);

            return mdns_api;
        }
    }
}

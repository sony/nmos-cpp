#include "nmos/manifest_api.h"

#include <boost/range/adaptor/filtered.hpp>
#include "nmos/api_utils.h"
#include "nmos/connection_api.h"
#include "nmos/is05_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_unmounted_manifest_api(const nmos::node_model& model, slog::base_gate& gate);

        web::http::experimental::listener::api_router make_manifest_api(const nmos::node_model& model, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router manifest_api;

            manifest_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-manifest/") }, req, res));
                return pplx::task_from_result(true);
            });

            manifest_api.mount(U("/x-manifest"), make_unmounted_manifest_api(model, gate));

            return manifest_api;
        }

        web::http::experimental::listener::api_router make_unmounted_manifest_api(const nmos::node_model& model, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router manifest_api;

            manifest_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("senders/") }, req, res));
                return pplx::task_from_result(true);
            });

            manifest_api.support(U("/") + nmos::patterns::senderType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();
                auto& resources = model.node_resources;

                const string_t resourceType = parameters.at(nmos::patterns::senderType.name);

                const auto match = [&](const nmos::resource& resource)
                {
                    return resource.type == nmos::types::sender;
                };

                size_t count = 0;

                // experimental extension, to support human-readable HTML rendering of NMOS responses
                if (experimental::details::is_html_response_preferred(req, web::http::details::mime_types::application_json))
                {
                    set_reply(res, status_codes::OK,
                        web::json::serialize_array(resources
                            | boost::adaptors::filtered(match)
                            | boost::adaptors::transformed(
                                [&count, &req](const nmos::resource& resource) { ++count; return experimental::details::make_html_response_a_tag(resource.id + U("/"), req); }
                            )),
                        web::http::details::mime_types::application_json);
                }
                else
                {
                    set_reply(res, status_codes::OK,
                        web::json::serialize_array(resources
                            | boost::adaptors::filtered(match)
                            | boost::adaptors::transformed(
                                [&count](const nmos::resource& resource) { ++count; return value(resource.id + U("/")); }
                            )),
                        web::http::details::mime_types::application_json);
                }

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << count << " matching " << resourceType;

                return pplx::task_from_result(true);
            });

            manifest_api.support(U("/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();
                auto& resources = model.node_resources;

                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    std::set<utility::string_t> sub_routes{ U("manifest/") };

                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body(std::move(sub_routes), req, res));
                }
                else if (nmos::details::is_erased_resource(resources, id_type))
                {
                    set_error_reply(res, status_codes::NotFound, U("Not Found; ") + nmos::details::make_erased_resource_error());
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            manifest_api.support(U("/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/manifest/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);

                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };

                const auto accept = req.headers().find(web::http::header_names::accept);
                const auto accept_or_empty = req.headers().end() != accept ? accept->second : utility::string_t{};

                const auto version = with_read_lock(model.mutex, [&model] { return *nmos::is05_versions::from_settings(model.settings).rbegin(); });
                nmos::details::handle_connection_resource_transportfile(res, model, version, id_type, accept_or_empty, gate);

                return pplx::task_from_result(true);
            });

            return manifest_api;
        }
    }
}

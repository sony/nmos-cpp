#include "nmos/system_api.h"

#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/json_validator.h"
#include "nmos/api_utils.h"
#include "nmos/is09_versions.h"
#include "nmos/json_schema.h"
#include "nmos/log_manip.h"
#include "nmos/model.h"
#include "nmos/thread_utils.h"

namespace nmos
{
    inline web::http::experimental::listener::api_router make_unmounted_system_api(nmos::registry_model& model, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_system_api(nmos::registry_model& model, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router system_api;

        system_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
            return pplx::task_from_result(true);
        });

        system_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("system/") }, req, res));
            return pplx::task_from_result(true);
        });

        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is09_versions::from_settings(model.settings); });
        system_api.support(U("/x-nmos/") + nmos::patterns::system_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
            return pplx::task_from_result(true);
        });

        system_api.mount(U("/x-nmos/") + nmos::patterns::system_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_system_api(model, gate));

        return system_api;
    }

    inline web::http::experimental::listener::api_router make_unmounted_system_api(nmos::registry_model& model, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router system_api;

        // check for supported API version
        const std::set<api_version> versions{ { 1, 0 } };
        system_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        system_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("global/") }, req, res));
            return pplx::task_from_result(true);
        });

        system_api.support(U("/global/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();

            if (model.system_global_resource.has_data())
            {
                set_reply(res, status_codes::OK, model.system_global_resource.data);
            }
            else
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "System global configuration resource not configured!";
                set_reply(res, status_codes::InternalError); // rather than Not Found, since the System API doesn't allow a 404 response
            }

            return pplx::task_from_result(true);
        });

        const web::json::experimental::json_validator validator
        {
            nmos::experimental::load_json_schema,
            boost::copy_range<std::vector<web::uri>>(is09_versions::all | boost::adaptors::transformed(experimental::make_systemapi_global_schema_uri))
        };

        // experimental extension, to allow the global configuration resource to be replaced
        system_api.support(U("/global/?"), methods::PUT, [&model, validator, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            return details::extract_json(req, gate).then([&model, &validator, req, res, parameters, gate](value body) mutable
            {
                auto lock = model.write_lock();

                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

                // Validate JSON syntax according to the schema

                const bool allow_invalid_resources = nmos::experimental::fields::allow_invalid_resources(model.settings);
                if (!allow_invalid_resources)
                {
                    validator.validate(body, experimental::make_systemapi_global_schema_uri(version));
                }
                else
                {
                    try
                    {
                        validator.validate(body, experimental::make_systemapi_global_schema_uri(version));
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "JSON error: " << e.what();
                    }
                }

                const auto& data = body;

                model.system_global_resource = { version, types::global, data, true };

                // notify anyone who cares...
                model.notify();

                set_reply(res, status_codes::Created, data);

                return true;
            });
        });

        // experimental extension, to allow the global configuration resource to be modified
        system_api.support(U("/global/?"), methods::PATCH, [&model, validator, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            return details::extract_json(req, gate).then([&model, &validator, req, res, parameters, gate](value body) mutable
            {
                auto lock = model.write_lock();

                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

                if (model.system_global_resource.has_data()) // and version is the same?
                {
                    // Merge the updates

                    auto patched = model.system_global_resource.data;
                    web::json::merge_patch(patched, body, true);

                    // Validate JSON syntax according to the schema

                    const bool allow_invalid_resources = nmos::experimental::fields::allow_invalid_resources(model.settings);
                    if (!allow_invalid_resources)
                    {
                        validator.validate(patched, experimental::make_systemapi_global_schema_uri(version));
                    }
                    else
                    {
                        try
                        {
                            validator.validate(patched, experimental::make_systemapi_global_schema_uri(version));
                        }
                        catch (const web::json::json_exception& e)
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "JSON error: " << e.what();
                        }
                    }

                    if (model.system_global_resource.id == nmos::fields::id(patched))
                    {
                        model.system_global_resource.data = patched;

                        // notify anyone who cares...
                        model.notify();

                        set_reply(res, status_codes::OK, patched);
                    }
                    else
                    {
                        set_error_reply(res, status_codes::BadRequest, U("Bad Request; cannot modify resource id with a PATCH request"));
                    }
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "System global configuration resource not configured!";
                    set_reply(res, status_codes::InternalError); // rather than Not Found, since the System API doesn't allow a 404 response
                }

                return true;
            });
        });

        return system_api;
    }
}

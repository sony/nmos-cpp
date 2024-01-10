#include "nmos/configuration_api.h"

//#include "cpprest/json_validator.h"
#include "nmos/api_utils.h"
#include "nmos/is14_versions.h"
//#include "nmos/json_schema.h"
#include "nmos/log_manip.h"
#include "nmos/model.h"

namespace nmos
{
    inline web::http::experimental::listener::api_router make_unmounted_configuration_api(nmos::node_model& model, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_configuration_api(nmos::node_model& model, web::http::experimental::listener::route_handler validate_authorization, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router configuration_api;

        configuration_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
            return pplx::task_from_result(true);
        });

        configuration_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("configuration/") }, req, res));
            return pplx::task_from_result(true);
        });

        if (validate_authorization)
        {
            configuration_api.support(U("/x-nmos/") + nmos::patterns::configuration_api.pattern + U("/?"), validate_authorization);
            configuration_api.support(U("/x-nmos/") + nmos::patterns::configuration_api.pattern + U("/.*"), validate_authorization);
        }

        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is14_versions::from_settings(model.settings); });
        configuration_api.support(U("/x-nmos/") + nmos::patterns::configuration_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
            return pplx::task_from_result(true);
        });

        configuration_api.mount(U("/x-nmos/") + nmos::patterns::configuration_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_configuration_api(model, gate));

        return configuration_api;
    }

    namespace details
    {
        namespace fields
        {
            const web::json::field_as_string_or level{ U("level"), {} };
            const web::json::field_as_string_or index{ U("index"), {} };
            const web::json::field_as_string_or describe{ U("describe"), {} };
        }

        utility::string_t make_query_parameters(web::json::value flat_query_params)
        {
            // any non-string query parameters need serializing before encoding

            // all other string values need encoding
            nmos::details::encode_elements(flat_query_params);

            return web::json::query_from_value(flat_query_params);
        }

        web::json::value parse_query_parameters(const utility::string_t& query)
        {
            auto flat_query_params = web::json::value_from_query(query);

            // all other string values need decoding
            nmos::details::decode_elements(flat_query_params);

            // any non-string query parameters need parsing after decoding...
            if (flat_query_params.has_field(nmos::fields::nc::level))
            {
                flat_query_params[nmos::fields::nc::level] = web::json::value::parse(nmos::details::fields::level(flat_query_params));
            }
            if (flat_query_params.has_field(nmos::details::fields::index))
            {
                flat_query_params[nmos::details::fields::index] = web::json::value::parse(nmos::details::fields::index(flat_query_params));
            }
            if (flat_query_params.has_field(nmos::details::fields::describe))
            {
                flat_query_params[nmos::details::fields::describe] = web::json::value::parse(nmos::details::fields::describe(flat_query_params));
            }

            return flat_query_params;
        }
    }

    inline web::http::experimental::listener::api_router make_unmounted_configuration_api(nmos::node_model& model, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router configuration_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is14_versions::from_settings(model.settings); });
        configuration_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        configuration_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("root/") }, req, res));
            return pplx::task_from_result(true);
        });

        configuration_api.mount(U("/root/?"), methods::GET, [](http_request req, http_response res, const string_t& route_path, const route_parameters& parameters)
        {
            using web::json::value;
            using web::json::value_of;

            // extarct the role path
            //auto role_path = req.relative_uri().path();

            // extract and decode the query string
            const auto flat_query_params = details::parse_query_parameters(req.request_uri().query());

            value data;

            if (flat_query_params.has_integer_field(nmos::details::fields::level) && flat_query_params.has_integer_field(nmos::details::fields::index))
            {
                if (flat_query_params.has_boolean_field(nmos::details::fields::describe))
                {
                    // Get datatype descriptor
                    // {baseUrl}/{rolePath}?level={propertyLevel}&index={propertyIndex}&describe=true
                    // see https://specs.amwa.tv/is-device-configuration/branches/publish-CR/docs/API_requests.html#getting-the-datatype-descriptor-of-a-property
                    data = value_of({ U("Get datatype descriptor") });
                }
                else
                {
                    // Get a property
                    // {baseUrl}/{rolePath}?level={propertyLevel}&index={propertyIndex}
                    // see https://specs.amwa.tv/is-device-configuration/branches/publish-CR/docs/API_requests.html#getting-a-property
                    data = value_of({ U("Get a property") });
                }
            }
            else
            {
                if (flat_query_params.has_boolean_field(nmos::details::fields::describe))
                {
                    // Get class descriptor
                    // {baseUrl}/{rolePath}?describe=true
                    // see https://specs.amwa.tv/is-device-configuration/branches/publish-CR/docs/API_requests.html#getting-the-class-descriptor-of-an-object
                    data = value_of({ U("Get class descriptor") });
                }
                else
                {
                    // Get block members
                    // {baseUrl}/{rolePath}
                    // see https://specs.amwa.tv/is-device-configuration/branches/publish-CR/docs/API_requests.html#getting-the-members-of-a-block
                    data = value_of({ U("Get block members") });
                }
            }

            // parse role path

            set_reply(res, status_codes::OK, data);
            return pplx::task_from_result(true);
        });

        //const web::json::experimental::json_validator validator
        //{
        //    nmos::experimental::load_json_schema,
        //    boost::copy_range<std::vector<web::uri>>(is14_versions::all | boost::adaptors::transformed(experimental::make_systemapi_global_schema_uri))
        //};

        return configuration_api;
    }
}

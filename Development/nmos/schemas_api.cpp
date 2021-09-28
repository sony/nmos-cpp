#include "nmos/schemas_api.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/json_visit.h"
#include "nmos/api_utils.h"
#include "nmos/json_schema.h"
#include "nmos/log_manip.h"

namespace nmos
{
    namespace experimental
    {
        namespace patterns
        {
            const route_pattern schemas = make_route_pattern(U("api"), U("schemas"));
            const route_pattern schemasRepository = make_route_pattern(U("repository"), U("nmos-[^/]+"));
            const route_pattern schemasTag = make_route_pattern(U("tag"), U("v[0-9]+\\.[^/]+"));
            const route_pattern schemasRef = make_route_pattern(U("ref"), U("[^/]+"));
        }

        namespace details
        {
            struct json_schema_html_response_body_visitor : web::json::value_assigning_visitor
            {
                json_schema_html_response_body_visitor(web::json::value& value, const web::uri& base_uri)
                    : web::json::value_assigning_visitor(value)
                    , base_uri(base_uri)
                    , ref()
                {}

                using web::json::value_assigning_visitor::operator();
                void operator()(web::json::value value, web::json::string_tag)
                {
                    if (name)
                    {
                        push_field(value.as_string());

                        ref = U("$ref") == value.as_string();
                    }
                    else if (!ref)
                    {
                        assign(std::move(value));
                    }
                    else
                    {
                        const auto href = web::uri_builder(base_uri).append_path(value.as_string()).to_uri();
                        assign(nmos::experimental::details::make_html_response_a_tag(href, value));
                    }
                }

                void operator()(web::json::value value, web::json::object_tag)
                {
                    web::json::visit_object(*this, std::move(value));
                }
                void operator()(web::json::value value, web::json::array_tag)
                {
                    web::json::visit_array(*this, std::move(value));
                }

            protected:
                web::uri base_uri;

                bool ref;
            };

            // render JSON Schema "$ref" values as links
            web::json::value make_json_schema_html_response_body(const web::uri& base_uri, const web::json::value& data)
            {
                web::json::value result;
                json_schema_html_response_body_visitor visitor(result, base_uri);
                web::json::visit(visitor, data);
                return result;
            }
        }

        web::http::experimental::listener::api_router make_schemas_api(slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router schemas_api;

            schemas_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("schemas/") }, req, res));
                return pplx::task_from_result(true);
            });

            // all schema URIs from the NMOS repositories are of the form https://github.com/AMWA-TV/{repository}/raw/{tag}/APIs/schemas/{ref}
            // hmm, could use a route_pattern to get the fields rather than using web::uri::split_path and indices below?

            typedef std::map<web::uri, web::json::value> schemas_t;
            typedef schemas_t::value_type schema_t;
            const auto schemas = boost::copy_range<schemas_t>(nmos::details::make_schemas() | boost::adaptors::filtered([](const schema_t& schema)
            {
                return schema.first.has_same_authority(web::uri(U("https://github.com/")));
            }));
            const auto paths = boost::copy_range<std::vector<std::vector<utility::string_t>>>(schemas | boost::adaptors::transformed([](const schema_t& schema)
            {
                return web::uri::split_path(schema.first.path());
            }) | boost::adaptors::filtered([](const std::vector<utility::string_t>& components)
            {
                return 7 == components.size() && U("AMWA-TV") == components[0] && U("raw") == components[2] && U("APIs") == components[4] && U("schemas") == components[5];
            }));

            schemas_api.support(U("/schemas/?"), methods::GET, [paths](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                const auto repositories = boost::copy_range<std::set<utility::string_t>>(paths | boost::adaptors::transformed([](const std::vector<utility::string_t>& components)
                {
                    return components[1] + U("/");
                }));

                if (!repositories.empty())
                {
                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body(repositories, req, res));
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            schemas_api.support(U("/schemas/") + nmos::experimental::patterns::schemasRepository.pattern + U("/?"), methods::GET, [paths](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                const auto repository = parameters.at(nmos::experimental::patterns::schemasRepository.name);

                const auto tags = boost::copy_range<std::set<utility::string_t>>(paths | boost::adaptors::filtered([&](const std::vector<utility::string_t>& components)
                {
                    return repository == components[1];
                }) | boost::adaptors::transformed([](const std::vector<utility::string_t>& components)
                {
                    return components[3] + U("/");
                }));

                if (!tags.empty())
                {
                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body(tags, req, res));
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            schemas_api.support(U("/schemas/") + nmos::experimental::patterns::schemasRepository.pattern + U("/") + nmos::experimental::patterns::schemasTag.pattern + U("/?"), methods::GET, [paths](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                const auto repository = parameters.at(nmos::experimental::patterns::schemasRepository.name);
                const auto tag = parameters.at(nmos::experimental::patterns::schemasTag.name);

                const auto refs = boost::copy_range<std::set<utility::string_t>>(paths | boost::adaptors::filtered([&](const std::vector<utility::string_t>& components)
                {
                    return repository == components[1] && tag == components[3];
                }) | boost::adaptors::transformed([](const std::vector<utility::string_t>& components)
                {
                    return components[6];
                }));

                if (!refs.empty())
                {
                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body(refs, req, res));
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            schemas_api.support(U("/schemas/") + nmos::experimental::patterns::schemasRepository.pattern + U("/") + nmos::experimental::patterns::schemasTag.pattern + U("/") + nmos::experimental::patterns::schemasRef.pattern, methods::GET, [schemas](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                const auto repository = parameters.at(nmos::experimental::patterns::schemasRepository.name);
                const auto tag = parameters.at(nmos::experimental::patterns::schemasTag.name);
                const auto ref = parameters.at(nmos::experimental::patterns::schemasRef.name);

                const auto path = U("/AMWA-TV/") + repository + U("/raw/") + tag + U("/APIs/schemas/") + ref;
                const auto found = std::find_if(schemas.begin(), schemas.end(), [&](const schema_t& schema)
                {
                    return path == schema.first.path();
                });

                if (schemas.end() != found)
                {
                    res.headers().set_content_type(U("application/schema+json"));

                    // experimental extension, to support human-readable HTML rendering of NMOS responses
                    if (experimental::details::is_html_response_preferred(req, U("application/schema+json")))
                    {
                        const auto base_uri = web::uri_builder().set_path(U("/schemas/") + repository + U("/") + tag + U("/")).to_uri();
                        set_reply(res, status_codes::OK, details::make_json_schema_html_response_body(base_uri, found->second));
                    }
                    else
                    {
                        set_reply(res, status_codes::OK, found->second);
                    }
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            return schemas_api;
        }
    }
}

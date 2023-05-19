#include "nmos/annotation_api.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/json_validator.h"
#include "nmos/api_utils.h"
#include "nmos/is13_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    web::http::experimental::listener::api_router make_unmounted_annotation_api(nmos::model& model, nmos::annotation_patch_merger merge_patch, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_annotation_api(nmos::model& model, nmos::annotation_patch_merger merge_patch, slog::base_gate& gate)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router annotation_api;

        annotation_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
            return pplx::task_from_result(true);
        });

        annotation_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("annotation/") }, req, res));
            return pplx::task_from_result(true);
        });

        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is13_versions::from_settings(model.settings); });
        annotation_api.support(U("/x-nmos/") + nmos::patterns::annotation_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
            return pplx::task_from_result(true);
        });

        annotation_api.mount(U("/x-nmos/") + nmos::patterns::annotation_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_annotation_api(model, std::move(merge_patch), gate));

        return annotation_api;
    }

    web::json::value make_annotation_patch(const nmos::resource& resource)
    {
        using web::json::value_of;
        return value_of({
            { nmos::fields::label, resource.data.at(nmos::fields::label) },
            { nmos::fields::description, resource.data.at(nmos::fields::description) },
            { nmos::fields::tags, resource.data.at(nmos::fields::tags) }
        });
    }

    web::json::value make_annotation_response(const nmos::resource& resource)
    {
        using web::json::value_of;
        return value_of({
            { nmos::fields::id, resource.data.at(nmos::fields::id) },
            { nmos::fields::version, resource.data.at(nmos::fields::version) },
            { nmos::fields::label, resource.data.at(nmos::fields::label) },
            { nmos::fields::description, resource.data.at(nmos::fields::description) },
            { nmos::fields::tags, resource.data.at(nmos::fields::tags) }
        });
    }

    namespace details
    {
        bool is_read_only_tag(const utility::string_t& key)
        {
            return boost::algorithm::starts_with(key, U("urn:x-nmos:tag:asset:"))
                || boost::algorithm::starts_with(key, U("urn:x-nmos:tag:grouphint/"));
        }

        void merge_annotation_patch(web::json::value& value, const web::json::value& patch)
        {
            // reject changes to read-ony tags

            if (patch.has_object_field(nmos::fields::tags))
            {
                const auto& tags = nmos::fields::tags(patch);
                auto patch_readonly = std::find_if(tags.begin(), tags.end(), [](const std::pair<utility::string_t, web::json::value>& field)
                {
                    return is_read_only_tag(field.first);
                });
                if (tags.end() != patch_readonly) throw std::runtime_error("cannot patch read-only tag: " + utility::us2s(patch_readonly->first));
            }

            // save existing read-only tags

            auto readonly_tags = web::json::value_from_fields(nmos::fields::tags(value) | boost::adaptors::filtered([](const std::pair<utility::string_t, web::json::value>& field)
            {
                return is_read_only_tag(field.first);
            }));

            // apply patch

            web::json::merge_patch(value, patch, true);

            // apply defaults to properties that have been reset

            web::json::insert(value, std::make_pair(nmos::fields::label, U("")));
            web::json::insert(value, std::make_pair(nmos::fields::description, U("")));
            web::json::insert(value, std::make_pair(nmos::fields::tags, readonly_tags));
        }

        void assign_annotation_patch(web::json::value& value, web::json::value&& patch)
        {
            if (value.has_string_field(nmos::fields::label)) value[nmos::fields::label] = std::move(patch.at(nmos::fields::label));
            if (value.has_string_field(nmos::fields::description)) value[nmos::fields::description] = std::move(patch.at(nmos::fields::description));
            if (value.has_object_field(nmos::fields::tags)) value[nmos::fields::tags] = std::move(patch.at(nmos::fields::tags));
        }

        void handle_annotation_patch(nmos::resources& resources, const nmos::resource& resource, const web::json::value& patch, const nmos::annotation_patch_merger& merge_patch, slog::base_gate& gate)
        {
            auto merged = nmos::make_annotation_patch(resource);
            try
            {
                if (merge_patch)
                {
                    merge_patch(resource, merged, patch);
                }
                else
                {
                    nmos::merge_annotation_patch(resource, merged, patch);
                }
            }
            catch (const web::json::json_exception& e)
            {
                throw std::logic_error(e.what());
            }
            catch (const std::runtime_error& e)
            {
                throw std::logic_error(e.what());
            }
            modify_resource(resources, resource.id, [&merged](nmos::resource& resource)
            {
                resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());
                details::assign_annotation_patch(resource.data, std::move(merged));
            });
        }
    }

    web::http::experimental::listener::api_router make_unmounted_annotation_api(nmos::model& model, nmos::annotation_patch_merger merge_patch, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router annotation_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is13_versions::from_settings(model.settings); });
        annotation_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        annotation_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("node/") }, req, res));
            return pplx::task_from_result(true);
        });

        annotation_api.support(U("/node/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("self/"), U("devices/"), U("sources/"), U("flows/"), U("senders/"), U("receivers/") }, req, res));
            return pplx::task_from_result(true);
        });

        annotation_api.support(U("/node/self/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            auto resource = nmos::find_self_resource(resources);
            if (resources.end() != resource)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Returning self resource: " << resource->id;
                set_reply(res, status_codes::OK, nmos::make_annotation_response(*resource));
            }
            else
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Self resource not found!";
                set_reply(res, status_codes::InternalError); // rather than Not Found, since the Annotation API doesn't allow a 404 response
            }

            return pplx::task_from_result(true);
        });

        const web::json::experimental::json_validator validator
        {
            nmos::experimental::load_json_schema,
            boost::copy_range<std::vector<web::uri>>(versions | boost::adaptors::transformed(experimental::make_annotationapi_resource_core_patch_request_schema_uri))
        };

        annotation_api.support(U("/node/self/?"), methods::PATCH, [&model, validator, merge_patch, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);

            return details::extract_json(req, gate).then([&model, &validator, merge_patch, req, res, parameters, gate](value body) mutable
            {
                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

                validator.validate(body, experimental::make_annotationapi_resource_core_patch_request_schema_uri(version));

                auto lock = model.write_lock();
                auto& resources = model.node_resources;

                auto resource = nmos::find_self_resource(resources);
                if (resources.end() != resource)
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Patching self resource: " << resource->id;

                    details::handle_annotation_patch(resources, *resource, body, merge_patch, gate);

                    set_reply(res, status_codes::OK, nmos::make_annotation_response(*resource));

                    model.notify();
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Self resource not found!";
                    set_reply(res, status_codes::InternalError); // rather than Not Found, since the Annotation API doesn't allow a 404 response
                }

                return true;
            });
        });

        annotation_api.support(U("/node/") + nmos::patterns::subresourceType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const string_t resourceType = parameters.at(nmos::patterns::subresourceType.name);

            const auto match = [&](const nmos::resources::value_type& resource) { return resource.type == nmos::type_from_resourceType(resourceType); };

            size_t count = 0;

            set_reply(res, status_codes::OK,
                web::json::serialize_array(resources
                    | boost::adaptors::filtered(match)
                    | boost::adaptors::transformed(
                        [&count](const nmos::resources::value_type& resource) { ++count; return nmos::make_annotation_response(resource); }
                    )),
                web::http::details::mime_types::application_json);

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << count << " matching " << resourceType;

            return pplx::task_from_result(true);
        });

        annotation_api.support(U("/node/") + nmos::patterns::subresourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);
            auto lock = model.read_lock();
            auto& resources = model.node_resources;

            const string_t resourceType = parameters.at(nmos::patterns::subresourceType.name);
            const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

            const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
            auto resource = find_resource(resources, id_type);
            if (resources.end() != resource)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Returning " << id_type;
                set_reply(res, status_codes::OK, nmos::make_annotation_response(*resource));
            }
            else
            {
                set_reply(res, status_codes::NotFound);
            }

            return pplx::task_from_result(true);
        });

        annotation_api.support(U("/node/") + nmos::patterns::subresourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::PATCH, [&model, validator, merge_patch, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
        {
            nmos::api_gate gate(gate_, req, parameters);

            return details::extract_json(req, gate).then([&model, &validator, merge_patch, req, res, parameters, gate](value body) mutable
            {
                const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

                validator.validate(body, experimental::make_annotationapi_resource_core_patch_request_schema_uri(version));

                auto lock = model.write_lock();
                auto& resources = model.node_resources;

                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Patching " << id_type;

                    details::handle_annotation_patch(resources, *resource, body, merge_patch, gate);

                    set_reply(res, status_codes::OK, nmos::make_annotation_response(*resource));

                    model.notify();
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return true;
            });
        });

        return annotation_api;
    }
}

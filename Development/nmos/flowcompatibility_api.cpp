#include "nmos/flowcompatibility_api.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/containerstream.h"
#include "nmos/is11_versions.h"
#include "nmos/flowcompatibility_resources.h"
#include "nmos/model.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_unmounted_flowcompatibility_api(nmos::node_model& model, details::flowcompatibility_base_edid_put_handler base_edid_put_handler, details::flowcompatibility_base_edid_delete_handler base_edid_delete_handler, details::flowcompatibility_effective_edid_setter effective_edid_setter, slog::base_gate& gate);

        web::http::experimental::listener::api_router make_flowcompatibility_api(nmos::node_model& model, details::flowcompatibility_base_edid_put_handler base_edid_put_handler, details::flowcompatibility_base_edid_delete_handler base_edid_delete_handler, details::flowcompatibility_effective_edid_setter effective_edid_setter, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router flowcompatibility_api;

            flowcompatibility_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
                return pplx::task_from_result(true);
            });

            flowcompatibility_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("flowcompatibility/") }, req, res));
                return pplx::task_from_result(true);
            });

            const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is11_versions::from_settings(model.settings); });
            flowcompatibility_api.support(U("/x-nmos/") + nmos::patterns::flowcompatibility_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
                return pplx::task_from_result(true);
            });

            flowcompatibility_api.mount(U("/x-nmos/") + nmos::patterns::flowcompatibility_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_flowcompatibility_api(model, base_edid_put_handler, base_edid_delete_handler, effective_edid_setter, gate));

            return flowcompatibility_api;
        }

        void set_edid_endpoint_as_reply(web::http::http_response& res, const std::pair<nmos::id, nmos::type>& id_type, const web::json::value& edid_endpoint, nmos::api_gate& gate)
        {
            if (!edid_endpoint.is_null())
            {
                // The base edid endpoint may be an empty object which signalizes that Base EDID is currently absent but it's allowed to be set
                // The effective edid and edid endpoints should contain either edid_binary or edid_href
                auto& edid_binary = nmos::fields::edid_binary(edid_endpoint);
                auto& edid_href = nmos::fields::edid_href(edid_endpoint);

                if (!edid_binary.is_null())
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning EDID binary for " << id_type;

                    auto i_stream = concurrency::streams::bytestream::open_istream(edid_binary.as_string());
                    set_reply(res, web::http::status_codes::OK, i_stream);
                }
                else if (!edid_href.is_null())
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Redirecting to EDID file for " << id_type;

                    set_reply(res, web::http::status_codes::TemporaryRedirect);
                    res.headers().add(web::http::header_names::location, nmos::fields::edid_href(edid_endpoint));
                }
                else
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "EDID requested for " << id_type << " does not exist";

                    set_error_reply(res, web::http::status_codes::NoContent);
                }
            }
            else
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF) << "EDID requested for " << id_type << " does not exist";

                set_error_reply(res, web::http::status_codes::NoContent);
            }
        }

        // it's expected that read lock is already catched for the model
        bool all_resources_exist(nmos::resources& resources, const web::json::array& resource_ids, const nmos::type& type)
        {
            for (const auto& resource_id : resource_ids)
            {
                if (resources.end() == find_resource(resources, std::make_pair(resource_id.as_string(), type)))
                {
                    return false;
                }
            }
            return true;
        }

        // it's expected that read lock is already catched for the model
        void update_version(nmos::resources& resources, const web::json::array& resource_ids, const utility::string_t& new_version)
        {
            for (const auto& resource_id : resource_ids)
            {
                modify_resource(resources, resource_id.as_string(), [&new_version](nmos::resource& resource)
                {
                    resource.data[nmos::fields::version] = web::json::value::string(new_version);
                });
            }
        }

        // it's expected that write lock is already catched for the model and an input with the resource_id exists
        void update_effective_edid(nmos::node_model& model, const details::flowcompatibility_effective_edid_setter& effective_edid_setter, const utility::string_t resource_id)
        {
            boost::variant<utility::string_t, web::uri> effective_edid;
            bst::optional<web::json::value> effective_edid_properties = boost::none;

            effective_edid_setter(resource_id, effective_edid, effective_edid_properties);

            utility::string_t updated_timestamp;

            modify_resource(model.flowcompatibility_resources, resource_id, [&effective_edid, &effective_edid_properties, &updated_timestamp](nmos::resource& input)
            {
                input.data[nmos::fields::endpoint_effective_edid] = boost::apply_visitor(edid_file_visitor(), effective_edid);

                if (effective_edid_properties.has_value())
                {
                    input.data[nmos::fields::effective_edid_properties] = effective_edid_properties.value();
                }

                updated_timestamp = nmos::make_version();
                input.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
            });

            const std::pair<nmos::id, nmos::type> id_type{ resource_id, nmos::types::input };
            auto resource = find_resource(model.flowcompatibility_resources, id_type);

            update_version(model.node_resources, nmos::fields::senders(resource->data), updated_timestamp);
        }

        web::http::experimental::listener::api_router make_unmounted_flowcompatibility_api(nmos::node_model& model, details::flowcompatibility_base_edid_put_handler base_edid_put_handler, details::flowcompatibility_base_edid_delete_handler base_edid_delete_handler, details::flowcompatibility_effective_edid_setter effective_edid_setter, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router flowcompatibility_api;

            // check for supported API version
            const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is11_versions::from_settings(model.settings); });

            flowcompatibility_api.support(U(".*"), nmos::details::make_api_version_handler(versions, gate_));

            flowcompatibility_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("senders/"), U("receivers/"), U("inputs/"), U("outputs/") }, req, res));
                return pplx::task_from_result(true);
            });

            flowcompatibility_api.support(U("/") + nmos::patterns::flowCompatibilityResourceType.pattern + U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, web::json::value::array());
                return pplx::task_from_result(true);
            });

            flowcompatibility_api.support(U("/") + nmos::patterns::flowCompatibilityResourceType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::flowCompatibilityResourceType.name);

                const auto match = [&resourceType](const nmos::resources::value_type& resource) { return resource.type == nmos::type_from_resourceType(resourceType); };

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
                            )
                        ),
                        web::http::details::mime_types::application_json);
                }

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Returning " << count << " matching " << resourceType;

                return pplx::task_from_result(true);
            });

            flowcompatibility_api.support(U("/") + nmos::patterns::flowCompatibilityResourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::flowCompatibilityResourceType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    if (nmos::types::sender == resource->type ||
                        nmos::types::receiver == resource->type)
                    {
                        auto matching_resource = find_resource(model.node_resources, id_type);
                        if (model.node_resources.end() == matching_resource)
                        {
                            throw std::logic_error("matching IS-04 resource not found");
                        }
                    }

                    std::set<utility::string_t> sub_routes;
                    if (nmos::types::sender == resource->type)
                    {
                        sub_routes = { U("inputs/"), U("status/"), U("constraints/") };
                    }
                    else if (nmos::types::receiver == resource->type)
                    {
                        sub_routes = { U("outputs/"), U("status/") };
                    }
                    else
                    {
                        sub_routes = { U("edid/"), U("properties/") };
                    }

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

            flowcompatibility_api.support(U("/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/") + nmos::patterns::senderReceiverSubrouteType.pattern + U("/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);
                const string_t associatedResourceType = parameters.at(nmos::patterns::senderReceiverSubrouteType.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    auto matching_resource = find_resource(model.node_resources, id_type);
                    if (model.node_resources.end() == matching_resource)
                    {
                        throw std::logic_error("matching IS-04 resource not found");
                    }

                    const auto match = [&](const nmos::resources::value_type& resource)
                    {
                        return resource.type == nmos::type_from_resourceType(resourceType) && nmos::fields::id(resource.data) == resourceId;
                    };

                    const auto filter = (nmos::types::input == nmos::type_from_resourceType(associatedResourceType)) ?
                        nmos::fields::inputs :
                        nmos::fields::outputs;

                    set_reply(res, status_codes::OK,
                        web::json::serialize_array(resources
                            | boost::adaptors::filtered(match)
                            | boost::adaptors::transformed(
                                [&filter](const nmos::resource& resource) { return value_from_elements(filter(resource.data)); }
                            )
                        ),
                        web::http::details::mime_types::application_json);
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

            flowcompatibility_api.support(U("/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/status/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    auto matching_resource = find_resource(model.node_resources, id_type);
                    if (model.node_resources.end() == matching_resource)
                    {
                        throw std::logic_error("matching IS-04 resource not found");
                    }

                    set_reply(res, status_codes::OK, nmos::fields::status(resource->data));
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

            flowcompatibility_api.support(U("/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    auto matching_resource = find_resource(model.node_resources, id_type);
                    if (model.node_resources.end() == matching_resource)
                    {
                        throw std::logic_error("matching IS-04 resource not found");
                    }

                    std::set<utility::string_t> sub_routes{ U("active/"), U("supported/") };

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

            flowcompatibility_api.support(U("/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/") + nmos::patterns::constraintsType.pattern + U("/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t constraintsType = parameters.at(nmos::patterns::constraintsType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    auto matching_resource = find_resource(model.node_resources, id_type);
                    if (model.node_resources.end() == matching_resource)
                    {
                        throw std::logic_error("matching IS-04 resource not found");
                    }

                    if ("active" == constraintsType) {
                        set_reply(res, status_codes::OK, nmos::fields::active_constraint_sets(resource->data));
                    }
                    else if ("supported" == constraintsType) {
                        set_reply(res, status_codes::OK, nmos::fields::supported_param_constraints(resource->data));
                    }
                    else {
                        set_reply(res, status_codes::NotFound);
                    }
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

            flowcompatibility_api.support(U("/") + nmos::patterns::inputOutputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/properties/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::inputOutputType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    auto data = resource->data;

                    if (nmos::types::input == nmos::type_from_resourceType(resourceType))
                    {
                        if (!nmos::fields::endpoint_base_edid(resource->data).is_null())
                        {
                            data.erase(nmos::fields::endpoint_base_edid);
                        }
                        if (!nmos::fields::endpoint_effective_edid(resource->data).is_null())
                        {
                            data.erase(nmos::fields::endpoint_effective_edid);
                        }
                    }
                    else if (nmos::types::output == nmos::type_from_resourceType(resourceType))
                    {
                        if (!nmos::fields::endpoint_edid(resource->data).is_null())
                        {
                            data.erase(nmos::fields::endpoint_edid);
                        }
                    }

                    set_reply(res, status_codes::OK, data);
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

            flowcompatibility_api.support(U("/") + nmos::patterns::inputOutputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/edid/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::inputOutputType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    if (nmos::types::input == nmos::type_from_resourceType(resourceType)) {
                        std::set<utility::string_t> sub_routes{ U("base/"), U("effective/") };

                        set_reply(res, status_codes::OK, nmos::make_sub_routes_body(std::move(sub_routes), req, res));
                    }
                    else if (nmos::types::output == nmos::type_from_resourceType(resourceType)) {
                        auto& edid_endpoint = nmos::fields::endpoint_edid(resource->data);

                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "EDID requested for " << id_type;

                        set_edid_endpoint_as_reply(res, id_type, edid_endpoint, gate);
                    }
                    else {
                        set_reply(res, status_codes::NotImplemented);
                    }
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

            flowcompatibility_api.support(U("/") + nmos::patterns::inputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/edid/") + nmos::patterns::edidType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);
                const string_t edidType = parameters.at(nmos::patterns::edidType.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::input };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    const auto filter = ("base" == edidType) ?
                        nmos::fields::endpoint_base_edid :
                        nmos::fields::endpoint_effective_edid;

                    auto& edid_endpoint = filter(resource->data);

                    slog::log<slog::severities::info>(gate, SLOG_FLF) << edidType << " EDID requested for " << id_type;

                    set_edid_endpoint_as_reply(res, id_type, edid_endpoint, gate);
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

            flowcompatibility_api.support(U("/") + nmos::patterns::inputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/edid/base/?"), methods::PUT, [&model, base_edid_put_handler, effective_edid_setter, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.write_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::input };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    auto& endpoint_base_edid = nmos::fields::endpoint_base_edid(resource->data);

                    if (!endpoint_base_edid.is_null())
                    {
                        if (!nmos::fields::temporarily_locked(endpoint_base_edid))
                        {
                            bst::optional<web::json::value> base_edid_properties = boost::none;

                            const auto request_body = req.content_ready().get().extract_vector().get();
                            const utility::string_t base_edid{ request_body.begin(), request_body.end() };

                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Base EDID update is requested for " << id_type << " with file size " << base_edid.size();

                            if (base_edid_put_handler)
                            {
                                base_edid_put_handler(resourceId, base_edid, base_edid_properties);
                            }

                            // Check senders exist before modifying the input and therefore versions of associated senders
                            if (!all_resources_exist(model.node_resources, nmos::fields::senders(resource->data), nmos::types::sender))
                            {
                                throw std::logic_error("associated IS-04 sender not found");
                            }

                            utility::string_t updated_timestamp;

                            // Update Base EDID in flowcompatibility_resources
                            modify_resource(resources, resourceId, [&base_edid, &base_edid_properties, &updated_timestamp](nmos::resource& input)
                            {
                                if (base_edid_properties.has_value())
                                {
                                    input.data[nmos::fields::base_edid_properties] = base_edid_properties.value();
                                }

                                input.data[nmos::fields::endpoint_base_edid] = make_flowcompatibility_edid_endpoint(base_edid);

                                updated_timestamp = nmos::make_version();
                                input.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                            });

                            update_version(model.node_resources, nmos::fields::senders(resource->data), updated_timestamp);

                            if (effective_edid_setter)
                            {
                                update_effective_edid(model, effective_edid_setter, resourceId);
                            }

                            model.notify();

                            set_reply(res, status_codes::NoContent);
                        }
                        else
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Base EDID update is requested for " << id_type << " but this input is locked";

                            set_error_reply(res, status_codes::Locked);
                        }
                    }
                    else
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Base EDID update is requested for " << id_type << " but this input is not configured to allow it";

                        set_error_reply(res, status_codes::MethodNotAllowed);
                    }
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

            flowcompatibility_api.support(U("/") + nmos::patterns::inputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/edid/base/?"), methods::DEL, [&model, base_edid_delete_handler, effective_edid_setter, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.write_lock();
                auto& resources = model.flowcompatibility_resources;

                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::input };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    auto& endpoint_base_edid = nmos::fields::endpoint_base_edid(resource->data);

                    if (!endpoint_base_edid.is_null())
                    {
                        if (!nmos::fields::temporarily_locked(endpoint_base_edid))
                        {
                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Base EDID deletion is requested for " << id_type;

                            if (base_edid_delete_handler)
                            {
                                base_edid_delete_handler(resourceId);
                            }

                            // Check senders exist before modifying the input and therefore versions of associated senders
                            if (!all_resources_exist(model.node_resources, nmos::fields::senders(resource->data), nmos::types::sender))
                            {
                                throw std::logic_error("associated IS-04 sender not found");
                            }

                            utility::string_t updated_timestamp;

                            modify_resource(resources, resourceId, [&effective_edid_setter, &updated_timestamp](nmos::resource& input)
                            {
                                input.data[nmos::fields::endpoint_base_edid] = make_flowcompatibility_dummy_edid_endpoint();

                                updated_timestamp = nmos::make_version();
                                input.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                            });

                            update_version(model.node_resources, nmos::fields::senders(resource->data), updated_timestamp);

                            if (effective_edid_setter)
                            {
                                update_effective_edid(model, effective_edid_setter, resourceId);
                            }

                            model.notify();

                            set_reply(res, status_codes::NoContent);
                        }
                        else
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Base EDID deletion is requested for " << id_type << " but this input is locked";

                            set_error_reply(res, status_codes::Locked);
                        }
                    }
                    else
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Base EDID deletion is requested for " << id_type << " but this input is not configured to allow it";

                        set_error_reply(res, status_codes::MethodNotAllowed);
                    }
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

            return flowcompatibility_api;
        }
    }
}

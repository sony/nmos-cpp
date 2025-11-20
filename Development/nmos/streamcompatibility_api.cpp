#include "nmos/streamcompatibility_api.h"
#include "nmos/streamcompatibility_utils.h"

#include <unordered_set>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "cpprest/containerstream.h"
#include "cpprest/json_validator.h"
#include "nmos/api_utils.h" // for set_error_reply
#include "nmos/capabilities.h" // for nmos::fields::constraint_sets
#include "nmos/streamcompatibility_resources.h"
#include "nmos/is11_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
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
                        // Convert wchar_t to uint8_t if utility::string_t consists of wide chars
                        auto edid_string = edid_binary.as_string();
                        std::vector<uint8_t> edid_vector;
                        std::transform(edid_string.begin(), edid_string.end(), std::back_inserter(edid_vector), [](utility::char_t char_element) {
                            return static_cast<uint8_t>(char_element);
                        });
                        auto i_stream = concurrency::streams::bytestream::open_istream(edid_vector);
                        set_reply(res, web::http::status_codes::OK, i_stream);
                    }
                    else if (!edid_href.is_null())
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Redirecting to EDID file for " << id_type;

                        set_reply(res, web::http::status_codes::TemporaryRedirect);
                        res.headers().add(web::http::header_names::location, nmos::fields::edid_href(edid_endpoint).as_string());
                    }
                    else
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "EDID requested for " << id_type << " does not exist";

                        set_reply(res, web::http::status_codes::NoContent);
                    }
                }
                else
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "EDID requested for " << id_type << " does not exist";

                    set_reply(res, web::http::status_codes::NoContent);
                }
            }

            // it's expected that model read lock is already obtained
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

            bool validate_constraint_sets(const web::json::array& constraint_sets, const std::unordered_set<utility::string_t>& supported_param_constraints)
            {
                for (const auto& constraint_set : constraint_sets)
                {
                    const auto& param_constraints = constraint_set.as_object();
                    if (param_constraints.end() != std::find_if(param_constraints.begin(), param_constraints.end(), [&supported_param_constraints](const std::pair<utility::string_t, web::json::value>& constraint)
                    {
                        return supported_param_constraints.count(constraint.first) == 0;
                    })) {
                        return false;
                    }
                }

                return true;
            }

            // it's expected that model write lock is already obtained and an input with the input_id exists
            void update_effective_edid(nmos::node_model& model, const streamcompatibility_effective_edid_setter& effective_edid_setter, const nmos::id& input_id)
            {
                boost::variant<utility::string_t, web::uri> effective_edid;

                // get effective_edid from application code
                effective_edid_setter(input_id, effective_edid);

                utility::string_t updated_timestamp{ nmos::make_version() };

                modify_resource(model.streamcompatibility_resources, input_id, [&effective_edid, updated_timestamp](nmos::resource& input)
                {
                    input.data[nmos::fields::endpoint_effective_edid] = boost::apply_visitor(edid_file_visitor(), effective_edid);

                    input.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                });

                const std::pair<nmos::id, nmos::type> id_type{ input_id, nmos::types::input };
                auto resource = find_resource(model.streamcompatibility_resources, id_type);

                // hmmmmmmmmm, to fix
                // `When properties of any Input is changed, then the version attribute of the relevant IS-04 Device MUST be incremented. Inputs identify the corresponding Devices via the device_id property.`
                // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
                update_version(model.node_resources, nmos::fields::senders(resource->data), updated_timestamp);
            }

            // it's expected that model write lock is already obtained and IS-11 sender exists
            void set_active_constraints(nmos::node_model& model, const nmos::id& sender_id, const web::json::value& constraints, const web::json::value& intersection, const streamcompatibility_effective_edid_setter& effective_edid_setter)
            {
                const std::pair<nmos::id, nmos::type> sender_id_type{ sender_id, nmos::types::sender };
                auto resource = find_resource(model.streamcompatibility_resources, sender_id_type);
                auto matching_resource = find_resource(model.node_resources, sender_id_type);
                if (model.node_resources.end() == matching_resource)
                {
                    throw std::logic_error("matching IS-04 resource not found");
                }

                // Pre-check for resources existence before Active Constraints modified and effective_edid_setter executed
                if (effective_edid_setter)
                {
                    for (const auto& input_id : nmos::fields::inputs(resource->data))
                    {
                        const std::pair<nmos::id, nmos::type> input_id_type{ input_id.as_string(), nmos::types::input };
                        auto input = find_resource(model.streamcompatibility_resources, input_id_type);
                        if (model.streamcompatibility_resources.end() != input)
                        {
                            if (!all_resources_exist(model.node_resources, nmos::fields::senders(input->data), nmos::types::sender))
                            {
                                throw std::logic_error("associated IS-04 sender not found");
                            }
                        }
                        else
                        {
                            throw std::logic_error("associated IS-11 input not found");
                        }
                    }
                }

                utility::string_t updated_timestamp{ nmos::make_version() };

                // Update Active Constraints in streamcompatibility_resources
                modify_resource(model.streamcompatibility_resources, sender_id, [&constraints, &intersection, &updated_timestamp](nmos::resource& sender)
                {
                    sender.data[nmos::fields::intersection_of_caps_and_constraints] = intersection;
                    sender.data[nmos::fields::endpoint_active_constraints] = make_streamcompatibility_active_constraints_endpoint(constraints);

                    sender.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                });

                update_version(model.node_resources, sender_id, updated_timestamp);

                // Update IS-11 input effective EDID, which is provided by the application code
                if (effective_edid_setter)
                {
                    for (const auto& input_id : nmos::fields::inputs(resource->data))
                    {
                        details::update_effective_edid(model, effective_edid_setter, input_id.as_string());
                    }
                }

                model.notify();
            }
        }

        web::http::experimental::listener::api_router make_unmounted_streamcompatibility_api(nmos::node_model& model, details::streamcompatibility_base_edid_handler base_edid_changed, details::streamcompatibility_effective_edid_setter effective_edid_setter, details::streamcompatibility_active_constraints_handler active_constraints_handler, slog::base_gate& gate);

        web::http::experimental::listener::api_router make_streamcompatibility_api(nmos::node_model& model, details::streamcompatibility_base_edid_handler base_edid_changed, details::streamcompatibility_effective_edid_setter effective_edid_setter, details::streamcompatibility_active_constraints_handler active_constraints_handler, web::http::experimental::listener::route_handler validate_authorization, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router streamcompatibility_api;

            streamcompatibility_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-nmos/") }, req, res));
                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/x-nmos/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("streamcompatibility/") }, req, res));
                return pplx::task_from_result(true);
            });

            if (validate_authorization)
            {
                streamcompatibility_api.support(U("/x-nmos/") + nmos::patterns::streamcompatibility_api.pattern + U("/?"), validate_authorization);
                streamcompatibility_api.support(U("/x-nmos/") + nmos::patterns::streamcompatibility_api.pattern + U("/.*"), validate_authorization);
            }

            const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is11_versions::from_settings(model.settings); });
            streamcompatibility_api.support(U("/x-nmos/") + nmos::patterns::streamcompatibility_api.pattern + U("/?"), methods::GET, [versions](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body(nmos::make_api_version_sub_routes(versions), req, res));
                return pplx::task_from_result(true);
            });

            streamcompatibility_api.mount(U("/x-nmos/") + nmos::patterns::streamcompatibility_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_streamcompatibility_api(model, base_edid_changed, effective_edid_setter, active_constraints_handler, gate));

            return streamcompatibility_api;
        }

        web::http::experimental::listener::api_router make_unmounted_streamcompatibility_api(nmos::node_model& model, details::streamcompatibility_base_edid_handler base_edid_changed, details::streamcompatibility_effective_edid_setter effective_edid_setter, details::streamcompatibility_active_constraints_handler active_constraints_handler, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router streamcompatibility_api;

            // check for supported API version
            const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is11_versions::from_settings(model.settings); });

            const web::json::experimental::json_validator validator
            {
                nmos::experimental::load_json_schema,
                boost::copy_range<std::vector<web::uri>>(versions | boost::adaptors::transformed(experimental::make_streamcompatibilityapi_senders_active_constraints_put_request_uri))
            };

            streamcompatibility_api.support(U(".*"), nmos::details::make_api_version_handler(versions, gate_));

            streamcompatibility_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("senders/"), U("receivers/"), U("inputs/"), U("outputs/") }, req, res));
                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::streamCompatibilityResourceType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::streamCompatibilityResourceType.name);

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

            streamcompatibility_api.support(U("/") + nmos::patterns::streamCompatibilityResourceType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::streamCompatibilityResourceType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    // check if there is a matching IS-04 sender/receiver resource
                    if (nmos::types::sender == resource->type ||
                        nmos::types::receiver == resource->type)
                    {
                        auto matching_resource = find_resource(model.node_resources, id_type);
                        if (model.node_resources.end() == matching_resource)
                        {
                            throw std::logic_error("matching IS-04 " + utility::us2s(id_type.second.name) + " not found");
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
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/") + nmos::patterns::streamCompatibilityInputOutputType.pattern + U("/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.streamcompatibility_resources;

                const auto resourceType = nmos::type_from_resourceType(parameters.at(nmos::patterns::connectorType.name));
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);
                const auto associatedResourceType = nmos::type_from_resourceType(parameters.at(nmos::patterns::streamCompatibilityInputOutputType.name));

                const bool consistentTypes
                {
                    (nmos::types::sender == resourceType && nmos::types::input == associatedResourceType) ||
                    (nmos::types::receiver == resourceType && nmos::types::output == associatedResourceType)
                };

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, resourceType };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource && consistentTypes)
                {
                    // check if there is a matching IS-04 sender/receiver resource
                    auto matching_resource = find_resource(model.node_resources, id_type);
                    if (model.node_resources.end() == matching_resource)
                    {
                        throw std::logic_error("matching IS-04 " + utility::us2s(id_type.second.name) + " not found");
                    }

                    const auto filter = (nmos::types::input == associatedResourceType) ?
                        nmos::fields::inputs :
                        nmos::fields::outputs;

                    set_reply(res, status_codes::OK, web::json::value_from_elements(filter(resource->data)));
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::connectorType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/status/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::connectorType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    // check if there is a matching IS-04 sender/receiver resource
                    auto matching_resource = find_resource(model.node_resources, id_type);
                    if (model.node_resources.end() == matching_resource)
                    {
                        throw std::logic_error("matching IS-04 " + utility::us2s(id_type.second.name) + " not found");
                    }

                    set_reply(res, status_codes::OK, nmos::fields::status(resource->data));
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    // check if there is a matching IS-04 sender/receiver resource
                    auto matching_resource = find_resource(model.node_resources, id_type);
                    if (model.node_resources.end() == matching_resource)
                    {
                        throw std::logic_error("matching IS-04 " + utility::us2s(id_type.second.name) + " not found");
                    }

                    std::set<utility::string_t> sub_routes{ U("active/"), U("supported/") };

                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body(std::move(sub_routes), req, res));
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/") + nmos::patterns::constraintsType.pattern + U("/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t constraintsType = parameters.at(nmos::patterns::constraintsType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    // check if there is a matching IS-04 sender/receiver resource
                    auto matching_resource = find_resource(model.node_resources, id_type);
                    if (model.node_resources.end() == matching_resource)
                    {
                        throw std::logic_error("matching IS-04 " + utility::us2s(id_type.second.name) + " not found");
                    }

                    if (U("active") == constraintsType)
                    {
                        set_reply(res, status_codes::OK, nmos::fields::active_constraint_sets(nmos::fields::endpoint_active_constraints(resource->data)));
                    }
                    else if (U("supported") == constraintsType)
                    {
                        set_reply(res, status_codes::OK, nmos::fields::supported_param_constraints(resource->data));
                    }
                    else
                    {
                        // should never happen
                        set_reply(res, status_codes::NotFound);
                    }
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::inputOutputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/properties/?"), methods::GET, [&model](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                auto lock = model.read_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::inputOutputType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    auto data = resource->data;

                    // Note: input/output resources hold information about the EDID binary and they shouldn't be included in the response
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
                        data.erase(nmos::fields::senders);
                    }
                    else if (nmos::types::output == nmos::type_from_resourceType(resourceType))
                    {
                        if (!nmos::fields::endpoint_edid(resource->data).is_null())
                        {
                            data.erase(nmos::fields::endpoint_edid);
                        }
                        data.erase(nmos::fields::receivers);
                    }
                    else
                    {
                        // should never happen
                        set_reply(res, status_codes::NotImplemented);
                    }

                    set_reply(res, status_codes::OK, data);
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::inputOutputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/edid/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t resourceType = parameters.at(nmos::patterns::inputOutputType.name);
                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::type_from_resourceType(resourceType) };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    if (nmos::types::input == nmos::type_from_resourceType(resourceType))
                    {
                        std::set<utility::string_t> sub_routes{ U("base/"), U("effective/") };

                        set_reply(res, status_codes::OK, nmos::make_sub_routes_body(std::move(sub_routes), req, res));
                    }
                    else if (nmos::types::output == nmos::type_from_resourceType(resourceType))
                    {
                        auto& edid_endpoint = nmos::fields::endpoint_edid(resource->data);

                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "GET EDID requested for " << id_type;

                        details::set_edid_endpoint_as_reply(res, id_type, edid_endpoint, gate);
                    }
                    else
                    {
                        // should never happen
                        set_reply(res, status_codes::NotImplemented);
                    }
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::inputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/edid/") + nmos::patterns::edidType.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.read_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);
                const string_t edidType = parameters.at(nmos::patterns::edidType.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::input };
                auto resource = find_resource(resources, id_type);
                if (resources.end() != resource)
                {
                    const auto filter = (U("base") == edidType) ?
                        nmos::fields::endpoint_base_edid :
                        nmos::fields::endpoint_effective_edid;

                    auto& edid_endpoint = filter(resource->data);

                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "GET EDID " << edidType << " requested for " << id_type;

                    details::set_edid_endpoint_as_reply(res, id_type, edid_endpoint, gate);
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::inputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/edid/base/?"), methods::PUT, [&model, base_edid_changed, effective_edid_setter, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.write_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t input_Id = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ input_Id, nmos::types::input };
                auto resource = find_resource(resources, id_type);

                // Extract "adjust_to_caps" query parameter
                bool adjust_to_caps = false;
                const auto query_params = web::json::value_from_query((req.request_uri().query()));
                if (query_params.has_field(nmos::fields::adjust_to_caps))
                {
                    const auto& query_adjust_to_caps = query_params.at(nmos::fields::adjust_to_caps).as_string();
                    if (query_adjust_to_caps == U("true") || query_adjust_to_caps == U("1")) // hmm, should "1" be allowed instead of "true" ?
                    {
                        adjust_to_caps = true;
                    }
                }

                if (resources.end() != resource)
                {
                    auto& endpoint_base_edid = nmos::fields::endpoint_base_edid(resource->data);

                    if (!endpoint_base_edid.is_null())
                    {
                        if (!nmos::fields::temporarily_locked(endpoint_base_edid))
                        {
                            // hmm, should use task to extract body ???
                            const auto request_body = req.content_ready().get().extract_vector().get();
                            const utility::string_t base_edid_binary{ request_body.begin(), request_body.end() };

                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "PUT Base EDID binary requested for " << id_type << " with binary data size " << base_edid_binary.size();

                            // Notify the application code that the Base EDID for the IS-11 input has changed
                            if (base_edid_changed)
                            {
                                base_edid_changed(input_Id, base_edid_binary);
                            }

                            // Pre-check for resources existence before Base EDID modified and effective_edid_setter executed
                            if (effective_edid_setter)
                            {
                                if (!details::all_resources_exist(model.node_resources, nmos::fields::senders(resource->data), nmos::types::sender))
                                {
                                    throw std::logic_error("associated IS-04 sender not found");
                                }
                            }

                            utility::string_t updated_timestamp{ nmos::make_version() };

                            // Update Base EDID in streamcompatibility_resources
                            modify_resource(resources, input_Id, [&base_edid_binary, &updated_timestamp, &adjust_to_caps](nmos::resource& input)
                            {
                                input.data[nmos::fields::endpoint_base_edid] = make_streamcompatibility_edid_endpoint(base_edid_binary);
                                input.data[nmos::fields::adjust_to_caps] = value::boolean(adjust_to_caps);

                                input.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                            });

                            // hmmmmmm,
                            // `When properties of any Input/Output are changed, then the version attribute of the relevant IS-04 Device MUST be incremented. Inputs/Outputs identify the corresponding Devices via the device_id property.`
                            update_version(model.node_resources, nmos::fields::senders(resource->data), updated_timestamp);

                            // Update IS-11 input effective EDID, which is provided by the application code
                            if (effective_edid_setter)
                            {
                                details::update_effective_edid(model, effective_edid_setter, input_Id);
                            }

                            model.notify();

                            set_reply(res, status_codes::NoContent);
                        }
                        else
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Rejecting PUT Base EDID request for " << id_type << " due to operation is locked";
                            set_error_reply(res, status_codes::Locked);
                        }
                    }
                    else
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Rejecting PUT Base EDID request for " << id_type << " due to this input not configured to allow it";
                        set_error_reply(res, status_codes::MethodNotAllowed);
                    }
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::inputType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/edid/base/?"), methods::DEL, [&model, base_edid_changed, effective_edid_setter, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.write_lock();
                auto& resources = model.streamcompatibility_resources;

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
                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Delete " << id_type << " Base EDID";

                            // notify the application code that the Base EDID for the IS-11 input has changed (cleared)
                            if (base_edid_changed)
                            {
                                base_edid_changed(resourceId, {});
                            }

                            // Pre-check for resources existence before Base EDID modified and effective_edid_setter executed
                            if (effective_edid_setter)
                            {
                                if (!details::all_resources_exist(model.node_resources, nmos::fields::senders(resource->data), nmos::types::sender))
                                {
                                    throw std::logic_error("associated IS-04 sender not found");
                                }
                            }

                            utility::string_t updated_timestamp{ nmos::make_version() };

                            modify_resource(resources, resourceId, [&updated_timestamp](nmos::resource& input)
                            {
                                input.data[nmos::fields::endpoint_base_edid] = make_streamcompatibility_dummy_edid_endpoint();

                                input.data[nmos::fields::version] = web::json::value::string(updated_timestamp);
                            });

                            update_version(model.node_resources, nmos::fields::senders(resource->data), updated_timestamp);

                            // Update IS-11 input effective EDID, which is provided by the application code
                            if (effective_edid_setter)
                            {
                                details::update_effective_edid(model, effective_edid_setter, resourceId);
                            }

                            model.notify();

                            set_reply(res, status_codes::NoContent);
                        }
                        else
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Rejecting DELETE Base EDID request for " << id_type << " due to operation is locked";
                            set_error_reply(res, status_codes::Locked);
                        }
                    }
                    else
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Rejecting DELETE Base EDID request for " << id_type << " due to this input not configured to allow it";
                        set_error_reply(res, status_codes::MethodNotAllowed);
                    }
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/active/?"), methods::PUT, [&model, validator, active_constraints_handler, effective_edid_setter, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                return nmos::details::extract_json(req, gate).then([&model, req, res, parameters, &validator, active_constraints_handler, effective_edid_setter, gate](value data) mutable
                {
                    const nmos::api_version version = nmos::parse_api_version(parameters.at(nmos::patterns::version.name));

                    validator.validate(data, experimental::make_streamcompatibilityapi_senders_active_constraints_put_request_uri(version));

                    auto lock = model.write_lock();
                    auto& resources = model.streamcompatibility_resources;

                    const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                    const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };
                    auto streamcompatibility_sender = find_resource(resources, id_type);
                    if (resources.end() != streamcompatibility_sender)
                    {
                        auto& endpoint_active_constraints = nmos::fields::endpoint_active_constraints(streamcompatibility_sender->data);

                        if (!nmos::fields::temporarily_locked(endpoint_active_constraints))
                        {
                            const auto supported_param_constraints = boost::copy_range<std::unordered_set<utility::string_t>>(nmos::fields::parameter_constraints(nmos::fields::supported_param_constraints(streamcompatibility_sender->data)) | boost::adaptors::transformed([](const web::json::value& param_constraint)
                            {
                                return std::unordered_set<utility::string_t>::value_type{ param_constraint.as_string() };
                            }));

                            if (details::validate_constraint_sets(nmos::fields::constraint_sets(data).as_array(), supported_param_constraints))
                            {
                                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Update " << id_type << " Active Constraints: " << nmos::fields::constraint_sets(data).serialize();

                                bool can_adhere = true;
                                web::json::value intersection = web::json::value::array();

                                // sender capabilities against active constraints, returns the intersection of capabilities constraints (hmmmmm, may throw!!!!)
                                if (active_constraints_handler)
                                {
                                    if (!active_constraints_handler(*streamcompatibility_sender, data, intersection))
                                    {
                                        can_adhere = false;

                                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Rejecting PUT Active Constraints request for " << id_type << " due to sender can't adhere to these Constraints: " << nmos::fields::constraint_sets(data).serialize();
                                        set_error_reply(res, status_codes::UnprocessableEntity);
                                    }
                                }

                                if (can_adhere)
                                {
                                    details::set_active_constraints(model, resourceId, nmos::fields::constraint_sets(data), intersection, effective_edid_setter);
                                    set_reply(res, status_codes::OK, data);
                                }
                            }
                            else
                            {
                                set_error_reply(res, status_codes::BadRequest, U("The requested Constraint Set uses Parameter Constraints unsupported by this Sender"));
                            }
                        }
                        else
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Rejecting PUT Active Constraints request for " << id_type << " due to operation is locked";
                            set_error_reply(res, status_codes::Locked);
                        }
                    }
                    else
                    {
                        set_reply(res, status_codes::NotFound);
                    }

                    return true;
                });
            });

            streamcompatibility_api.support(U("/") + nmos::patterns::senderType.pattern + U("/") + nmos::patterns::resourceId.pattern + U("/constraints/active/?"), methods::DEL, [&model, effective_edid_setter, active_constraints_handler, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                auto lock = model.write_lock();
                auto& resources = model.streamcompatibility_resources;

                const string_t resourceId = parameters.at(nmos::patterns::resourceId.name);

                const std::pair<nmos::id, nmos::type> id_type{ resourceId, nmos::types::sender };
                const auto streamcompatibility_sender = find_resource(resources, id_type);

                if (resources.end() != streamcompatibility_sender)
                {
                    const auto& endpoint_active_constraints = nmos::fields::endpoint_active_constraints(streamcompatibility_sender->data);

                    if (!nmos::fields::temporarily_locked(endpoint_active_constraints))
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Delete " << id_type << " Active Constraints";

                        const auto active_constraints = web::json::value_of({
                            { nmos::fields::constraint_sets, web::json::value::array() }
                        });
                        web::json::value intersection = web::json::value::array();

                        // hmm, test active_constraints_handler before use
                        active_constraints_handler(*streamcompatibility_sender, active_constraints, intersection);
                        details::set_active_constraints(model, resourceId, nmos::fields::constraint_sets(active_constraints), intersection, effective_edid_setter);

                        set_reply(res, status_codes::OK, nmos::fields::active_constraint_sets(nmos::fields::endpoint_active_constraints(streamcompatibility_sender->data)));
                    }
                    else
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Rejecting DELETE Active Constraints request for " << id_type << " due to operation is locked";
                        set_error_reply(res, status_codes::Locked);
                    }
                }
                else
                {
                    set_reply(res, status_codes::NotFound);
                }

                return pplx::task_from_result(true);
            });

            return streamcompatibility_api;
        }
    }
}

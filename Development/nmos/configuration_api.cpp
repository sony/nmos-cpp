#include "nmos/configuration_api.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/iterator/filter_iterator.hpp>
//#include "cpprest/json_validator.h"
#include "nmos/api_utils.h"
#include "nmos/control_protocol_methods.h"
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/is14_versions.h"
//#include "nmos/json_schema.h"
#include "nmos/log_manip.h"
#include "nmos/model.h"

namespace nmos
{
    inline web::http::experimental::listener::api_router make_unmounted_configuration_api(nmos::node_model& model, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate);

    web::http::experimental::listener::api_router make_configuration_api(nmos::node_model& model, web::http::experimental::listener::route_handler validate_authorization, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate)
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

        configuration_api.mount(U("/x-nmos/") + nmos::patterns::configuration_api.pattern + U("/") + nmos::patterns::version.pattern, make_unmounted_configuration_api(model, get_control_protocol_class_descriptor, gate));

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

        void build_role_paths(const resources& resources, const nmos::resource& resource, const utility::string_t& base_role_path, std::set<utility::string_t>& role_paths)
        {
            if (resource.data.has_field(nmos::fields::nc::members))
            {
                const auto& members = nmos::fields::nc::members(resource.data);

                for (const auto& member : members)
                {
                    const auto role_path = base_role_path + U(".") + nmos::fields::nc::role(member);
                    role_paths.insert(role_path + U("/"));

                    // get members on all NcBlock(s)
                    if (nmos::is_nc_block(nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(member))))
                    {
                        // get resource based on the oid
                        const auto& oid = nmos::fields::nc::oid(member);
                        const auto& found = find_resource(resources, utility::s2us(std::to_string(oid)));
                        if (resources.end() != found)
                        {
                            build_role_paths(resources, *found, role_path, role_paths);
                        }
                    }
                }
            }
        }

        web::json::value get_nc_block_member_descriptor(const resources& resources, const nmos::resource& parent_nc_block_resource, std::list<utility::string_t>& role_path_segments)
        {
            if (parent_nc_block_resource.data.has_field(nmos::fields::nc::members))
            {
                const auto& members = nmos::fields::nc::members(parent_nc_block_resource.data);

                const auto role_path_segement = role_path_segments.front();
                role_path_segments.pop_front();
                // find the role_path_segment member
                auto member_found = std::find_if(members.begin(), members.end(), [&](const web::json::value& member)
                {
                    return role_path_segement == nmos::fields::nc::role(member);
                });

                if (members.end() != member_found)
                {
                    if (role_path_segments.empty())
                    {
                        // NcBlockMemberDescriptor
                        return *member_found;
                    }

                    // get the role_path_segement member resource
                    if (is_nc_block(nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(*member_found))))
                    {
                        // get resource based on the oid
                        const auto& oid = nmos::fields::nc::oid(*member_found);
                        const auto& found = find_resource(resources, utility::s2us(std::to_string(oid)));
                        if (resources.end() != found)
                        {
                            return get_nc_block_member_descriptor(resources, *found, role_path_segments);
                        }
                    }
                }
            }
            return web::json::value{};
        }

        web::json::value get_nc_object(const resources& resources, std::list<utility::string_t>& role_path_segments)
        {
            auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(nmos::root_block_oid)));
            if (resources.end() != resource)
            {
                const auto role = nmos::fields::nc::role(resource->data);
                if (role_path_segments.size() && role == role_path_segments.front())
                {
                    role_path_segments.pop_front();

                    if (role_path_segments.size())
                    {
                        const auto& block_member_descriptor = details::get_nc_block_member_descriptor(resources, *resource, role_path_segments);
                        if (!block_member_descriptor.is_null())
                        {
                            const auto& oid = nmos::fields::nc::oid(block_member_descriptor);
                            const auto& found = find_resource(resources, utility::s2us(std::to_string(oid)));
                            if (resources.end() != found)
                            {
                                return found->data;
                            }
                        }
                    }
                    else
                    {
                        return resource->data;
                    }
                }
            }
            return web::json::value{};
        }

        web::json::value get_nc_object(const resources& resources, const utility::string_t& role_path)
        {
            // tokenize the role_path with the '.' delimiter
            std::list<utility::string_t> role_path_segments;
            boost::algorithm::split(role_path_segments, role_path, [](utility::char_t c) { return '.' == c; });

            return  get_nc_object(resources, role_path_segments);
        }

        nc_property_id parse_formatted_property_id(const utility::string_t& property_id)
        {
            // Assume that property_id is in form "<level>p<index>" as validated by the propertyId regular expression pattern
            const utility::string_t::size_type delimiter = property_id.find('p');
            utility::string_t level = property_id.substr(0, delimiter);
            utility::string_t index = property_id.substr(delimiter + 1);
            return { uint16_t(web::json::value::parse(level).as_integer()), uint16_t(web::json::value::parse(index).as_integer()) };
        }
    }

    inline web::http::experimental::listener::api_router make_unmounted_configuration_api(nmos::node_model& model, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, slog::base_gate& gate_)
    {
        using namespace web::http::experimental::listener::api_router_using_declarations;

        api_router configuration_api;

        // check for supported API version
        const auto versions = with_read_lock(model.mutex, [&model] { return nmos::is14_versions::from_settings(model.settings); });
        configuration_api.support(U(".*"), details::make_api_version_handler(versions, gate_));

        configuration_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
        {
            set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("rolePaths/") }, req, res));
            return pplx::task_from_result(true);
        });

        configuration_api.support(U("/rolePaths/?"), methods::GET, [&model](http_request req, http_response res, const string_t& route_path, const route_parameters& parameters)
        {
            using web::json::value;

            auto lock = model.read_lock();
            auto& resources = model.control_protocol_resources;

            std::set<utility::string_t> role_paths;

            // start at the root block resource
            auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(nmos::root_block_oid)));
            if (resources.end() != resource)
            {
                // add root to role_paths
                const auto role_path = nmos::fields::nc::role(resource->data);
                role_paths.insert(role_path + U("/"));

                // add rest to the role_paths
                details::build_role_paths(resources, *resource, role_path, role_paths);
            }

            set_reply(res, status_codes::OK, nmos::make_sub_routes_body(role_paths, req, res));
            return pplx::task_from_result(true);
        });

        configuration_api.support(U("/rolePaths/") + nmos::patterns::rolePath.pattern + U("/?"), methods::GET, [&model, &gate_](http_request req, http_response res, const string_t& route_path, const route_parameters& parameters)
        {
            const utility::string_t role_path = parameters.at(nmos::patterns::rolePath.name);

            auto lock = model.read_lock();
            auto& resources = model.control_protocol_resources;

            if (!details::get_nc_object(resources, role_path).is_null())
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("bulkProperties/"), U("descriptor/"), U("methods/"), U("properties/") }, req, res));
            }
            else
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + role_path);
            }

            return pplx::task_from_result(true);
        });

        configuration_api.support(U("/rolePaths/") + nmos::patterns::rolePath.pattern + U("/properties/?"), methods::GET, [&model, get_control_protocol_class_descriptor, &gate_](http_request req, http_response res, const string_t& route_path, const route_parameters& parameters)
        {
            const string_t role_path = parameters.at(nmos::patterns::rolePath.name);

            auto lock = model.read_lock();
            auto& resources = model.control_protocol_resources;

            const auto& nc_object = details::get_nc_object(resources, role_path);
            if (!nc_object.is_null())
            {
                std::set<utility::string_t> properties_routes;

                auto class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(nc_object));
                while (!class_id.empty())
                {
                    const auto& control_class = get_control_protocol_class_descriptor(class_id);
                    auto& property_descriptors = control_class.property_descriptors.as_array();

                    auto properties_route = boost::copy_range<std::set<utility::string_t>>(property_descriptors | boost::adaptors::transformed([](const web::json::value& property_descriptor)
                    {
                        auto make_property_id = [](const web::json::value& property_descriptor)
                        {
                            auto property_id = nmos::fields::nc::id(property_descriptor);
                            utility::ostringstream_t os;
                            os << nmos::fields::nc::level(property_id) << 'p' << nmos::fields::nc::index(property_id);
                            return os.str();
                        };

                        return make_property_id(property_descriptor) + U("/");
                    }));

                    properties_routes.insert(properties_route.begin(), properties_route.end());

                    class_id.pop_back();
                }

                set_reply(res, status_codes::OK, nmos::make_sub_routes_body(properties_routes, req, res));
            }
            else
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + role_path);
            }

            return pplx::task_from_result(true);
        });

        configuration_api.support(U("/rolePaths/") + nmos::patterns::rolePath.pattern + U("/methods/?"), methods::GET, [&model, get_control_protocol_class_descriptor, &gate_](http_request req, http_response res, const string_t& route_path, const route_parameters& parameters)
        {
            const utility::string_t role_path = parameters.at(nmos::patterns::rolePath.name);

            auto lock = model.read_lock();
            auto& resources = model.control_protocol_resources;

            const auto& nc_object = details::get_nc_object(resources, role_path);
            if (!nc_object.is_null())
            {
                std::set<utility::string_t> methods_routes;

                nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(nc_object));
                while (!class_id.empty())
                {
                    const auto& control_class = get_control_protocol_class_descriptor(class_id);
                    auto& method_descriptors = control_class.method_descriptors;

                    auto methods_route = boost::copy_range<std::set<utility::string_t>>(method_descriptors | boost::adaptors::transformed([](const nmos::experimental::method& method)
                    {
                        auto make_method_id = [](const nmos::experimental::method& method)
                        {
                            // method tuple definition described in control_protocol_handlers.h
                            auto& nc_method_descriptor = std::get<0>(method);
                            auto method_id = nmos::fields::nc::id(nc_method_descriptor);
                            utility::ostringstream_t os;
                            os << nmos::fields::nc::level(method_id) << 'm' << nmos::fields::nc::index(method_id);
                            return os.str();
                        };

                        return make_method_id(method) + U("/");
                    }));

                    methods_routes.insert(methods_route.begin(), methods_route.end());

                    class_id.pop_back();
                }

                set_reply(res, status_codes::OK, nmos::make_sub_routes_body(methods_routes, req, res));
            }
            else
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + role_path);
            }

            return pplx::task_from_result(true);
        });

        configuration_api.support(U("/rolePaths/") + nmos::patterns::rolePath.pattern + U("/descriptor/?"), methods::GET, [&model, get_control_protocol_class_descriptor, &gate_](http_request req, http_response res, const string_t& route_path, const route_parameters& parameters)
        {
            using web::json::value_from_elements;

            const string_t role_path = parameters.at(nmos::patterns::rolePath.name);

            auto lock = model.read_lock();
            auto& resources = model.control_protocol_resources;

            const auto& nc_object = details::get_nc_object(resources, role_path);
            if (!nc_object.is_null())
            {
                nc_class_id class_id = nmos::details::parse_nc_class_id(nmos::fields::nc::class_id(nc_object));

                if (!class_id.empty())
                {
                    const auto& control_class = get_control_protocol_class_descriptor(class_id);

                    auto& description = control_class.description;
                    auto& name = control_class.name;
                    auto& fixed_role = control_class.fixed_role;
                    auto property_descriptors = control_class.property_descriptors;
                    auto method_descriptors = value::array();
                    for (const auto& method_descriptor : control_class.method_descriptors) { web::json::push_back(method_descriptors, std::get<0>(method_descriptor)); }
                    auto event_descriptors = control_class.event_descriptors;

                    auto inherited_class_id = class_id;
                    inherited_class_id.pop_back();

                    while (!inherited_class_id.empty())
                    {
                        const auto& inherited_control_class = get_control_protocol_class_descriptor(inherited_class_id);
                        {
                            for (const auto& property_descriptor : inherited_control_class.property_descriptors.as_array()) { web::json::push_back(property_descriptors, property_descriptor); }
                            for (const auto& method_descriptor : inherited_control_class.method_descriptors) { web::json::push_back(method_descriptors, std::get<0>(method_descriptor)); }
                            for (const auto& event_descriptor : inherited_control_class.event_descriptors.as_array()) { web::json::push_back(event_descriptors, event_descriptor); }
                        }
                        inherited_class_id.pop_back();
                    }

                    auto class_descriptor = fixed_role.is_null()
                        ? details::make_nc_class_descriptor(description, class_id, name, property_descriptors, method_descriptors, event_descriptors)
                        : details::make_nc_class_descriptor(description, class_id, name, fixed_role.as_string(), property_descriptors, method_descriptors, event_descriptors);

                    set_reply(res, status_codes::OK, class_descriptor);
                }
            }
            else
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + role_path);
            }

            return pplx::task_from_result(true);
        });

        configuration_api.support(U("/rolePaths/") + nmos::patterns::rolePath.pattern + U("/properties/") + nmos::patterns::propertyId.pattern + U("/?"), methods::GET, [&model, get_control_protocol_class_descriptor, &gate_](http_request req, http_response res, const string_t& route_path, const route_parameters& parameters)
        {
            const utility::string_t property_id = parameters.at(nmos::patterns::propertyId.name);
            const utility::string_t role_path = parameters.at(nmos::patterns::rolePath.name);

            auto lock = model.read_lock();
            auto& resources = model.control_protocol_resources;

            const auto& nc_object = details::get_nc_object(resources, role_path);
            if (!nc_object.is_null())
            {
                //// find the relevant nc_property_descriptor
                const auto& property_descriptor = find_property_descriptor(details::parse_formatted_property_id(property_id), details::parse_nc_class_id(nmos::fields::nc::class_id(nc_object)), get_control_protocol_class_descriptor);
                if (property_descriptor.is_null())
                {
                    set_error_reply(res, status_codes::NotFound, U("Not Found; ") + property_id);
                }
                else
                {
                    set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("descriptor/"), U("value/") }, req, res));
                }
            }
            else
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + role_path);
            }

            return pplx::task_from_result(true);
        });

        configuration_api.support(U("/rolePaths/") + nmos::patterns::rolePath.pattern + U("/properties/") + nmos::patterns::propertyId.pattern + U("/descriptor/?"), methods::GET, [&model, get_control_protocol_class_descriptor, &gate_](http_request req, http_response res, const string_t& route_path, const route_parameters& parameters)
        {
            const utility::string_t property_id = parameters.at(nmos::patterns::propertyId.name);
            const utility::string_t role_path = parameters.at(nmos::patterns::rolePath.name);

            web::json::value property_value{};

            auto lock = model.read_lock();
            auto& resources = model.control_protocol_resources;

            const auto& nc_object = details::get_nc_object(resources, role_path);
            if (!nc_object.is_null())
            {
                //// find the relevant nc_property_descriptor
                const auto& property_descriptor = find_property_descriptor(details::parse_formatted_property_id(property_id), details::parse_nc_class_id(nmos::fields::nc::class_id(nc_object)), get_control_protocol_class_descriptor);
                if (property_descriptor.is_null())
                {
                    set_error_reply(res, status_codes::NotFound, U("Not Found; ") + property_id);
                }
                else
                {
                    set_reply(res, status_codes::OK, property_descriptor);
                }
            }
            else
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + role_path);
            }

            return pplx::task_from_result(true);
        });

        configuration_api.support(U("/rolePaths/") + nmos::patterns::rolePath.pattern + U("/properties/") + nmos::patterns::propertyId.pattern + U("/value/?"), methods::GET, [&model, get_control_protocol_class_descriptor, &gate_](http_request req, http_response res, const string_t& route_path, const route_parameters& parameters)
        {
            const utility::string_t property_id = parameters.at(nmos::patterns::propertyId.name);
            const utility::string_t role_path = parameters.at(nmos::patterns::rolePath.name);

            web::json::value property_value{};

            auto lock = model.read_lock();
            auto& resources = model.control_protocol_resources;

            const auto& nc_object = details::get_nc_object(resources, role_path);
            if (!nc_object.is_null())
            {
                //// find the relevant nc_property_descriptor
                const auto& property_descriptor = find_property_descriptor(details::parse_formatted_property_id(property_id), details::parse_nc_class_id(nmos::fields::nc::class_id(nc_object)), get_control_protocol_class_descriptor);
                if (property_descriptor.is_null())
                {
                    set_error_reply(res, status_codes::NotFound, U("Not Found; ") + property_id);
                }
                else
                {
                    web::json::value method_result = details::make_nc_method_result({ nmos::fields::nc::is_deprecated(property_descriptor) ? nmos::nc_method_status::property_deprecated : nmos::nc_method_status::ok }, nc_object.at(nmos::fields::nc::name(property_descriptor)));

                    set_reply(res, status_codes::OK, method_result);
                }
            }
            else
            {
                set_error_reply(res, status_codes::NotFound, U("Not Found; ") + role_path);
            }

            return pplx::task_from_result(true);
        });

        return configuration_api;
    }


}

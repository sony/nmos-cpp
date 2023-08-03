#include "nmos/control_protocol_ws_api.h"

#include <boost/range/join.hpp>
#include "cpprest/json_validator.h"
#include "cpprest/regex_utils.h"
#include "nmos/api_utils.h"
#include "nmos/control_protocol_resources.h"
#include "nmos/is12_versions.h"
#include "nmos/json_schema.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/slog.h"
#include "nmos/resources.h"

namespace nmos
{
    namespace details
    {
        static const web::json::experimental::json_validator& controlprotocol_validator()
        {
            // hmm, could be based on supported API versions from settings, like other APIs' validators?
            static const web::json::experimental::json_validator validator
            {
                nmos::experimental::load_json_schema,
                boost::copy_range<std::vector<web::uri>>(boost::join(boost::join(
                    is12_versions::all | boost::adaptors::transformed(experimental::make_controlprotocolapi_base_message_schema_uri),
                    is12_versions::all | boost::adaptors::transformed(experimental::make_controlprotocolapi_command_message_schema_uri)),
                    is12_versions::all | boost::adaptors::transformed(experimental::make_controlprotocolapi_subscription_message_schema_uri)
                ))
            };
            return validator;
        }

        // Validate against specification schema
        // throws web::json::json_exception on failure, which results in a 400 Badly-formed command
        void validate_controlprotocolapi_base_message_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            controlprotocol_validator().validate(request_data, experimental::make_controlprotocolapi_base_message_schema_uri(version));
        }
        void validate_controlprotocolapi_command_message_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            controlprotocol_validator().validate(request_data, experimental::make_controlprotocolapi_command_message_schema_uri(version));
        }
        void validate_controlprotocolapi_subscription_message_schema(const nmos::api_version& version, const web::json::value& request_data)
        {
            controlprotocol_validator().validate(request_data, experimental::make_controlprotocolapi_subscription_message_schema_uri(version));
        }
    }

    // IS-12 Control Protocol WebSocket API

    web::websockets::experimental::listener::validate_handler make_control_protocol_ws_validate_handler(nmos::node_model& model, slog::base_gate& gate_)
    {
        return [&model, &gate_](web::http::http_request req)
        {
            nmos::ws_api_gate gate(gate_, req.request_uri());

            // RFC 6750 defines two methods of sending bearer access tokens which are applicable to WebSocket
            // Clients SHOULD use the "Authorization Request Header Field" method.
            // Clients MAY use a "URI Query Parameter".
            // See https://tools.ietf.org/html/rfc6750#section-2

            // For now just return true
            const auto& ws_ncp_path = req.request_uri().path();
            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Validating websocket connection to: " << ws_ncp_path;

            return true;
        };
    }

    web::websockets::experimental::listener::open_handler make_control_protocol_ws_open_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        using web::json::value;
        using web::json::value_of;

        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.control_protocol_resources;

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Opening websocket connection to: " << ws_ncp_path;

            // create a subscription (1-1 relationship with the connection)
            resources::const_iterator subscription;

            {
                const bool secure = nmos::experimental::fields::client_secure(model.settings);

                const auto ws_href = web::uri_builder()
                    .set_scheme(web::ws_scheme(secure))
                    .set_host(nmos::get_host(model.settings))
                    .set_port(nmos::fields::events_ws_port(model.settings))
                    .set_path(ws_ncp_path)
                    .to_uri();

                const bool non_persistent = false;
                value data = value_of({
                    { nmos::fields::id, nmos::make_id() },
                    { nmos::fields::max_update_rate_ms, 0 },
                    { nmos::fields::resource_path, U('/') + nmos::resourceType_from_type(nmos::types::source) },
                    { nmos::fields::params, value_of({ { U("query.rql"), U("in(id,())") } }) },
                    { nmos::fields::persist, non_persistent },
                    { nmos::fields::secure, secure },
                    { nmos::fields::ws_href, ws_href.to_string() }
                }, true);

                // hm, could version be determined from ws_resource_path?
                nmos::resource subscription_{ is12_versions::v1_0, nmos::types::subscription, std::move(data), non_persistent };

                subscription = insert_resource(resources, std::move(subscription_)).first;
            }

            {
                // create a websocket connection resource

                value data;
                nmos::id id = nmos::make_id();
                data[nmos::fields::id] = value::string(id);
                data[nmos::fields::subscription_id] = value::string(subscription->id);

                // create an initial websocket message with no data

                const auto resource_path = nmos::fields::resource_path(subscription->data);
                const auto topic = resource_path + U('/');
                // source_id and flow_id are set per-message depending on the source, unlike Query WebSocket API
                data[nmos::fields::message] = details::make_grain({}, {}, topic);

                resource grain{ is12_versions::v1_0, nmos::types::grain, std::move(data), false };
                insert_resource(resources, std::move(grain));

                websockets.insert({ id, connection_id });

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Creating websocket connection: " << id;

                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying control protocol websockets thread"; // and anyone else who cares...
                model.notify();
            }
        };
    }

    web::websockets::experimental::listener::close_handler make_control_protocol_ws_close_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        return [&model, &websockets, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, web::websockets::websocket_close_status close_status, const utility::string_t& close_reason)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);
            auto lock = model.write_lock();
            auto& resources = model.control_protocol_resources;

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing websocket connection to: " << ws_ncp_path << " [" << (int)close_status << ": " << close_reason << "]";

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(resources, { websocket->second, nmos::types::grain });

                if (resources.end() != grain)
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Deleting websocket connection";

                    // subscriptions have a 1-1 relationship with the websocket connection and both should now be erased immediately
                    auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });

                    if (resources.end() != subscription)
                    {
                        // this should erase grain too, as a subscription's subresource
                        erase_resource(resources, subscription->id);
                    }
                    else
                    {
                        // a grain without a subscription shouldn't be possible, but let's be tidy
                        erase_resource(resources, grain->id);
                    }
                    //erase_resource(resources, grain->id);
                }

                websockets.right.erase(websocket);

                model.notify();
            }
        };
    }

    web::websockets::experimental::listener::message_handler make_control_protocol_ws_message_handler(nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        using web::json::value;
        using web::json::value_of;

        // NcObject properties
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncobject
        const details::properties nc_object_properties =
        {
            // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncobject
            { value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 1 } }), nmos::fields::nc::class_id },
            { value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 2 } }), nmos::fields::nc::oid },
            { value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 3 } }), nmos::fields::nc::constant_oid },
            { value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 4 } }), nmos::fields::nc::owner },
            { value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 5 } }), nmos::fields::nc::role },
            { value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 6 } }), nmos::fields::nc::user_label },
            { value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 7 } }), nmos::fields::nc::touchpoints },
            { value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 8 } }), nmos::fields::nc::runtime_property_constraints }
        };

        // NcBlock properties
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncblock
        const details::properties nc_block_properties =
        {
            // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncblock
            { value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 1 } }), nmos::fields::nc::enabled },
            { value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 2 } }), nmos::fields::nc::members }
        };

        // NcWorker properties
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncworker
        const details::properties nc_worker_properties =
        {
            // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncworker
            { value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 1 } }), nmos::fields::nc::enabled }
        };

        // NcManager has no property
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmanager
        const details::properties nc_manager_properties;

        // NcDeviceManager properties
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdevicemanager
        const details::properties nc_device_manager_properties =
        {
            // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdevicemanager
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 1 } }), nmos::fields::nc::nc_version },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 2 } }), nmos::fields::nc::manufacturer },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 3 } }), nmos::fields::nc::product },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 4 } }), nmos::fields::nc::serial_number },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 5 } }), nmos::fields::nc::user_inventory_code },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 6 } }), nmos::fields::nc::device_name },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 7 } }), nmos::fields::nc::device_role },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 8 } }), nmos::fields::nc::operational_state },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 9 } }), nmos::fields::nc::reset_cause },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 10 } }), nmos::fields::nc::message }
        };

        // NcClassManager properties
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassmanager
        const details::properties nc_class_manager_properties =
        {
            // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassmanager
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 1 } }), nmos::fields::nc::control_classes },
            { value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 2 } }), nmos::fields::nc::datatypes }
        };

        // method handlers for the different classes
        details::methods nc_object_method_handlers; // method_id vs NcObject method_handler
        details::methods nc_block_method_handlers; // method_id vs NcBlock method_handler
        details::methods nc_worker_method_handlers; // method_id vs NcWorker method_handler
        details::methods nc_manager_method_handlers; // method_id vs NcManager method_handler
        details::methods nc_device_manager_method_handlers; // method_id vs NcDeviceManager method_handler
        details::methods nc_class_manager_method_handlers; // method_id vs NcClassManager method_handler

        // NcObject methods implementation
        // get property
        auto get = [&model](const details::properties& properties, int32_t handle, int32_t oid, const value& arguments)
        {
            // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

            auto& resources = model.control_protocol_resources;
            auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
            if (resources.end() != resource)
            {
                // where arguments is the property id = (level, index)
                const auto& property_id = nmos::fields::nc::id(arguments);

                // is property_id defined in properties map
                auto property_found = properties.find(property_id);
                if (property_found != properties.end())
                {
                    return details::make_control_protocol_response(handle, { details::nc_method_status::ok }, resource->data.at(property_found->second));
                }

                // unknown property
                utility::stringstream_t ss;
                ss << U("unknown property: ") << property_id.serialize() << " to do get";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::property_not_implemented }, ss.str());
            }

            // resource not found for the given oid
            utility::stringstream_t ss;
            ss << U("unknown oid: ") << oid << " to do get";
            return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
        };
        // set property
        auto set = [&model](const details::properties& properties, int32_t handle, int32_t oid, const value& arguments)
        {
            // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

            auto& resources = model.control_protocol_resources;
            auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
            if (resources.end() != resource)
            {
                const auto& property_id = nmos::fields::nc::id(arguments);
                const auto& val = nmos::fields::nc::value(arguments);

                // hmm, todo check property_id allowed in resource's class_id

                // is property_id defined in properties map
                auto property_found = properties.find(property_id);
                if (property_found != properties.end())
                {
                    resources.modify(resource, [&](nmos::resource& resource)
                        {
                            resource.data[property_found->second] = val;

                            resource.updated = strictly_increasing_update(resources);
                        });
                    return details::make_control_protocol_response(handle, { details::nc_method_status::ok });
                }
                else
                {
                    // hmm, find property function from user properties map
                }

                // unknown property
                utility::stringstream_t ss;
                ss << U("unknown property: ") << property_id.serialize() << " to do set";
                return details::make_control_protocol_error_response(handle, { details::nc_method_status::property_not_implemented }, ss.str());
            }

            // resource not found for the given oid
            utility::stringstream_t ss;
            ss << U("unknown oid: ") << oid << " to do set";
            return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
        };

        // NcBlock methods implementation
        // get descriptors of members of the block
        auto get_member_descriptors = [&model](const details::properties& properties, int32_t handle, int32_t oid, const value& arguments)
        {
            // note, model mutex is already locked by the outter function, so access to control_protocol_resources is OK...

            auto& resources = model.control_protocol_resources;
            auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
            if (resources.end() != resource)
            {
                // where arguments is the boolean recurse value
                // hmm, If recurse is set to true, nested members is to be retrieved
                const auto& recurse = nmos::fields::nc::recurse(arguments);

                return details::make_control_protocol_response(handle, { details::nc_method_status::ok }, resource->data.at(nmos::fields::nc::members));
            }

            // resource not found for the given oid
            utility::stringstream_t ss;
            ss << U("unknown oid: ") << oid << " to get member descriptors";
            return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
        };

        // NcClassManager methods implementation
        auto get_control_class = [&model](const details::properties& properties, int32_t handle, int32_t oid, const value& arguments)
        {
            // resource not found for the given oid
            utility::stringstream_t ss;
            ss << U("unknown oid: ") << oid << " to get control class";
            return details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str());
        };

        // NcObject methods
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncobject
        nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 1 } })] = get;
        nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 2 } })] = set;
        //nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 3 } })] = get_sequence_item;
        //nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 4 } })] = set_sequence_item;
        //nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 5 } })] = add_sequence_item;
        //nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 6 } })] = remove_sequence_item;
        //nc_object_method_handlers[value_of({ { nmos::fields::nc::level, 1 }, { nmos::fields::nc::index, 7 } })] = get_sequence_length;

        // NcBlock methods
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncblock
        nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 1 } })] = get_member_descriptors;
        //nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 2 } })] = find_members_by_path;
        //nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 3 } })] = find_members_by_role;
        //nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 2 }, { nmos::fields::nc::index, 4 } })] = find_members_by_class_id;

        // NcWorker has no extended method
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncworker

        // NcManager has no extended method
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncmanager

        // NcDeviceManger has no extended method
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdevicemanager

        // NcClassManager methods
        // see https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassmanager
        //nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 1 } })] = get_control_class;
        //nc_block_method_handlers[value_of({ { nmos::fields::nc::level, 3 }, { nmos::fields::nc::index, 2 } })] = get_datatype;

        // create properties and method handlers based on resource type
        auto create_properties_methods = [=](const nmos::type& type)
        {
            details::properties properties;
            details::methods methods;

            // all start from NcObject
            properties.insert(nc_object_properties.begin(), nc_object_properties.end());
            methods.insert(nc_object_method_handlers.begin(), nc_object_method_handlers.end());
            if (type == nmos::types::nc_block)
            {
                properties.insert(nc_block_properties.begin(), nc_block_properties.end());
                methods.insert(nc_block_method_handlers.begin(), nc_block_method_handlers.end());
            }
            else if (type == nmos::types::nc_worker)
            {
                properties.insert(nc_worker_properties.begin(), nc_worker_properties.end());
                methods.insert(nc_worker_method_handlers.begin(), nc_worker_method_handlers.end());
            }
            else if (type == nmos::types::nc_manager)
            {
                properties.insert(nc_manager_properties.begin(), nc_manager_properties.end());
                methods.insert(nc_manager_method_handlers.begin(), nc_manager_method_handlers.end());
            }
            else if (type == nmos::types::nc_device_manager)
            {
                properties.insert(nc_manager_properties.begin(), nc_manager_properties.end());
                properties.insert(nc_device_manager_properties.begin(), nc_device_manager_properties.end());
                methods.insert(nc_manager_method_handlers.begin(), nc_manager_method_handlers.end());
                methods.insert(nc_device_manager_method_handlers.begin(), nc_device_manager_method_handlers.end());
            }
            else if (type == nmos::types::nc_class_manager)
            {
                properties.insert(nc_manager_properties.begin(), nc_manager_properties.end());
                properties.insert(nc_class_manager_properties.begin(), nc_class_manager_properties.end());
                methods.insert(nc_manager_method_handlers.begin(), nc_manager_method_handlers.end());
                methods.insert(nc_class_manager_method_handlers.begin(), nc_class_manager_method_handlers.end());
            }

            // hmm, add user properties
            //if (!user_properties.empty())
            //{
            //    properties.insert(user_properties.begin(), user_properties.end());
            //}

            // hmm, add user method handlers
            //if (!user_methods.empty())
            //{
            //    methods.insert(user_methods.begin(), user_methods.end());
            //}

            return std::pair<details::properties, details::methods>(properties, methods);
        };

        return [&model, &websockets, create_properties_methods, &gate_](const web::uri& connection_uri, const web::websockets::experimental::listener::connection_id& connection_id, const web::websockets::websocket_incoming_message& msg_)
        {
            nmos::ws_api_gate gate(gate_, connection_uri);

            auto lock = model.write_lock();
            auto& resources = model.control_protocol_resources;

            // theoretically blocking, but in fact not
            auto msg = msg_.extract_string().get();

            const auto& ws_ncp_path = connection_uri.path();
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received websocket message: " << msg << " on connection: " << ws_ncp_path;

            // hmm todo: extract the version from the ws_ncp_path
            const nmos::api_version version = is12_versions::v1_0;
            //const nmos::api_version version = nmos::parse_api_version(ws_ncp_path(nmos::patterns::version.name));

            auto websocket = websockets.right.find(connection_id);
            if (websockets.right.end() != websocket)
            {
                auto grain = find_resource(resources, { websocket->second, nmos::types::grain });

                if (resources.end() != grain)
                {
                    auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });

                    if (resources.end() != subscription)
                    {
                        try
                        {
                            const auto message = value::parse(utility::conversions::to_string_t(msg));

                            // validate the base-message
                            details::validate_controlprotocolapi_base_message_schema(version, message);

                            const auto msg_type = nmos::fields::nc::message_type(message);
                            switch (msg_type)
                            {
                            case details::nc_message_type::command:
                            {
                                // validate command-message
                                details::validate_controlprotocolapi_command_message_schema(version, message);

                                auto responses = value::array();
                                auto& commands = nmos::fields::nc::commands(message);
                                for (const auto& cmd : commands)
                                {
                                    const auto handle = nmos::fields::nc::handle(cmd);
                                    const auto oid = nmos::fields::nc::oid(cmd);

                                    // get methodId
                                    const auto& method_id = nmos::fields::nc::method_id(cmd);

                                    // get arguments
                                    const auto& arguments = nmos::fields::nc::arguments(cmd);

                                    auto resource = nmos::find_resource(resources, utility::s2us(std::to_string(oid)));
                                    if (resources.end() != resource)
                                    {
                                        // create properties and method handlers based on resource type
                                        auto properties_methods = create_properties_methods(resource->type);
                                        auto& properties = properties_methods.first;
                                        auto& methods = properties_methods.second;

                                        // find the relevent method handler to execute
                                        auto method = methods.find(method_id);
                                        if (method != methods.end())
                                        {
                                            // execute the relevant method handler, then accumulating up their response to reponses
                                            web::json::push_back(responses, method->second(properties, handle, oid, arguments));
                                        }
                                        else
                                        {
                                            utility::stringstream_t ss;
                                            ss << U("unsupported method id: ") << method_id.serialize();
                                            web::json::push_back(responses,
                                                details::make_control_protocol_error_response(handle, { details::nc_method_status::method_not_implemented }, ss.str()));
                                        }
                                    }
                                    else
                                    {
                                        // resource not found for the given oid
                                        utility::stringstream_t ss;
                                        ss << U("unknown oid: ") << oid;
                                        web::json::push_back(responses,
                                            details::make_control_protocol_error_response(handle, { details::nc_method_status::bad_oid }, ss.str()));
                                    }
                                }

                                // add command_response for the control protocol response thread to return to the client
                                resources.modify(grain, [&](nmos::resource& grain)
                                {
                                    web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                        details::make_control_protocol_message_response(details::nc_message_type::command_response, responses));

                                    grain.updated = strictly_increasing_update(resources);
                                });
                            }
                            break;
                            case details::nc_message_type::subscription:
                            {
                                // hmm, todo...
                            }
                                break;
                            default:
                                // unexpected message type
                                break;
                            }

                        }
                        catch (const web::json::json_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();

                            resources.modify(grain, [&](nmos::resource& grain)
                            {
                                web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                    details::make_control_protocol_error_message({ details::nc_method_status::bad_command_format }, utility::s2us(e.what())));

                                grain.updated = strictly_increasing_update(resources);
                            });
                        }
                        catch (const std::exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception while handing control protocol command: " << e.what();

                            resources.modify(grain, [&](nmos::resource& grain)
                            {
                                web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                    details::make_control_protocol_error_message({ details::nc_method_status::bad_command_format },
                                        utility::s2us(std::string("Unexpected exception while handing control protocol command : ") + e.what())));

                                grain.updated = strictly_increasing_update(resources);
                            });
                        }
                        catch (...)
                        {
                            slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception for handing control protocol command";

                            resources.modify(grain, [&](nmos::resource& grain)
                            {
                                web::json::push_back(nmos::fields::message_grain_data(grain.data),
                                    details::make_control_protocol_error_message({ details::nc_method_status::bad_command_format },
                                        U("Unexpected unknown exception while handing control protocol command")));

                                grain.updated = strictly_increasing_update(resources);
                            });
                        }
                        model.notify();
                    }
                }
            }
        };
    }

    // observe_websocket_exception is the same as the one defined in events_ws_api
    namespace details
    {
        struct observe_websocket_exception
        {
            observe_websocket_exception(slog::base_gate& gate) : gate(gate) {}

            void operator()(pplx::task<void> finally)
            {
                try
                {
                    finally.get();
                }
                catch (const web::websockets::websocket_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "WebSocket error: " << e.what() << " [" << e.error_code() << "]";
                }
            }

            slog::base_gate& gate;
        };
    }

    void send_control_protocol_ws_messages_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::node_model& model, nmos::websockets& websockets, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::send_control_protocol_ws_messages));

        using web::json::value;
        using web::json::value_of;

        // could start out as a shared/read lock, only upgraded to an exclusive/write lock when a grain in the resources is actually modified
        auto lock = model.write_lock();
        auto& condition = model.condition;
        auto& shutdown = model.shutdown;
        auto& resources = model.control_protocol_resources;

        tai most_recent_message{};
        auto earliest_necessary_update = (tai_clock::time_point::max)();

        for (;;)
        {
            // wait for the thread to be interrupted either because there are resource changes, or because the server is being shut down
            // or because message sending was throttled earlier
            details::wait_until(condition, lock, earliest_necessary_update, [&] { return shutdown || most_recent_message < most_recent_update(resources); });
            if (shutdown) break;
            most_recent_message = most_recent_update(resources);

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Got notification on control protocol websockets thread";

            earliest_necessary_update = (tai_clock::time_point::max)();

            std::vector<std::pair<web::websockets::experimental::listener::connection_id, web::websockets::websocket_outgoing_message>> outgoing_messages;

            for (auto wit = websockets.left.begin(); websockets.left.end() != wit;)
            {
                const auto& websocket = *wit;

                // for each websocket connection that has valid grain and subscription resources
                const auto grain = find_resource(resources, { websocket.first, nmos::types::grain });
                if (resources.end() == grain)
                {
                    auto close = listener.close(websocket.second, web::websockets::websocket_close_status::server_terminate, U("Expired"))
                        .then(details::observe_websocket_exception(gate));
                    // theoretically blocking, but in fact not
                    close.wait();

                    wit = websockets.left.erase(wit);
                    continue;
                }
                const auto subscription = find_resource(resources, { nmos::fields::subscription_id(grain->data), nmos::types::subscription });
                if (resources.end() == subscription)
                {
                    // a grain without a subscription shouldn't be possible, but let's be tidy
                    erase_resource(resources, grain->id);

                    auto close = listener.close(websocket.second, web::websockets::websocket_close_status::server_terminate, U("Expired"))
                        .then(details::observe_websocket_exception(gate));
                    // theoretically blocking, but in fact not
                    close.wait();

                    wit = websockets.left.erase(wit);
                    continue;
                }
                // and has events to send
                if (0 == nmos::fields::message_grain_data(grain->data).size())
                {
                    ++wit;
                    continue;
                }

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing to send " << nmos::fields::message_grain_data(grain->data).size() << " events on websocket connection: " << grain->id;

                for (const auto& event : nmos::fields::message_grain_data(grain->data).as_array())
                {
                    web::websockets::websocket_outgoing_message message;

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "outgoing_message: " << event.serialize();
                    message.set_utf8_message(utility::us2s(event.serialize()));
                    outgoing_messages.push_back({ websocket.second, message });
                }

                // reset the grain for next time
                resources.modify(grain, [&resources](nmos::resource& grain)
                {
                    // all messages have now been prepared
                    nmos::fields::message_grain_data(grain.data) = value::array();
                    grain.updated = strictly_increasing_update(resources);
                });

                ++wit;
            }

            // send the messages without the lock on resources
            details::reverse_lock_guard<nmos::write_lock> unlock{ lock };

            if (!outgoing_messages.empty()) slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sending " << outgoing_messages.size() << " websocket messages";

            for (auto& outgoing_message : outgoing_messages)
            {
                // hmmm, no way to cancel this currently...

                auto send = listener.send(outgoing_message.first, outgoing_message.second)
                    .then(details::observe_websocket_exception(gate));
                // current websocket_listener implementation is synchronous in any case, but just to make clear...
                // for now, wait for the message to be sent
                send.wait();
            }
        }
    }
}

#include "nmos/connection_resources.h"

#include <boost/logic/tribool.hpp>
#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/api_utils.h" // for nmos::http_scheme
#include "nmos/is05_versions.h"
#include "nmos/is07_versions.h"
#include "nmos/resource.h"

namespace nmos
{
    web::uri make_connection_api_transportfile(const nmos::id& sender_id, const nmos::settings& settings)
    {
        const auto version = *nmos::is05_versions::from_settings(settings).begin();

        return web::uri_builder()
            .set_scheme(nmos::http_scheme(settings))
            .set_host(nmos::get_host(settings))
            .set_port(nmos::fields::connection_port(settings))
            .set_path(U("/x-nmos/connection/") + make_api_version(version) + U("/single/senders/") + sender_id + U("/transportfile"))
            .to_uri();
    }

    namespace details
    {
        // IS-05 v1.1 doesn't limit the number of legs that a sender or receiver may have, but for all supported transport types
        // so far, there are either one or two (redundant) legs
        web::json::value legs_of(const web::json::value& value, bool redundant)
        {
            using web::json::value_of;
            return redundant ? value_of({ value, value }) : value_of({ value });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender-response-schema.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver-response-schema.json
        web::json::value make_connection_resource_staging_core(bool redundant)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::master_enable, false },
                { nmos::fields::activation, value_of({
                    { nmos::fields::mode, value::null() },
                    { nmos::fields::requested_time, value::null() },
                    { nmos::fields::activation_time, value::null() }
                }) },
                { nmos::fields::transport_params, legs_of(value::object(), redundant) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver-transport-file.json
        web::json::value make_connection_receiver_staging_transport_file()
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::data, value::null() },
                { nmos::fields::type, value::null() }
            });
        }

        web::json::value make_connection_resource_core(const nmos::id& id, bool redundant)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::id, id },
                { nmos::fields::device_id, U("these are not the droids you are looking for") },
                { nmos::fields::endpoint_constraints, legs_of(value::object(), redundant) },
                { nmos::fields::endpoint_staged, make_connection_resource_staging_core(redundant) },
                { nmos::fields::endpoint_active, make_connection_resource_staging_core(redundant) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#sender-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/constraints-schema-rtp.json
        web::json::value make_connection_rtp_sender_core_constraints()
        {
            using web::json::value;
            using web::json::value_of;

            const auto unconstrained = value::object();
            return value_of({
                { nmos::fields::source_ip, unconstrained },
                { nmos::fields::destination_ip, unconstrained },
                { nmos::fields::source_port, unconstrained },
                { nmos::fields::destination_port, unconstrained },
                { nmos::fields::rtp_enabled, unconstrained }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#sender-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_rtp.json
        web::json::value make_connection_rtp_sender_staged_core_parameter_set()
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::source_ip, U("auto") },
                { nmos::fields::destination_ip, U("auto") },
                { nmos::fields::source_port, U("auto") },
                { nmos::fields::destination_port, U("auto") },
                { nmos::fields::rtp_enabled, true }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#receiver-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/constraints-schema-rtp.json
        web::json::value make_connection_rtp_receiver_core_constraints()
        {
            using web::json::value;
            using web::json::value_of;

            const auto unconstrained = value::object();
            return value_of({
                { nmos::fields::source_ip, unconstrained },
                { nmos::fields::interface_ip, unconstrained },
                { nmos::fields::destination_port, unconstrained },
                { nmos::fields::rtp_enabled, unconstrained },
                { nmos::fields::multicast_ip, unconstrained }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#receiver-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_rtp.json
        web::json::value make_connection_rtp_receiver_staged_core_parameter_set()
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::source_ip, value::null() },
                { nmos::fields::interface_ip, U("auto") },
                { nmos::fields::destination_port, U("auto") },
                { nmos::fields::rtp_enabled, true },
                { nmos::fields::multicast_ip, value::null() }
            });
        }
    }

    nmos::resource make_connection_rtp_sender(const nmos::id& id, bool smpte2022_7)
    {
        using web::json::value;
        using web::json::value_of;

        auto data = details::make_connection_resource_core(id, smpte2022_7);

        data[nmos::fields::endpoint_constraints] = details::legs_of(details::make_connection_rtp_sender_core_constraints(), smpte2022_7);

        data[nmos::fields::endpoint_staged][nmos::fields::receiver_id] = value::null();
        data[nmos::fields::endpoint_staged][nmos::fields::transport_params] = details::legs_of(details::make_connection_rtp_sender_staged_core_parameter_set(), smpte2022_7);

        data[nmos::fields::endpoint_active] = data[nmos::fields::endpoint_staged];
        // The caller must resolve all instances of "auto" in the /active endpoint into the actual values that will be used!
        // See nmos::resolve_rtp_auto

        // Note that the transporttype endpoint is implemented in terms of the matching IS-04 sender

        return{ is05_versions::v1_1, types::sender, std::move(data), false };
    }

    web::json::value make_connection_rtp_sender_transportfile(const utility::string_t& transportfile)
    {
        using web::json::value;
        using web::json::value_of;

        return value_of({
            { nmos::fields::transportfile_data, transportfile },
            { nmos::fields::transportfile_type, U("application/sdp") }
        });
    }

    web::json::value make_connection_rtp_sender_transportfile(const web::uri& transportfile)
    {
        using web::json::value;
        using web::json::value_of;

        return value_of({
            { nmos::fields::transportfile_href, transportfile.to_string() }
        });
    }

    nmos::resource make_connection_rtp_sender(const nmos::id& id, bool smpte2022_7, const utility::string_t& transportfile)
    {
        auto resource = make_connection_rtp_sender(id, smpte2022_7);

        const utility::string_t sdp_magic(U("v=0"));

        if (sdp_magic == transportfile.substr(0, sdp_magic.size()))
        {
            resource.data[nmos::fields::endpoint_transportfile] = make_connection_rtp_sender_transportfile(transportfile);
        }
        else
        {
            resource.data[nmos::fields::endpoint_transportfile] = make_connection_rtp_sender_transportfile(web::uri(transportfile));
        }
        return resource;
    }

    nmos::resource make_connection_rtp_receiver(const nmos::id& id, bool smpte2022_7)
    {
        using web::json::value;

        auto data = details::make_connection_resource_core(id, smpte2022_7);

        data[nmos::fields::endpoint_constraints] = details::legs_of(details::make_connection_rtp_receiver_core_constraints(), smpte2022_7);

        data[nmos::fields::endpoint_staged][nmos::fields::sender_id] = value::null();
        data[nmos::fields::endpoint_staged][nmos::fields::transport_file] = details::make_connection_receiver_staging_transport_file();
        data[nmos::fields::endpoint_staged][nmos::fields::transport_params] = details::legs_of(details::make_connection_rtp_receiver_staged_core_parameter_set(), smpte2022_7);

        data[nmos::fields::endpoint_active] = data[nmos::fields::endpoint_staged];
        // The caller must resolve all instances of "auto" in the /active endpoint into the actual values that will be used!
        // See nmos::resolve_rtp_auto

        // Note that the transporttype endpoint is implemented in terms of the matching IS-04 receiver

        return{ is05_versions::v1_1, types::receiver, std::move(data), false };
    }

    namespace details
    {
        inline web::json::value boolean_or_unconstrained(boost::tribool v)
        {
            using web::json::value;
            using web::json::value_of;

            const auto unconstrained = value::object();
            return indeterminate(v) ? unconstrained : value_of({
                { nmos::fields::constraint_enum, value_of({
                    web::json::value::boolean(bool(v))
                }) }
            });
        }

        inline web::json::value boolean_or_auto(boost::tribool v)
        {
            return indeterminate(v) ? web::json::value::string(U("auto")) : web::json::value::boolean(bool(v));
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/constraints-schema-websocket.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_websocket.json
        web::json::value make_connection_websocket_sender_core_constraints(const web::uri& connection_uri, boost::tribool connection_authorization)
        {
            using web::json::value;
            using web::json::value_of;

            // connection_uri for a sender is currently fixed, basically read-only
            // see https://github.com/AMWA-TV/nmos-device-connection-management/issues/70
            return value_of({
                { nmos::fields::connection_uri, value_of({
                    { nmos::fields::constraint_enum, value_of({
                        connection_uri.to_string()
                    }) }
                }) },
                { nmos::fields::connection_authorization, boolean_or_unconstrained(connection_authorization) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.3.%20Behaviour%20-%20WebSocket%20Transport%20Type.md#sender-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_websocket.json
        web::json::value make_connection_websocket_sender_staged_core_parameter_set(const web::uri& connection_uri, boost::tribool connection_authorization)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::connection_uri, connection_uri.to_string() },
                { nmos::fields::connection_authorization, boolean_or_auto(connection_authorization) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/constraints-schema-websocket.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_websocket.json
        web::json::value make_connection_websocket_receiver_core_constraints(boost::tribool connection_authorization)
        {
            using web::json::value;
            using web::json::value_of;

            const auto unconstrained = value::object();
            return value_of({
                { nmos::fields::connection_uri, unconstrained },
                { nmos::fields::connection_authorization, boolean_or_unconstrained(connection_authorization) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.3.%20Behaviour%20-%20WebSocket%20Transport%20Type.md#receiver-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_websocket.json
        web::json::value make_connection_websocket_receiver_staged_core_parameter_set(boost::tribool connection_authorization)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::connection_uri, value::null() },
                { nmos::fields::connection_authorization, boolean_or_auto(connection_authorization) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/constraints-schema-mqtt.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_mqtt.json
        web::json::value make_connection_mqtt_sender_core_constraints(boost::tribool broker_secure, boost::tribool broker_authorization, const utility::string_t& broker_topic, const utility::string_t& connection_status_broker_topic)
        {
            using web::json::value;
            using web::json::value_of;

            const auto unconstrained = value::object();
            return value_of({
                { nmos::fields::destination_host, unconstrained },
                { nmos::fields::destination_port, unconstrained },
                { nmos::fields::broker_protocol, indeterminate(broker_secure) ? unconstrained : value_of({
                    { nmos::fields::constraint_enum, value_of({
                        nmos::mqtt_scheme(bool(broker_secure))
                    }) }
                }) },
                { nmos::fields::broker_authorization, boolean_or_unconstrained(broker_authorization) },
                { nmos::fields::broker_topic, broker_topic.empty() ? unconstrained : value_of({
                    { nmos::fields::constraint_enum, value_of({
                        broker_topic
                    }) }
                }) },
                { nmos::fields::connection_status_broker_topic, connection_status_broker_topic.empty() ? unconstrained : value_of({
                    { nmos::fields::constraint_enum, value_of({
                        connection_status_broker_topic
                    }) }
                }) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.2.%20Behaviour%20-%20MQTT%20Transport%20Type.md#sender-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/sender_transport_params_mqtt.json
        web::json::value make_connection_mqtt_sender_staged_core_parameter_set(boost::tribool broker_secure, boost::tribool broker_authorization, const utility::string_t& broker_topic, const utility::string_t& connection_status_broker_topic)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::destination_host, U("auto") },
                { nmos::fields::destination_port,  U("auto") },
                { nmos::fields::broker_protocol, indeterminate(broker_secure) ? U("auto") : nmos::mqtt_scheme(bool(broker_secure)) },
                { nmos::fields::broker_authorization, boolean_or_auto(broker_authorization) },
                { nmos::fields::broker_topic, broker_topic.empty() ? value::null() : value::string(broker_topic) },
                { nmos::fields::connection_status_broker_topic, connection_status_broker_topic.empty() ? value::null() : value::string(connection_status_broker_topic) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/constraints-schema-mqtt.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_mqtt.json
        web::json::value make_connection_mqtt_receiver_core_constraints(boost::tribool broker_secure, boost::tribool broker_authorization)
        {
            using web::json::value;
            using web::json::value_of;

            const auto unconstrained = value::object();
            return value_of({
                { nmos::fields::source_host, unconstrained },
                { nmos::fields::source_port, unconstrained },
                { nmos::fields::broker_protocol, indeterminate(broker_secure) ? unconstrained : value_of({
                    { nmos::fields::constraint_enum, value_of({
                        nmos::mqtt_scheme(bool(broker_secure))
                    }) }
                }) },
                { nmos::fields::broker_authorization, boolean_or_unconstrained(broker_authorization) },
                { nmos::fields::broker_topic, unconstrained },
                { nmos::fields::connection_status_broker_topic, unconstrained }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/docs/4.2.%20Behaviour%20-%20MQTT%20Transport%20Type.md#receiver-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1/APIs/schemas/receiver_transport_params_mqtt.json
        web::json::value make_connection_mqtt_receiver_staged_core_parameter_set(boost::tribool broker_secure, boost::tribool broker_authorization)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::source_host, U("auto") },
                { nmos::fields::source_port, U("auto") },
                { nmos::fields::broker_protocol, indeterminate(broker_secure) ? U("auto") : nmos::mqtt_scheme(bool(broker_secure)) },
                { nmos::fields::broker_authorization, boolean_or_auto(broker_authorization) },
                { nmos::fields::broker_topic, value::null() },
                { nmos::fields::connection_status_broker_topic, value::null() }
            });
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.2.%20Transport%20-%20Websocket.md#3-connection-management
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/sender_transport_params_ext.json
        web::json::value make_connection_events_websocket_sender_ext_constraints(const nmos::id& source_id, const web::uri& rest_api_url)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::ext_is_07_source_id, value_of({
                    { nmos::fields::constraint_enum, value_of({
                        source_id
                    }) }
                }) },
                { nmos::fields::ext_is_07_rest_api_url, value_of({
                    { nmos::fields::constraint_enum, value_of({
                        rest_api_url.to_string()
                    }) }
                }) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.2.%20Transport%20-%20Websocket.md#3-connection-management
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/sender_transport_params_ext.json
        web::json::value make_connection_events_websocket_sender_staged_ext_parameter_set(const nmos::id& source_id, const web::uri& rest_api_url)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::ext_is_07_source_id, source_id },
                { nmos::fields::ext_is_07_rest_api_url, rest_api_url.to_string() }
            });
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.2.%20Transport%20-%20Websocket.md#3-connection-management
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/receiver_transport_params_ext.json
        web::json::value make_connection_events_websocket_receiver_ext_constraints()
        {
            using web::json::value;
            using web::json::value_of;

            const auto unconstrained = value::object();
            return value_of({
                { nmos::fields::ext_is_07_source_id, unconstrained },
                { nmos::fields::ext_is_07_rest_api_url, unconstrained }
            });
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.2.%20Transport%20-%20Websocket.md#3-connection-management
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/receiver_transport_params_ext.json
        web::json::value make_connection_events_websocket_receiver_staged_ext_parameter_set()
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::ext_is_07_source_id, value::null() },
                { nmos::fields::ext_is_07_rest_api_url, value::null() }
            });
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.1.%20Transport%20-%20MQTT.md#3-connection-management
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/sender_transport_params_ext.json
        web::json::value make_connection_events_mqtt_sender_ext_constraints(const web::uri& rest_api_url)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::ext_is_07_rest_api_url, value_of({
                    { nmos::fields::constraint_enum, value_of({
                        rest_api_url.to_string()
                    }) }
                }) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.1.%20Transport%20-%20MQTT.md#3-connection-management
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/sender_transport_params_ext.json
        web::json::value make_connection_events_mqtt_sender_staged_ext_parameter_set(const web::uri& rest_api_url)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::ext_is_07_rest_api_url, rest_api_url.to_string() }
            });
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.1.%20Transport%20-%20MQTT.md#3-connection-management
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/receiver_transport_params_ext.json
        web::json::value make_connection_events_mqtt_receiver_ext_constraints()
        {
            using web::json::value;
            using web::json::value_of;

            const auto unconstrained = value::object();
            return value_of({
                { nmos::fields::ext_is_07_rest_api_url, unconstrained }
            });
        }

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.1.%20Transport%20-%20MQTT.md#3-connection-management
        // and https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/APIs/schemas/receiver_transport_params_ext.json
        web::json::value make_connection_events_mqtt_receiver_staged_ext_parameter_set()
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::ext_is_07_rest_api_url, value::null() }
            });
        }
    }

    // Although these functions make "connection" (IS-05) resources, the details are defined by IS-07 Event & Tally
    // so maybe these belong in nmos/events_resources.h or their own file, e.g. nmos/connection_events_resources.h?
    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.2.%20Transport%20-%20Websocket.md#3-connection-management
    nmos::resource make_connection_events_websocket_sender(const nmos::id& id, const nmos::id& device_id, const nmos::id& source_id, const nmos::settings& settings)
    {
        using web::json::value;
        using web::json::value_of;

        auto data = details::make_connection_resource_core(id, false);

        const auto connection_uri = make_events_ws_api_connection_uri(device_id, settings);
        const auto connection_authorization = false;
        const auto rest_api_url = make_events_api_ext_is_07_rest_api_url(source_id, settings);

        auto constraints = details::make_connection_websocket_sender_core_constraints(connection_uri, connection_authorization);
        auto ext_constraints = details::make_connection_events_websocket_sender_ext_constraints(source_id, rest_api_url);
        web::json::insert(constraints, ext_constraints.as_object().begin(), ext_constraints.as_object().end());
        data[nmos::fields::endpoint_constraints] = details::legs_of(constraints, false);

        data[nmos::fields::endpoint_staged][nmos::fields::receiver_id] = value::null();

        auto transport_params = details::make_connection_websocket_sender_staged_core_parameter_set(connection_uri, connection_authorization);
        auto ext_transport_params = details::make_connection_events_websocket_sender_staged_ext_parameter_set(source_id, rest_api_url);
        web::json::insert(transport_params, ext_transport_params.as_object().begin(), ext_transport_params.as_object().end());
        data[nmos::fields::endpoint_staged][nmos::fields::transport_params] = details::legs_of(transport_params, false);

        data[nmos::fields::endpoint_active] = data[nmos::fields::endpoint_staged];

        return{ is05_versions::v1_1, types::sender, std::move(data), false };
    }

    nmos::resource make_connection_events_websocket_receiver(const nmos::id& id, const nmos::settings& settings)
    {
        using web::json::value;
        using web::json::value_of;

        auto data = details::make_connection_resource_core(id, false);

        const auto connection_authorization = false;

        auto constraints = details::make_connection_websocket_receiver_core_constraints(connection_authorization);
        auto ext_constraints = details::make_connection_events_websocket_receiver_ext_constraints();
        web::json::insert(constraints, ext_constraints.as_object().begin(), ext_constraints.as_object().end());
        data[nmos::fields::endpoint_constraints] = details::legs_of(constraints, false);

        data[nmos::fields::endpoint_staged][nmos::fields::sender_id] = value::null();

        data[nmos::fields::endpoint_staged][nmos::fields::transport_file] = details::make_connection_receiver_staging_transport_file();

        auto transport_params = details::make_connection_websocket_receiver_staged_core_parameter_set(connection_authorization);
        auto ext_transport_params = details::make_connection_events_websocket_receiver_staged_ext_parameter_set();
        web::json::insert(transport_params, ext_transport_params.as_object().begin(), ext_transport_params.as_object().end());
        data[nmos::fields::endpoint_staged][nmos::fields::transport_params] = details::legs_of(transport_params, false);

        data[nmos::fields::endpoint_active] = data[nmos::fields::endpoint_staged];

        return{ is05_versions::v1_1, types::receiver, std::move(data), false };
    }

    web::uri make_events_ws_api_connection_uri(const nmos::id& device_id, const nmos::settings& settings)
    {
        const auto version = *nmos::is07_versions::from_settings(settings).rbegin();

        // hmm, since only one WebSocket server is used for all devices, there's no real reason
        // that the connection_uri shouldn't just finish with the API version?
        // see https://github.com/AMWA-TV/nmos-event-tally/issues/33
        return web::uri_builder()
            .set_scheme(nmos::ws_scheme(settings))
            .set_host(nmos::get_host(settings))
            .set_port(nmos::fields::events_ws_port(settings))
            .set_path(U("/x-nmos/events/") + make_api_version(version) + U("/devices/") + device_id)
            .to_uri();
    }

    web::uri make_events_api_ext_is_07_rest_api_url(const nmos::id& source_id, const nmos::settings& settings)
    {
        const auto version = *nmos::is07_versions::from_settings(settings).rbegin();

        // "The sender should append the relative path sources/{source_id}/"
        // I'd rather be consistent with the general guidance regarding trailing slashes
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.1/docs/4.0.%20Core%20models.md#ext_is_07_rest_api_url
        return web::uri_builder()
            .set_scheme(nmos::http_scheme(settings))
            .set_host(nmos::get_host(settings))
            .set_port(nmos::fields::events_port(settings))
            .set_path(U("/x-nmos/events/") + make_api_version(version) + U("/sources/") + source_id)
            .to_uri();
    }

    utility::string_t make_events_mqtt_broker_topic(const nmos::id& source_id, const nmos::settings& settings)
    {
        const auto version = *nmos::is07_versions::from_settings(settings).rbegin();

        // "To facilitate filtering, the recommended format is x-nmos/events/{version}/sources/{sourceId},
        // where {version} is the version of this specification, e.g. v1.0, and {sourceId} is the associated source id."
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.1/docs/5.1.%20Transport%20-%20MQTT.md#32-broker_topic
        return U("x-nmos/events/") + make_api_version(version) + U("/sources/") + source_id;
    }

    utility::string_t make_events_mqtt_connection_status_broker_topic(const nmos::id& connection_id, const nmos::settings& settings)
    {
        const auto version = *nmos::is07_versions::from_settings(settings).rbegin();

        // "The connection_status_broker_topic parameter holds the sender's MQTT connection status topic.
        // The recommended format is x-nmos/events/{version}/connections/{connectionId}."
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.1/docs/5.1.%20Transport%20-%20MQTT.md#33-connection_status_broker_topic
        return U("x-nmos/events/") + make_api_version(version) + U("/connections/") + connection_id;
    }
}

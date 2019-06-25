#include "nmos/connection_resources.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/api_utils.h" // for nmos::http_scheme
#include "nmos/connection_api.h" // for nmos::resolve_auto
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
        web::json::value legs_of(const web::json::value& value, bool smpte2022_7)
        {
            using web::json::value_of;
            return smpte2022_7 ? value_of({ value, value }) : value_of({ value });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-sender-response-schema.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-receiver-response-schema.json
        web::json::value make_connection_resource_staging_core(bool smpte2022_7)
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
                { nmos::fields::transport_params, legs_of(value::object(), smpte2022_7) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-receiver-response-schema.json
        web::json::value make_connection_receiver_staging_transport_file()
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::data, value::null() },
                { nmos::fields::type, U("application/sdp") }
            });
        }

        web::json::value make_connection_resource_core(const nmos::id& id, bool smpte2022_7)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::id, id },
                { nmos::fields::device_id, U("these are not the droids you are looking for") },
                { nmos::fields::endpoint_constraints, legs_of(value::object(), smpte2022_7) },
                { nmos::fields::endpoint_staged, make_connection_resource_staging_core(smpte2022_7) },
                { nmos::fields::endpoint_active, make_connection_resource_staging_core(smpte2022_7) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#sender-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-constraints-schema.json
        web::json::value make_connection_sender_core_constraints()
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

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#sender-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0_sender_transport_params_rtp.json
        web::json::value make_connection_sender_staged_core_parameter_set()
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

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#receiver-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-constraints-schema.json
        web::json::value make_connection_receiver_core_constraints()
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

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/4.1.%20Behaviour%20-%20RTP%20Transport%20Type.md#receiver-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0_receiver_transport_params_rtp.json
        web::json::value make_connection_receiver_staged_core_parameter_set()
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

    nmos::resource make_connection_sender(const nmos::id& id, bool smpte2022_7)
    {
        using web::json::value;
        using web::json::value_of;

        auto data = details::make_connection_resource_core(id, smpte2022_7);

        data[nmos::fields::endpoint_constraints] = details::legs_of(details::make_connection_sender_core_constraints(), smpte2022_7);

        data[nmos::fields::endpoint_staged][nmos::fields::receiver_id] = value::null();
        data[nmos::fields::endpoint_staged][nmos::fields::transport_params] = details::legs_of(details::make_connection_sender_staged_core_parameter_set(), smpte2022_7);

        data[nmos::fields::endpoint_active] = data[nmos::fields::endpoint_staged];
        // All instances of "auto" should be resolved into the actual values that will be used
        // but in some cases the behaviour is more complex, and may be determined by the vendor.
        // This function does not select a value for e.g. sender "source_ip" or receiver "interface_ip".
        nmos::resolve_auto(types::sender, data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

        // Note that the transporttype endpoint is implemented in terms of the matching IS-04 sender

        return{ is05_versions::v1_1, types::sender, data, false };
    }

    web::json::value make_connection_sender_transportfile(const utility::string_t& transportfile)
    {
        using web::json::value;
        using web::json::value_of;

        return value_of({
            { nmos::fields::transportfile_data, transportfile },
            { nmos::fields::transportfile_type, U("application/sdp") }
        });
    }

    web::json::value make_connection_sender_transportfile(const web::uri& transportfile)
    {
        using web::json::value;
        using web::json::value_of;

        return value_of({
            { nmos::fields::transportfile_href, transportfile.to_string() }
        });
    }

    nmos::resource make_connection_sender(const nmos::id& id, bool smpte2022_7, const utility::string_t& transportfile)
    {
        auto resource = make_connection_sender(id, smpte2022_7);

        const utility::string_t sdp_magic(U("v=0"));

        if (sdp_magic == transportfile.substr(0, sdp_magic.size()))
        {
            resource.data[nmos::fields::endpoint_transportfile] = make_connection_sender_transportfile(transportfile);
        }
        else
        {
            resource.data[nmos::fields::endpoint_transportfile] = make_connection_sender_transportfile(web::uri(transportfile));
        }
        return resource;
    }

    nmos::resource make_connection_receiver(const nmos::id& id, bool smpte2022_7)
    {
        using web::json::value;

        auto data = details::make_connection_resource_core(id, smpte2022_7);

        data[nmos::fields::endpoint_constraints] = details::legs_of(details::make_connection_receiver_core_constraints(), smpte2022_7);

        data[nmos::fields::endpoint_staged][nmos::fields::sender_id] = value::null();
        data[nmos::fields::endpoint_staged][nmos::fields::transport_file] = details::make_connection_receiver_staging_transport_file();
        data[nmos::fields::endpoint_staged][nmos::fields::transport_params] = details::legs_of(details::make_connection_receiver_staged_core_parameter_set(), smpte2022_7);

        data[nmos::fields::endpoint_active] = data[nmos::fields::endpoint_staged];
        // All instances of "auto" should be resolved into the actual values that will be used
        // but in some cases the behaviour is more complex, and may be determined by the vendor.
        // This function does not select a value for e.g. sender "source_ip" or receiver "interface_ip".
        nmos::resolve_auto(types::receiver, data[nmos::fields::endpoint_active][nmos::fields::transport_params]);

        // Note that the transporttype endpoint is implemented in terms of the matching IS-04 receiver

        return{ is05_versions::v1_1, types::receiver, data, false };
    }

    namespace details
    {
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/APIs/schemas/constraints-schema-websocket.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/APIs/schemas/sender_transport_params_websocket.json
        web::json::value make_connection_websocket_sender_core_constraints(const web::uri& connection_uri)
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
                }) }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/docs/4.3.%20Behaviour%20-%20WebSocket%20Transport%20Type.md#sender-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/APIs/schemas/sender_transport_params_websocket.json
        web::json::value make_connection_websocket_sender_staged_core_parameter_set(const web::uri& connection_uri)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::connection_uri, connection_uri.to_string() }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/APIs/schemas/constraints-schema-websocket.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/APIs/schemas/receiver_transport_params_websocket.json
        web::json::value make_connection_websocket_receiver_core_constraints()
        {
            using web::json::value;
            using web::json::value_of;

            const auto unconstrained = value::object();
            return value_of({
                { nmos::fields::connection_uri, unconstrained }
            });
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/docs/4.3.%20Behaviour%20-%20WebSocket%20Transport%20Type.md#receiver-parameter-sets
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1-dev/APIs/schemas/receiver_transport_params_websocket.json
        web::json::value make_connection_websocket_receiver_staged_core_parameter_set()
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::connection_uri, value::null() }
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
        const auto rest_api_url = make_events_api_ext_is_07_rest_api_url(source_id, settings);

        auto constraints = details::make_connection_websocket_sender_core_constraints(connection_uri);
        auto ext_constraints = details::make_connection_events_websocket_sender_ext_constraints(source_id, rest_api_url);
        web::json::insert(constraints, ext_constraints.as_object().begin(), ext_constraints.as_object().end());

        data[nmos::fields::endpoint_constraints] = details::legs_of(constraints, false);
        data[nmos::fields::endpoint_staged][nmos::fields::receiver_id] = value::null();
        data[nmos::fields::endpoint_staged][nmos::fields::master_enable] = value::boolean(false);

        auto transport_params = details::make_connection_websocket_sender_staged_core_parameter_set(connection_uri);
        auto ext_transport_params = details::make_connection_events_websocket_sender_staged_ext_parameter_set(source_id, rest_api_url);
        web::json::insert(transport_params, ext_transport_params.as_object().begin(), ext_transport_params.as_object().end());

        data[nmos::fields::endpoint_staged][nmos::fields::transport_params] = details::legs_of(transport_params, false);
        data[nmos::fields::endpoint_active] = data[nmos::fields::endpoint_staged];

        return{ is05_versions::v1_1, types::sender, data, false };
    }

    nmos::resource make_connection_events_websocket_receiver(const nmos::id& id, const nmos::settings& settings)
    {
        using web::json::value;
        using web::json::value_of;

        auto data = details::make_connection_resource_core(id, false);

        auto constraints = details::make_connection_websocket_receiver_core_constraints();
        auto ext_constraints = details::make_connection_events_websocket_receiver_ext_constraints();
        web::json::insert(constraints, ext_constraints.as_object().begin(), ext_constraints.as_object().end());

        data[nmos::fields::endpoint_constraints] = details::legs_of(constraints, false);
        data[nmos::fields::endpoint_staged][nmos::fields::receiver_id] = value::null();
        data[nmos::fields::endpoint_staged][nmos::fields::master_enable] = value::boolean(false);

        auto transport_params = details::make_connection_websocket_receiver_staged_core_parameter_set();
        auto ext_transport_params = details::make_connection_events_websocket_receiver_staged_ext_parameter_set();
        web::json::insert(transport_params, ext_transport_params.as_object().begin(), ext_transport_params.as_object().end());

        data[nmos::fields::endpoint_staged][nmos::fields::transport_params] = details::legs_of(transport_params, false);
        data[nmos::fields::endpoint_active] = data[nmos::fields::endpoint_staged];

        return{ is05_versions::v1_1, types::receiver, data, false };
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
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0.x/docs/4.0.%20Core%20models.md#ext_is_07_rest_api_url
        return web::uri_builder()
            .set_scheme(nmos::http_scheme(settings))
            .set_host(nmos::get_host(settings))
            .set_port(nmos::fields::events_port(settings))
            .set_path(U("/x-nmos/events/") + make_api_version(version) + U("/sources/") + source_id)
            .to_uri();
    }
}

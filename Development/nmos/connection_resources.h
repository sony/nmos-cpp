#ifndef NMOS_CONNECTION_RESOURCES_H
#define NMOS_CONNECTION_RESOURCES_H

#include "nmos/id.h"
#include "nmos/settings.h"

namespace web
{
    class uri;
}

namespace nmos
{
    struct resource;

    // Optional IS-05 RTP parameter sets supported by a Sender or Receiver.
    // Core parameters are always present; the _core enumerator is a zero-value call-site token
    // when no optional sets are selected. The no-argument factory overloads preserve historical
    // defaults (sender: core only; receiver: core + multicast).
    // rtp_sender_parameter_sets is a bitmask
    enum rtp_sender_parameter_sets
    {
        rtp_sender_parameter_sets_core = 0x0000,
        rtp_sender_parameter_sets_fec = 0x0001,
        rtp_sender_parameter_sets_rtcp = 0x0002
    };
    // so for some convenience...
    inline rtp_sender_parameter_sets operator|(rtp_sender_parameter_sets lhs, rtp_sender_parameter_sets rhs)
    {
        return rtp_sender_parameter_sets((int)lhs | (int)rhs);
    }

    // rtp_receiver_parameter_sets is a bitmask
    enum rtp_receiver_parameter_sets
    {
        rtp_receiver_parameter_sets_core = 0x0000,
        rtp_receiver_parameter_sets_multicast = 0x0001,
        rtp_receiver_parameter_sets_fec = 0x0002,
        rtp_receiver_parameter_sets_rtcp = 0x0004
    };
    // so for some convenience...
    inline rtp_receiver_parameter_sets operator|(rtp_receiver_parameter_sets lhs, rtp_receiver_parameter_sets rhs)
    {
        return rtp_receiver_parameter_sets((int)lhs | (int)rhs);
    }

    // make an absolute URL for the /transportfile endpoint of the specified sender
    // e.g. for use in the manifest_href property of the IS-04 Node API sender
    web::uri make_connection_api_transportfile(const nmos::id& sender_id, const nmos::settings& settings);

    // IS-05 Connection API resources
    // "The UUIDs used to advertise Senders and Receivers in the Connection Management API must match
    // those used in a corresponding IS-04 implementation."
    // See https://specs.amwa.tv/is-05/releases/v1.0.0/docs/3.1._Interoperability_-_NMOS_IS-04.html#sender--receiver-ids
    // Whereas the data of the IS-04 resources corresponds to a particular Node API resource endpoint,
    // each IS-05 resource's data is a json object with an "id" field and a field for each Connection API
    // endpoint of that logical single resource
    // i.e.
    // a "constraints" field, which must have an array value conforming to the constraints-schema,
    // "staged" and "active" fields, which must each have a value conforming to the sender-response-schema or receiver-response-schema,
    // and for senders, also a "transportfile" field, the value of which must be an object, with either
    // "data" and "type" fields, or an "href" field
    // See https://specs.amwa.tv/is-05/releases/v1.1.0/APIs/schemas/with-refs/constraints-schema.html
    // and https://specs.amwa.tv/is-05/releases/v1.1.0/APIs/schemas/with-refs/sender-response-schema.html
    // and https://specs.amwa.tv/is-05/releases/v1.1.0/APIs/schemas/with-refs/receiver-response-schema.html

    // The caller must resolve all instances of "auto" in the /active endpoint into the actual values that will be used!
    // See nmos::resolve_rtp_auto
    nmos::resource make_connection_rtp_sender(const nmos::id& id, bool smpte2022_7);
    nmos::resource make_connection_rtp_sender(const nmos::id& id, bool smpte2022_7, rtp_sender_parameter_sets parameter_sets);

    web::json::value make_connection_rtp_sender_transportfile(const utility::string_t& transportfile);
    web::json::value make_connection_rtp_sender_transportfile(const web::uri& transportfile);

    // The caller must resolve all instances of "auto" in the /active endpoint into the actual values that will be used!
    // See nmos::resolve_rtp_auto
    // transportfile may be URL or the contents of the SDP file (yeah, yuck)
    nmos::resource make_connection_rtp_sender(const nmos::id& id, bool smpte2022_7, const utility::string_t& transportfile);
    nmos::resource make_connection_rtp_sender(const nmos::id& id, bool smpte2022_7, rtp_sender_parameter_sets parameter_sets, const utility::string_t& transportfile);

    // The caller must resolve all instances of "auto" in the /active endpoint into the actual values that will be used!
    // See nmos::resolve_rtp_auto
    nmos::resource make_connection_rtp_receiver(const nmos::id& id, bool smpte2022_7);
    nmos::resource make_connection_rtp_receiver(const nmos::id& id, bool smpte2022_7, rtp_receiver_parameter_sets parameter_sets);

    // Although these functions make "connection" (IS-05) resources, the details are defined by IS-07 Event & Tally
    // so maybe these belong in nmos/events_resources.h or their own file, e.g. nmos/connection_events_resources.h?
    // See https://specs.amwa.tv/is-07/releases/v1.0.1/docs/5.2._Transport_-_Websocket.html#3-connection-management
    nmos::resource make_connection_events_websocket_sender(const nmos::id& id, const nmos::id& device_id, const nmos::id& source_id, const nmos::settings& settings);
    nmos::resource make_connection_events_websocket_receiver(const nmos::id& id, const nmos::settings& settings);

    web::uri make_events_ws_api_connection_uri(const nmos::id& device_id, const nmos::settings& settings);
    web::uri make_events_api_ext_is_07_rest_api_url(const nmos::id& source_id, const nmos::settings& settings);

    utility::string_t make_events_mqtt_broker_topic(const nmos::id& source_id, const nmos::settings& settings);
    utility::string_t make_events_mqtt_connection_status_broker_topic(const nmos::id& connection_id, const nmos::settings& settings);

    nmos::resource make_connection_mxl_sender(const nmos::id& id, const nmos::id& mxl_domain_id, const nmos::id& mxl_flow_id);
    nmos::resource make_connection_mxl_receiver(const nmos::id& id, const nmos::id& mxl_domain_id);
}

#endif

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

    web::json::value make_connection_rtp_sender_transportfile(const utility::string_t& transportfile);
    web::json::value make_connection_rtp_sender_transportfile(const web::uri& transportfile);

    // The caller must resolve all instances of "auto" in the /active endpoint into the actual values that will be used!
    // See nmos::resolve_rtp_auto
    // transportfile may be URL or the contents of the SDP file (yeah, yuck)
    nmos::resource make_connection_rtp_sender(const nmos::id& id, bool smpte2022_7, const utility::string_t& transportfile);

    // The caller must resolve all instances of "auto" in the /active endpoint into the actual values that will be used!
    // See nmos::resolve_rtp_auto
    nmos::resource make_connection_rtp_receiver(const nmos::id& id, bool smpte2022_7);

    // Although these functions make "connection" (IS-05) resources, the details are defined by IS-07 Event & Tally
    // so maybe these belong in nmos/events_resources.h or their own file, e.g. nmos/connection_events_resources.h?
    // See https://specs.amwa.tv/is-07/releases/v1.0.1/docs/5.2._Transport_-_Websocket.html#3-connection-management
    nmos::resource make_connection_events_websocket_sender(const nmos::id& id, const nmos::id& device_id, const nmos::id& source_id, const nmos::settings& settings);
    nmos::resource make_connection_events_websocket_receiver(const nmos::id& id, const nmos::settings& settings);

    web::uri make_events_ws_api_connection_uri(const nmos::id& device_id, const nmos::settings& settings);
    web::uri make_events_api_ext_is_07_rest_api_url(const nmos::id& source_id, const nmos::settings& settings);

    utility::string_t make_events_mqtt_broker_topic(const nmos::id& source_id, const nmos::settings& settings);
    utility::string_t make_events_mqtt_connection_status_broker_topic(const nmos::id& connection_id, const nmos::settings& settings);
}

#endif

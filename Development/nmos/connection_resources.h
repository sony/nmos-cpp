#ifndef NMOS_CONNECTION_RESOURCES_H
#define NMOS_CONNECTION_RESOURCES_H

#include "nmos/id.h"
#include "nmos/settings.h"

namespace nmos
{
    // make an absolute URL for the /transportfile endpoint of the specified sender
    // e.g. for use in the manifest_href property of the IS-04 Node API sender
    web::uri make_connection_api_transportfile(const nmos::id& sender_id, const nmos::settings& settings);

    // IS-05 Connection API resources
    // "The UUIDs used to advertise Senders and Receivers in the Connection Management API must match
    // those used in a corresponding IS-04 implementation."
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/3.1.%20Interoperability%20-%20NMOS%20IS-04.md#sender--receiver-ids
    // Whereas the data of the IS-04 resources corresponds to a particular Node API resource endpoint,
    // each IS-05 resource's data is a json object with an "id" field and a field for each Connection API
    // endpoint of that logical single resource
    // i.e.
    // a "constraints" field, which must have an array value conforming to the v1.0-constraints-schema,
    // "staged" and "active" fields, which must each have a value conforming to the v1.0-sender-response-schema or v1.0-receiver-response-schema,
    // and for senders, also a "transportfile" field, the value of which must be an object, with either
    // "data" and "type" fields, or an "href" field
    // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-constraints-schema.json
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-sender-response-schema.json
    // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-receiver-response-schema.json

    nmos::resource make_connection_sender(const nmos::id& id, bool smpte2022_7);
    web::json::value make_connection_sender_transportfile(const utility::string_t& transportfile);
    web::json::value make_connection_sender_transportfile(const web::uri& transportfile);
    // transportfile may be URL or the contents of the SDP file (yeah, yuck)
    nmos::resource make_connection_sender(const nmos::id& id, bool smpte2022_7, const utility::string_t& transportfile);

    nmos::resource make_connection_receiver(const nmos::id& id, bool smpte2022_7);
}

#endif

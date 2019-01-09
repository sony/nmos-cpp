#ifndef NMOS_JSON_SCHEMA_H
#define NMOS_JSON_SCHEMA_H

#include "cpprest/base_uri.h"
#include "cpprest/json.h"

namespace nmos
{
    struct api_version;
    struct type;

    namespace experimental
    {
        web::uri make_registrationapi_resource_post_request_schema_uri(const nmos::api_version& version);
        web::uri make_queryapi_subscriptions_post_request_schema_uri(const nmos::api_version& version);

        web::uri make_nodeapi_receiver_target_put_request_schema_uri(const nmos::api_version& version);
        web::uri make_connectionapi_staged_patch_request_schema_uri(const nmos::api_version& version, const nmos::type& type);

        // load the json schema for the specified base URI
        web::json::value load_json_schema(const web::uri& id);
    }
}

#endif

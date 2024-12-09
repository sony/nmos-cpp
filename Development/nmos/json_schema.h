#ifndef NMOS_JSON_SCHEMA_H
#define NMOS_JSON_SCHEMA_H

#include "cpprest/base_uri.h"
#include "cpprest/json.h"

namespace nmos
{
    struct api_version;
    struct type;

    namespace details
    {
        std::map<web::uri, web::json::value> make_schemas();
    }

    namespace experimental
    {
        web::uri make_systemapi_global_schema_uri(const nmos::api_version& version);

        web::uri make_registrationapi_resource_post_request_schema_uri(const nmos::api_version& version);
        web::uri make_queryapi_subscriptions_post_request_schema_uri(const nmos::api_version& version);

        web::uri make_nodeapi_receiver_target_put_request_schema_uri(const nmos::api_version& version);

        web::uri make_connectionapi_staged_patch_request_schema_uri(const nmos::api_version& version, const nmos::type& type);
        web::uri make_connectionapi_sender_staged_patch_request_schema_uri(const nmos::api_version& version);
        web::uri make_connectionapi_receiver_staged_patch_request_schema_uri(const nmos::api_version& version);

        web::uri make_channelmappingapi_map_activations_post_request_schema_uri(const nmos::api_version& version);

        web::uri make_authapi_auth_metadata_schema_uri(const nmos::api_version& version);
        web::uri make_authapi_jwks_response_schema_uri(const nmos::api_version& version);
        web::uri make_authapi_register_client_response_uri(const nmos::api_version& version);
        web::uri make_authapi_token_error_response_uri(const nmos::api_version& version);
        web::uri make_authapi_token_schema_schema_uri(const nmos::api_version& version);
        web::uri make_authapi_token_response_schema_uri(const nmos::api_version& version);

        web::uri make_controlprotocolapi_base_message_schema_uri(const nmos::api_version& version);
        web::uri make_controlprotocolapi_command_message_schema_uri(const nmos::api_version& version);
        web::uri make_controlprotocolapi_subscription_message_schema_uri(const nmos::api_version& version);

        web::uri make_configurationapi_bulkProperties_set_request_schema_uri(const nmos::api_version& version);
        web::uri make_configurationapi_bulkProperties_validate_request_schema_uri(const nmos::api_version& version);
        web::uri make_configurationapi_method_patch_request_schema_uri(const nmos::api_version& version);
        web::uri make_configurationapi_property_value_put_request_schema_uri(const nmos::api_version& version);

        // load the json schema for the specified base URI
        web::json::value load_json_schema(const web::uri& id);
    }
}

#endif

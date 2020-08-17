#include "nmos/json_schema.h"

#include "cpprest/basic_utils.h"
#include "nmos/is04_versions.h"
#include "nmos/is04_schemas/is04_schemas.h"
#include "nmos/is05_versions.h"
#include "nmos/is05_schemas/is05_schemas.h"
#include "nmos/is08_versions.h"
#include "nmos/is08_schemas/is08_schemas.h"
#include "nmos/is09_versions.h"
#include "nmos/is09_schemas/is09_schemas.h"
#include "nmos/type.h"

namespace nmos
{
    namespace is04_schemas
    {
        web::uri make_schema_uri(const utility::string_t& tag, const utility::string_t& ref = {})
        {
            return{ _XPLATSTR("https://github.com/AMWA-TV/nmos-discovery-registration/raw/") + tag + _XPLATSTR("/APIs/schemas/") + ref };
        }

        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3.x/APIs/schemas/
        namespace v1_3
        {
            using namespace nmos::is04_schemas::v1_3_x;
            const utility::string_t tag(_XPLATSTR("v1.3.x"));

            const web::uri registrationapi_resource_post_request_uri = make_schema_uri(tag, _XPLATSTR("registrationapi-resource-post-request.json"));
            const web::uri queryapi_subscriptions_post_request_uri = make_schema_uri(tag, _XPLATSTR("queryapi-subscriptions-post-request.json"));
            const web::uri nodeapi_receiver_target_put_request_uri = make_schema_uri(tag, _XPLATSTR("nodeapi-receiver-target.json"));
        }

        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2.x/APIs/schemas/
        namespace v1_2
        {
            using namespace nmos::is04_schemas::v1_2_x;
            const utility::string_t tag(_XPLATSTR("v1.2.x"));

            const web::uri registrationapi_resource_post_request_uri = make_schema_uri(tag, _XPLATSTR("registrationapi-resource-post-request.json"));
            const web::uri queryapi_subscriptions_post_request_uri = make_schema_uri(tag, _XPLATSTR("queryapi-subscriptions-post-request.json"));
            const web::uri nodeapi_receiver_target_put_request_uri = make_schema_uri(tag, _XPLATSTR("nodeapi-receiver-target.json"));
        }

        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.1.x/APIs/schemas/
        namespace v1_1
        {
            using namespace nmos::is04_schemas::v1_1_x;
            const utility::string_t tag(_XPLATSTR("v1.1.x"));

            const web::uri registrationapi_resource_post_request_uri = make_schema_uri(tag, _XPLATSTR("registrationapi-resource-post-request.json"));
            const web::uri queryapi_subscriptions_post_request_uri = make_schema_uri(tag, _XPLATSTR("queryapi-subscriptions-post-request.json"));
            const web::uri nodeapi_receiver_target_put_request_uri = make_schema_uri(tag, _XPLATSTR("nodeapi-receiver-target.json"));
        }

        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.0.x/APIs/schemas/
        namespace v1_0
        {
            using namespace nmos::is04_schemas::v1_0_x;
            const utility::string_t tag(_XPLATSTR("v1.0.x"));

            const web::uri registrationapi_resource_post_request_uri = make_schema_uri(tag, _XPLATSTR("registrationapi-v1.0-resource-post-request.json"));
            const web::uri queryapi_subscriptions_post_request_uri = make_schema_uri(tag, _XPLATSTR("queryapi-v1.0-subscriptions-post-request.json"));
            const web::uri nodeapi_receiver_target_put_request_uri = make_schema_uri(tag, _XPLATSTR("nodeapi-receiver-target.json"));
        }
    }

    namespace is05_schemas
    {
        web::uri make_schema_uri(const utility::string_t& tag, const utility::string_t& ref = {})
        {
            return{ _XPLATSTR("https://github.com/AMWA-TV/nmos-device-connection-management/raw/") + tag + _XPLATSTR("/APIs/schemas/") + ref };
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.1.x/APIs/schemas/
        namespace v1_1
        {
            using namespace nmos::is05_schemas::v1_1_x;
            const utility::string_t tag(_XPLATSTR("v1.1.x"));

            const web::uri connectionapi_sender_staged_patch_request_uri = make_schema_uri(tag, _XPLATSTR("sender-stage-schema.json"));
            const web::uri connectionapi_receiver_staged_patch_request_uri = make_schema_uri(tag, _XPLATSTR("receiver-stage-schema.json"));
        }

        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0.x/APIs/schemas/
        namespace v1_0
        {
            using namespace nmos::is05_schemas::v1_0_x;
            const utility::string_t tag(_XPLATSTR("v1.0.x"));

            const web::uri connectionapi_sender_staged_patch_request_uri = make_schema_uri(tag, _XPLATSTR("v1.0-sender-stage-schema.json"));
            const web::uri connectionapi_receiver_staged_patch_request_uri = make_schema_uri(tag, _XPLATSTR("v1.0-receiver-stage-schema.json"));
        }
    }

    namespace is08_schemas
    {
        web::uri make_schema_uri(const utility::string_t& tag, const utility::string_t& ref = {})
        {
            return{ _XPLATSTR("https://github.com/AMWA-TV/nmos-audio-channel-mapping/raw/") + tag + _XPLATSTR("/APIs/schemas/") + ref };
        }

        namespace v1_0
        {
            using namespace nmos::is08_schemas::v1_0_x;
            const utility::string_t tag(_XPLATSTR("v1.0.x"));

            const web::uri map_activations_post_request_uri = make_schema_uri(tag, _XPLATSTR("map-activations-post-request-schema.json"));
        }
    }

    namespace is09_schemas
    {
        web::uri make_schema_uri(const utility::string_t& tag, const utility::string_t& ref = {})
        {
            return{ _XPLATSTR("https://github.com/AMWA-TV/nmos-system/raw/") + tag + _XPLATSTR("/APIs/schemas/") + ref };
        }

        // See https://github.com/AMWA-TV/nmos-system/blob/v1.0.x/APIs/schemas/
        namespace v1_0
        {
            using namespace nmos::is09_schemas::v1_0_x;
            const utility::string_t tag(_XPLATSTR("v1.0.x"));

            const web::uri systemapi_global_schema_uri = make_schema_uri(tag, _XPLATSTR("global.json"));
        }
    }
}

namespace nmos
{
    namespace details
    {
        static web::json::value make_schema(const char* schema)
        {
            return web::json::value::parse(utility::s2us(schema));
        }

        static std::map<web::uri, web::json::value> make_is04_schemas()
        {
            using namespace nmos::is04_schemas;

            return
            {
                // v1.3
                { make_schema_uri(v1_3::tag, _XPLATSTR("registrationapi-resource-post-request.json")), make_schema(v1_3::registrationapi_resource_post_request) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("clock_internal.json")), make_schema(v1_3::clock_internal) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("clock_ptp.json")), make_schema(v1_3::clock_ptp) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("resource_core.json")), make_schema(v1_3::resource_core) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("device.json")), make_schema(v1_3::device) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow.json")), make_schema(v1_3::flow) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_video_raw.json")), make_schema(v1_3::flow_video_raw) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_video_coded.json")), make_schema(v1_3::flow_video_coded) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_audio_raw.json")), make_schema(v1_3::flow_audio_raw) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_audio_coded.json")), make_schema(v1_3::flow_audio_coded) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_data.json")), make_schema(v1_3::flow_data) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_json_data.json")), make_schema(v1_3::flow_json_data) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_sdianc_data.json")), make_schema(v1_3::flow_sdianc_data) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_mux.json")), make_schema(v1_3::flow_mux) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_audio.json")), make_schema(v1_3::flow_audio) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_core.json")), make_schema(v1_3::flow_core) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("flow_video.json")), make_schema(v1_3::flow_video) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("node.json")), make_schema(v1_3::node) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("receiver.json")), make_schema(v1_3::receiver) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("receiver_video.json")), make_schema(v1_3::receiver_video) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("receiver_audio.json")), make_schema(v1_3::receiver_audio) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("receiver_data.json")), make_schema(v1_3::receiver_data) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("receiver_mux.json")), make_schema(v1_3::receiver_mux) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("receiver_core.json")), make_schema(v1_3::receiver_core) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("sender.json")), make_schema(v1_3::sender) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("source.json")), make_schema(v1_3::source) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("source_generic.json")), make_schema(v1_3::source_generic) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("source_audio.json")), make_schema(v1_3::source_audio) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("source_core.json")), make_schema(v1_3::source_core) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("source_data.json")), make_schema(v1_3::source_data) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("queryapi-subscriptions-post-request.json")), make_schema(v1_3::queryapi_subscriptions_post_request) },
                { make_schema_uri(v1_3::tag, _XPLATSTR("nodeapi-receiver-target.json")), make_schema(v1_3::nodeapi_receiver_target) },
                // v1.2
                { make_schema_uri(v1_2::tag, _XPLATSTR("registrationapi-resource-post-request.json")), make_schema(v1_2::registrationapi_resource_post_request) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("clock_internal.json")), make_schema(v1_2::clock_internal) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("clock_ptp.json")), make_schema(v1_2::clock_ptp) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("resource_core.json")), make_schema(v1_2::resource_core) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("device.json")), make_schema(v1_2::device) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow.json")), make_schema(v1_2::flow) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_video_raw.json")), make_schema(v1_2::flow_video_raw) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_video_coded.json")), make_schema(v1_2::flow_video_coded) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_audio_raw.json")), make_schema(v1_2::flow_audio_raw) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_audio_coded.json")), make_schema(v1_2::flow_audio_coded) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_data.json")), make_schema(v1_2::flow_data) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_sdianc_data.json")), make_schema(v1_2::flow_sdianc_data) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_mux.json")), make_schema(v1_2::flow_mux) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_audio.json")), make_schema(v1_2::flow_audio) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_core.json")), make_schema(v1_2::flow_core) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("flow_video.json")), make_schema(v1_2::flow_video) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("node.json")), make_schema(v1_2::node) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("receiver.json")), make_schema(v1_2::receiver) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("receiver_video.json")), make_schema(v1_2::receiver_video) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("receiver_audio.json")), make_schema(v1_2::receiver_audio) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("receiver_data.json")), make_schema(v1_2::receiver_data) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("receiver_mux.json")), make_schema(v1_2::receiver_mux) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("receiver_core.json")), make_schema(v1_2::receiver_core) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("sender.json")), make_schema(v1_2::sender) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("source.json")), make_schema(v1_2::source) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("source_generic.json")), make_schema(v1_2::source_generic) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("source_audio.json")), make_schema(v1_2::source_audio) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("source_core.json")), make_schema(v1_2::source_core) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("queryapi-subscriptions-post-request.json")), make_schema(v1_2::queryapi_subscriptions_post_request) },
                { make_schema_uri(v1_2::tag, _XPLATSTR("nodeapi-receiver-target.json")), make_schema(v1_2::nodeapi_receiver_target) },
                // v1.1
                { make_schema_uri(v1_1::tag, _XPLATSTR("registrationapi-resource-post-request.json")), make_schema(v1_1::registrationapi_resource_post_request) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("clock_internal.json")), make_schema(v1_1::clock_internal) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("clock_ptp.json")), make_schema(v1_1::clock_ptp) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("resource_core.json")), make_schema(v1_1::resource_core) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("device.json")), make_schema(v1_1::device) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow.json")), make_schema(v1_1::flow) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_video_raw.json")), make_schema(v1_1::flow_video_raw) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_video_coded.json")), make_schema(v1_1::flow_video_coded) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_audio_raw.json")), make_schema(v1_1::flow_audio_raw) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_audio_coded.json")), make_schema(v1_1::flow_audio_coded) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_data.json")), make_schema(v1_1::flow_data) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_sdianc_data.json")), make_schema(v1_1::flow_sdianc_data) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_mux.json")), make_schema(v1_1::flow_mux) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_audio.json")), make_schema(v1_1::flow_audio) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_core.json")), make_schema(v1_1::flow_core) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("flow_video.json")), make_schema(v1_1::flow_video) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("node.json")), make_schema(v1_1::node) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver.json")), make_schema(v1_1::receiver) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_video.json")), make_schema(v1_1::receiver_video) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_audio.json")), make_schema(v1_1::receiver_audio) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_data.json")), make_schema(v1_1::receiver_data) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_mux.json")), make_schema(v1_1::receiver_mux) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_core.json")), make_schema(v1_1::receiver_core) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("sender.json")), make_schema(v1_1::sender) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("source.json")), make_schema(v1_1::source) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("source_generic.json")), make_schema(v1_1::source_generic) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("source_audio.json")), make_schema(v1_1::source_audio) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("source_core.json")), make_schema(v1_1::source_core) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("queryapi-subscriptions-post-request.json")), make_schema(v1_1::queryapi_subscriptions_post_request) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("nodeapi-receiver-target.json")), make_schema(v1_1::nodeapi_receiver_target) },
                // v1.0
                { make_schema_uri(v1_0::tag, _XPLATSTR("registrationapi-v1.0-resource-post-request.json")), make_schema(v1_0::registrationapi_v1_0_resource_post_request) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("device.json")), make_schema(v1_0::device) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("flow.json")), make_schema(v1_0::flow) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("node.json")), make_schema(v1_0::node) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("receiver.json")), make_schema(v1_0::receiver) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("sender.json")), make_schema(v1_0::sender) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("source.json")), make_schema(v1_0::source) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("queryapi-v1.0-subscriptions-post-request.json")), make_schema(v1_0::queryapi_v1_0_subscriptions_post_request) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("nodeapi-receiver-target.json")), make_schema(v1_0::nodeapi_receiver_target) },
            };
        }

        static std::map<web::uri, web::json::value> make_is05_schemas()
        {
            using namespace nmos::is05_schemas;

            return
            {
                // v1.1
                { make_schema_uri(v1_1::tag, _XPLATSTR("sender-stage-schema.json")), make_schema(v1_1::sender_stage_schema) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver-stage-schema.json")), make_schema(v1_1::receiver_stage_schema) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver-transport-file.json")), make_schema(v1_1::receiver_transport_file) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("activation-schema.json")), make_schema(v1_1::activation_schema) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("sender_transport_params.json")), make_schema(v1_1::sender_transport_params) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("sender_transport_params_rtp.json")), make_schema(v1_1::sender_transport_params_rtp) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("sender_transport_params_dash.json")), make_schema(v1_1::sender_transport_params_dash) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("sender_transport_params_websocket.json")), make_schema(v1_1::sender_transport_params_websocket) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("sender_transport_params_mqtt.json")), make_schema(v1_1::sender_transport_params_mqtt) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("sender_transport_params_ext.json")), make_schema(v1_1::sender_transport_params_ext) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_transport_params.json")), make_schema(v1_1::receiver_transport_params) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_transport_params_rtp.json")), make_schema(v1_1::receiver_transport_params_rtp) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_transport_params_dash.json")), make_schema(v1_1::receiver_transport_params_dash) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_transport_params_websocket.json")), make_schema(v1_1::receiver_transport_params_websocket) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_transport_params_mqtt.json")), make_schema(v1_1::receiver_transport_params_mqtt) },
                { make_schema_uri(v1_1::tag, _XPLATSTR("receiver_transport_params_ext.json")), make_schema(v1_1::receiver_transport_params_ext) },
                // v1.0
                { make_schema_uri(v1_0::tag, _XPLATSTR("v1.0-sender-stage-schema.json")), make_schema(v1_0::v1_0_sender_stage_schema) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("v1.0-receiver-stage-schema.json")), make_schema(v1_0::v1_0_receiver_stage_schema) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("v1.0-activation-schema.json")), make_schema(v1_0::v1_0_activation_schema) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("v1.0_sender_transport_params_rtp.json")), make_schema(v1_0::v1_0_sender_transport_params_rtp) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("v1.0_sender_transport_params_dash.json")), make_schema(v1_0::v1_0_sender_transport_params_dash) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("v1.0_receiver_transport_params_rtp.json")), make_schema(v1_0::v1_0_receiver_transport_params_rtp) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("v1.0_receiver_transport_params_dash.json")), make_schema(v1_0::v1_0_receiver_transport_params_dash) }
            };
        }

        static std::map<web::uri, web::json::value> make_is08_schemas()
        {
            using namespace nmos::is08_schemas;

            return
            {
                // v1.0
                { make_schema_uri(v1_0::tag, _XPLATSTR("activation-schema.json")), make_schema(v1_0::activation_schema) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("map-activations-post-request-schema.json")), make_schema(v1_0::map_activations_post_request_schema) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("map-entries-schema.json")), make_schema(v1_0::map_entries_schema) },
            };
        }

        static std::map<web::uri, web::json::value> make_is09_schemas()
        {
            using namespace nmos::is09_schemas;

            return
            {
                // v1.0
                { make_schema_uri(v1_0::tag, _XPLATSTR("global.json")), make_schema(v1_0::global) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("resource_core.json")), make_schema(v1_0::resource_core) }
            };
        }

        inline void merge(std::map<web::uri, web::json::value>& to, std::map<web::uri, web::json::value>&& from)
        {
            to.insert(from.begin(), from.end()); // std::map::merge in C++17
        }

        std::map<web::uri, web::json::value> make_schemas()
        {
            auto result = make_is04_schemas();
            merge(result, make_is05_schemas());
            merge(result, make_is08_schemas());
            merge(result, make_is09_schemas());
            return result;
        }

        static std::map<web::uri, web::json::value> schemas = make_schemas();
    }

    namespace experimental
    {
        web::uri make_systemapi_global_schema_uri(const nmos::api_version& version)
        {
            return is09_schemas::v1_0::systemapi_global_schema_uri;
        }

        web::uri make_registrationapi_resource_post_request_schema_uri(const nmos::api_version& version)
        {
            if (is04_versions::v1_3 <= version) return is04_schemas::v1_3::registrationapi_resource_post_request_uri;
            if (is04_versions::v1_2 <= version) return is04_schemas::v1_2::registrationapi_resource_post_request_uri;
            if (is04_versions::v1_1 <= version) return is04_schemas::v1_1::registrationapi_resource_post_request_uri;
            return is04_schemas::v1_0::registrationapi_resource_post_request_uri;
        }

        web::uri make_queryapi_subscriptions_post_request_schema_uri(const nmos::api_version& version)
        {
            if (is04_versions::v1_3 <= version) return is04_schemas::v1_3::queryapi_subscriptions_post_request_uri;
            if (is04_versions::v1_2 <= version) return is04_schemas::v1_2::queryapi_subscriptions_post_request_uri;
            if (is04_versions::v1_1 <= version) return is04_schemas::v1_1::queryapi_subscriptions_post_request_uri;
            return is04_schemas::v1_0::queryapi_subscriptions_post_request_uri;
        }

        web::uri make_nodeapi_receiver_target_put_request_schema_uri(const nmos::api_version& version)
        {
            if (is04_versions::v1_3 <= version) return is04_schemas::v1_3::nodeapi_receiver_target_put_request_uri;
            if (is04_versions::v1_2 <= version) return is04_schemas::v1_2::nodeapi_receiver_target_put_request_uri;
            if (is04_versions::v1_1 <= version) return is04_schemas::v1_1::nodeapi_receiver_target_put_request_uri;
            return is04_schemas::v1_0::nodeapi_receiver_target_put_request_uri;
        }

        web::uri make_connectionapi_staged_patch_request_schema_uri(const nmos::api_version& version, const nmos::type& type)
        {
            return nmos::types::sender == type
                ? make_connectionapi_sender_staged_patch_request_schema_uri(version)
                : make_connectionapi_receiver_staged_patch_request_schema_uri(version);
        }

        web::uri make_connectionapi_sender_staged_patch_request_schema_uri(const nmos::api_version& version)
        {
            if (is05_versions::v1_1 <= version) return is05_schemas::v1_1::connectionapi_sender_staged_patch_request_uri;
            return is05_schemas::v1_0::connectionapi_sender_staged_patch_request_uri;
        }

        web::uri make_connectionapi_receiver_staged_patch_request_schema_uri(const nmos::api_version& version)
        {
            if (is05_versions::v1_1 <= version) return is05_schemas::v1_1::connectionapi_receiver_staged_patch_request_uri;
            return is05_schemas::v1_0::connectionapi_receiver_staged_patch_request_uri;
        }

        web::uri make_channelmappingapi_map_activations_post_request_schema_uri(const nmos::api_version& version)
        {
            return is08_schemas::v1_0::map_activations_post_request_uri;
        }

        // load the json schema for the specified base URI
        web::json::value load_json_schema(const web::uri& id)
        {
            auto found = nmos::details::schemas.find(id);

            if (nmos::details::schemas.end() == found)
            {
                throw web::json::json_exception((_XPLATSTR("schema not found for ") + id.to_string()).c_str());
            }

            return found->second;
        }
    }
}

#include "nmos/json_schema.h"

#include "cpprest/basic_utils.h"
#include "nmos/api_version.h"
#include "nmos/is04_schemas/is04_schemas.h"

namespace nmos
{
    namespace is04_schemas
    {
        web::uri make_schema_uri(const utility::string_t& tag, const utility::string_t& ref = {})
        {
            return{ _XPLATSTR("https://github.com/AMWA-TV/nmos-discovery-registration/raw/") + tag + _XPLATSTR("/APIs/schemas/") + ref };
        }

        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2.x/APIs/schemas/
        namespace v1_2
        {
            using namespace nmos::is04_schemas::v1_2_1;
            const utility::string_t tag(_XPLATSTR("v1.2.1"));

            const web::uri registrationapi_resource_post_request_uri = make_schema_uri(tag, _XPLATSTR("registrationapi-resource-post-request.json"));
            const web::uri queryapi_subscriptions_post_request_uri = make_schema_uri(tag, _XPLATSTR("queryapi-subscriptions-post-request.json"));
        }

        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.1.x/APIs/schemas/
        namespace v1_1
        {
            using namespace nmos::is04_schemas::v1_1_2;
            const utility::string_t tag(_XPLATSTR("v1.1.2"));

            const web::uri registrationapi_resource_post_request_uri = make_schema_uri(tag, _XPLATSTR("registrationapi-resource-post-request.json"));
            const web::uri queryapi_subscriptions_post_request_uri = make_schema_uri(tag, _XPLATSTR("queryapi-subscriptions-post-request.json"));
        }

        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.0.x/APIs/schemas/
        namespace v1_0
        {
            using namespace nmos::is04_schemas::v1_0_3;
            const utility::string_t tag(_XPLATSTR("v1.0.3"));

            const web::uri registrationapi_resource_post_request_uri = make_schema_uri(tag, _XPLATSTR("registrationapi-v1.0-resource-post-request.json"));
            const web::uri queryapi_subscriptions_post_request_uri = make_schema_uri(tag, _XPLATSTR("queryapi-v1.0-subscriptions-post-request.json"));
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

        static std::map<web::uri, web::json::value> make_schemas()
        {
            using namespace nmos::is04_schemas;

            return
            {
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
                // v1.0
                { make_schema_uri(v1_0::tag, _XPLATSTR("registrationapi-v1.0-resource-post-request.json")), make_schema(v1_0::registrationapi_v1_0_resource_post_request) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("device.json")), make_schema(v1_0::device) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("flow.json")), make_schema(v1_0::flow) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("node.json")), make_schema(v1_0::node) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("receiver.json")), make_schema(v1_0::receiver) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("sender.json")), make_schema(v1_0::sender) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("source.json")), make_schema(v1_0::source) },
                { make_schema_uri(v1_0::tag, _XPLATSTR("queryapi-v1.0-subscriptions-post-request.json")), make_schema(v1_0::queryapi_v1_0_subscriptions_post_request) }
            };
        }

        static std::map<web::uri, web::json::value> schemas = make_schemas();
    }

    namespace experimental
    {
        web::uri make_registrationapi_resource_post_request_schema_uri(const nmos::api_version& version)
        {
            if (is04_versions::v1_2 <= version) return is04_schemas::v1_2::registrationapi_resource_post_request_uri;
            if (is04_versions::v1_1 == version) return is04_schemas::v1_1::registrationapi_resource_post_request_uri;
            return is04_schemas::v1_0::registrationapi_resource_post_request_uri;
        }

        web::uri make_queryapi_subscriptions_post_request_schema_uri(const nmos::api_version& version)
        {
            if (is04_versions::v1_2 <= version) return is04_schemas::v1_2::queryapi_subscriptions_post_request_uri;
            if (is04_versions::v1_1 == version) return is04_schemas::v1_1::queryapi_subscriptions_post_request_uri;
            return is04_schemas::v1_0::queryapi_subscriptions_post_request_uri;
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

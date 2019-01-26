#include "nmos/node_resources.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/channels.h"
#include "nmos/colorspace.h"
#include "nmos/connection_api.h" // for nmos::resolve_auto
#include "nmos/components.h"
#include "nmos/device_type.h"
#include "nmos/format.h"
#include "nmos/interlace_mode.h"
#include "nmos/is04_versions.h"
#include "nmos/is05_versions.h"
#include "nmos/media_type.h"
#include "nmos/model.h"
#include "nmos/node_resource.h"
#include "nmos/resource.h"
#include "nmos/slog.h"
#include "nmos/transfer_characteristic.h"
#include "nmos/transport.h"
#include "nmos/version.h"

namespace nmos
{
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/device.json
    nmos::resource make_device(const nmos::id& id, const nmos::id& node_id, const std::vector<nmos::id>& senders, const std::vector<nmos::id>& receivers, const nmos::settings& settings)
    {
        using web::json::value;
        using web::json::value_of;
        using web::json::value_from_elements;

        auto data = details::make_resource_core(id, settings);

        data[U("type")] = value::string(nmos::device_types::generic.name);
        data[U("node_id")] = value::string(node_id);
        data[U("senders")] = value_from_elements(senders);
        data[U("receivers")] = value_from_elements(receivers);

        const auto at_least_one_host_address = value_of({ value::string(nmos::fields::host_address(settings)) });
        const auto& host_addresses = settings.has_field(nmos::fields::host_addresses) ? nmos::fields::host_addresses(settings) : at_least_one_host_address.as_array();

        for (const auto& version : nmos::is05_versions::from_settings(settings))
        {
            auto connection_uri = web::uri_builder()
                .set_scheme(U("http"))
                .set_port(nmos::fields::connection_port(settings))
                .set_path(U("/x-nmos/connection/") + make_api_version(version));
            auto type = U("urn:x-nmos:control:sr-ctrl/") + make_api_version(version);

            for (const auto& host_address : host_addresses)
            {
                web::json::push_back(data[U("controls")], value_of({
                    { U("href"), connection_uri.set_host(host_address.as_string()).to_uri().to_string() },
                    { U("type"), type }
                }));
            }
        }

        return{ is04_versions::v1_3, types::device, data, false };
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_core.json
    nmos::resource make_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        using web::json::value;

        auto data = details::make_resource_core(id, settings);

        if (0 != grain_rate) data[U("grain_rate")] = nmos::make_rational(grain_rate); // optional
        data[U("caps")] = value::object();
        data[U("device_id")] = value::string(device_id);
        data[U("parents")] = value::array();
        data[U("clock_name")] = value::null();

        return{ is04_versions::v1_3, types::source, data, false };
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_generic.json
    nmos::resource make_generic_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::format& format, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_source(id, device_id, grain_rate, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(format.name);

        return resource;
    }

    nmos::resource make_video_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        return make_generic_source(id, device_id, grain_rate, nmos::formats::video, settings);
    }

    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        return make_generic_source(id, device_id, grain_rate, nmos::formats::data, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_audio.json
    nmos::resource make_audio_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const std::vector<channel>& channels, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_source(id, device_id, grain_rate, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::audio.name);

        auto channels_data = value::array();
        for (auto channel : channels)
        {
            web::json::push_back(channels_data, make_channel(channel));
        }
        data[U("channels")] = std::move(channels_data);

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_core.json
    nmos::resource make_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings)
    {
        using web::json::value;

        auto data = details::make_resource_core(id, settings);

        if (0 != grain_rate) data[U("grain_rate")] = nmos::make_rational(grain_rate); // optional

        data[U("source_id")] = value::string(source_id);
        data[U("device_id")] = value::string(device_id);
        data[U("parents")] = value::array();

        return{ is04_versions::v1_3, types::flow, data, false };
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_video.json
    nmos::resource make_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode, const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_flow(id, source_id, device_id, grain_rate, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::video.name);
        data[U("frame_width")] = frame_width;
        data[U("frame_height")] = frame_height;
        if (interlace_modes::none != interlace_mode) data[U("interlace_mode")] = value::string(interlace_mode.name); // optional
        data[U("colorspace")] = value::string(colorspace.name);
        if (transfer_characteristics::none != transfer_characteristic) data[U("transfer_characteristic")] = value::string(transfer_characteristic.name); // optional

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_video_raw.json
    nmos::resource make_raw_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode, const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, chroma_subsampling chroma_subsampling, unsigned int bit_depth, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_video_flow(id, source_id, device_id, grain_rate, frame_width, frame_height, interlace_mode, colorspace, transfer_characteristic, settings);
        auto& data = resource.data;

        data[U("media_type")] = value::string(nmos::media_types::video_raw.name);

        data[U("components")] = make_components(chroma_subsampling, frame_width, frame_height, bit_depth);

        return resource;
    }

    nmos::resource make_raw_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings)
    {
        return make_raw_video_flow(id, source_id, device_id, {}, 1920, 1080, nmos::interlace_modes::interlaced_bff, nmos::colorspaces::BT709, nmos::transfer_characteristics::SDR, YCbCr422, 10, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_audio.json
    nmos::resource make_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_flow(id, source_id, device_id, {}, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::audio.name);
        data[U("sample_rate")] = make_rational(sample_rate);

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_audio_raw.json
    nmos::resource make_raw_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, unsigned int bit_depth, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_audio_flow(id, source_id, device_id, sample_rate, settings);
        auto& data = resource.data;

        data[U("media_type")] = value::string(nmos::media_types::audio_L(bit_depth).name);
        data[U("bit_depth")] = bit_depth;

        return resource;
    }

    nmos::resource make_raw_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings)
    {
        return make_raw_audio_flow(id, source_id, device_id, 48000, 24, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_sdianc_data.json
    nmos::resource make_sdianc_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_flow(id, source_id, device_id, {}, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::data.name);
        data[U("media_type")] = value::string(nmos::media_types::video_smpte291.name);

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_sdianc_data.json
    nmos::resource make_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_flow(id, source_id, device_id, {}, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::data.name);
        data[U("media_type")] = value::string(U("application/json"));

        return resource;
    }

    web::uri make_connection_api_transportfile(const nmos::id& sender_id, const nmos::settings& settings)
    {
        const auto version = *nmos::is05_versions::from_settings(settings).begin();

        return web::uri_builder()
            .set_scheme(U("http"))
            .set_host(nmos::fields::host_address(settings))
            .set_port(nmos::fields::connection_port(settings))
            .set_path(U("/x-nmos/connection/") + make_api_version(version) + U("/single/senders/") + sender_id + U("/transportfile"))
            .to_uri();
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/sender.json
    nmos::resource make_sender(const nmos::id& id, const nmos::id& flow_id, const nmos::transport& transport, const nmos::id& device_id, const utility::string_t& manifest_href, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
        using web::json::value;

        auto data = details::make_resource_core(id, settings);

        //data[U("caps")] = value::object(); // optional

        data[U("flow_id")] = !flow_id.empty() ? value::string(flow_id) : value::null();
        data[U("transport")] = value::string(transport.name);
        data[U("device_id")] = value::string(device_id);
        data[U("manifest_href")] = value::string(manifest_href);

        auto& interface_bindings = data[U("interface_bindings")] = value::array();
        for (const auto& interface : interfaces)
        {
            web::json::push_back(interface_bindings, interface);
        }

        value subscription;
        subscription[U("receiver_id")] = value::null();
        subscription[U("active")] = value::boolean(false);
        data[U("subscription")] = std::move(subscription);

        return{ is04_versions::v1_3, types::sender, data, false };
    }

    nmos::resource make_sender(const nmos::id& id, const nmos::id& flow_id, const nmos::id& device_id, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
        return make_sender(id, flow_id, nmos::transports::rtp_mcast, device_id, make_connection_api_transportfile(id, settings).to_string(), interfaces, settings);
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_core.json
    nmos::resource make_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
        using web::json::value;

        auto data = details::make_resource_core(id, settings);

        data[U("device_id")] = value::string(device_id);
        data[U("transport")] = value::string(transport.name);

        auto& interface_bindings = data[U("interface_bindings")] = value::array();
        for (const auto& interface : interfaces)
        {
            web::json::push_back(interface_bindings, interface);
        }

        value subscription;
        subscription[U("sender_id")] = value::null();
        subscription[U("active")] = value::boolean(false);
        data[U("subscription")] = std::move(subscription);

        return{ is04_versions::v1_3, types::receiver, data, false };
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_video.json
    nmos::resource make_video_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_receiver(id, device_id, transport, interfaces, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::video.name);
        data[U("caps")][U("media_types")][0] = value::string(nmos::media_types::video_raw.name);

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_audio.json
    nmos::resource make_audio_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, unsigned int bit_depth, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_receiver(id, device_id, transport, interfaces, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::audio.name);
        data[U("caps")][U("media_types")][0] = value::string(nmos::media_types::audio_L(bit_depth).name);

        return resource;
    }

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_data.json
    nmos::resource make_sdianc_data_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings)
    {
        using web::json::value;

        auto resource = make_receiver(id, device_id, transport, interfaces, settings);
        auto& data = resource.data;

        data[U("format")] = value::string(nmos::formats::data.name);
        data[U("caps")][U("media_types")][0] = value::string(nmos::media_types::video_smpte291.name);

        return resource;
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

        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0-dev/docs/5.2.%20Transport%20-%20Websocket.md
        // and ???
        web::json::value make_connection_receiver_staged_is07_core_parameter_set(const nmos::id & source_id)
        {
            using web::json::value;
            using web::json::value_of;

            return value_of({
                { nmos::fields::connection_uri, value::null() },
                { nmos::fields::ext_is_07_source_id, source_id },
                { nmos::fields::ext_is_07_rest_api_url, value::null() }
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


    nmos::resource make_type_event_source(const nmos::id& id, const nmos::id& device_id, utility::string_t event_type, const nmos::settings& settings) 
    {
        nmos::resource resource = make_data_source(id, device_id, {}, settings);
        auto& data = resource.data;

        data[nmos::fields::event_tally_types] = web::json::value::string(event_type);

        return resource;
    }

    nmos::resource make_state_event_source(const nmos::id& id, const nmos::id& device_id, utility::string_t event_state, const nmos::settings& settings)
    {
        nmos::resource resource = make_data_source(id, device_id, 0, settings);
        auto& data = resource.data;

        data[nmos::fields::event_tally_states] = web::json::value::string(event_state);

        return resource;
    }


    nmos::resource add_event_tally_connection(const nmos::id& source_id, const nmos::id& sender_id, const utility::string_t & type, const web::json::value & payload, const nmos::settings& settings) 
    {
        using web::json::value;
      
        auto data = details::make_connection_resource_core(sender_id, false);
      
        data[nmos::fields::endpoint_staged][nmos::fields::sender_id] = value::null();
        data[nmos::fields::endpoint_staged][nmos::fields::transport_params] =  details::legs_of(details::make_connection_receiver_staged_is07_core_parameter_set(source_id), false);
      
        data[nmos::fields::endpoint_active] = data[nmos::fields::endpoint_staged];
        // All instances of "auto" should be resolved into the actual values that will be used
        // but in some cases the behaviour is more complex, and may be determined by the vendor.
        // This function does not select a value for e.g. sender "source_ip" or receiver "interface_ip".
        nmos::resolve_auto(types::receiver, data[nmos::fields::endpoint_active][nmos::fields::transport_params]);
      
        return{ is05_versions::v1_0, types::receiver, data, false };          
    }

    nmos::resource add_event_tally(const nmos::id& source_id, const nmos::id& device_id, const nmos::id& flow_id, utility::string_t type, web::json::value payload, const nmos::settings& settings)
    {
        using web::json::value;
      
        value data;            
        data[U("id")] = value::string(source_id);
        data[U("version")] = value::string(nmos::make_version());
      
        std::vector<std::pair<utility::string_t, web::json::value>> obj;
        obj.push_back({"source_id", value::string(source_id)});
        obj.push_back({"flow_id",   value::string(flow_id)});
        data[U("identity")]   = web::json::value::object(obj);
        data[U("timing")]     = value::object({{"creation_timestamp", value::string(make_version(nmos::tai_now()))}});
        data[U("event_type")] = value::string(type);
        data[U("payload")]    = value::object({{"value", payload}});
        data[U("type")]        = value::string(type);
      
        return{ is04_versions::v1_2, types::event_tally, data, false };
    }

    namespace experimental
    {
        void insert_event_tally(nmos::resources& node_resources, nmos::resources& event_tally_resources, nmos::resources& connection_resources, const nmos::id& device_id, const nmos::settings& settings) {
            const auto& seed_id = nmos::experimental::fields::seed_id(settings);
#if 1
            static int i;
            auto source_id =  utility::string_t("dead5eef-1111-3333-8888-dead5eef888"+std::to_string(i)); 
            auto flow_id =  utility::string_t("dead5eef-1111-3333-9999-dead5eef999"+std::to_string(i++)); 
            auto sender_id =  utility::string_t("dead5eef-1111-4444-8888-dead5eef888"+std::to_string(i)); 
            auto receiver_id =  utility::string_t("dead5eef-1111-4444-9999-dead5eef999"+std::to_string(i++)); 
#else
            auto source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/event_tally/0"));
            auto flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/event_tally/0"));
            auto sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/event_tally/0"));
            auto receiver_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/receiver/event_tally/0"));
#endif
            
            insert_resource(node_resources, make_type_event_source(source_id, device_id, "boolean", settings));
            insert_resource(node_resources, make_data_flow(flow_id, source_id, device_id, settings));
            insert_resource(node_resources, make_sender(sender_id, flow_id, nmos::transports::mqtt, device_id, "", {}, settings));
            insert_resource(node_resources, make_receiver(receiver_id, device_id, nmos::transports::mqtt, {}, settings));

            insert_resource(event_tally_resources, add_event_tally(source_id, flow_id, device_id, "boolean", {}, settings));
            insert_resource(connection_resources, add_event_tally_connection(source_id, sender_id, "boolean", {}, settings));
        }

        // insert a node resource, and sub-resources, according to the settings; return an iterator to the inserted node resource,
        // or to a resource that prevented the insertion, and a bool denoting whether the insertion took place
        std::pair<resources::iterator, bool> insert_node_resources(nmos::resources& node_resources, const nmos::settings& settings)
        {
            const auto& seed_id = nmos::experimental::fields::seed_id(settings);
            auto node_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/self"));
            auto device_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/device/0"));
            auto source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/0"));
            auto flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/0"));
            auto sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/0"));
            auto receiver_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/receiver/0"));

            auto result = insert_resource(node_resources, make_node(node_id, settings));
            insert_resource(node_resources, make_device(device_id, node_id, { sender_id }, { receiver_id }, settings));
            insert_resource(node_resources, make_video_source(source_id, device_id, { 25, 1 }, settings));
            insert_resource(node_resources, make_raw_video_flow(flow_id, source_id, device_id, settings));
            insert_resource(node_resources, make_sender(sender_id, flow_id, device_id, {}, settings));
            insert_resource(node_resources, make_video_receiver(receiver_id, device_id, nmos::transports::rtp_mcast, {}, settings));
            return result;
        }
    }
}

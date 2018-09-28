#include "nmos/node_resources.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/channels.h"
#include "nmos/colorspace.h"
#include "nmos/components.h"
#include "nmos/device_type.h"
#include "nmos/format.h"
#include "nmos/interlace_mode.h"
#include "nmos/log_manip.h"
#include "nmos/media_type.h"
#include "nmos/model.h"
#include "nmos/node_resource.h"
#include "nmos/resource.h"
#include "nmos/thread_utils.h"
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

        for (const auto& host_address : host_addresses)
        {
            value control;
            auto connection_uri = web::uri_builder()
                .set_scheme(U("http"))
                .set_host(host_address.as_string())
                .set_port(nmos::fields::connection_port(settings))
                .set_path(U("/x-nmos/connection/v1.0"))
                .to_uri();
            control[U("href")] = value::string(connection_uri.to_string());
            control[U("type")] = value::string(U("urn:x-nmos:control:sr-ctrl/v1.0"));
            web::json::push_back(data[U("controls")], control);
        }

        return{ is04_versions::v1_2, types::device, data, false };
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

        return{ is04_versions::v1_2, types::source, data, false };
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

        return{ is04_versions::v1_2, types::flow, data, false };
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

    web::uri make_connection_api_transportfile(const nmos::id& sender_id, const nmos::settings& settings)
    {
        return web::uri_builder()
            .set_scheme(U("http"))
            .set_host(nmos::fields::host_address(settings))
            .set_port(nmos::fields::connection_port(settings))
            .set_path(U("/x-nmos/connection/v1.0/single/senders/") + sender_id + U("/transportfile"))
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

        return{ is04_versions::v1_2, types::sender, data, false };
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

        return{ is04_versions::v1_2, types::receiver, data, false };
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

    namespace experimental
    {
        // insert a node resource, and sub-resources, according to the settings; return an iterator to the inserted node resource,
        // or to a resource that prevented the insertion, and a bool denoting whether the insertion took place
        std::pair<resources::iterator, bool> insert_node_resources(nmos::resources& resources, const nmos::settings& settings)
        {
            const auto& seed_id = nmos::experimental::fields::seed_id(settings);
            auto node_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/self"));
            auto device_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/device/0"));
            auto source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/0"));
            auto flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/0"));
            auto sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/0"));
            auto receiver_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/receiver/0"));

            auto result = insert_resource(resources, make_node(node_id, settings));
            insert_resource(resources, make_device(device_id, node_id, { sender_id }, { receiver_id }, settings));
            insert_resource(resources, make_video_source(source_id, device_id, { 25, 1 }, settings));
            insert_resource(resources, make_raw_video_flow(flow_id, source_id, device_id, settings));
            insert_resource(resources, make_sender(sender_id, flow_id, device_id, {}, settings));
            insert_resource(resources, make_video_receiver(receiver_id, device_id, nmos::transports::rtp_mcast, {}, settings));
            return result;
        }

        // insert a node resource, and sub-resources, according to the settings, and then wait for shutdown
        void node_resources_thread(nmos::model& model, const bool& shutdown, nmos::condition_variable& shutdown_condition, slog::base_gate& gate)
        {
            const auto seed_id = nmos::with_read_lock(model.mutex, [&] { return nmos::experimental::fields::seed_id(model.settings); });
            auto node_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/self"));
            auto device_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/device/0"));
            auto source_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/source/0"));
            auto flow_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/flow/0"));
            auto sender_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/sender/0"));
            auto receiver_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/receiver/0"));

            auto lock = model.write_lock(); // in order to update the resources

            // any delay between updates to the model resources is unnecessary
            // this just serves as a slightly more realistic example!
            const unsigned int delay_millis{ 50 };

            const auto insert_resource_after = [&](unsigned int milliseconds, nmos::resource&& resource, slog::base_gate& gate)
            {
                // using wait_until rather than wait_for as a workaround for an awful bug in VS2015, resolved in VS2017
                if (!shutdown_condition.wait_until(lock, std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds), [&] { return shutdown; }))
                {
                    const std::pair<nmos::id, nmos::type> id_type{ resource.id, resource.type };
                    const bool success = insert_resource(model.node_resources, std::move(resource)).second;

                    if (success)
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Updated model with " << id_type;
                    else
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Model update error: " << id_type;

                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Notifying node behaviour thread"; // and anyone else who cares...
                    model.notify();
                }
            };

            insert_resource_after(delay_millis, make_node(node_id, model.settings), gate);
            insert_resource_after(delay_millis, make_device(device_id, node_id, { sender_id }, { receiver_id }, model.settings), gate);
            insert_resource_after(delay_millis, make_video_source(source_id, device_id, { 25, 1 }, model.settings), gate);
            insert_resource_after(delay_millis, make_raw_video_flow(flow_id, source_id, device_id, model.settings), gate);
            insert_resource_after(delay_millis, make_sender(sender_id, flow_id, device_id, {}, model.settings), gate);
            insert_resource_after(delay_millis, make_video_receiver(receiver_id, device_id, nmos::transports::rtp_mcast, {}, model.settings), gate);

            shutdown_condition.wait(lock, [&] { return shutdown; });
        }
    }
}

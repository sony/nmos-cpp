#include "nmos/node_resources.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/rational.h"
#include "nmos/version.h"

namespace nmos
{
    namespace experimental
    {
        nmos::resource make_node_node(const nmos::id& id, const nmos::settings& settings)
        {
            using web::json::value;
            using web::json::value_of;

            const auto hostname = value::string(nmos::fields::host_name(settings));

            value data;

            // nmos-discovery-registration/APIs/schemas/resource_core.json

            data[U("id")] = value::string(id);
            data[U("version")] = value::string(nmos::make_version());
            data[U("label")] = hostname;
            data[U("description")] = hostname;
            data[U("tags")] = value::object();

            // nmos-discovery-registration/APIs/schemas/node.json

            auto uri = web::uri_builder()
                .set_scheme(U("http"))
                .set_host(nmos::fields::host_address(settings))
                .set_port(nmos::fields::node_port(settings));

            data[U("href")] = value::string(uri.to_string());
            data[U("hostname")] = hostname;
            data[U("api")][U("versions")] = value_of({ JU("v1.0"), JU("v1.1"), JU("v1.2") });

            const auto at_least_one_host_address = value_of({ value::string(nmos::fields::host_address(settings)) });
            const auto& host_addresses = settings.has_field(nmos::fields::host_addresses) ? nmos::fields::host_addresses(settings) : at_least_one_host_address.as_array();

            for (const auto& host_address : host_addresses)
            {
                value endpoint;
                endpoint[U("host")] = host_address;
                endpoint[U("port")] = uri.port();
                endpoint[U("protocol")] = value::string(uri.scheme());
                web::json::push_back(data[U("api")][U("endpoints")], endpoint);
            }

            data[U("caps")] = value::object();

            data[U("services")] = value::array();

            data[U("clocks")] = value::array();

            // need to populate this... each interface needs chassis_id, port_id and a name which can be referenced by senders and receivers
            data[U("interfaces")] = value::array();

            return{ is04_versions::v1_2, types::node, data, false };
        }

        nmos::resource make_device(const nmos::id& id, const nmos::id& node_id, const std::vector<nmos::id>& senders, const std::vector<nmos::id>& receivers, const nmos::settings& settings)
        {
            using web::json::value;
            using web::json::value_of;
            using web::json::value_from_elements;

            const auto hostname = value::string(nmos::fields::host_name(settings));

            value data;

            // nmos-discovery-registration/APIs/schemas/resource_core.json

            data[U("id")] = value::string(id);
            data[U("version")] = value::string(nmos::make_version());
            data[U("label")] = hostname;
            data[U("description")] = hostname;
            data[U("tags")] = value::object();

            // nmos-discovery-registration/APIs/schemas/device.json

            data[U("type")] = JU("urn:x-nmos:device:generic");
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
                    .set_path(U("/x-nmos/connection/v1.0"));
                control[U("href")] = value::string(connection_uri.to_string());
                control[U("type")] = value(U("urn:x-nmos:control:sr-ctrl/v1.0"));
                web::json::push_back(data[U("controls")], control);
            }

            return{ is04_versions::v1_2, types::device, data, false };
        }

        nmos::resource make_source(const nmos::id& id, const nmos::id& device_id, const nmos::settings& settings)
        {
            using web::json::value;

            const auto hostname = value::string(nmos::fields::host_name(settings));

            value data;

            // nmos-discovery-registration/APIs/schemas/resource_core.json

            data[U("id")] = value::string(id);
            data[U("version")] = value::string(nmos::make_version());
            data[U("label")] = hostname;
            data[U("description")] = hostname;
            data[U("tags")] = value::object();

            // nmos-discovery-registration/APIs/schemas/source_core.json

            data[U("grain_rate")] = nmos::make_rational(50, 1); // optional
            data[U("caps")] = value::object();
            data[U("device_id")] = value::string(device_id);
            data[U("parents")] = value::array();
            data[U("clock_name")] = value::null();

            // nmos-discovery-registration/APIs/schemas/source_generic.json

            data[U("format")] = JU("urn:x-nmos:format:video");

            return{ is04_versions::v1_2, types::source, data, false };
        }

        nmos::resource make_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings)
        {
            using web::json::value;
            using web::json::value_of;

            const auto hostname = value::string(nmos::fields::host_name(settings));

            value data;

            // nmos-discovery-registration/APIs/schemas/resource_core.json

            data[U("id")] = value::string(id);
            data[U("version")] = value::string(nmos::make_version());
            data[U("label")] = hostname;
            data[U("description")] = hostname;
            data[U("tags")] = value::object();

            // nmos-discovery-registration/APIs/schemas/flow_core.json

            data[U("grain_rate")] = nmos::make_rational(50, 1);

            data[U("source_id")] = value::string(source_id);
            data[U("device_id")] = value::string(device_id);
            data[U("parents")] = value::array();

            // nmos-discovery-registration/APIs/schemas/flow_video.json

            data[U("format")] = JU("urn:x-nmos:format:video");
            data[U("frame_width")] = 1920;
            data[U("frame_height")] = 1080;
            data[U("interlace_mode")] = JU("progressive"); // optional
            data[U("colorspace")] = JU("BT709");
            data[U("transfer_characteristic")] = JU("SDR"); // optional

            // nmos-discovery-registration/APIs/schemas/flow_video_raw.json

            data[U("media_type")] = JU("video/raw");
            data[U("components")] = value_of({
                value_of({
                    { U("name"), JU("Y") },
                    { U("width"), 1920 },
                    { U("height"), 1080 },
                    { U("bit_depth"), 10 }
                }),
                value_of({
                    { U("name"), JU("Cb") },
                    { U("width"), 960 },
                    { U("height"), 1080 },
                    { U("bit_depth"), 10 }
                }),
                value_of({
                    { U("name"), JU("Cr") },
                    { U("width"), 960 },
                    { U("height"), 1080 },
                    { U("bit_depth"), 10 }
                })
            });

            return{ is04_versions::v1_2, types::flow, data, false };
        }

        nmos::resource make_sender(const nmos::id& id, const nmos::id& flow_id, const nmos::id& device_id, const nmos::settings& settings)
        {
            using web::json::value;

            const auto hostname = value::string(nmos::fields::host_name(settings));

            value data;

            // nmos-discovery-registration/APIs/schemas/resource_core.json

            data[U("id")] = value::string(id);
            data[U("version")] = value::string(nmos::make_version());
            data[U("label")] = hostname;
            data[U("description")] = hostname;
            data[U("tags")] = value::object();

            // nmos-discovery-registration/APIs/schemas/sender.json

            //data[U("caps")] = value::object(); // optional

            data[U("flow_id")] = value::string(flow_id); // may be null
            data[U("transport")] = JU("urn:x-nmos:transport:rtp");
            data[U("device_id")] = value::string(device_id);

            auto sdp_uri = web::uri_builder()
                .set_scheme(U("http"))
                .set_host(nmos::fields::host_address(settings))
                .set_port(nmos::fields::connection_port(settings))
                .set_path(U("/x-nmos/connection/v1.0/single/senders/") + id + U("/transportfile"));
            data[U("manifest_href")] = value::string(sdp_uri.to_string());

            // need to populate this...
            data[U("interface_bindings")] = value::array();

            value subscription;
            subscription[U("receiver_id")] = value::null();
            subscription[U("active")] = value::boolean(false);
            data[U("subscription")] = subscription;

            return{ is04_versions::v1_2, types::sender, data, false };
        }

        nmos::resource make_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::settings& settings)
        {
            using web::json::value;

            const auto hostname = value::string(nmos::fields::host_name(settings));

            value data;

            // nmos-discovery-registration/APIs/schemas/resource_core.json

            data[U("id")] = value::string(id);
            data[U("version")] = value::string(nmos::make_version());
            data[U("label")] = hostname;
            data[U("description")] = hostname;
            data[U("tags")] = value::object();

            // nmos-discovery-registration/APIs/schemas/receiver_core.json

            data[U("device_id")] = value::string(device_id);
            data[U("transport")] = JU("urn:x-nmos:transport:rtp");
            // need to populate this...
            data[U("interface_bindings")] = value::array();

            value subscription;
            subscription[U("sender_id")] = value::null();
            subscription[U("active")] = value::boolean(false);
            data[U("subscription")] = subscription;

            // nmos-discovery-registration/APIs/schemas/receiver_video.json

            data[U("format")] = JU("urn:x-nmos:format:video");

            data[U("caps")][U("media_types")][0] = JU("video/raw");

            return{ is04_versions::v1_2, types::receiver, data, false };
        }

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

            auto result = insert_resource(resources, make_node_node(node_id, settings));
            insert_resource(resources, make_device(device_id, node_id, { sender_id }, { receiver_id }, settings));
            insert_resource(resources, make_source(source_id, device_id, settings));
            insert_resource(resources, make_flow(flow_id, source_id, device_id, settings));
            insert_resource(resources, make_sender(sender_id, flow_id, device_id, settings));
            insert_resource(resources, make_receiver(receiver_id, device_id, settings));
            return result;
        }
    }
}

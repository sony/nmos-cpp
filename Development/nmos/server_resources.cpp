#include "nmos/server_resources.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/version.h"

namespace nmos
{
    namespace experimental
    {
        nmos::resource make_server_node(const nmos::id& id, const nmos::settings& settings)
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

            value endpoint;
            endpoint[U("host")] = value::string(uri.host());
            endpoint[U("port")] = uri.port();
            endpoint[U("protocol")] = value::string(uri.scheme());
            data[U("api")][U("endpoints")][0] = endpoint;

            data[U("caps")] = value::object();

            // This is the experimental REST API for mDNS Service Discovery
            value mdns_service;
            auto mdns_uri = web::uri_builder()
                .set_scheme(U("http"))
                .set_host(nmos::fields::host_address(settings))
                .set_port(nmos::experimental::fields::mdns_port(settings));
            mdns_service[U("href")] = value::string(mdns_uri.to_string());
            mdns_service[U("type")] = JU("urn:x-mdns:v1.0");
            data[U("services")][0] = mdns_service;

            data[U("clocks")] = value::array();

            // need to populate this... each interface needs chassis_id, port_id and a name which can be referenced by senders and receivers
            data[U("interfaces")] = value::array();

            return{ is04_versions::v1_2, types::node, data, true };
        }

        void make_server_resources(nmos::resources& resources, const nmos::settings& settings)
        {
            auto node_id = nmos::make_id();

            insert_resource(resources, make_server_node(node_id, settings));
        }
    }
}

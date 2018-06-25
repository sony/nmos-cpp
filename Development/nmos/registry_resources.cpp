#include "nmos/registry_resources.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/version.h"

namespace nmos
{
    namespace experimental
    {
        nmos::resource make_registry_node(const nmos::id& id, const nmos::settings& settings)
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

            // This is the experimental REST API for mDNS Service Discovery
            for (const auto& host_address : host_addresses)
            {
                value mdns_service;
                auto mdns_uri = web::uri_builder()
                    .set_scheme(U("http"))
                    .set_host(host_address.as_string())
                    .set_port(nmos::experimental::fields::mdns_port(settings));
                mdns_service[U("href")] = value::string(mdns_uri.to_string());
                mdns_service[U("type")] = JU("urn:x-dns-sd/v1.0");
                web::json::push_back(data[U("services")], mdns_service);
            }

            data[U("clocks")] = value::array();

            // need to populate this... each interface needs chassis_id, port_id and a name which can be referenced by senders and receivers
            data[U("interfaces")] = value::array();

            return{ is04_versions::v1_2, types::node, data, true };
        }

        // insert a node resource according to the settings; return an iterator to the inserted node resource,
        // or to a resource that prevented the insertion, and a bool denoting whether the insertion took place
        std::pair<resources::iterator, bool> insert_registry_resources(nmos::resources& resources, const nmos::settings& settings)
        {
            const auto& seed_id = nmos::experimental::fields::seed_id(settings);
            auto node_id = nmos::make_repeatable_id(seed_id, U("/x-nmos/node/self"));

            return insert_resource(resources, make_registry_node(node_id, settings));
        }
    }
}

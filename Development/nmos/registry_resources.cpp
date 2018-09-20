#include "nmos/registry_resources.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/node_resource.h"

namespace nmos
{
    namespace experimental
    {
        nmos::resource make_registry_node(const nmos::id& id, const nmos::settings& settings)
        {
            using web::json::value;
            using web::json::value_of;

            auto resource = nmos::make_node(id, settings);
            auto& data = resource.data;

            const auto at_least_one_host_address = value_of({ value::string(nmos::fields::host_address(settings)) });
            const auto& host_addresses = settings.has_field(nmos::fields::host_addresses) ? nmos::fields::host_addresses(settings) : at_least_one_host_address.as_array();

            // This is the experimental REST API for mDNS Service Discovery
            for (const auto& host_address : host_addresses)
            {
                value mdns_service;
                auto mdns_uri = web::uri_builder()
                    .set_scheme(U("http"))
                    .set_host(host_address.as_string())
                    .set_port(nmos::experimental::fields::mdns_port(settings))
                    .to_uri();
                mdns_service[U("href")] = value::string(mdns_uri.to_string());
                mdns_service[U("type")] = value::string(U("urn:x-dns-sd/v1.0"));
                web::json::push_back(data[U("services")], mdns_service);
            }

            resource.health = health_forever;

            return resource;
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

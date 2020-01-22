#include "nmos/registry_resources.h"

#include "cpprest/uri_builder.h"
#include "nmos/api_utils.h" // for nmos::http_scheme
#include "nmos/node_interfaces.h"
#include "nmos/node_resource.h"

namespace nmos
{
    namespace experimental
    {
        nmos::resource make_registry_node(const nmos::id& id, const nmos::settings& settings)
        {
            using web::json::value;
            using web::json::value_of;

            auto resource = nmos::make_node(id, {}, nmos::make_node_interfaces(nmos::experimental::node_interfaces()), settings);
            auto& data = resource.data;

            const auto at_least_one_host_address = value_of({ value::string(nmos::fields::host_address(settings)) });
            const auto& host_addresses = settings.has_field(nmos::fields::host_addresses) ? nmos::fields::host_addresses(settings) : at_least_one_host_address.as_array();

            // This is the experimental REST API for DNS Service Discovery (DNS-SD)

            auto mdns_uri = web::uri_builder()
                .set_scheme(nmos::http_scheme(settings))
                .set_port(nmos::experimental::fields::mdns_port(settings))
                .set_path(U("/x-dns-sd/v1.0"));
            auto type = U("urn:x-dns-sd/v1.0");

            if (nmos::experimental::fields::client_secure(settings))
            {
                web::json::push_back(data[U("services")], value_of({
                    { U("href"), mdns_uri.set_host(nmos::get_host(settings)).to_uri().to_string() },
                    { U("type"), type }
                }));
            }
            else for (const auto& host_address : host_addresses)
            {
                web::json::push_back(data[U("services")], value_of({
                    { U("href"), mdns_uri.set_host(host_address.as_string()).to_uri().to_string() },
                    { U("type"), type }
                }));
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

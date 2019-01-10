#include "nmos/node_resource.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/is04_versions.h"
#include "nmos/resource.h"

namespace nmos
{
    // See  https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/schemas/node.json
    nmos::resource make_node(const nmos::id& id, const nmos::settings& settings)
    {
        using web::json::value;
        using web::json::value_of;

        const auto hostname = !nmos::fields::host_name(settings).empty() ? nmos::fields::host_name(settings) : web::http::experimental::host_name();

        auto data = details::make_resource_core(id, settings);

        auto uri = web::uri_builder()
            .set_scheme(U("http"))
            .set_host(nmos::fields::host_address(settings))
            .set_port(nmos::fields::node_port(settings))
            .to_uri();

        data[U("href")] = value::string(uri.to_string());
        data[U("hostname")] = value::string(hostname);
        data[U("api")][U("versions")] = value_of({ U("v1.0"), U("v1.1"), U("v1.2") });

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

        data[U("interfaces")] = value::array();

        return{ is04_versions::v1_2, types::node, data, false };
    }
}

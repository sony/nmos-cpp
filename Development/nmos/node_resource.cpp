#include "nmos/node_resource.h"

#include "cpprest/host_utils.h"
#include "cpprest/uri_builder.h"
#include "nmos/api_utils.h" // for nmos::http_scheme
#include "nmos/is04_versions.h"
#include "nmos/resource.h"

namespace nmos
{
    // See  https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/schemas/node.json
    nmos::resource make_node(const nmos::id& id, const nmos::settings& settings)
    {
        using web::json::value;
        using web::json::value_from_elements;
        using web::json::value_of;

        auto data = details::make_resource_core(id, settings);

        auto uri = web::uri_builder()
            .set_scheme(nmos::http_scheme(settings))
            .set_host(nmos::get_host(settings))
            .set_port(nmos::fields::node_port(settings))
            .to_uri();

        data[U("href")] = value::string(uri.to_string());
        data[U("hostname")] = value::string(nmos::get_host_name(settings));
        data[U("api")][U("versions")] = value_from_elements(nmos::is04_versions::from_settings(settings) | boost::adaptors::transformed(make_api_version));

        const auto at_least_one_host_address = value_of({ value::string(nmos::fields::host_address(settings)) });
        const auto& host_addresses = settings.has_field(nmos::fields::host_addresses) ? nmos::fields::host_addresses(settings) : at_least_one_host_address.as_array();

        if (nmos::experimental::fields::client_secure(settings))
        {
            web::json::push_back(data[U("api")][U("endpoints")], value_of({
                { U("host"), uri.host() },
                { U("port"), uri.port() },
                { U("protocol"), uri.scheme() }
            }));
        }
        else for (const auto& host_address : host_addresses)
        {
            web::json::push_back(data[U("api")][U("endpoints")], value_of({
                { U("host"), host_address },
                { U("port"), uri.port() },
                { U("protocol"), uri.scheme() }
            }));
        }

        data[U("caps")] = value::object();

        data[U("services")] = value::array();

        data[U("clocks")] = value::array();

        data[U("interfaces")] = value::array();

        return{ is04_versions::v1_3, types::node, data, false };
    }
}

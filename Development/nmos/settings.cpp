#include "nmos/settings.h"

#include "cpprest/host_utils.h"
#include "nmos/id.h"

namespace nmos
{
    namespace details
    {
        // Inserts run-time default settings for those which are impossible to determine at compile-time
        // if not already present in the specified settings
        void insert_default_settings(settings& settings, bool registry)
        {
            web::json::insert(settings, std::make_pair(nmos::experimental::fields::seed_id, web::json::value::string(nmos::make_id())));

            // if the "host_addresses" setting was omitted, add all the interface addresses
            if (!settings.has_field(nmos::fields::host_addresses))
            {
                const auto interfaces = web::hosts::experimental::host_interfaces();
                if (!interfaces.empty())
                {
                    std::vector<utility::string_t> addresses;
                    for (auto& interface : interfaces)
                    {
                        addresses.insert(addresses.end(), interface.addresses.begin(), interface.addresses.end());
                    }

                    web::json::insert(settings, std::make_pair(nmos::fields::host_addresses, web::json::value_from_elements(addresses)));
                }
            }

            // if the "host_address" setting was omitted, use the first of the "host_addresses"
            if (settings.has_field(nmos::fields::host_addresses))
            {
                web::json::insert(settings, std::make_pair(nmos::fields::host_address, nmos::fields::host_addresses(settings)[0]));
            }

            // if any of the specific "<api>_port" settings were omitted, use "http_port" if present
            if (settings.has_field(nmos::fields::http_port))
            {
                const auto http_port = nmos::fields::http_port(settings);
                if (registry) web::json::insert(settings, std::make_pair(nmos::fields::query_port, http_port));
                // can't share a port between an http_listener and a websocket_listener, so don't apply this one...
                //if (registry) web::json::insert(settings, std::make_pair(nmos::fields::query_ws_port, http_port));
                web::json::insert(settings, std::make_pair(nmos::fields::registration_port, http_port));
                web::json::insert(settings, std::make_pair(nmos::fields::node_port, http_port));
                if (registry) web::json::insert(settings, std::make_pair(nmos::fields::system_port, http_port));
                if (!registry) web::json::insert(settings, std::make_pair(nmos::fields::connection_port, http_port));
                if (!registry) web::json::insert(settings, std::make_pair(nmos::fields::events_port, http_port));
                // can't share a port between an http_listener and a websocket_listener, so don't apply this one...
                //if (!registry) web::json::insert(settings, std::make_pair(nmos::fields::events_ws_port, http_port));
                web::json::insert(settings, std::make_pair(nmos::experimental::fields::settings_port, http_port));
                web::json::insert(settings, std::make_pair(nmos::experimental::fields::logging_port, http_port));
                if (registry) web::json::insert(settings, std::make_pair(nmos::experimental::fields::admin_port, http_port));
                if (registry) web::json::insert(settings, std::make_pair(nmos::experimental::fields::mdns_port, http_port));
            }
        }
    }

    void insert_node_default_settings(settings& settings)
    {
        details::insert_default_settings(settings, false);
    }

    void insert_registry_default_settings(settings& settings)
    {
        details::insert_default_settings(settings, true);
    }

    // Get host name from settings or return the default (system) host name
    utility::string_t get_host_name(const settings& settings)
    {
        return !nmos::fields::host_name(settings).empty()
            ? nmos::fields::host_name(settings)
            : web::hosts::experimental::host_name();
    }

    // Get host name or address to be used to construct response headers (e.g. 'Link' or 'Location')
    // when a request URL is not available
    utility::string_t get_host(const settings& settings)
    {
        // if secure communications are in use, return a host name
        // otherwise, the preferred host address
        return nmos::experimental::fields::client_secure(settings)
            ? get_host_name(settings)
            : nmos::fields::host_address(settings);
    }
}

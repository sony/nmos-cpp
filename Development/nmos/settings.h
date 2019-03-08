#ifndef NMOS_SETTINGS_H
#define NMOS_SETTINGS_H

#include "cpprest/json_utils.h"

// Configuration settings and defaults for the NMOS Node, Query and Registration APIs, and the Connection API
namespace nmos
{
    typedef web::json::value settings;

    // Inserts run-time default settings for those which are impossible to determine at compile-time
    // if not already present in the specified settings
    void insert_node_default_settings(settings& settings);
    void insert_registry_default_settings(settings& settings);

    // Get host name from settings or return the default (system) host name
    utility::string_t get_host_name(const settings& settings);

    // Get host name or address to be used to construct response headers (e.g. 'Link' or 'Location')
    // when a request URL is not available
    utility::string_t get_host(const settings& settings);

    // Field accessors simplify access to fields in the settings and provide the compile-time defaults
    namespace fields
    {
        // error_log [registry, node]: filename for the error log or an empty string to write to stderr
        const web::json::field_as_string_or error_log{ U("error_log"), U("") };

        // access_log [registry, node]: filename for the access log (in Common Log Format) or an empty string to discard
        const web::json::field_as_string_or access_log{ U("access_log"), U("") };

        // logging_level [registry, node]: integer value, between 40 (least verbose, only fatal messages) and -40 (most verbose)
        const web::json::field_as_integer_or logging_level{ U("logging_level"), 0 }; // 0, rather than slog::severities::info or slog::nil_severity, just to avoid a #include

        // allow_invalid_resources [registry]: boolean value, true (cope with out-of-order Ledger and LAWO registrations) or false (a little less lax)
        const web::json::field_as_bool_or allow_invalid_resources{ U("allow_invalid_resources"), false };

        // host_name [registry, node]: the host name for which to advertise services or an empty string to use the default
        const web::json::field_as_string_or host_name{ U("host_name"), U("") };

        // domain [registry, node]: the domain on which to browse for services or an empty string to use the default domain (local/mDNS)
        const web::json::field_as_string_or domain{ U("domain"), U("") };

        // host_address/host_addresses [registry, node]: used to construct response headers (e.g. 'Link' or 'Location') and URL fields in the data model
        const web::json::field_as_string_or host_address{ U("host_address"), U("127.0.0.1") };
        const web::json::field_as_array host_addresses{ U("host_addresses") };

        // is04_versions [registry, node]: used to specify the enabled API versions (advertised via 'api_ver') for a version-locked configuration
        const web::json::field_as_array is04_versions{ U("is04_versions") }; // when omitted, nmos::is04_versions::all is used

        // is05_versions [node]: used to specify the enabled API versions for a version-locked configuration
        const web::json::field_as_array is05_versions{ U("is05_versions") }; // when omitted, nmos::is05_versions::all is used

        // pri [registry, node]: used for the 'pri' TXT record; specifying nmos::service_priorities::no_priority (maximum value) disables advertisement completely
        const web::json::field_as_integer_or pri{ U("pri"), 100 }; // default to highest_development_priority

        // highest_pri, lowest_pri [node]: used to specify the (inclusive) range of suitable 'pri' values of discovered APIs, to avoid development and live systems colliding
        const web::json::field_as_integer_or highest_pri{ U("highest_pri"), 0 }; // default to highest_active_priority; specifying no_priority disables discovery completely
        const web::json::field_as_integer_or lowest_pri{ U("lowest_pri"), (std::numeric_limits<int>::max)() }; // default to no_priority

        // discovery_backoff_min/discovery_backoff_max/discovery_backoff_factor [node]: used to back-off after errors interacting with all discoverable Registration APIs
        const web::json::field_as_integer_or discovery_backoff_min{ U("discovery_backoff_min"), 1 };
        const web::json::field_as_integer_or discovery_backoff_max{ U("discovery_backoff_max"), 30 };
        const web::json::field_with_default<double> discovery_backoff_factor{ U("discovery_backoff_factor"), 1.5 };

        // registry_address [node]: used to construct request URLs for registry APIs (if not discovered via DNS-SD)
        const web::json::field_as_string registry_address{ U("registry_address") };

        // registry_version [node]: used to construct request URLs for registry APIs (if not discovered via DNS-SD)
        const web::json::field_as_string_or registry_version{ U("registry_version"), U("v1.2") };

        // port numbers [registry, node]: ports to which clients should connect for each API

        // http_port [registry, node]: if specified, used in preference to the individual defaults for each HTTP API
        const web::json::field_as_integer_or http_port{ U("http_port"), 0 };

        const web::json::field_as_integer_or query_port{ U("query_port"), 3211 };
        const web::json::field_as_integer_or query_ws_port{ U("query_ws_port"), 3213 };
        // registration_port [node]: used to construct request URLs for the registry's Registration API (if not discovered via DNS-SD)
        const web::json::field_as_integer_or registration_port{ U("registration_port"), 3210 };
        const web::json::field_as_integer_or node_port{ U("node_port"), 3212 };
        const web::json::field_as_integer_or connection_port{ U("connection_port"), 3215 };
        const web::json::field_as_integer_or system_port{ U("system_port"), 10641 };

        // listen_backlog [registry, node]: the maximum length of the queue of pending connections, or zero for the implementation default (the implementation may not honour this value)
        const web::json::field_as_integer_or listen_backlog{ U("listen_backlog"), 0 };

        // registration_services [node]: the discovered list of Registration APIs, in the order they should be used
        // this list is created and maintained by nmos::node_behaviour_thread; each entry is a uri like http://example.api.com/x-nmos/registration/{version}
        const web::json::field_as_value registration_services{ U("registration_services") };

        // registration_heartbeat_interval [node]:
        // "Nodes are expected to peform a heartbeat every 5 seconds by default."
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#heartbeating
        const web::json::field_as_integer_or registration_heartbeat_interval{ U("registration_heartbeat_interval"), 5 };

        // registration_expiry_interval [registry]:
        // "Registration APIs should use a garbage collection interval of 12 seconds by default (triggered just after two failed heartbeats at the default 5 second interval)."
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#heartbeating
        const web::json::field_as_integer_or registration_expiry_interval{ U("registration_expiry_interval"), 12 };

        // registration_request_max [node]: timeout for interactions with the Registration API /resource endpoint
        const web::json::field_as_integer_or registration_request_max{ U("registration_request_max"), 30 };

        // registration_heartbeat_max [node]: timeout for interactions with the Registration API /health/nodes endpoint
        // Note that the node needs to be able to get a response to heartbeats (if not other requests) within the garbage collection interval of the Registration API on average,
        // but the worst case which could avoid triggering garbage collection is (almost) twice this value... see registration_expiry_interval
        const web::json::field_as_integer_or registration_heartbeat_max{ U("registration_heartbeat_max"), 2 * 12 };

        // query_paging_default/query_paging_limit [registry]: default/maximum number of results per "page" when using the Query API (a client may request a lower limit)
        const web::json::field_as_integer_or query_paging_default{ U("query_paging_default"), 10 };
        const web::json::field_as_integer_or query_paging_limit{ U("query_paging_limit"), 100 };

        // immediate_activation_max [node]: timeout for immediate activations within the Connection API /staged endpoint
        const web::json::field_as_integer_or immediate_activation_max{ U("immediate_activation_max"), 30 };
    }
}

// Configuration settings and defaults for experimental extensions
namespace nmos
{
    namespace experimental
    {
        // Field accessors simplify access to fields in the settings and provide the compile-time defaults
        namespace fields
        {
            // seed id [registry, node]: optional, used to generate repeatable id values when running with the same configuration
            const web::json::field_as_string seed_id{ U("seed_id") };

            // label [registry, node]: used in resource description/label fields
            const web::json::field_as_string_or label{ U("label"), U("") };

            // registration_available [registry]: used to flag the Registration API as temporarily unavailable
            const web::json::field_as_bool_or registration_available{ U("registration_available"), true };

            // port numbers [registry, node]: ports to which clients should connect for each API
            // see http_port

            const web::json::field_as_integer_or settings_port{ U("settings_port"), 3209 };
            const web::json::field_as_integer_or logging_port{ U("logging_port"), 5106 };

            // port numbers [registry]: ports to which clients should connect for each API
            // see http_port

            const web::json::field_as_integer_or admin_port{ U("admin_port"), 3208 };
            const web::json::field_as_integer_or mdns_port{ U("mdns_port"), 3214 };

            // port number for event-tally api
            const web::json::field_as_integer_or event_tally_port{ U("event_tally_port"), 3220 };

            // addresses [registry, node]: addresses on which to listen for each API, or empty string for the wildcard address

            const web::json::field_as_string_or settings_address{ U("settings_address"), U("") };
            const web::json::field_as_string_or logging_address{ U("logging_address"), U("") };

            // addresses [registry]: addresses on which to listen for each API, or empty string for the wildcard address

            const web::json::field_as_string_or admin_address{ U("admin_address"), U("") };
            const web::json::field_as_string_or mdns_address{ U("mdns_address"), U("") };

            // logging_limit [registry, node]: maximum number of log events cached for the Logging API
            const web::json::field_as_integer_or logging_limit{ U("logging_limit"), 1234 };

            // logging_paging_default/logging_paging_limit [registry, node]: default/maximum number of results per "page" when using the Logging API (a client may request a lower limit)
            const web::json::field_as_integer_or logging_paging_default{ U("logging_paging_default"), 100 };
            const web::json::field_as_integer_or logging_paging_limit{ U("logging_paging_limit"), 100 };

            // proxy_map [registry, node]: mapping between the port numbers to which the client connects, and the port numbers on which the server should listen, if different
            // each element of the array is an object like { "client_port": 80, "server_port": 8080 }
            const web::json::field_as_value_or proxy_map{ U("proxy_map"), web::json::value::array() };

            // proxy_address [registry, node]: address of the forward proxy to use when making HTTP requests or WebSocket connections, or an empty string for no proxy
            const web::json::field_as_string_or proxy_address{ U("proxy_address"), U("") };

            // proxy_port [registry, node]: forward proxy port
            const web::json::field_as_integer_or proxy_port{ U("proxy_port"), 8080 };
        }
    }
}

#endif

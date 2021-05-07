#ifndef NMOS_SETTINGS_H
#define NMOS_SETTINGS_H

#include "cpprest/json_utils.h"

namespace web
{
    namespace hosts
    {
        namespace experimental
        {
            struct host_interface;
        }
    }
}

// Configuration settings and defaults
namespace nmos
{
    typedef web::json::value settings;

    // Inserts run-time default settings for those which are impossible to determine at compile-time
    // if not already present in the specified settings
    void insert_node_default_settings(settings& settings);
    void insert_registry_default_settings(settings& settings);

    // Get domain from settings or return the default (system) domain
    utility::string_t get_domain(const settings& settings);

    // Get host name from settings or return the default (system) host name
    utility::string_t get_host_name(const settings& settings);

    // Get host name or address to be used to construct response headers (e.g. 'Link' or 'Location')
    // when a request URL is not available
    utility::string_t get_host(const settings& settings);

    // Get host name and/or addresses to be used to construct host and URL fields in the data model
    std::vector<utility::string_t> get_hosts(const settings& settings);

    // Get interfaces corresponding to the host addresses in the settings
    std::vector<web::hosts::experimental::host_interface> get_host_interfaces(const settings& settings);

    // Get a summary of the build configuration, including versions of dependencies
    utility::string_t get_build_settings_info();

    // Field accessors simplify access to fields in the settings and provide the compile-time defaults
    namespace fields
    {
        // Configuration settings and defaults for logging

        // error_log [registry, node]: filename for the error log or an empty string to write to stderr
        const web::json::field_as_string_or error_log{ U("error_log"), U("") };

        // access_log [registry, node]: filename for the access log (in Common Log Format) or an empty string to discard
        const web::json::field_as_string_or access_log{ U("access_log"), U("") };

        // logging_level [registry, node]: integer value, between 40 (least verbose, only fatal messages) and -40 (most verbose)
        const web::json::field_as_integer_or logging_level{ U("logging_level"), 0 }; // 0, rather than slog::severities::info or slog::nil_severity, just to avoid a #include

        // logging_categories [registry, node]: array of logging categories to be included in the error log
        const web::json::field_as_array logging_categories{ U("logging_categories") }; // when omitted, all log messages are included

        // Configuration settings and defaults for the NMOS APIs

        // host_name [registry, node]: the fully-qualified host name for which to advertise services, also used to construct response headers and fields in the data model
        const web::json::field_as_string_or host_name{ U("host_name"), U("") }; // when omitted or an empty string, the default is used

        // domain [registry, node]: the domain on which to browse for services or an empty string to use the default domain (specify "local." to explictly select mDNS)
        const web::json::field_as_string_or domain{ U("domain"), U("") };

        // host_address/host_addresses [registry, node]: IP addresses used to construct response headers (e.g. 'Link' or 'Location'), and host and URL fields in the data model
        const web::json::field_as_string_or host_address{ U("host_address"), U("127.0.0.1") };
        const web::json::field_as_array host_addresses{ U("host_addresses") };

        // is04_versions [registry, node]: used to specify the enabled API versions (advertised via 'api_ver') for a version-locked configuration
        const web::json::field_as_array is04_versions{ U("is04_versions") }; // when omitted, nmos::is04_versions::all is used

        // is05_versions [node]: used to specify the enabled API versions for a version-locked configuration
        const web::json::field_as_array is05_versions{ U("is05_versions") }; // when omitted, nmos::is05_versions::all is used

        // is07_versions [node]: used to specify the enabled API versions for a version-locked configuration
        const web::json::field_as_array is07_versions{ U("is07_versions") }; // when omitted, nmos::is07_versions::all is used

        // is08_versions [node]: used to specify the enabled API versions for a version-locked configuration
        const web::json::field_as_array is08_versions{ U("is08_versions") }; // when omitted, nmos::is08_versions::all is used

        // is09_versions [registry, node]: used to specify the enabled API versions for a version-locked configuration
        const web::json::field_as_array is09_versions{ U("is09_versions") }; // when omitted, nmos::is09_versions::all is used

        // pri [registry, node]: used for the 'pri' TXT record; specifying nmos::service_priorities::no_priority (maximum value) disables advertisement completely
        const web::json::field_as_integer_or pri{ U("pri"), 100 }; // default to highest_development_priority

        // highest_pri, lowest_pri [node]: used to specify the (inclusive) range of suitable 'pri' values of discovered APIs, to avoid development and live systems colliding
        const web::json::field_as_integer_or highest_pri{ U("highest_pri"), 0 }; // default to highest_active_priority; specifying no_priority disables discovery completely
        const web::json::field_as_integer_or lowest_pri{ U("lowest_pri"), (std::numeric_limits<int>::max)() }; // default to no_priority

        // discovery_backoff_min/discovery_backoff_max/discovery_backoff_factor [node]: used to back-off after errors interacting with all discoverable Registration APIs or System APIs
        const web::json::field_as_integer_or discovery_backoff_min{ U("discovery_backoff_min"), 1 };
        const web::json::field_as_integer_or discovery_backoff_max{ U("discovery_backoff_max"), 30 };
        const web::json::field_with_default<double> discovery_backoff_factor{ U("discovery_backoff_factor"), 1.5 };

        // registry_address [node]: IP address or host name used to construct request URLs for registry APIs (if not discovered via DNS-SD)
        const web::json::field_as_string registry_address{ U("registry_address") };

        // registry_version [node]: used to construct request URLs for registry APIs (if not discovered via DNS-SD)
        const web::json::field_as_string_or registry_version{ U("registry_version"), U("v1.3") };

        // port numbers [registry, node]: ports to which clients should connect for each API

        // http_port [registry, node]: if specified, used in preference to the individual defaults for each HTTP API
        const web::json::field_as_integer_or http_port{ U("http_port"), 0 };

        const web::json::field_as_integer_or query_port{ U("query_port"), 3211 };
        const web::json::field_as_integer_or query_ws_port{ U("query_ws_port"), 3213 };
        // registration_port [node]: used to construct request URLs for the registry's Registration API (if not discovered via DNS-SD)
        const web::json::field_as_integer_or registration_port{ U("registration_port"), 3210 };
        const web::json::field_as_integer_or node_port{ U("node_port"), 3212 };
        const web::json::field_as_integer_or connection_port{ U("connection_port"), 3215 };
        const web::json::field_as_integer_or events_port{ U("events_port"), 3216 };
        const web::json::field_as_integer_or events_ws_port{ U("events_ws_port"), 3217 };
        const web::json::field_as_integer_or channelmapping_port{ U("channelmapping_port"), 3215 };
        // system_port [node]: used to construct request URLs for the System API (if not discovered via DNS-SD)
        const web::json::field_as_integer_or system_port{ U("system_port"), 10641 };

        // listen_backlog [registry, node]: the maximum length of the queue of pending connections, or zero for the implementation default (the implementation may not honour this value)
        const web::json::field_as_integer_or listen_backlog{ U("listen_backlog"), 0 };

        // registration_services [node]: the discovered list of Registration APIs, in the order they should be used
        // this list is created and maintained by nmos::node_behaviour_thread; each entry is a uri like http://api.example.com/x-nmos/registration/{version}
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
        // Note that the default timeout is the same as the default heartbeat interval, in order that there is then a reasonable opportunity to try the next available Registration API
        // though in some circumstances registration expiry could potentially still be avoided with a timeout that is (almost) twice the garbage collection interval...
        const web::json::field_as_integer_or registration_heartbeat_max{ U("registration_heartbeat_max"), 5 };

        // query_paging_default/query_paging_limit [registry]: default/maximum number of results per "page" when using the Query API (a client may request a lower limit)
        const web::json::field_as_integer_or query_paging_default{ U("query_paging_default"), 10 };
        const web::json::field_as_integer_or query_paging_limit{ U("query_paging_limit"), 100 };

        // ptp_announce_receipt_timeout [registry]: number of PTP announce intervals that must pass before declaring timeout, between 2 and 10 inclusive (default is 3, as per SMPTE ST 2059-2)
        const web::json::field_as_integer_or ptp_announce_receipt_timeout{ U("ptp_announce_receipt_timeout"), 3 };

        // ptp_domain_number [registry]: the PTP domain number, between 0 and 127 (default is 127, as per SMPTE ST 2059-2)
        const web::json::field_as_integer_or ptp_domain_number{ U("ptp_domain_number"), 127 };

        // immediate_activation_max [node]: timeout for immediate activations within the Connection API /staged endpoint
        const web::json::field_as_integer_or immediate_activation_max{ U("immediate_activation_max"), 30 };

        // events_heartbeat_interval [node, client]:
        // "Upon connection, the client is required to report its health every 5 seconds in order to maintain its session and subscription."
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.2.%20Transport%20-%20Websocket.md#41-heartbeats
        const web::json::field_as_integer_or events_heartbeat_interval{ U("events_heartbeat_interval"), 5 };

        // events_expiry_interval [node]:
        // "The server is expected to check health commands and after a 12 seconds timeout (2 consecutive missed health commands plus 2 seconds to allow for latencies)
        // it should clear the subscriptions for that particular client and close the websocket connection."
        // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.0/docs/5.2.%20Transport%20-%20Websocket.md#41-heartbeats
        const web::json::field_as_integer_or events_expiry_interval{ U("events_expiry_interval"), 12 };

        // system_services [node]: the discovered list of System APIs, in the order they should be used
        // this list is created and maintained by nmos::node_system_behaviour_thread; each entry is a uri like http://api.example.com/x-nmos/system/{version}
        const web::json::field_as_value system_services{ U("system_services") };

        // system_address [node]: IP address or host name used to construct request URLs for the System API (if not discovered via DNS-SD)
        const web::json::field_as_string system_address{ U("system_address") };

        // system_version [node]: used to construct request URLs for the System API (if not discovered via DNS-SD)
        const web::json::field_as_string_or system_version{ U("system_version"), U("v1.0") };

        // system_request_max [node]: timeout for interactions with the System API
        const web::json::field_as_integer_or system_request_max{ U("system_request_max"), 30 };
    }

    // Configuration settings and defaults for experimental extensions
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

            // allow_invalid_resources [registry]: boolean value, true (attempt to ignore schema validation errors and cope with out-of-order registrations) or false (default)
            const web::json::field_as_bool_or allow_invalid_resources{ U("allow_invalid_resources"), false };

            // port numbers [registry, node]: ports to which clients should connect for each API
            // see http_port

            const web::json::field_as_integer_or manifest_port{ U("manifest_port"), 3212 };
            const web::json::field_as_integer_or settings_port{ U("settings_port"), 3209 };
            const web::json::field_as_integer_or logging_port{ U("logging_port"), 5106 };

            // port numbers [registry]: ports to which clients should connect for each API
            // see http_port

            const web::json::field_as_integer_or admin_port{ U("admin_port"), 3208 };
            const web::json::field_as_integer_or mdns_port{ U("mdns_port"), 3208 };
            const web::json::field_as_integer_or schemas_port{ U("schemas_port"), 3208 };

            // addresses [registry, node]: addresses on which to listen for each API, or empty string for the wildcard address

            const web::json::field_as_string_or settings_address{ U("settings_address"), U("") };
            const web::json::field_as_string_or logging_address{ U("logging_address"), U("") };

            // addresses [registry]: addresses on which to listen for each API, or empty string for the wildcard address

            const web::json::field_as_string_or admin_address{ U("admin_address"), U("") };
            const web::json::field_as_string_or mdns_address{ U("mdns_address"), U("") };
            const web::json::field_as_string_or schemas_address{ U("schemas_address"), U("") };

            // query_ws_paging_default/query_ws_paging_limit [registry]: default/maximum number of events per message when using the Query WebSocket API (a client may request a lower limit)
            const web::json::field_as_integer_or query_ws_paging_default{ U("query_ws_paging_default"), 10 };
            const web::json::field_as_integer_or query_ws_paging_limit{ U("query_ws_paging_limit"), 100 };

            // logging_limit [registry, node]: maximum number of log events cached for the Logging API
            const web::json::field_as_integer_or logging_limit{ U("logging_limit"), 1234 };

            // logging_paging_default/logging_paging_limit [registry, node]: default/maximum number of results per "page" when using the Logging API (a client may request a lower limit)
            const web::json::field_as_integer_or logging_paging_default{ U("logging_paging_default"), 100 };
            const web::json::field_as_integer_or logging_paging_limit{ U("logging_paging_limit"), 100 };

            // http_trace [registry, node]: whether server should enable (default) or disable support for HTTP TRACE
            const web::json::field_as_bool_or http_trace{ U("http_trace"), true };

            // proxy_map [registry, node]: mapping between the port numbers to which the client connects, and the port numbers on which the server should listen, if different
            // for use with a reverse proxy; each element of the array is an object like { "client_port": 80, "server_port": 8080 }
            const web::json::field_as_value_or proxy_map{ U("proxy_map"), web::json::value::array() };

            // proxy_address [registry, node]: address of the forward proxy to use when making HTTP requests or WebSocket connections, or an empty string for no proxy
            const web::json::field_as_string_or proxy_address{ U("proxy_address"), U("") };

            // proxy_port [registry, node]: forward proxy port
            const web::json::field_as_integer_or proxy_port{ U("proxy_port"), 8080 };

            // href_mode [registry, node]: whether the host name (1), addresses (2) or both (3) are used to construct response headers, and host and URL fields in the data model
            const web::json::field_as_integer_or href_mode{ U("href_mode"), 0 }; // when omitted, a default heuristic is used

            // client_secure [registry, node]: whether clients should use a secure connection for communication (https and wss)
            // when true, CA root certificates must also be configured, see nmos/certificate_settings.h
            const web::json::field_as_bool_or client_secure{ U("client_secure"), false };

            // server_secure [registry, node]: whether server should listen for secure connection for communication (https and wss)
            // e.g. typically false when using a reverse proxy, or the same as client_secure otherwise
            // when true, server certificates etc. must also be configured, see nmos/certificate_settings.h
            const web::json::field_as_bool_or server_secure{ U("server_secure"), false };

            // validate_certificates [registry, node]: boolean value, false (ignore all server certificate validation errors), or true (do not ignore, the default behaviour)
            const web::json::field_as_bool_or validate_certificates{ U("validate_certificates"), true };

            // system_interval_min/system_interval_max [node]: used to poll for System API changes; default is about one hour
            const web::json::field_as_integer_or system_interval_min{ U("system_interval_min"), 3600 };
            const web::json::field_as_integer_or system_interval_max{ U("system_interval_max"), 3660 };
        }
    }
}

#endif

#ifndef NMOS_SETTINGS_H
#define NMOS_SETTINGS_H

#include "cpprest/json_utils.h"

// Configuration settings and defaults for the NMOS Node, Query and Registration APIs, and the Connection API
namespace nmos
{
    typedef web::json::value settings;

    // Field accessors simplify access to fields in the settings
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

        // host_name [registry, node]: used in resource description/label fields
        const web::json::field_as_string_or host_name{ U("host_name"), U("localhost") };

        // host_address [registry, node]: used to construct response headers and URL fields
        const web::json::field_as_string_or host_address{ U("host_address"), U("127.0.0.1") };

        // pri [registry, node]: used for the 'pri' TXT record
        const web::json::field_as_integer_or pri{ U("pri"), 100 }; // default to highest_development_priority

        // registry_address [node]: used to make requests on registry APIs
        const web::json::field_as_string_or registry_address{ U("registry_address"), U("127.0.0.1") };

        // registry_version [node]: used to make requests on registry APIs
        const web::json::field_as_string_or registry_version{ U("registry_version"), U("v1.2") };

        // port numbers [registry, node]: ports on which to listen for each API

        const web::json::field_as_integer_or query_port{ U("query_port"), 3211 };
        const web::json::field_as_integer_or query_ws_port{ U("query_ws_port"), 3213 };
        // registration_port [node]: used to make requests on the registry's Registration API
        const web::json::field_as_integer_or registration_port{ U("registration_port"), 3210 };
        const web::json::field_as_integer_or node_port{ U("node_port"), 3212 };
        const web::json::field_as_integer_or connection_port{ U("connection_port"), 3215 };

        // listen_backlog [registry, node]: the maximum length of the queue of pending connections, or zero for the implementation default (the implementation may not honour this value)
        const web::json::field_as_integer_or listen_backlog{ U("listen_backlog"), 0 };

        // registration_heartbeat_interval [node]:
        // "Nodes are expected to peform a heartbeat every 5 seconds by default."
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#heartbeating
        const web::json::field_as_integer_or registration_heartbeat_interval{ U("registration_heartbeat_interval"), 5 };

        // registration_expiry_interval [registry]:
        // "Registration APIs should use a garbage collection interval of 12 seconds by default (triggered just after two failed heartbeats at the default 5 second interval)."
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#heartbeating
        const web::json::field_as_integer_or registration_expiry_interval{ U("registration_expiry_interval"), 12 };

        // query_paging_default/query_paging_limit [registry]: default/maximum number of results per "page" when using the Query API (a client may request a lower limit)
        const web::json::field_as_integer_or query_paging_default{ U("query_paging_default"), 10 };
        const web::json::field_as_integer_or query_paging_limit{ U("query_paging_limit"), 100 };
    }
}

// Configuration settings and defaults for experimental extensions
namespace nmos
{
    namespace experimental
    {
        // Field accessors simplify access to fields in the settings
        namespace fields
        {
            // port numbers [registry, node]: ports on which to listen for each API

            const web::json::field_as_integer_or settings_port{ U("settings_port"), 3209 };
            const web::json::field_as_integer_or logging_port{ U("logging_port"), 5106 };

            // port numbers [registry]: ports on which to listen for each API

            const web::json::field_as_integer_or admin_port{ U("admin_port"), 3208 };
            const web::json::field_as_integer_or mdns_port{ U("mdns_port"), 3214 };
        }
    }
}

#endif

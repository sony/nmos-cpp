#ifndef NMOS_SETTINGS_H
#define NMOS_SETTINGS_H

#include "cpprest/json_utils.h"

// Configuration settings and defaults for the NMOS Node, Query and Registration APIs, and the Connection API
namespace nmos
{
    typedef web::json::value settings;

    namespace fields
    {
        // fields in settings
        const web::json::field_as_integer_or logging_level{ U("logging_level"), 0 }; // 0, rather than slog::severities::info or slog::nil_severity, just to avoid a #include
        const web::json::field_as_bool_or allow_invalid_resources{ U("allow_invalid_resources"), false };

        const web::json::field_as_string_or host_name{ U("host_name"), U("localhost") };
        const web::json::field_as_string_or host_address{ U("host_address"), U("127.0.0.1") };

        const web::json::field_as_integer_or query_port{ U("query_port"), 3211 };
        const web::json::field_as_integer_or query_ws_port{ U("query_ws_port"), 3213 };
        const web::json::field_as_integer_or registration_port{ U("registration_port"), 3210 };
        const web::json::field_as_integer_or node_port{ U("node_port"), 3212 };
        const web::json::field_as_integer_or connection_port{ U("connection_port"), 3215 };

        // "Registration APIs should use a garbage collection interval of 12 seconds by default (triggered just after two failed heartbeats at the default 5 second interval)."
        const web::json::field_as_integer_or registration_expiry_interval{ U("registration_expiry_interval"), 12 };

        // Default number of results obtained per "page" when using the Query API
        const web::json::field_as_integer_or query_paging_limit{ U("query_paging_limit"), 10 };
    }
}

// Configuration settings and defaults for experimental extensions
namespace nmos
{
    namespace experimental
    {
        namespace fields
        {
            // fields in settings
            const web::json::field_as_integer_or settings_port{ U("settings_port"), 3209 };
            const web::json::field_as_integer_or logging_port{ U("logging_port"), 5106 };
            const web::json::field_as_integer_or admin_port{ U("admin_port"), 3208 };
            const web::json::field_as_integer_or mdns_port{ U("mdns_port"), 3214 };
        }
    }
}

#endif

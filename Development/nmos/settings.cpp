#include "nmos/settings.h"

#include <map>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/find_first_of.hpp>
#include <boost/version.hpp>
#include <openssl/opensslv.h>
#include "cpprest/host_utils.h"
#include "cpprest/http_utils.h"
#include "cpprest/json_validator.h"
#include "cpprest/version.h"
#include "nmos/id.h"
#include "websocketpp/version.hpp"

namespace nmos
{
    namespace details
    {
        // Useful value-type definitions for composing application-specific settings
        // schemas; see settings_definitions_schema().
        static const char* settings_definitions_schema_text = R"-schema-(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "definitions": {
        "nonNegativeInteger": { "type": "integer", "minimum": 0 },
        "positiveInteger": { "type": "integer", "minimum": 1 },
        "rational": {
            "title": "nmos::rational",
            "type": "object",
            "required": ["numerator"],
            "properties": {
                "numerator":   { "type": "integer" },
                "denominator": { "type": "integer", "not": { "enum": [0] } }
            }
        },
        "stringArray": { "type": "array", "items": { "type": "string" } },
        "tags": {
            "type": "object",
            "additionalProperties": { "$ref": "#/definitions/stringArray" }
        },
        "uuid": {
            "title": "nmos::id",
            "type": "string",
            "pattern": "^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$"
        },
        "interlaceMode": {
            "title": "nmos::interlace_mode",
            "type": "string",
            "enum": ["progressive", "interlaced_tff", "interlaced_bff", "interlaced_psf"]
        },
        "colorspace": {
            "title": "nmos::colorspace",
            "type": "string",
            "enum": ["BT601", "BT709", "BT2020", "BT2100", "ST2065-1", "ST2065-3", "XYZ"]
        },
        "transferCharacteristic": {
            "title": "nmos::transfer_characteristic",
            "type": "string",
            "enum": ["SDR", "PQ", "HLG", "LINEAR", "BT2100LINPQ", "BT2100LINHLG", "ST2065-1", "ST428-1", "DENSITY"]
        },
        "colorSampling": {
            "title": "sdp::sampling",
            "type": "string",
            "enum": ["RGBA", "RGB", "YCbCr-4:4:4", "YCbCr-4:2:2", "YCbCr-4:2:0", "YCbCr-4:1:1", "CLYCbCr-4:4:4", "CLYCbCr-4:2:2", "CLYCbCr-4:2:0", "ICtCp-4:4:4", "ICtCp-4:2:2", "ICtCp-4:2:0", "XYZ", "KEY", "UNSPECIFIED"]
        }
    }
}
        )-schema-";

        const std::pair<web::uri, web::json::value>& settings_definitions_schema()
        {
            static const std::pair<web::uri, web::json::value> instance{
                web::uri{ U("urn:x-nmos-cpp:schemas:defs") },
                web::json::value::parse(settings_definitions_schema_text)
            };
            return instance;
        }

        // JSON schema covering the known properties in nmos/settings.h and nmos/certificate_settings.h
        // (used by both validate_node_settings and validate_registry_settings; additionalProperties is
        // not set to false, so fields not enumerated here pass through unchecked which is what we want
        // both for forward compatibility and to allow downstream validators to layer their own checks)
        // definitions: aliases (in settings_definitions_schema() source order) first, then local;
        // properties: in the same order as in the headers; please keep them in sync
        static const char* settings_schema_text = R"-schema-(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "type": "object",
    "definitions": {
        "nonNegativeInteger": { "$ref": "urn:x-nmos-cpp:schemas:defs#/definitions/nonNegativeInteger" },
        "positiveInteger":    { "$ref": "urn:x-nmos-cpp:schemas:defs#/definitions/positiveInteger" },
        "stringArray":        { "$ref": "urn:x-nmos-cpp:schemas:defs#/definitions/stringArray" },
        "tags":               { "$ref": "urn:x-nmos-cpp:schemas:defs#/definitions/tags" },
        "uuid":               { "$ref": "urn:x-nmos-cpp:schemas:defs#/definitions/uuid" },
        "apiVersion": { "type": "string", "pattern": "^v[0-9]+\\.[0-9]+$" },
        "apiVersionArray": { "type": "array", "items": { "$ref": "#/definitions/apiVersion" } },
        "port": { "type": "integer", "minimum": -1, "maximum": 65535 }
    },
    "properties": {
        "error_log": { "type": "string" },
        "access_log": { "type": "string" },
        "logging_level": { "type": "integer" },
        "logging_categories": { "$ref": "#/definitions/stringArray" },

        "host_name": { "type": "string" },
        "domain": { "type": "string" },
        "dns_sd_browse_mode": { "type": "integer", "enum": [0, 1, 2] },
        "host_address": { "type": "string" },
        "host_addresses": { "$ref": "#/definitions/stringArray" },

        "is04_versions": { "$ref": "#/definitions/apiVersionArray" },
        "is05_versions": { "$ref": "#/definitions/apiVersionArray" },
        "is07_versions": { "$ref": "#/definitions/apiVersionArray" },
        "is08_versions": { "$ref": "#/definitions/apiVersionArray" },
        "is09_versions": { "$ref": "#/definitions/apiVersionArray" },
        "is10_versions": { "$ref": "#/definitions/apiVersionArray" },
        "is12_versions": { "$ref": "#/definitions/apiVersionArray" },
        "is14_versions": { "$ref": "#/definitions/apiVersionArray" },

        "pri":                          { "$ref": "#/definitions/nonNegativeInteger" },
        "highest_pri":                  { "$ref": "#/definitions/nonNegativeInteger" },
        "lowest_pri":                   { "$ref": "#/definitions/nonNegativeInteger" },
        "authorization_highest_pri":    { "$ref": "#/definitions/nonNegativeInteger" },
        "authorization_lowest_pri":     { "$ref": "#/definitions/nonNegativeInteger" },

        "discovery_backoff_min":    { "$ref": "#/definitions/nonNegativeInteger" },
        "discovery_backoff_max":    { "$ref": "#/definitions/nonNegativeInteger" },
        "discovery_backoff_factor": { "type": "number", "minimum": 1 },

        "service_name_prefix": { "type": "string" },
        "registry_address":    { "type": "string" },
        "registry_version":    { "$ref": "#/definitions/apiVersion" },

        "http_port":                { "$ref": "#/definitions/port" },
        "query_port":               { "$ref": "#/definitions/port" },
        "query_ws_port":            { "$ref": "#/definitions/port" },
        "registration_port":        { "$ref": "#/definitions/port" },
        "node_port":                { "$ref": "#/definitions/port" },
        "connection_port":          { "$ref": "#/definitions/port" },
        "events_port":              { "$ref": "#/definitions/port" },
        "events_ws_port":           { "$ref": "#/definitions/port" },
        "channelmapping_port":      { "$ref": "#/definitions/port" },
        "system_port":              { "$ref": "#/definitions/port" },
        "control_protocol_ws_port": { "$ref": "#/definitions/port" },
        "configuration_port":       { "$ref": "#/definitions/port" },

        "listen_backlog":                  { "$ref": "#/definitions/nonNegativeInteger" },
        "registration_heartbeat_interval": { "$ref": "#/definitions/positiveInteger" },
        "registration_expiry_interval":    { "$ref": "#/definitions/positiveInteger" },
        "registration_request_max":        { "$ref": "#/definitions/positiveInteger" },
        "registration_heartbeat_max":      { "$ref": "#/definitions/positiveInteger" },

        "query_paging_default": { "$ref": "#/definitions/positiveInteger" },
        "query_paging_limit":   { "$ref": "#/definitions/positiveInteger" },

        "ptp_announce_receipt_timeout": { "type": "integer", "minimum": 2, "maximum": 10 },
        "ptp_domain_number":            { "type": "integer", "minimum": 0, "maximum": 127 },

        "immediate_activation_max":  { "$ref": "#/definitions/positiveInteger" },
        "events_heartbeat_interval": { "$ref": "#/definitions/positiveInteger" },
        "events_expiry_interval":    { "$ref": "#/definitions/positiveInteger" },

        "system_address":     { "type": "string" },
        "system_version":     { "$ref": "#/definitions/apiVersion" },
        "system_request_max": { "$ref": "#/definitions/positiveInteger" },

        "seed_id":                 { "$ref": "#/definitions/uuid" },
        "label":                   { "type": "string" },
        "description":             { "type": "string" },
        "registration_available":  { "type": "boolean" },
        "allow_invalid_resources": { "type": "boolean" },

        "manifest_port":    { "$ref": "#/definitions/port" },
        "settings_port":    { "$ref": "#/definitions/port" },
        "logging_port":     { "$ref": "#/definitions/port" },
        "admin_port":       { "$ref": "#/definitions/port" },
        "mdns_port":        { "$ref": "#/definitions/port" },
        "schemas_port":     { "$ref": "#/definitions/port" },

        "server_address":   { "type": "string" },
        "settings_address": { "type": "string" },
        "logging_address":  { "type": "string" },
        "admin_address":    { "type": "string" },
        "mdns_address":     { "type": "string" },
        "schemas_address":  { "type": "string" },
        "client_address":   { "type": "string" },

        "query_ws_paging_default": { "$ref": "#/definitions/positiveInteger" },
        "query_ws_paging_limit":   { "$ref": "#/definitions/positiveInteger" },
        "logging_limit":           { "$ref": "#/definitions/positiveInteger" },
        "logging_paging_default":  { "$ref": "#/definitions/positiveInteger" },
        "logging_paging_limit":    { "$ref": "#/definitions/positiveInteger" },

        "http_trace": { "type": "boolean" },

        "proxy_map": {
            "type": "array",
            "items": {
                "type": "object",
                "required": ["client_port", "server_port"],
                "properties": {
                    "client_port": { "$ref": "#/definitions/port" },
                    "server_port": { "$ref": "#/definitions/port" }
                }
            }
        },
        "proxy_address": { "type": "string" },
        "proxy_port":    { "$ref": "#/definitions/port" },

        "href_mode": { "type": "integer", "enum": [0, 1, 2, 3] },

        "client_secure":         { "type": "boolean" },
        "server_secure":         { "type": "boolean" },
        "validate_certificates": { "type": "boolean" },

        "system_interval_min": { "$ref": "#/definitions/positiveInteger" },
        "system_interval_max": { "$ref": "#/definitions/positiveInteger" },

        "system_label":       { "type": "string" },
        "system_description": { "type": "string" },
        "system_tags":        { "$ref": "#/definitions/tags" },

        "system_syslog_host_name":   { "type": "string" },
        "system_syslog_port":        { "$ref": "#/definitions/port" },
        "system_syslogv2_host_name": { "type": "string" },
        "system_syslogv2_port":      { "$ref": "#/definitions/port" },

        "hsts_max_age":             { "type": "integer" },
        "hsts_include_sub_domains": { "type": "boolean" },

        "ocsp_interval_min": { "$ref": "#/definitions/positiveInteger" },
        "ocsp_interval_max": { "$ref": "#/definitions/positiveInteger" },
        "ocsp_request_max":  { "$ref": "#/definitions/positiveInteger" },

        "authorization_selector":     { "type": "string" },
        "authorization_address":      { "type": "string" },
        "authorization_port":         { "$ref": "#/definitions/port" },
        "authorization_version":      { "$ref": "#/definitions/apiVersion" },
        "authorization_request_max":  { "$ref": "#/definitions/positiveInteger" },
        "fetch_authorization_public_keys_interval_min": { "$ref": "#/definitions/positiveInteger" },
        "fetch_authorization_public_keys_interval_max": { "$ref": "#/definitions/positiveInteger" },
        "access_token_refresh_interval":  { "type": "integer", "minimum": -1 },
        "client_authorization":           { "type": "boolean" },
        "server_authorization":           { "type": "boolean" },
        "authorization_code_flow_max":    { "type": "integer", "minimum": -1 },
        "authorization_flow":             { "type": "string", "enum": ["authorization_code", "client_credentials"] },
        "authorization_redirect_port":    { "$ref": "#/definitions/port" },
        "initial_access_token":           { "type": "string" },
        "authorization_scopes":           { "$ref": "#/definitions/stringArray" },
        "token_endpoint_auth_method": {
            "type": "string",
            "enum": ["none", "client_secret_basic", "client_secret_post", "private_key_jwt", "client_secret_jwt"]
        },
        "jwks_uri_port":                                  { "$ref": "#/definitions/port" },
        "validate_openid_client":                         { "type": "boolean" },
        "no_trailing_dot_for_authorization_callback_uri": { "type": "boolean" },
        "service_unavailable_retry_after":                { "$ref": "#/definitions/nonNegativeInteger" },

        "manufacturer_name":      { "type": "string" },
        "product_name":           { "type": "string" },
        "product_key":            { "type": "string" },
        "product_revision_level": { "type": "string" },
        "serial_number":          { "type": "string" },

        "ca_certificate_file": { "type": "string" },
        "server_certificates": {
            "type": "array",
            "items": {
                "type": "object",
                "required": ["private_key_file", "certificate_chain_file"],
                "properties": {
                    "key_algorithm":          { "type": "string", "enum": ["ECDSA", "RSA"] },
                    "private_key_file":       { "type": "string" },
                    "certificate_chain_file": { "type": "string" }
                }
            }
        },
        "dh_param_file":           { "type": "string" },
        "private_key_files":       { "$ref": "#/definitions/stringArray" },
        "certificate_chain_files": { "$ref": "#/definitions/stringArray" }
    }
}
        )-schema-";

        const std::pair<web::uri, web::json::value>& settings_schema()
        {
            static const std::pair<web::uri, web::json::value> instance{
                web::uri{ U("urn:x-nmos-cpp:schemas:settings") },
                web::json::value::parse(settings_schema_text)
            };
            return instance;
        }

        void validate_settings(const settings& settings)
        {
            static const std::map<web::uri, web::json::value> known{
                settings_schema(),
                settings_definitions_schema()
            };
            static const web::json::experimental::json_validator validator
            {
                [](const web::uri& wanted) { return known.at(wanted); },
                boost::copy_range<std::vector<web::uri>>(known | boost::adaptors::map_keys)
            };
            validator.validate(settings, settings_schema().first);
        }
    }

    void validate_node_settings(const settings& settings)
    {
        details::validate_settings(settings);
    }

    void validate_registry_settings(const settings& settings)
    {
        details::validate_settings(settings);
    }

    namespace details
    {
        // Get default DNS domain name
        utility::string_t default_domain()
        {
            for (const auto& interface : web::hosts::experimental::host_interfaces())
            {
                if (!interface.domain.empty())
                    return interface.domain;
            }
            return U("local.");
        }

        // Get default (system) host name as an FQDN (fully qualified domain name)
        utility::string_t default_host_name(const utility::string_t& domain = default_domain())
        {
            return web::hosts::experimental::host_name() + U('.') + domain;
        }

        // Inserts run-time default settings for those which are impossible to determine at compile-time
        // if not already present in the specified settings
        void insert_default_settings(settings& settings, bool registry)
        {
            // note that web::json::insert won't overwrite an existing entry, just like std::map::insert, etc.
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
                // can't share a port between an http_listener and a websocket_listener, so use next higher port
                const auto ws_port = http_port + 1;
                // can't share a port between the events ws and the control protocol ws
                const auto ncp_ws_port = ws_port + 1;
                if (registry) web::json::insert(settings, std::make_pair(nmos::fields::query_port, http_port));
                if (registry) web::json::insert(settings, std::make_pair(nmos::fields::query_ws_port, ws_port));
                if (registry) web::json::insert(settings, std::make_pair(nmos::fields::registration_port, http_port));
                web::json::insert(settings, std::make_pair(nmos::fields::node_port, http_port));
                if (registry) web::json::insert(settings, std::make_pair(nmos::fields::system_port, http_port));
                if (!registry) web::json::insert(settings, std::make_pair(nmos::fields::connection_port, http_port));
                if (!registry) web::json::insert(settings, std::make_pair(nmos::fields::events_port, http_port));
                if (!registry) web::json::insert(settings, std::make_pair(nmos::fields::channelmapping_port, http_port));
                if (!registry) web::json::insert(settings, std::make_pair(nmos::fields::events_ws_port, ws_port));
                if (!registry) web::json::insert(settings, std::make_pair(nmos::experimental::fields::manifest_port, http_port));
                web::json::insert(settings, std::make_pair(nmos::experimental::fields::settings_port, http_port));
                web::json::insert(settings, std::make_pair(nmos::experimental::fields::logging_port, http_port));
                if (registry) web::json::insert(settings, std::make_pair(nmos::experimental::fields::admin_port, http_port));
                if (registry) web::json::insert(settings, std::make_pair(nmos::experimental::fields::mdns_port, http_port));
                if (registry) web::json::insert(settings, std::make_pair(nmos::experimental::fields::schemas_port, http_port));
                web::json::insert(settings, std::make_pair(nmos::experimental::fields::authorization_redirect_port, http_port));
                web::json::insert(settings, std::make_pair(nmos::experimental::fields::jwks_uri_port, http_port));
                if (!registry) web::json::insert(settings, std::make_pair(nmos::fields::control_protocol_ws_port, ncp_ws_port));
                if (!registry) web::json::insert(settings, std::make_pair(nmos::fields::configuration_port, http_port));
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

    // Get domain from settings or return the default (system) domain
    utility::string_t get_domain(const settings& settings)
    {
        return !nmos::fields::domain(settings).empty()
            ? nmos::fields::domain(settings)
            : details::default_domain();
    }

    // Get host name from settings or return the default (system) host name
    utility::string_t get_host_name(const settings& settings)
    {
        return !nmos::fields::host_name(settings).empty()
            ? nmos::fields::host_name(settings)
            : details::default_host_name(get_domain(settings));
    }

    namespace experimental
    {
        enum href_mode
        {
            href_mode_default = 0,
            href_mode_name = 1,
            href_mode_addresses = 2,
            href_mode_both = 3 // name + addresses
        };

        href_mode get_href_mode(const settings& settings)
        {
            const auto mode = href_mode(nmos::experimental::fields::href_mode(settings));
            // unless a mode is specified, use the host name if secure communications are in use
            // otherwise, the host address(es)
            return mode == href_mode_default
                ? nmos::experimental::fields::client_secure(settings) ? href_mode_name : href_mode_addresses
                : mode;
        }
    }

    // Get host name or address to be used to construct response headers (e.g. 'Link' or 'Location')
    // when a request URL is not available
    utility::string_t get_host(const settings& settings)
    {
        const auto mode = nmos::experimental::get_href_mode(settings);
        return 0 != (mode & nmos::experimental::href_mode_name)
            ? get_host_name(settings)
            : nmos::fields::host_address(settings);
    }

    // Get host name and/or addresses to be used to construct host and URL fields in the data model
    std::vector<utility::string_t> get_hosts(const settings& settings)
    {
        const auto mode = nmos::experimental::get_href_mode(settings);
        std::vector<utility::string_t> hosts;
        if (0 != (mode & nmos::experimental::href_mode_name))
        {
            hosts.push_back(get_host_name(settings));
        }
        if (0 != (mode & nmos::experimental::href_mode_addresses))
        {
            const auto at_least_one_host_address = web::json::value_of({ web::json::value::string(nmos::fields::host_address(settings)) });
            const auto& host_addresses = settings.has_field(nmos::fields::host_addresses) ? nmos::fields::host_addresses(settings) : at_least_one_host_address.as_array();
            for (const auto& host_address : host_addresses)
            {
                hosts.push_back(host_address.as_string());
            }
        }
        return hosts;
    }

    // Get interfaces corresponding to the host addresses in the settings
    std::vector<web::hosts::experimental::host_interface> get_host_interfaces(const settings& settings)
    {
        // filter network interfaces to those that correspond to the specified host_addresses
        const auto at_least_one_host_address = web::json::value_of({ web::json::value::string(nmos::fields::host_address(settings)) });
        const auto& host_addresses = settings.has_field(nmos::fields::host_addresses) ? nmos::fields::host_addresses(settings) : at_least_one_host_address.as_array();
        return boost::copy_range<std::vector<web::hosts::experimental::host_interface>>(web::hosts::experimental::host_interfaces() | boost::adaptors::filtered([&](const web::hosts::experimental::host_interface& interface)
        {
            return interface.addresses.end() != boost::range::find_first_of(interface.addresses, host_addresses, [](const utility::string_t& interface_address, const web::json::value& host_address)
            {
                return interface_address == host_address.as_string();
            });
        }));
    }

    namespace experimental
    {
        // Get HTTP Strict-Transport-Security settings
        bst::optional<web::http::experimental::hsts> get_hsts(const settings& settings)
        {
            // when using a reverse proxy for TLS termination, the proxy should be responsible for HSTS
            // so check server_secure rather than client_secure
            if (nmos::experimental::fields::server_secure(settings) && nmos::experimental::fields::hsts_max_age(settings) > 0)
                return web::http::experimental::hsts{ (uint32_t)nmos::experimental::fields::hsts_max_age(settings), nmos::experimental::fields::hsts_include_sub_domains(settings) };
            return bst::nullopt;
        }
    }

    // Get a summary of the build configuration, including versions of dependencies
    utility::string_t get_build_settings_info()
    {
        utility::ostringstream_t s;
        s.imbue(std::locale::classic());
        s
            << U("cpprestsdk/") << CPPREST_VERSION_MAJOR << U('.') << CPPREST_VERSION_MINOR << U('.') << CPPREST_VERSION_REVISION
            << U(" (")
            << U("listener=") // CPPREST_HTTP_LISTENER_IMPL
#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
            << U("asio")
#else
            << U("httpsys")
#endif
            << U("; ")
            << U("client=") // CPPREST_HTTP_CLIENT_IMPL
#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
            << U("asio")
#else
            << U("winhttp")
#endif
            << U("); ")
            << websocketpp::user_agent // "WebSocket++/<major_version>.<minor_version>.<patch_version>[-<prerelease_flag>]"
            << U("; ")
            << U("Boost ") << (BOOST_VERSION / 100000) << U('.') << (BOOST_VERSION / 100 % 1000) << U('.') << (BOOST_VERSION % 100)
            << U("; ")
            << U(OPENSSL_VERSION_TEXT)
            ;
        return s.str();
    }
}

// The first "test" is of course whether the header compiles standalone
#include "nmos/settings.h"

#include <cstring>
#include "bst/test/test.h"
#include "cpprest/json_utils.h"

////////////////////////////////////////////////////////////////////////////////////////////
// validate_node_settings/validate_registry_settings type and range checks
////////////////////////////////////////////////////////////////////////////////////////////

BST_TEST_CASE(testValidateNodeSettingsEmpty)
{
    // an empty settings object should be valid - defaults are used for everything
    BST_CHECK_NO_THROW(nmos::validate_node_settings(web::json::value::object()));
}

BST_TEST_CASE(testValidateRegistrySettingsEmpty)
{
    BST_CHECK_NO_THROW(nmos::validate_registry_settings(web::json::value::object()));
}

BST_TEST_CASE(testValidateNodeSettingsAfterDefaults)
{
    // the run-time defaults shouldn't introduce any invalid values
    nmos::settings settings;
    nmos::insert_node_default_settings(settings);
    BST_CHECK_NO_THROW(nmos::validate_node_settings(settings));
}

BST_TEST_CASE(testValidateRegistrySettingsAfterDefaults)
{
    nmos::settings settings;
    nmos::insert_registry_default_settings(settings);
    BST_CHECK_NO_THROW(nmos::validate_registry_settings(settings));
}

BST_TEST_CASE(testValidateNodeSettingsWrongType)
{
    using web::json::value_of;

    // all validation errors are reported as web::json::json_exception and the
    // schema validator includes the JSON pointer (e.g. /http_port) in the message
    {
        const auto bad = value_of({ { U("http_port"), U("3210") } }); // should be an int
        try
        {
            nmos::validate_node_settings(bad);
            BST_CHECK(false); // expected json_exception
        }
        catch (const web::json::json_exception& e)
        {
            BST_CHECK(std::strstr(e.what(), "/http_port") != nullptr);
        }
    }
    {
        const auto bad = value_of({ { U("host_addresses"), U("127.0.0.1") } }); // should be an array
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        const auto bad = value_of({ { U("http_trace"), U("true") } }); // should be a bool
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
}

BST_TEST_CASE(testValidateNodeSettingsRangeErrors)
{
    using web::json::value_of;

    {
        const auto bad = value_of({ { U("http_port"), 65536 } });
        try
        {
            nmos::validate_node_settings(bad);
            BST_CHECK(false);
        }
        catch (const web::json::json_exception& e)
        {
            BST_CHECK(std::strstr(e.what(), "/http_port") != nullptr);
            BST_CHECK(std::strstr(e.what(), "65536") != nullptr);
        }
    }
    {
        // -1 is OK for any port (disables the corresponding feature); -2 is not
        const auto ok = value_of({ { U("http_port"), -1 } });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
        const auto bad = value_of({ { U("http_port"), -2 } });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        // logging_level is unconstrained beyond being an integer (named slog severities
        // are in [-40, 40] but the sentinels never_log_severity/reset_log_severity sit
        // at INT_MAX/INT_MIN)
        const auto ok = value_of({ { U("logging_level"), 41 } });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        const auto bad = value_of({ { U("dns_sd_browse_mode"), 3 } });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        const auto bad = value_of({ { U("discovery_backoff_factor"), 0.5 } });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
}

BST_TEST_CASE(testValidateRegistrySettingsRangeErrors)
{
    using web::json::value_of;

    {
        const auto bad = value_of({ { U("ptp_announce_receipt_timeout"), 1 } }); // valid range is 2-10
        BST_CHECK_THROW(nmos::validate_registry_settings(bad), web::json::json_exception);
    }
    {
        const auto bad = value_of({ { U("ptp_domain_number"), 128 } }); // valid range is 0-127
        BST_CHECK_THROW(nmos::validate_registry_settings(bad), web::json::json_exception);
    }
    {
        const auto ok = value_of({ { U("ptp_domain_number"), 0 } });
        BST_CHECK_NO_THROW(nmos::validate_registry_settings(ok));
    }
}

BST_TEST_CASE(testValidateNodeSettingsVersions)
{
    using web::json::value_of;

    {
        const auto ok = value_of({
            { U("is04_versions"), value_of({ U("v1.2"), U("v1.3") }) },
            { U("is05_versions"), value_of({ U("v1.0"), U("v1.1") }) }
        });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        // any well-formed v<major>.<minor> string is acceptable to the schema;
        // whether the library actually implements it is checked at point of use
        const auto ok = value_of({ { U("is04_versions"), value_of({ U("v1.99") }) } });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        const auto bad = value_of({ { U("is04_versions"), value_of({ U("not-a-version") }) } });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        const auto bad = value_of({ { U("registry_version"), U("1.0") } }); // missing the "v"
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        const auto bad = value_of({ { U("registry_version"), U("v1") } }); // missing the minor
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        const auto bad = value_of({ { U("registry_version"), U("v1.2.3") } }); // too many parts
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
}

BST_TEST_CASE(testValidateNodeSettingsEnumStrings)
{
    using web::json::value_of;

    {
        const auto ok = value_of({ { U("authorization_flow"), U("client_credentials") } });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        const auto bad = value_of({ { U("authorization_flow"), U("password") } });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        const auto ok = value_of({ { U("authorization_scopes"), value_of({ U("registration"), U("node"), U("connection") }) } });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        // authorization scope strings aren't validated against an enum (see nmos/scope.h);
        // adding a new NMOS API just adds a new scope and the schema shouldn't need updating
        const auto ok = value_of({ { U("authorization_scopes"), value_of({ U("some-future-scope") }) } });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        const auto bad = value_of({ { U("authorization_scopes"), value_of({ 42 }) } }); // must still be strings
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        const auto ok = value_of({ { U("token_endpoint_auth_method"), U("private_key_jwt") } });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        const auto bad = value_of({ { U("token_endpoint_auth_method"), U("not-a-method") } });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
}

BST_TEST_CASE(testValidateNodeSettingsSeedId)
{
    using web::json::value_of;

    {
        // a well-formed UUID is accepted
        const auto ok = value_of({ { U("seed_id"), U("12345678-1234-1234-89ab-1234567890ab") } });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        // an arbitrary string is rejected (seed_id is used as a v5 UUID namespace)
        const auto bad = value_of({ { U("seed_id"), U("not-a-uuid") } });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        // upper-case hex is rejected (NMOS UUIDs are lower-case)
        const auto bad = value_of({ { U("seed_id"), U("12345678-1234-1234-89AB-1234567890ab") } });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
}

BST_TEST_CASE(testValidateServerCertificates)
{
    using web::json::value_of;

    {
        const auto ok = value_of({
            { U("server_certificates"), value_of({
                value_of({
                    { U("key_algorithm"), U("ECDSA") },
                    { U("private_key_file"), U("key.pem") },
                    { U("certificate_chain_file"), U("chain.pem") }
                })
            }) }
        });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        const auto bad = value_of({
            { U("server_certificates"), value_of({
                value_of({ { U("key_algorithm"), U("DSA") } })
            }) }
        });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        // omitting key_algorithm is fine - omission is the way to say "don't care"
        const auto ok = value_of({
            { U("server_certificates"), value_of({
                value_of({
                    { U("private_key_file"), U("key.pem") },
                    { U("certificate_chain_file"), U("chain.pem") }
                })
            }) }
        });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
    {
        // but explicitly setting it to the empty string is a sign of user confusion
        const auto bad = value_of({
            { U("server_certificates"), value_of({
                value_of({ { U("key_algorithm"), U("") } })
            }) }
        });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        // an entry without both private_key_file and certificate_chain_file is meaningless;
        // the cert-loader logs warnings and pushes an empty certificate that fails at TLS time
        const auto bad = value_of({
            { U("server_certificates"), value_of({
                value_of({ { U("key_algorithm"), U("ECDSA") } })
            }) }
        });
        BST_CHECK_THROW(nmos::validate_node_settings(bad), web::json::json_exception);
    }
    {
        // empty server_certificates array is the right way to say "no server certs"
        const auto ok = value_of({ { U("server_certificates"), web::json::value::array() } });
        BST_CHECK_NO_THROW(nmos::validate_node_settings(ok));
    }
}

BST_TEST_CASE(testValidateSettingsNonObject)
{
    BST_CHECK_THROW(nmos::validate_node_settings(web::json::value::string(U("hi"))), web::json::json_exception);
    BST_CHECK_THROW(nmos::validate_registry_settings(web::json::value::array()), web::json::json_exception);
}

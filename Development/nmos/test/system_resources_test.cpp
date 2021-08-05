// The first "test" is of course whether the header compiles standalone
#include "nmos/system_resources.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSystemGlobal)
{
    using web::json::value_of;

    nmos::settings settings = value_of({
        { U("system_label"), U("ZBQ System") },
        { U("system_description"), U("System Global Information for ZBQ") },
        { U("system_tags"), value_of({
            { U("location"), value_of({
                { U("Salford") },
                { U("Media City") }
            }) },
            { U("studio"), value_of({
                U("HQ1")
            }) }
        }) },
        { U("system_syslog_host_name"), U("syslog.example.com") },
        { U("system_syslog_port"), 514 },
        { U("system_syslogv2_host_name"), U("syslogv2.example.com") },
        { U("system_syslogv2_port"), 6514 },
        { U("ptp_announce_receipt_timeout"), 3 },
        { U("ptp_domain_number"), 127 },
        { U("registration_heartbeat_interval"), 5 }
    });

    const auto id = nmos::make_id();

    auto result = nmos::parse_system_global_data(nmos::make_system_global_data(id, settings));

    BST_REQUIRE_EQUAL(settings, result.second);
}

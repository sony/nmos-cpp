// The first "test" is of course whether the header compiles standalone
#include "nmos/log_gate.h"

#include <sstream>
#include "bst/test/test.h"

namespace
{
    struct test_gate : nmos::experimental::log_gate
    {
        test_gate(std::ostream& error_log, std::ostream& access_log, nmos::experimental::log_model& model)
            : nmos::experimental::log_gate(error_log, access_log, model) {}
        using nmos::experimental::log_gate::pertinent;
    };
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testLogGatePertinentCategories)
{
    using web::json::value_of;

    std::ostringstream error_log;
    std::ostringstream access_log;
    nmos::experimental::log_model model;
    test_gate gate(error_log, access_log, model);

    const std::list<nmos::category> no_categories;
    const std::list<nmos::category> access{ "access" };
    const std::list<nmos::category> send_query_ws_events{ "send_query_ws_events" };
    const std::list<nmos::category> both{ "send_query_ws_events", "access" };

    // when logging_categories is omitted, all messages are pertinent
    BST_REQUIRE(gate.pertinent(no_categories));
    BST_REQUIRE(gate.pertinent(access));
    BST_REQUIRE(gate.pertinent(both));

    // when logging_categories is empty, no messages are pertinent
    model.settings[nmos::fields::logging_categories] = web::json::value::array();
    BST_REQUIRE(!gate.pertinent(no_categories));
    BST_REQUIRE(!gate.pertinent(access));
    BST_REQUIRE(!gate.pertinent(both));

    // positive categories select the messages to be logged
    model.settings[nmos::fields::logging_categories] = value_of({ U("send_query_ws_events") });
    BST_REQUIRE(!gate.pertinent(no_categories));
    BST_REQUIRE(gate.pertinent(send_query_ws_events));
    BST_REQUIRE(!gate.pertinent(access));
    BST_REQUIRE(gate.pertinent(both));

    // the empty string selects messages with no category
    model.settings[nmos::fields::logging_categories] = value_of({ U("") });
    BST_REQUIRE(gate.pertinent(no_categories));
    BST_REQUIRE(!gate.pertinent(access));

    // a category prefixed with '!' excludes matching messages, even if another
    // category matches positively
    model.settings[nmos::fields::logging_categories] = value_of({ U("send_query_ws_events"), U("!access") });
    BST_REQUIRE(gate.pertinent(send_query_ws_events));
    BST_REQUIRE(!gate.pertinent(access));
    BST_REQUIRE(!gate.pertinent(both));
    BST_REQUIRE(!gate.pertinent(no_categories));

    // when only excluded categories are specified, all other messages are pertinent
    model.settings[nmos::fields::logging_categories] = value_of({ U("!access") });
    BST_REQUIRE(gate.pertinent(no_categories));
    BST_REQUIRE(gate.pertinent(send_query_ws_events));
    BST_REQUIRE(!gate.pertinent(access));
    BST_REQUIRE(!gate.pertinent(both));
}

// The first "test" is of course whether the header compiles standalone
#include "nmos/log_gate.h"

#include <sstream>
#include "bst/test/test.h"
#include "cpprest/basic_utils.h" // for utility::us2s

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

    // a negative match takes precedence over the same category listed positively
    model.settings[nmos::fields::logging_categories] = value_of({ U("access"), U("!access") });
    BST_REQUIRE(!gate.pertinent(access));
    BST_REQUIRE(!gate.pertinent(both));
    BST_REQUIRE(!gate.pertinent(no_categories));
    BST_REQUIRE(!gate.pertinent(send_query_ws_events));

    // "!" excludes messages with no category (negation of "")
    model.settings[nmos::fields::logging_categories] = value_of({ U("!") });
    BST_REQUIRE(!gate.pertinent(no_categories));
    BST_REQUIRE(gate.pertinent(access));
    BST_REQUIRE(gate.pertinent(send_query_ws_events));

    model.settings[nmos::fields::logging_categories] = value_of({ U("!"), U("!access") });
    BST_REQUIRE(!gate.pertinent(no_categories));
    BST_REQUIRE(!gate.pertinent(access));
    BST_REQUIRE(gate.pertinent(send_query_ws_events));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testInsertLogEventNonUtf8)
{
    // this covers the path that crashed: a log message containing locale-dependent narrow
    // bytes, e.g. from std::exception::what(), being serialized to JSON on the async logging
    // worker thread, where utility::s2us threw std::range_error and nothing caught it
    nmos::experimental::log_events events;

    // 0xFC is u with diaeresis in the Windows Western code page, and is not valid UTF-8
    const slog::async_log_message message("M\xFCller.cpp", 42, "f\xFCnf", slog::severities::error, "caf\xFC");

    BST_REQUIRE_NO_THROW(nmos::experimental::insert_log_event(events, message, U("42")));
    BST_REQUIRE_EQUAL(size_t(1), events.size());

    const auto& data = events.front().data;

    // the message and source location are preserved, not dropped or emptied
    BST_REQUIRE(!data.at(U("message")).as_string().empty());
    BST_REQUIRE(!data.at(U("source_location")).at(U("file")).as_string().empty());
    BST_REQUIRE(!data.at(U("source_location")).at(U("function")).as_string().empty());

    // and the event is well-formed, i.e. the Logging API can serialize and return it
    BST_REQUIRE_NO_THROW(web::json::value::parse(data.serialize()));

    // each converted field must be valid UTF-8, which is what makes the JSON well-formed
    // to_utf16string is the strict decoder, so this asserts the fix on a narrow build too,
    // where s2us is a pass-through and would otherwise have let the invalid bytes through
    for (const auto& field : { data.at(U("message")), data.at(U("source_location")).at(U("file")), data.at(U("source_location")).at(U("function")) })
    {
        BST_REQUIRE_NO_THROW(utility::conversions::to_utf16string(utility::us2s(field.as_string())));
    }

    // valid UTF-8 is unaffected, i.e. it still arrives exactly as logged
    const slog::async_log_message utf8_message("test.cpp", 42, "test", slog::severities::error, "caf\xC3\xA9");
    BST_REQUIRE_NO_THROW(nmos::experimental::insert_log_event(events, utf8_message, U("43")));
    BST_REQUIRE_EQUAL(size_t(2), events.size());
    BST_REQUIRE_EQUAL(utility::s2us("caf\xC3\xA9"), events.front().data.at(U("message")).as_string());
}

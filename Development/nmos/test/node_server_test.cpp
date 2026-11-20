// The first "test" is of course whether the header compiles standalone
#include "nmos/node_server.h"

#include <cstddef>
#include <utility>
#include "boost/iostreams/device/null.hpp"
#include "boost/iostreams/stream.hpp"
#include "bst/test/test.h"
#include "nmos/control_protocol_ws_api.h"
#include "nmos/log_gate.h"
#include "nmos/model.h"
#include "nmos/server.h"
#include "nmos/settings.h"

namespace
{
    std::pair<std::size_t, std::size_t> make_test_node_server(int events_ws_port, int control_protocol_ws_port)
    {
        nmos::node_model model;
        model.settings[U("events_ws_port")] = web::json::value::number(events_ws_port);
        model.settings[U("control_protocol_ws_port")] = web::json::value::number(control_protocol_ws_port);
        nmos::insert_node_default_settings(model.settings);

        boost::iostreams::stream<boost::iostreams::null_sink> null_ostream((boost::iostreams::null_sink()));
        nmos::experimental::log_model log_model;
        nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

        auto server = nmos::experimental::make_node_server(model, nmos::experimental::node_implementation(), log_model, gate);
        return { server.ws_handlers.size(), server.ws_listeners.size() };
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testNodeServerWebSocketPorts)
{
    // IS-07 and IS-12 may use one listener because their endpoint paths do not overlap.
    {
        const auto listener_counts = make_test_node_server(3217, 3217);
        BST_REQUIRE_EQUAL(1, listener_counts.first);
        BST_REQUIRE_EQUAL(1, listener_counts.second);
    }

    // Existing distinct-port configurations still create independent listeners.
    {
        const auto listener_counts = make_test_node_server(3217, 3218);
        BST_REQUIRE_EQUAL(2, listener_counts.first);
        BST_REQUIRE_EQUAL(2, listener_counts.second);
    }

    // IS-07 remains independently usable when IS-12 is disabled.
    {
        const auto listener_counts = make_test_node_server(3217, -1);
        BST_REQUIRE_EQUAL(1, listener_counts.first);
        BST_REQUIRE_EQUAL(1, listener_counts.second);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testControlProtocolWebSocketRoutes)
{
    nmos::node_model model;
    nmos::insert_node_default_settings(model.settings);

    boost::iostreams::stream<boost::iostreams::null_sink> null_ostream((boost::iostreams::null_sink()));
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(null_ostream, null_ostream, log_model);

    const auto validate = nmos::make_control_protocol_ws_validate_handler(model, nullptr, gate);
    web::http::http_request valid_request;
    valid_request.set_request_uri(web::uri(U("/x-nmos/ncp/v1.0")));
    BST_REQUIRE(validate(valid_request));

    web::http::http_request invalid_request;
    invalid_request.set_request_uri(web::uri(U("/x-nmos/ncp/not-a-version")));
    BST_REQUIRE(!validate(invalid_request));

    web::http::http_request conflicting_request;
    conflicting_request.set_request_uri(web::uri(U("/x-nmos/events/v1.0/devices/not-an-is12-route")));
    BST_REQUIRE(!validate(conflicting_request));
}

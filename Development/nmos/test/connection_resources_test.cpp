// The first "test" is of course whether the header compiles standalone
#include "nmos/connection_resources.h"

#include "bst/test/test.h"
#include "nmos/connection_api.h"
#include "nmos/json_fields.h"
#include "nmos/resource.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testConnectionRtpParameterSets)
{
    {
        const auto resource = nmos::make_connection_rtp_receiver(U("receiver"), false);
        const auto& constraints = resource.data.at(nmos::fields::endpoint_constraints).at(0);
        const auto& transport_params = resource.data.at(nmos::fields::endpoint_staged).at(nmos::fields::transport_params).at(0);

        // Preserve the historical factory behaviour: core + multicast, without FEC or RTCP.
        BST_REQUIRE(constraints.has_field(nmos::fields::multicast_ip));
        BST_REQUIRE(!constraints.has_field(nmos::fields::fec_enabled));
        BST_REQUIRE(!constraints.has_field(nmos::fields::rtcp_enabled));
        BST_REQUIRE(transport_params.has_field(nmos::fields::multicast_ip));
        BST_REQUIRE(!transport_params.has_field(nmos::fields::fec_enabled));
        BST_REQUIRE(!transport_params.has_field(nmos::fields::rtcp_enabled));
    }

    {
        const nmos::rtp_sender_parameter_sets parameter_sets{ true, true };
        const auto resource = nmos::make_connection_rtp_sender(U("sender"), true, parameter_sets);
        const auto& constraints = resource.data.at(nmos::fields::endpoint_constraints);
        const auto& staged = resource.data.at(nmos::fields::endpoint_staged);
        const auto& transport_params = staged.at(nmos::fields::transport_params);

        BST_REQUIRE_EQUAL(2, constraints.size());
        BST_REQUIRE_EQUAL(2, transport_params.size());
        for (size_t leg = 0; leg < constraints.size(); ++leg)
        {
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec_enabled));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec_type));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec_block_width));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::rtcp_enabled));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::rtcp_source_port));

            BST_REQUIRE(!nmos::fields::fec_enabled(transport_params.at(leg)));
            BST_REQUIRE_EQUAL(U("XOR"), nmos::fields::fec_type(transport_params.at(leg)));
            BST_REQUIRE(!nmos::fields::rtcp_enabled(transport_params.at(leg)));
            BST_REQUIRE_EQUAL(U("auto"), nmos::fields::rtcp_destination_ip(transport_params.at(leg)).as_string());
        }
        BST_REQUIRE(staged == resource.data.at(nmos::fields::endpoint_active));
    }

    {
        const nmos::rtp_receiver_parameter_sets parameter_sets{ false, true, true };
        const auto resource = nmos::make_connection_rtp_receiver(U("receiver"), false, parameter_sets);
        const auto& constraints = resource.data.at(nmos::fields::endpoint_constraints).at(0);
        const auto& transport_params = resource.data.at(nmos::fields::endpoint_staged).at(nmos::fields::transport_params).at(0);

        BST_REQUIRE(!constraints.has_field(nmos::fields::multicast_ip));
        BST_REQUIRE(constraints.has_field(nmos::fields::fec_enabled));
        BST_REQUIRE(constraints.has_field(nmos::fields::fec_mode));
        BST_REQUIRE(constraints.has_field(nmos::fields::rtcp_enabled));
        BST_REQUIRE(!constraints.has_field(nmos::fields::rtcp_source_port));

        BST_REQUIRE(!transport_params.has_field(nmos::fields::multicast_ip));
        BST_REQUIRE(!nmos::fields::fec_enabled(transport_params));
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec_mode(transport_params).as_string());
        BST_REQUIRE(!nmos::fields::rtcp_enabled(transport_params));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testConnectionRtpOptionalParameterSetsResolveAuto)
{
    {
        const nmos::rtp_sender_parameter_sets parameter_sets{ false, true };
        auto resource = nmos::make_connection_rtp_sender(U("sender"), false, parameter_sets);
        auto& transport_params = resource.data.at(nmos::fields::endpoint_active).at(nmos::fields::transport_params);
        auto& params = transport_params.at(0);
        params[nmos::fields::source_ip] = web::json::value::string(U("192.0.2.10"));
        params[nmos::fields::destination_ip] = web::json::value::string(U("232.21.21.133"));

        nmos::resolve_rtp_auto(nmos::types::sender, transport_params, 5000);

        BST_REQUIRE_EQUAL(U("232.21.21.133"), nmos::fields::rtcp_destination_ip(params).as_string());
        BST_REQUIRE_EQUAL(5001, nmos::fields::rtcp_destination_port(params).as_integer());
        BST_REQUIRE_EQUAL(5001, nmos::fields::rtcp_source_port(params).as_integer());
    }

    {
        const nmos::rtp_receiver_parameter_sets parameter_sets{ false, false, true };
        auto resource = nmos::make_connection_rtp_receiver(U("receiver"), false, parameter_sets);
        auto& transport_params = resource.data.at(nmos::fields::endpoint_active).at(nmos::fields::transport_params);
        auto& params = transport_params.at(0);
        params[nmos::fields::interface_ip] = web::json::value::string(U("192.0.2.20"));

        nmos::resolve_rtp_auto(nmos::types::receiver, transport_params, 5000);

        BST_REQUIRE_EQUAL(U("192.0.2.20"), nmos::fields::rtcp_destination_ip(params).as_string());
        BST_REQUIRE_EQUAL(5001, nmos::fields::rtcp_destination_port(params).as_integer());
    }
}

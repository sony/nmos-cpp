// The first "test" is of course whether the header compiles standalone
#include "nmos/connection_resources.h"

#include "bst/test/test.h"
#include "nmos/connection_api.h"
#include "nmos/json_fields.h"
#include "nmos/resource.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testConnectionRtpParameterSets)
{
    // Historical factory defaults: core only for senders; core + multicast for receivers.
    // See https://specs.amwa.tv/is-05/releases/v1.1.0/docs/4.1._Behaviour_-_RTP_Transport_Type.html#parameter-sets
    {
        const auto resource = nmos::make_connection_rtp_sender(U("sender"), false);
        const auto& constraints = resource.data.at(nmos::fields::endpoint_constraints).at(0);
        const auto& transport_params = resource.data.at(nmos::fields::endpoint_staged).at(nmos::fields::transport_params).at(0);

        BST_REQUIRE(constraints.has_field(nmos::fields::source_ip));
        BST_REQUIRE(constraints.has_field(nmos::fields::destination_ip));
        BST_REQUIRE(constraints.has_field(nmos::fields::source_port));
        BST_REQUIRE(constraints.has_field(nmos::fields::destination_port));
        BST_REQUIRE(constraints.has_field(nmos::fields::rtp_enabled));
        BST_REQUIRE(!constraints.has_field(nmos::fields::fec_enabled));
        BST_REQUIRE(!constraints.has_field(nmos::fields::rtcp_enabled));

        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::source_ip(transport_params).as_string());
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::destination_ip(transport_params).as_string());
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::source_port(transport_params).as_string());
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::destination_port(transport_params).as_string());
        BST_REQUIRE(nmos::fields::rtp_enabled(transport_params));
        BST_REQUIRE(!transport_params.has_field(nmos::fields::fec_enabled));
        BST_REQUIRE(!transport_params.has_field(nmos::fields::rtcp_enabled));
    }

    {
        const auto resource = nmos::make_connection_rtp_receiver(U("receiver"), false);
        const auto& constraints = resource.data.at(nmos::fields::endpoint_constraints).at(0);
        const auto& transport_params = resource.data.at(nmos::fields::endpoint_staged).at(nmos::fields::transport_params).at(0);

        BST_REQUIRE(constraints.has_field(nmos::fields::source_ip));
        BST_REQUIRE(constraints.has_field(nmos::fields::interface_ip));
        BST_REQUIRE(constraints.has_field(nmos::fields::destination_port));
        BST_REQUIRE(constraints.has_field(nmos::fields::rtp_enabled));
        BST_REQUIRE(constraints.has_field(nmos::fields::multicast_ip));
        BST_REQUIRE(!constraints.has_field(nmos::fields::fec_enabled));
        BST_REQUIRE(!constraints.has_field(nmos::fields::rtcp_enabled));

        BST_REQUIRE(nmos::fields::source_ip(transport_params).is_null());
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::interface_ip(transport_params).as_string());
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::destination_port(transport_params).as_string());
        BST_REQUIRE(nmos::fields::rtp_enabled(transport_params));
        BST_REQUIRE(nmos::fields::multicast_ip(transport_params).is_null());
        BST_REQUIRE(!transport_params.has_field(nmos::fields::fec_enabled));
        BST_REQUIRE(!transport_params.has_field(nmos::fields::rtcp_enabled));
    }

    // Optional sender FEC and RTCP parameter sets, including schema-aligned staged defaults.
    // See https://specs.amwa.tv/is-05/releases/v1.1.0/APIs/schemas/with-refs/sender_transport_params_rtp.html
    {
        const auto parameter_sets = nmos::rtp_sender_parameter_sets_fec | nmos::rtp_sender_parameter_sets_rtcp;
        const auto resource = nmos::make_connection_rtp_sender(U("sender"), true, parameter_sets);
        const auto& constraints = resource.data.at(nmos::fields::endpoint_constraints);
        const auto& staged = resource.data.at(nmos::fields::endpoint_staged);
        const auto& transport_params = staged.at(nmos::fields::transport_params);

        BST_REQUIRE_EQUAL(2, constraints.size());
        BST_REQUIRE_EQUAL(2, transport_params.size());
        for (size_t leg = 0; leg < constraints.size(); ++leg)
        {
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec_enabled));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec_destination_ip));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec_type));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec_mode));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec_block_width));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec_block_height));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec1D_destination_port));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec2D_destination_port));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec1D_source_port));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::fec2D_source_port));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::rtcp_enabled));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::rtcp_destination_ip));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::rtcp_destination_port));
            BST_REQUIRE(constraints.at(leg).has_field(nmos::fields::rtcp_source_port));

            const auto& params = transport_params.at(leg);
            BST_REQUIRE(!nmos::fields::fec_enabled(params));
            BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec_destination_ip(params).as_string());
            BST_REQUIRE_EQUAL(U("XOR"), nmos::fields::fec_type(params));
            // Sender schema allows only "1D" / "2D" (no "auto")
            BST_REQUIRE_EQUAL(U("1D"), nmos::fields::fec_mode(params).as_string());
            BST_REQUIRE_EQUAL(4, nmos::fields::fec_block_width(params));
            BST_REQUIRE_EQUAL(4, nmos::fields::fec_block_height(params));
            BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec1D_destination_port(params).as_string());
            BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec2D_destination_port(params).as_string());
            BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec1D_source_port(params).as_string());
            BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec2D_source_port(params).as_string());

            BST_REQUIRE(!nmos::fields::rtcp_enabled(params));
            BST_REQUIRE_EQUAL(U("auto"), nmos::fields::rtcp_destination_ip(params).as_string());
            BST_REQUIRE_EQUAL(U("auto"), nmos::fields::rtcp_destination_port(params).as_string());
            BST_REQUIRE_EQUAL(U("auto"), nmos::fields::rtcp_source_port(params).as_string());
        }
        BST_REQUIRE(staged == resource.data.at(nmos::fields::endpoint_active));
    }

    // Optional receiver FEC and RTCP parameter sets (without multicast), including schema-aligned staged defaults.
    // See https://specs.amwa.tv/is-05/releases/v1.1.0/APIs/schemas/with-refs/receiver_transport_params_rtp.html
    {
        const auto parameter_sets = nmos::rtp_receiver_parameter_sets_fec | nmos::rtp_receiver_parameter_sets_rtcp;
        const auto resource = nmos::make_connection_rtp_receiver(U("receiver"), false, parameter_sets);
        const auto& constraints = resource.data.at(nmos::fields::endpoint_constraints).at(0);
        const auto& transport_params = resource.data.at(nmos::fields::endpoint_staged).at(nmos::fields::transport_params).at(0);

        BST_REQUIRE(!constraints.has_field(nmos::fields::multicast_ip));
        BST_REQUIRE(constraints.has_field(nmos::fields::fec_enabled));
        BST_REQUIRE(constraints.has_field(nmos::fields::fec_destination_ip));
        BST_REQUIRE(constraints.has_field(nmos::fields::fec_mode));
        BST_REQUIRE(constraints.has_field(nmos::fields::fec1D_destination_port));
        BST_REQUIRE(constraints.has_field(nmos::fields::fec2D_destination_port));
        BST_REQUIRE(!constraints.has_field(nmos::fields::fec_type));
        BST_REQUIRE(!constraints.has_field(nmos::fields::fec_block_width));
        BST_REQUIRE(!constraints.has_field(nmos::fields::fec1D_source_port));
        BST_REQUIRE(constraints.has_field(nmos::fields::rtcp_enabled));
        BST_REQUIRE(constraints.has_field(nmos::fields::rtcp_destination_ip));
        BST_REQUIRE(constraints.has_field(nmos::fields::rtcp_destination_port));
        BST_REQUIRE(!constraints.has_field(nmos::fields::rtcp_source_port));

        BST_REQUIRE(!transport_params.has_field(nmos::fields::multicast_ip));
        BST_REQUIRE(!nmos::fields::fec_enabled(transport_params));
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec_destination_ip(transport_params).as_string());
        // Receiver schema defines "auto" as the highest available number of dimensions
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec_mode(transport_params).as_string());
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec1D_destination_port(transport_params).as_string());
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::fec2D_destination_port(transport_params).as_string());

        BST_REQUIRE(!nmos::fields::rtcp_enabled(transport_params));
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::rtcp_destination_ip(transport_params).as_string());
        BST_REQUIRE_EQUAL(U("auto"), nmos::fields::rtcp_destination_port(transport_params).as_string());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testConnectionRtpOptionalParameterSetsResolveAuto)
{
    // Schema-documented auto resolutions for sender RTCP / FEC.
    // See sender_transport_params_rtp.json descriptions for rtcp_* and fec*_* ports / destination_ip.
    {
        auto resource = nmos::make_connection_rtp_sender(U("sender"), false, nmos::rtp_sender_parameter_sets_rtcp);
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
        auto resource = nmos::make_connection_rtp_sender(U("sender"), false, nmos::rtp_sender_parameter_sets_fec);
        auto& transport_params = resource.data.at(nmos::fields::endpoint_active).at(nmos::fields::transport_params);
        auto& params = transport_params.at(0);
        params[nmos::fields::source_ip] = web::json::value::string(U("192.0.2.10"));
        params[nmos::fields::destination_ip] = web::json::value::string(U("232.21.21.133"));

        nmos::resolve_rtp_auto(nmos::types::sender, transport_params, 5000);

        BST_REQUIRE_EQUAL(U("232.21.21.133"), nmos::fields::fec_destination_ip(params).as_string());
        BST_REQUIRE_EQUAL(5002, nmos::fields::fec1D_destination_port(params).as_integer());
        BST_REQUIRE_EQUAL(5004, nmos::fields::fec2D_destination_port(params).as_integer());
        BST_REQUIRE_EQUAL(5002, nmos::fields::fec1D_source_port(params).as_integer());
        BST_REQUIRE_EQUAL(5004, nmos::fields::fec2D_source_port(params).as_integer());
    }

    // Schema-documented auto resolutions for receiver RTCP / FEC.
    // See receiver_transport_params_rtp.json: unicast uses interface_ip for fec/rtcp destination_ip.
    {
        auto resource = nmos::make_connection_rtp_receiver(U("receiver"), false, nmos::rtp_receiver_parameter_sets_rtcp);
        auto& transport_params = resource.data.at(nmos::fields::endpoint_active).at(nmos::fields::transport_params);
        auto& params = transport_params.at(0);
        params[nmos::fields::interface_ip] = web::json::value::string(U("192.0.2.20"));

        nmos::resolve_rtp_auto(nmos::types::receiver, transport_params, 5000);

        BST_REQUIRE_EQUAL(U("192.0.2.20"), nmos::fields::rtcp_destination_ip(params).as_string());
        BST_REQUIRE_EQUAL(5001, nmos::fields::rtcp_destination_port(params).as_integer());
    }

    {
        auto resource = nmos::make_connection_rtp_receiver(U("receiver"), false, nmos::rtp_receiver_parameter_sets_fec);
        auto& transport_params = resource.data.at(nmos::fields::endpoint_active).at(nmos::fields::transport_params);
        auto& params = transport_params.at(0);
        params[nmos::fields::interface_ip] = web::json::value::string(U("192.0.2.20"));

        nmos::resolve_rtp_auto(nmos::types::receiver, transport_params, 5000);

        BST_REQUIRE_EQUAL(U("192.0.2.20"), nmos::fields::fec_destination_ip(params).as_string());
        BST_REQUIRE_EQUAL(5002, nmos::fields::fec1D_destination_port(params).as_integer());
        BST_REQUIRE_EQUAL(5004, nmos::fields::fec2D_destination_port(params).as_integer());
    }
}

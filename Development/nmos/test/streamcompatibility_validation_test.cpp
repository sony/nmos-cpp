#include "nmos/st2110_21_sender_type.h"
#include "nmos/streamcompatibility_validation.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testResourceValidator)
{
    {
        using web::json::value_of;
        using nmos::experimental::flow_parameter_constraints;
        using nmos::experimental::sender_parameter_constraints;
        using nmos::experimental::detail::match_resource_parameters_constraint_set;

        auto constraint_set = value_of({
            { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::video_raw.name }) },
            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({ 1920 }) },
            { nmos::caps::format::frame_height, nmos::make_caps_integer_constraint({ 1080 }) },
            { nmos::caps::transport::st2110_21_sender_type, nmos::make_caps_string_constraint({ sdp::type_parameters::type_N.name, sdp::type_parameters::type_NL.name }) }
        });

        auto flow = value_of({
            { nmos::fields::media_type, nmos::media_types::video_raw.name },
            { nmos::fields::frame_width, 1920 },
            { nmos::fields::frame_height, 1080 }
        });

        auto sender = value_of({
            { nmos::fields::st2110_21_sender_type, nmos::st2110_21_sender_types::type_N.name }
        });

        BST_REQUIRE(match_resource_parameters_constraint_set(flow_parameter_constraints, flow, constraint_set));
        BST_REQUIRE(match_resource_parameters_constraint_set(sender_parameter_constraints, sender, constraint_set));
    }
}

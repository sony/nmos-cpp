#include "nmos/capabilities.h"
#include "nmos/constraints.h"
#include "nmos/interlace_mode.h"
#include "nmos/json_fields.h"
#include "nmos/media_type.h"
#include "nmos/sdp_utils.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSimpleCase)
{
    {
        using web::json::value;
        using web::json::value_of;
        using nmos::experimental::is_constraint_subset;

        auto a = value_of({
            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({}, 1920) }
        });

        auto b1 = value_of({
            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({}, 2000) },
            { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::video_raw.name }) },
        });

        auto b2 = value_of({
            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({}, 1900) }
        });

        auto b3 = value::object();

        BST_REQUIRE(is_constraint_subset(a, b1));
        BST_REQUIRE(!is_constraint_subset(a, b2));
        BST_REQUIRE(!is_constraint_subset(a, b3));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testLessConstraints)
{
    {
        using web::json::value_of;
        using nmos::experimental::is_constraint_subset;

        auto a = value_of({
            { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::video_raw.name }) },
            { nmos::caps::format::grain_rate, nmos::make_caps_rational_constraint({ nmos::rates::rate25, nmos::rates::rate29_97 }) },
            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({ 1920 }) },
            { nmos::caps::format::frame_height, nmos::make_caps_integer_constraint({ 1080 }) },
            { nmos::caps::format::color_sampling, nmos::make_caps_string_constraint({ sdp::samplings::YCbCr_4_2_2.name }) },
            { nmos::caps::format::interlace_mode, nmos::make_caps_string_constraint({ nmos::interlace_modes::interlaced_bff.name, nmos::interlace_modes::interlaced_tff.name, nmos::interlace_modes::interlaced_psf.name }) },
            { nmos::caps::format::colorspace, nmos::make_caps_string_constraint({ sdp::colorimetries::BT2020.name, sdp::colorimetries::BT709.name }) },
            { nmos::caps::format::transfer_characteristic, nmos::make_caps_string_constraint({ sdp::transfer_characteristic_systems::SDR.name }) },
            { nmos::caps::format::component_depth, nmos::make_caps_integer_constraint({}, 8, 12) },
            { nmos::caps::transport::st2110_21_sender_type, nmos::make_caps_string_constraint({ sdp::type_parameters::type_N.name }) }
        });

        auto b = value_of({
            { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::video_raw.name }) },
            { nmos::caps::format::grain_rate, nmos::make_caps_rational_constraint({ nmos::rates::rate25, nmos::rates::rate29_97 }) },
            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({ 1920 }) },
            { nmos::caps::format::frame_height, nmos::make_caps_integer_constraint({ 1080 }) }
        });

        BST_REQUIRE(!is_constraint_subset(a, b));
        BST_REQUIRE(is_constraint_subset(b, a));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testRoundTrip)
{
    {
        using web::json::value_of;
        using nmos::experimental::is_constraint_subset;

        auto constraint_set = value_of({
            { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::video_raw.name }) },
            { nmos::caps::format::grain_rate, nmos::make_caps_rational_constraint({ nmos::rates::rate25, nmos::rates::rate29_97 }) },
            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({ 1920 }) },
            { nmos::caps::format::frame_height, nmos::make_caps_integer_constraint({ 1080 }) },
            { nmos::caps::format::color_sampling, nmos::make_caps_string_constraint({ sdp::samplings::YCbCr_4_2_2.name }) },
            { nmos::caps::format::interlace_mode, nmos::make_caps_string_constraint({ nmos::interlace_modes::interlaced_bff.name, nmos::interlace_modes::interlaced_tff.name, nmos::interlace_modes::interlaced_psf.name }) },
            { nmos::caps::format::colorspace, nmos::make_caps_string_constraint({ sdp::colorimetries::BT2020.name, sdp::colorimetries::BT709.name }) },
            { nmos::caps::format::transfer_characteristic, nmos::make_caps_string_constraint({ sdp::transfer_characteristic_systems::SDR.name }) },
            { nmos::caps::format::component_depth, nmos::make_caps_integer_constraint({}, 8, 12) },
            { nmos::caps::transport::st2110_21_sender_type, nmos::make_caps_string_constraint({ sdp::type_parameters::type_N.name }) }
        });

        BST_REQUIRE(is_constraint_subset(constraint_set, constraint_set));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testSubconstraints)
{
    {
        using web::json::value_of;
        using nmos::experimental::is_constraint_subset;

        auto constraint_set = value_of({
            { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::video_raw.name }) },
            { nmos::caps::format::grain_rate, nmos::make_caps_rational_constraint({ nmos::rates::rate25, nmos::rates::rate29_97 }) },
            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({ 1920 }) },
            { nmos::caps::format::frame_height, nmos::make_caps_integer_constraint({ 1080 }) },
            { nmos::caps::format::color_sampling, nmos::make_caps_string_constraint({ sdp::samplings::YCbCr_4_2_2.name }) },
            { nmos::caps::format::interlace_mode, nmos::make_caps_string_constraint({ nmos::interlace_modes::interlaced_bff.name, nmos::interlace_modes::interlaced_tff.name, nmos::interlace_modes::interlaced_psf.name }) },
            { nmos::caps::format::colorspace, nmos::make_caps_string_constraint({ sdp::colorimetries::BT2020.name, sdp::colorimetries::BT709.name }) },
            { nmos::caps::format::transfer_characteristic, nmos::make_caps_string_constraint({ sdp::transfer_characteristic_systems::SDR.name }) },
            { nmos::caps::format::component_depth, nmos::make_caps_integer_constraint({}, 8, 12) },
            { nmos::caps::transport::st2110_21_sender_type, nmos::make_caps_string_constraint({ sdp::type_parameters::type_N.name }) }
        });

        auto constraint_subset = value_of({
            { nmos::caps::format::media_type, nmos::make_caps_string_constraint({ nmos::media_types::video_raw.name }) },
            { nmos::caps::format::grain_rate, nmos::make_caps_rational_constraint({ nmos::rates::rate25 }) },
            { nmos::caps::format::frame_width, nmos::make_caps_integer_constraint({ 1920 }) },
            { nmos::caps::format::frame_height, nmos::make_caps_integer_constraint({ 1080 }) },
            { nmos::caps::format::color_sampling, nmos::make_caps_string_constraint({ sdp::samplings::YCbCr_4_2_2.name }) },
            { nmos::caps::format::interlace_mode, nmos::make_caps_string_constraint({ nmos::interlace_modes::interlaced_tff.name, nmos::interlace_modes::interlaced_psf.name }) },
            { nmos::caps::format::colorspace, nmos::make_caps_string_constraint({ sdp::colorimetries::BT2020.name, sdp::colorimetries::BT709.name }) },
            { nmos::caps::format::transfer_characteristic, nmos::make_caps_string_constraint({ sdp::transfer_characteristic_systems::SDR.name }) },
            { nmos::caps::format::component_depth, nmos::make_caps_integer_constraint({ 10 }) },
            { nmos::caps::transport::st2110_21_sender_type, nmos::make_caps_string_constraint({ sdp::type_parameters::type_N.name }) }
        });

        BST_REQUIRE(is_constraint_subset(constraint_set, constraint_subset));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testRationalMinMaxSubconstraints)
{
    {
        using web::json::value_of;
        using nmos::experimental::is_subconstraint;

        auto wideRange = nmos::make_caps_rational_constraint({}, nmos::rates::rate25, nmos::rates::rate30);
        auto narrowRange = nmos::make_caps_rational_constraint({}, nmos::rates::rate25, nmos::rates::rate29_97);

        BST_REQUIRE(is_subconstraint(wideRange, narrowRange));
    }
}

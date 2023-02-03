// The first "test" is of course whether the header compiles standalone
#include "nmos/capabilities.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMatchConstraint)
{
    using web::json::value;

    BST_REQUIRE(nmos::match_string_constraint(U("purr"), nmos::make_caps_string_constraint()));
    BST_REQUIRE(nmos::match_integer_constraint(42, nmos::make_caps_integer_constraint({})));
    BST_REQUIRE(nmos::match_number_constraint(4.2, nmos::make_caps_number_constraint({})));
    BST_REQUIRE(nmos::match_boolean_constraint(true, nmos::make_caps_boolean_constraint({})));
    BST_REQUIRE(nmos::match_rational_constraint(nmos::rates::rate29_97, nmos::make_caps_rational_constraint()));

    BST_REQUIRE(nmos::match_string_constraint(U("purr"), nmos::make_caps_string_constraint({ U("meow"), U("purr"), U("hiss") }, U("^(meow|purr|hiss)$"))));
    BST_REQUIRE(!nmos::match_string_constraint(U("purr"), nmos::make_caps_string_constraint({ U("meow"), U("hiss") })));
    BST_REQUIRE(nmos::match_string_constraint(U("purr"), nmos::make_caps_string_constraint({}, U("^(meow|purr|hiss)$"))));
    BST_REQUIRE(!nmos::match_string_constraint(U("bark"), nmos::make_caps_string_constraint({}, U("^(meow|purr|hiss)$"))));

    BST_REQUIRE(nmos::match_integer_constraint(42, nmos::make_caps_integer_constraint({ 37, 42, 57 }, 37, 57)));
    BST_REQUIRE(!nmos::match_integer_constraint(42, nmos::make_caps_integer_constraint({ 37, 57 })));
    for (auto i : { 37, 42, 57 })
        BST_REQUIRE(nmos::match_integer_constraint(i, nmos::make_caps_integer_constraint({}, 37, 57)));
    for (auto i : { -100, 0, 100 })
        BST_REQUIRE(!nmos::match_integer_constraint(i, nmos::make_caps_integer_constraint({}, 37, 57)));
    BST_REQUIRE(nmos::match_integer_constraint(INT64_C(0xBADC0FFEE), nmos::make_caps_integer_constraint({}, INT64_C(0xC0FFEE), INT64_C(0xC01DC0FFEE))));

    BST_REQUIRE(nmos::match_number_constraint(4.2, nmos::make_caps_number_constraint({ 3.7, 4.2, 5.7 }, 3.7, 5.7)));
    BST_REQUIRE(!nmos::match_number_constraint(4.2, nmos::make_caps_number_constraint({ 3.7, 5.7 })));
    for (auto d : { 3.7, 4.2, 5.7 })
        BST_REQUIRE(nmos::match_number_constraint(d, nmos::make_caps_number_constraint({}, 3.7, 5.7)));
    for (auto d : { -10.0, 0.0, 10.0 })
        BST_REQUIRE(!nmos::match_number_constraint(d, nmos::make_caps_number_constraint({}, 3.7, 5.7)));

    BST_REQUIRE(nmos::match_boolean_constraint(true, nmos::make_caps_boolean_constraint({ false, true })));
    BST_REQUIRE(!nmos::match_boolean_constraint(true, nmos::make_caps_boolean_constraint({ false })));

    BST_REQUIRE(nmos::match_rational_constraint(nmos::rates::rate29_97, nmos::make_caps_rational_constraint({ nmos::rates::rate25, nmos::rates::rate29_97, nmos::rates::rate30 })));
    BST_REQUIRE(!nmos::match_rational_constraint(nmos::rates::rate29_97, nmos::make_caps_rational_constraint({ nmos::rates::rate25, nmos::rates::rate30 })));
    for (auto r : { nmos::rates::rate25, nmos::rates::rate29_97, nmos::rates::rate30 })
       BST_REQUIRE(nmos::match_rational_constraint(r, nmos::make_caps_rational_constraint({}, nmos::rates::rate25, nmos::rates::rate30)));
    for (auto r : { nmos::rational{}, nmos::rates::rate23_98, nmos::rates::rate59_94 })
        BST_REQUIRE(!nmos::match_rational_constraint(r, nmos::make_caps_rational_constraint({}, nmos::rates::rate25, nmos::rates::rate30)));

    BST_REQUIRE(nmos::match_constraint(value(U("purr")), nmos::make_caps_string_constraint({ U("meow"), U("purr"), U("hiss") }, U("^(meow|purr|hiss)$"))));
    BST_REQUIRE(!nmos::match_constraint(value(U("purr")), nmos::make_caps_string_constraint({ U("meow"), U("hiss") })));
    BST_REQUIRE(nmos::match_constraint(value(42), nmos::make_caps_integer_constraint({ 37, 42, 57 }, 37, 57)));
    BST_REQUIRE(!nmos::match_constraint(value(42), nmos::make_caps_integer_constraint({ 37, 57 })));
    BST_REQUIRE(nmos::match_constraint(value(INT64_C(0xBADC0FFEE)), nmos::make_caps_integer_constraint({}, INT64_C(0xC0FFEE), INT64_C(0xC01DC0FFEE))));
    BST_REQUIRE(nmos::match_constraint(value(4.2), nmos::make_caps_number_constraint({ 3.7, 4.2, 5.7 }, 3.7, 5.7)));
    BST_REQUIRE(!nmos::match_constraint(value(4.2), nmos::make_caps_number_constraint({ 3.7, 5.7 })));
    BST_REQUIRE(nmos::match_constraint(value(true), nmos::make_caps_boolean_constraint({ false, true })));
    BST_REQUIRE(!nmos::match_constraint(value(true), nmos::make_caps_boolean_constraint({ false })));
    BST_REQUIRE(nmos::match_constraint(nmos::make_rational(nmos::rates::rate29_97), nmos::make_caps_rational_constraint({ nmos::rates::rate25, nmos::rates::rate29_97, nmos::rates::rate30 })));
    BST_REQUIRE(!nmos::match_constraint(nmos::make_rational(nmos::rates::rate29_97), nmos::make_caps_rational_constraint({ nmos::rates::rate25, nmos::rates::rate30 })));
}

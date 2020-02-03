// The first "test" is of course whether the header compiles standalone
#include "cpprest/json_visit.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testValueAssigningVisitor)
{
    const auto expected = U("[[],37,42,\"foo\",{\"bar\":57,\"baz\":false,\"qux\":[]},3.14159275,null]");

    web::json::value v;
    web::json::value_assigning_visitor cons(v);
    cons(web::json::enter_array_tag{});
    {
        cons(web::json::enter_element_tag{});
        {
            cons(web::json::enter_array_tag{});
            cons(web::json::leave_array_tag{});
        }
        cons(web::json::leave_element_tag{});
        cons(web::json::array_separator_tag{});
        cons(web::json::enter_element_tag{});
        {
            visit(cons, web::json::value::number(37));
        }
        cons(web::json::leave_element_tag{});
        cons(web::json::array_separator_tag{});
        cons(web::json::enter_element_tag{});
        {
            visit(cons, web::json::value::number(42));
        }
        cons(web::json::leave_element_tag{});
        cons(web::json::array_separator_tag{});
        cons(web::json::enter_element_tag{});
        {
            visit(cons, web::json::value::string(U("foo")));
        }
        cons(web::json::leave_element_tag{});
        cons(web::json::array_separator_tag{});
        cons(web::json::enter_element_tag{});
        {
            cons(web::json::enter_object_tag{});
            {
                cons(web::json::enter_field_tag{});
                {
                    cons(web::json::value::string(U("bar")), web::json::string_tag{});
                }
                cons(web::json::field_separator_tag{});
                {
                    visit(cons, web::json::value::number(57));
                }
                cons(web::json::leave_field_tag{});
                cons(web::json::object_separator_tag{});
                cons(web::json::enter_field_tag{});
                {
                    cons(web::json::value::string(U("baz")), web::json::string_tag{});
                }
                cons(web::json::field_separator_tag{});
                {
                    visit(cons, web::json::value::boolean(false));
                }
                cons(web::json::leave_field_tag{});
                cons(web::json::object_separator_tag{});
                cons(web::json::enter_field_tag{});
                {
                    cons(web::json::value::string(U("qux")), web::json::string_tag{});
                }
                cons(web::json::field_separator_tag{});
                {
                    cons(web::json::enter_array_tag{});
                    cons(web::json::leave_array_tag{});
                }
                cons(web::json::leave_field_tag{});
            }
            cons(web::json::leave_object_tag{});
        }
        cons(web::json::leave_element_tag{});
        cons(web::json::array_separator_tag{});
        cons(web::json::enter_element_tag{});
        {
            visit(cons, web::json::value::number(3.14159275));
        }
        cons(web::json::leave_element_tag{});
        cons(web::json::array_separator_tag{});
        cons(web::json::enter_element_tag{});
        {
            visit(cons, web::json::value::null());
        }
        cons(web::json::leave_element_tag{});
    }
    cons(web::json::leave_array_tag{});

    BST_REQUIRE_STRING_EQUAL(expected, v.serialize());

    web::json::value w;
    visit(web::json::value_assigning_visitor(w), v);
    BST_REQUIRE_EQUAL(v, w);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testOstreamVisitorNumbers)
{
    {
        web::json::value two_thirds(2.0 / 3.0);
        const auto expected = two_thirds.serialize();
        utility::ostringstream_t os;
        web::json::visit(web::json::ostream_visitor(os), two_thirds);
        BST_REQUIRE_EQUAL(two_thirds.serialize(), os.str());
    }
    {
        web::json::value big_uint(UINT64_C(12345678901234567890));
        const auto expected = big_uint.serialize();
        utility::ostringstream_t os;
        web::json::visit(web::json::ostream_visitor(os), big_uint);
        BST_REQUIRE_EQUAL(big_uint.serialize(), os.str());
    }
    {
        web::json::value big_int(INT64_C(-1234567890123456789));
        const auto expected = big_int.serialize();
        utility::ostringstream_t os;
        web::json::visit(web::json::ostream_visitor(os), big_int);
        BST_REQUIRE_EQUAL(big_int.serialize(), os.str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testOstreamVisitor)
{
    const auto expected = U("[[],37,42,\"foo\",{\"bar\":57,\"baz\":false,\"qux\":[]},3.14159275,null]");

    {
        utility::ostringstream_t os;
        web::json::visit(web::json::ostream_visitor(os), web::json::value::parse(expected));
        BST_REQUIRE_STRING_EQUAL(expected, os.str());
    }
    {
        std::stringstream os;
        web::json::visit(web::json::basic_ostream_visitor<char>(os), web::json::value::parse(expected));
        BST_REQUIRE_STRING_EQUAL(utility::conversions::to_utf8string(expected), os.str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testPrettyVisitor)
{
    const auto ugly = U("[[],37,42,\"foo\",{\"bar\":57,\"baz\":false,\"qux\":[]},3.14159275,null]");

    const auto pretty = R"-json-(
[
  [],
  37,
  42,
  "foo",
  {
    "bar": 57,
    "baz": false,
    "qux": []
  },
  3.14159275,
  null
]
)-json-";

    std::stringstream os;
    os << std::endl;
    web::json::visit(web::json::basic_pretty_visitor<char>(os, 2), web::json::value::parse(ugly));
    os << std::endl;
    BST_REQUIRE_STRING_EQUAL(pretty, os.str());
}

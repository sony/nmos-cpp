// The first "test" is of course whether the header compiles standalone
#include "cpprest/json_utils.h"

#include "bst/test/test.h"

namespace
{
    web::json::value merged_original(web::json::value target, const web::json::value& source)
    {
        web::json::merge_patch(target, source, false);
        return target;
    }

    web::json::value merged_permissive(web::json::value target, const web::json::value& source)
    {
        web::json::merge_patch(target, source, true);
        return target;
    }

    const auto non_composites = web::json::value_of({
        _XPLATSTR("What do you get if you multiple six by nine?"),
        57,
        3.1419275,
        false,
        web::json::value::null()
    });

    const auto empty_composites = web::json::value_of({
        web::json::value::array(),
        web::json::value::object()
    });

    const utility::string_t foo = _XPLATSTR("foo");
    const utility::string_t bar = _XPLATSTR("bar");
    const utility::string_t baz = _XPLATSTR("baz");
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMergePatch)
{
    const auto merged = merged_original;

    using web::json::value;
    using web::json::value_of;
    using web::json::json_exception;

    for (const auto& source : non_composites.as_array())
    {
        for (const auto& target : non_composites.as_array())
        {
            BST_REQUIRE_EQUAL(source, merged(target, source));
        }
        for (const auto& target : empty_composites.as_array())
        {
            BST_REQUIRE_THROW(merged(target, source), json_exception);
        }
    }

    for (const auto& source : empty_composites.as_array())
    {
        for (const auto& target : non_composites.as_array())
        {
            BST_REQUIRE_THROW(merged(target, source), json_exception);
        }
    }

    BST_REQUIRE_EQUAL(value::array(), merged(value::array(), value::array()));
    BST_REQUIRE_EQUAL(value::object(), merged(value::object(), value::object()));
    BST_REQUIRE_THROW(merged(value::object(), value::array()), json_exception);
    BST_REQUIRE_THROW(merged(value::array(), value::object()), json_exception);

    BST_REQUIRE_EQUAL(value_of({ 3, 4 }), merged(value_of({ 1, 2 }), value_of({ 3, 4 })));
    BST_REQUIRE_THROW(merged(value_of({ 1, 2 }), value_of({ 3 })), json_exception);
    BST_REQUIRE_THROW(merged(value_of({ 1 }), value_of({ 2, 3 })), json_exception);

    BST_REQUIRE_EQUAL(value_of({ { foo, 3 }, { bar, 2 } }), merged(value_of({ { foo, 1 }, { bar, 2 } }), value_of({ { foo, 3 } })));
    BST_REQUIRE_THROW(merged(value_of({ { foo, 1 }, { bar, 2 } }), value_of({ { baz, 3 } })), json_exception);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMergePatchPermissive)
{
    const auto merged = merged_permissive;

    using web::json::value;
    using web::json::value_of;
    using web::json::json_exception;

    for (const auto& source : non_composites.as_array())
    {
        for (const auto& target : non_composites.as_array())
        {
            BST_REQUIRE_EQUAL(source, merged(target, source));
        }
        for (const auto& target : empty_composites.as_array())
        {
            BST_REQUIRE_EQUAL(source, merged(target, source));
        }
    }

    for (const auto& source : empty_composites.as_array())
    {
        for (const auto& target : non_composites.as_array())
        {
            BST_REQUIRE_EQUAL(source, merged(target, source));
        }
    }

    BST_REQUIRE_EQUAL(value::array(), merged(value::array(), value::array()));
    BST_REQUIRE_EQUAL(value::object(), merged(value::object(), value::object()));
    BST_REQUIRE_EQUAL(value::array(), merged(value::object(), value::array()));
    BST_REQUIRE_EQUAL(value::object(), merged(value::array(), value::object()));

    BST_REQUIRE_EQUAL(value_of({ 3, 4 }), merged(value_of({ 1, 2 }), value_of({ 3, 4 })));
    BST_REQUIRE_EQUAL(value_of({ 3 }), merged(value_of({ 1, 2 }), value_of({ 3 })));
    BST_REQUIRE_EQUAL(value_of({ 2, 3 }), merged(value_of({ 1 }), value_of({ 2, 3 })));

    BST_REQUIRE_EQUAL(value_of({ { foo, 3 }, { bar, 2 } }), merged(value_of({ { foo, 1 }, { bar, 2 } }), value_of({ { foo, 3 } })));
    BST_REQUIRE_EQUAL(value_of({ { foo, 1 }, { bar, 2 }, { baz, 3 } }), merged(value_of({ { foo, 1 }, { bar, 2 } }), value_of({ { baz, 3 } })));

    // additional tests for permissive mode

    const auto ofoo = value_of({ { foo, 1 } });
    const auto obar = value_of({ { bar, 2 } });
    const auto ofoobar = value_of({ { foo, 1 }, { bar, 2 } });

    BST_REQUIRE_EQUAL(value_of({ ofoo, obar }), merged(value_of({ ofoo, obar }), value_of({ value::object(), value::object() })));
    BST_REQUIRE_EQUAL(value_of({ ofoo }), merged(value_of({ ofoo, obar }), value_of({ value::object() })));
    BST_REQUIRE_EQUAL(value_of({ ofoo }), merged(value_of({ ofoo, obar }), value_of({ value::object(), value::null() })));
    BST_REQUIRE_EQUAL(value_of({ obar }), merged(value_of({ ofoo, obar }), value_of({ value::null(), value::object() })));
    BST_REQUIRE_EQUAL(value::array(), merged(value_of({ ofoo, obar }), value_of({ value::null() })));
    BST_REQUIRE_EQUAL(value_of({ ofoo, obar }), merged(value_of({ ofoobar, ofoobar }), value_of({ value_of({ { bar, value::null() } }), value_of({ { foo, value::null() } }) })));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMergePatchRFC7386)
{
    // See https://tools.ietf.org/html/rfc7386#section-3

    const char* example = R"-json-(
       {
         "title": "Goodbye!",
         "author" : {
       "givenName" : "John",
       "familyName" : "Doe"
         },
         "tags":[ "example", "sample" ],
         "content": "This will be unchanged"
       }
    )-json-";

    const char* request = R"-json-(
       {
         "title": "Hello!",
         "phoneNumber": "+01-123-456-7890",
         "author": {
       "familyName": null
         },
         "tags": [ "example" ]
       }
    )-json-";

    const char* result = R"-json-(
       {
         "title": "Hello!",
         "author" : {
       "givenName" : "John"
         },
         "tags": [ "example" ],
         "content": "This will be unchanged",
         "phoneNumber": "+01-123-456-7890"
       }
    )-json-";

    const auto target = web::json::value::parse(utility::conversions::to_string_t(example));
    const auto source = web::json::value::parse(utility::conversions::to_string_t(request));
    const auto expected = web::json::value::parse(utility::conversions::to_string_t(result));

    BST_REQUIRE_EQUAL(expected, merged_permissive(target, source));
}

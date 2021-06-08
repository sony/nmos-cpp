// The first "test" is of course whether the header compiles standalone
#include "cpprest/json_validator.h"

#include "bst/test/test.h"
#include "cpprest/basic_utils.h" // for utility::us2s, utility::s2us
#include "cpprest/json_utils.h"

namespace
{
    const auto id = web::uri{ U("/test") };

    web::json::experimental::json_validator make_validator(const web::json::value& schema, const web::uri& id)
    {
        return web::json::experimental::json_validator
        {
            [&](const web::uri&) { return schema; },
            { id }
        };
    };
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testInvalidTypeSchema)
{
    using web::json::value_of;

    const bool keep_order = true;

    const auto schema = value_of({
        { U("$schema"), U("http://json-schema.org/draft-04/schema#")},
        { U("type"), U("object")},
        { U("properties"), value_of({
            { U("foo"), value_of({
                { U("anyOf"), value_of({
                    value_of({
                        { U("type"), U("string") },
                        { U("pattern"), U("^auto$") }
                    }, keep_order),
                    U("bad")
                })
            }})
        }})
    }});

   // invalid JSON-type for schema
   BST_REQUIRE_THROW(make_validator(schema, id), std::invalid_argument);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testEnumSchema)
{
    using web::json::value_of;

    const bool keep_order = true;

    const auto schema = value_of({
        { U("$schema"), U("http://json-schema.org/draft-04/schema#")},
        { U("type"), U("object")},
        { U("properties"), value_of({
            { U("foo"), value_of({
                { U("anyOf"), value_of({
                    value_of({
                        { U("type"), U("string") },
                        { U("pattern"), U("^auto$") }
                        }, keep_order),
                    value_of({
                        { U("enum"), value_of({
                            { U("good") }
                        })
                    }})
                })
            }})
        }})
    }});

    auto validator = make_validator(schema, id);

    // not in anyOf
    BST_REQUIRE_THROW(validator.validate(value_of({ { U("foo"), U("bad") } }), id), web::json::json_exception);

    // in anyOf, pattern
    validator.validate(value_of({ { U("foo"), U("auto") } }), id);
    BST_REQUIRE(true);

    // in anyOf, enum
    validator.validate(value_of({ { U("foo"), U("good") } }), id);
    BST_REQUIRE(true);
}

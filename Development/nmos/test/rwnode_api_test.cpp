// The first "test" is of course whether the header compiles standalone
#include "nmos/rwnode_api.h"

#include "bst/test/test.h"
#include "nmos/group_hint.h"
#include "nmos/json_fields.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMergeRwnodePatch)
{
    using web::json::value;
    using web::json::value_of;

    const auto source = value_of({
        { nmos::fields::label, U("meow") },
        { nmos::fields::description, U("purr") },
        { nmos::fields::tags, value_of({
            { U("foo"), value_of({ U("hiss"), U("yowl") }) },
            { nmos::fields::group_hint, value_of({ nmos::make_group_hint({ U("bar"), U("baz") }) }) }
        }) }
    });

    // empty patch
    {
        auto merged{ source };
        nmos::details::merge_rwnode_patch(merged, value::object());
        BST_REQUIRE_EQUAL(source, merged);
    }

    // reset everything
    {
        auto merged{ source };
        nmos::details::merge_rwnode_patch(merged, value_of({
            { nmos::fields::label, {} },
            { nmos::fields::description, {} },
            { nmos::fields::tags, {} }
        }));
        BST_REQUIRE(nmos::fields::label(merged).empty());
        BST_REQUIRE(nmos::fields::description(merged).empty());
        const auto& tags = merged.at(nmos::fields::tags);
        BST_REQUIRE_EQUAL(1, tags.size());
        const auto& group_hint = nmos::fields::group_hint(tags);
        BST_REQUIRE_EQUAL(1, group_hint.size());
        BST_REQUIRE_EQUAL(U("bar:baz"), group_hint.at(0).as_string());
    }

    // try to reset read-only tag
    {
        auto merged{ source };
        BST_REQUIRE_THROW(nmos::details::merge_rwnode_patch(merged, value_of({
            { nmos::fields::tags, value_of({
                { nmos::fields::group_hint, {} }
            }) }
        })), std::runtime_error);
    }

    // try to update read-only tag
    {
        auto merged{ source };
        BST_REQUIRE_THROW(nmos::details::merge_rwnode_patch(merged, value_of({
            { nmos::fields::tags, value_of({
                { nmos::fields::group_hint, value_of({ nmos::make_group_hint({ U("qux"), U("quux") }) }) }
            }) }
        })), std::runtime_error);
    }

    // add and remove tags
    {
        auto merged{ source };
        nmos::details::merge_rwnode_patch(merged, value_of({
            { nmos::fields::tags, value_of({
                { U("foo"), {} },
                { U("bar"), value_of({ U("woof"), U("bark") }) },
                { U("baz"), {} }
            }) }
        }));
        const auto& tags = merged.at(nmos::fields::tags);
        BST_REQUIRE_EQUAL(2, tags.size());
        BST_REQUIRE(!tags.has_field(U("foo")));
        BST_REQUIRE(tags.has_field(U("bar")));
        const auto& bar = tags.at(U("bar"));
        BST_REQUIRE_EQUAL(2, bar.size());
        BST_REQUIRE_EQUAL(U("bark"), bar.at(1).as_string());
    }

    // change label, description and tags
    {
        auto merged{ source };
        nmos::details::merge_rwnode_patch(merged, value_of({
            { nmos::fields::label, U("woof") },
            { nmos::fields::description, U("bark") },
            { nmos::fields::tags, value_of({
                { U("foo"), value_of({ U("growl") })}
            }) }
        }));
        BST_REQUIRE_EQUAL(U("woof"), nmos::fields::label(merged));
        BST_REQUIRE_EQUAL(U("bark"), nmos::fields::description(merged));
        const auto& tags = merged.at(nmos::fields::tags);
        BST_REQUIRE_EQUAL(2, tags.size());
        BST_REQUIRE(tags.has_field(U("foo")));
        const auto& foo = tags.at(U("foo"));
        BST_REQUIRE_EQUAL(1, foo.size());
        BST_REQUIRE_EQUAL(U("growl"), foo.at(0).as_string());
    }
}

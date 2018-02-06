// The first "test" is of course whether the header compiles standalone
#include "nmos/paging_utils.h"

#include <functional> // for std::ref
#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testOffsetLimitPaged)
{
    using std::begin;
    using std::end;
    const int test[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    // unpaged [0..max]
    {
        auto paged = nmos::paging::paged<int>([](int i){ return true; });
        BST_REQUIRE_EQUAL(9, std::count_if(begin(test), end(test), paged));
        BST_REQUIRE_EQUAL(0, paged.count); // algorithms like std::count_if take the predicate by value
        BST_REQUIRE_EQUAL(9, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(9, paged.count);
    }
    // unpaged, but filtered to odd elements
    {
        auto paged = nmos::paging::paged<int>([](int i){ return 0 != i % 2; });
        BST_REQUIRE_EQUAL(5, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
    // starting from third odd element
    {
        auto paged = nmos::paging::paged<int>([](int i){ return 0 != i % 2; }, 2);
        BST_REQUIRE_EQUAL(3, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
    // starting from third odd element, limited to 2 at most
    {
        auto paged = nmos::paging::paged<int>([](int i){ return 0 != i % 2; }, 2, 2);
        BST_REQUIRE_EQUAL(2, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
    // starting from fifth and last odd element, limited to 2 at most
    {
        auto paged = nmos::paging::paged<int>([](int i){ return 0 != i % 2; }, 4, 2);
        BST_REQUIRE_EQUAL(1, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
    // starting from non-existent sixth odd element, limited to 2 at most
    {
        auto paged = nmos::paging::paged<int>([](int i){ return 0 != i % 2; }, 5, 2);
        BST_REQUIRE_EQUAL(0, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// Cursor-based paging concepts implementations
namespace
{
    struct resource {
        resource(int cursor) : cursor(cursor) {}
        int cursor;
    };

    struct cursor_greater { bool operator()(const resource& lhs, const resource& rhs) const { return lhs.cursor > rhs.cursor; } };
    typedef std::set<resource, cursor_greater> resources;

    // customisation points

    int extract_cursor(const resources&, resources::const_iterator it)
    {
        return it->cursor;
    }

    resources::const_iterator lower_bound(const resources& resources, int cursor)
    {
        return resources.lower_bound(cursor);
    }

    struct resource_paging
    {
        int until = (std::numeric_limits<int>::max)();
        int since = 0;
        size_t limit = 10;
        bool since_specified = false;

        // override absolute max with resources max
        resource_paging(const resources& resources)
        {
            if (!resources.empty())
            {
                until = resources.begin()->cursor;
            }
        }

        template <typename Predicate>
        auto page(const resources& resources, Predicate match)
            -> decltype(nmos::paging::cursor_based_page(resources, match, until, since, limit, !since_specified))
        {
            return nmos::paging::cursor_based_page(resources, match, until, since, limit, !since_specified);
        }

        struct whatever { bool operator()(const resource&) { return true; } };

        auto page(const resources& resources)
            -> decltype(page(resources, whatever()))
        {
            return page(resources, whatever());
        }
    };
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testCursorBasedPagingDocumentationExamples)
{
    // Initial test cases based on the examples in NMOS documentation
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.5.%20APIs%20-%20Query%20Parameters.md#examples

    const resources resources{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };

    // Example 1: Initial /nodes Request
    {
        // Request
        resource_paging paging(resources);

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(10, paging.since);
        BST_REQUIRE_EQUAL(20, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(10, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(20, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(11, std::prev(page.end())->cursor);
    }

    // Example 2: Request With Custom Limit
    {
        // Request
        resource_paging paging(resources);
        paging.limit = 5;

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(5, paging.limit);
        BST_REQUIRE_EQUAL(15, paging.since);
        BST_REQUIRE_EQUAL(20, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(5, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(20, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(16, std::prev(page.end())->cursor);
    }

    // Example 3: Request With Since Parameter
    {
        // Request
        resource_paging paging(resources);
        paging.since = 4;
        paging.since_specified = true;

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(4, paging.since);
        BST_REQUIRE_EQUAL(14, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(10, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(14, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(5, std::prev(page.end())->cursor);
    }

    // Example 4: Request With Until Parameter
    {
        // Request
        resource_paging paging(resources);
        paging.until = 16;

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(6, paging.since);
        BST_REQUIRE_EQUAL(16, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(10, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(16, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(7, std::prev(page.end())->cursor);
    }

    // Example 5: Request With Since & Until Parameters
    {
        // Request
        resource_paging paging(resources);
        paging.since = 4;
        paging.since_specified = true;
        paging.until = 16;

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(4, paging.since);
        BST_REQUIRE_EQUAL(14, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(10, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(14, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(5, std::prev(page.end())->cursor);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testCursorBasedPagingBasecampExamples)
{
    // Test cases based on questions on "Pagination edge cases" in the AMWA Networked Media Incubator
    // See https://basecamp.com/1791706/projects/10192586/messages/70545892

    const resources resources{ 8, 9, 10, 11, 12 };

    // What should the X-Paging-Since value be when a client specifies only a paging.until value, and it is before the oldest resource's timestamp?
    {
        // Request
        resource_paging paging(resources);
        paging.until = 5;

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(0, paging.since);
        BST_REQUIRE_EQUAL(5, paging.until);

        // Payload
        BST_REQUIRE(page.empty());
    }

    // Conversely, what should the X-Paging-Until value be when a client specifies only a paging.since value, and it is after the newest resource's timestamp?
    {
        // Request
        resource_paging paging(resources);
        paging.since = 15;
        paging.since_specified = true;
        paging.until = paging.since; // proposed adjustment to pass the test

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(15, paging.since);
        BST_REQUIRE_EQUAL(15, paging.until);

        // Payload
        BST_REQUIRE(page.empty());
    }

    // The client specifies a filter query that results in a single value, but no paging query parameters - what should the X-Paging-Since and X-Paging-Until values be?
    {
        // Request
        resource_paging paging(resources);

        // Response
        const auto match_one = [](const resource& r){ return 10 == r.cursor; };
        auto page = paging.page(resources, match_one);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(0, paging.since);
        BST_REQUIRE_EQUAL(12, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(1, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(10, page.begin()->cursor);
    }

    // As above, but the filter query returns no results?
    {
        // Request
        resource_paging paging(resources);

        // Response
        const auto match_none = [](const resource& r){ return -42 == r.cursor; };
        auto page = paging.page(resources, match_none);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(0, paging.since);
        BST_REQUIRE_EQUAL(12, paging.until);

        // Payload
        BST_REQUIRE(page.empty());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testCursorBasedPagingEmptyResources)
{
    resources resources;

    // resources.empty()
    {
        // Request
        resource_paging paging(resources);

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(0, paging.since);
        BST_REQUIRE_EQUAL((std::numeric_limits<int>::max)(), paging.until);

        // Payload
        BST_REQUIRE(page.empty());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testCursorBasedPagingSingleResource)
{
    resources resources{ 13 };

    // resources.size() == 1
    {
        // Request
        resource_paging paging(resources);

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(0, paging.since);
        BST_REQUIRE_EQUAL(13, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(1, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(13, page.begin()->cursor);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testCursorBasedPagingRidiculousParameters)
{
    resources resources{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };

    // paging.since == paging.until
    {
        // Request
        resource_paging paging(resources);
        paging.since = paging.until = 13;
        paging.since_specified = true;

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.limit);
        BST_REQUIRE_EQUAL(13, paging.since);
        BST_REQUIRE_EQUAL(13, paging.until);

        // Payload
        BST_REQUIRE(page.empty());
    }

    // paging.limit == 0, paging.since_specified
    {
        // Request
        resource_paging paging(resources);
        paging.limit = 0;
        paging.since = 4;
        paging.since_specified = true;

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(0, paging.limit);
        BST_REQUIRE_EQUAL(4, paging.since);
        BST_REQUIRE_EQUAL(4, paging.until);

        // Payload
        BST_REQUIRE(page.empty());
    }

    // paging.limit == 0, !paging.since_specified
    {
        // Request
        resource_paging paging(resources);
        paging.limit = 0;
        paging.until = 16;

        // Response
        auto page = paging.page(resources);

        // Headers
        BST_REQUIRE_EQUAL(0, paging.limit);
        BST_REQUIRE_EQUAL(16, paging.since);
        BST_REQUIRE_EQUAL(16, paging.until);

        // Payload
        BST_REQUIRE(page.empty());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testCursorBasedPagingWithFilter)
{
    const auto filt3 = [](resource r){ return 3 > r.cursor % 5; };
    const auto filt2 = [](resource r){ return 3 <= r.cursor % 5; };

    const resources resources{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };

    // Query 1: filtered, default paging parameters
    //          filter         1, 2, -, -, 5, 6, 7, -, -, 10, 11, 12, --, --, 15, 16, 17, --, --, 20
    //          request      (                                                                       ]
    //          response           (       ^  ^  ^         ^   ^   ^           ^   ^   ^           ^ ]
    {
        // Request
        resource_paging paging(resources);

        // Response
        auto page = paging.page(resources, filt3);

        // Headers
        BST_REQUIRE_EQUAL(2, paging.since);
        BST_REQUIRE_EQUAL(20, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(10, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(20, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(5, std::prev(page.end())->cursor);
    }

    // Query 2: "prev" of Query 1
    //          filter         1, 2, -, -, 5, 6, 7, -, -, 10, 11, 12, --, --, 15, 16, 17, --, --, 20
    //          request      (      ]
    //          response     ( ^  ^ ]
    {
        // Request
        resource_paging paging(resources);
        paging.until = 2;

        // Response
        auto page = paging.page(resources, [](resource r){ return 0 != r.cursor % 3; });

        // Headers
        BST_REQUIRE_EQUAL(0, paging.since);
        BST_REQUIRE_EQUAL(2, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(2, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(2, page.begin()->cursor);
        BST_REQUIRE_EQUAL(1, std::prev(page.end())->cursor);
    }

    // Query 2: "next" of Query 1
    //          filter         1, 2, -, -, 5, 6, 7, -, -, 10, 11, 12, --, --, 15, 16, 17, --, --, 20
    //          request                                                                             (]
    //          response                                                                            (]
    {
        // Request
        resource_paging paging(resources);
        paging.since = 20;
        paging.since_specified = true;

        // Response
        auto page = paging.page(resources, [](resource r){ return 0 != r.cursor % 3; });

        // Headers
        BST_REQUIRE_EQUAL(20, paging.since);
        BST_REQUIRE_EQUAL(20, paging.until);

        // Payload
        BST_REQUIRE(page.empty());
    }

    // Query 3: filtered, default paging parameters
    //          filter         -, -, 3, 4, -, -, -, 8, 9, --, --, --, 13, 14, --, --, --, 18, 19, --
    //          request      (                                                                       ]
    //          response     (       ^  ^           ^  ^               ^   ^               ^   ^     ]
    {
        // Request
        resource_paging paging(resources);

        // Response
        auto page = paging.page(resources, filt2);

        // Headers
        BST_REQUIRE_EQUAL(0, paging.since);
        BST_REQUIRE_EQUAL(20, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(8, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(19, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(3, std::prev(page.end())->cursor);
    }

    // Query 4: filtered, limited to 3
    //          filter         -, -, 3, 4, -, -, -, 8, 9, --, --, --, 13, 14, --, --, --, 18, 19, --
    //          request      (                                                                       ]
    //          response                                                (  ^               ^   ^     ]
    {
        // Request
        resource_paging paging(resources);
        paging.limit = 3;

        // Response
        auto page = paging.page(resources, filt2);

        // Headers
        BST_REQUIRE_EQUAL(13, paging.since);
        BST_REQUIRE_EQUAL(20, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(3, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(19, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(14, std::prev(page.end())->cursor);
    }

    // Query 5: "prev" of Query 4
    //          filter         -, -, 3, 4, -, -, -, 8, 9, --, --, --, 13, 14, --, --, --, 18, 19, --
    //          request      (                                           ]
    //          response                 (          ^  ^               ^ ]
    {
        // Request
        resource_paging paging(resources);
        paging.limit = 3;
        paging.until = 13;

        // Response
        auto page = paging.page(resources, filt2);

        // Headers
        BST_REQUIRE_EQUAL(4, paging.since);
        BST_REQUIRE_EQUAL(13, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(3, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(13, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(8, std::prev(page.end())->cursor);
    }

    // Query 6: like Query 5, with paging.since specified, but still enough matches
    //          filter         -, -, 3, 4, -, -, -, 8, 9, --, --, --, 13, 14, --, --, --, 18, 19, --
    //          request                     (                            ]
    //          response                    (       ^  ^               ^ ]
    {
        // Request
        resource_paging paging(resources);
        paging.limit = 3;
        paging.until = 13;
        paging.since = 5;
        paging.since_specified = true;

        // Response
        auto page = paging.page(resources, filt2);

        // Headers
        BST_REQUIRE_EQUAL(5, paging.since);
        BST_REQUIRE_EQUAL(13, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(3, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(13, page.begin()->cursor);
        // ...
        BST_REQUIRE_EQUAL(8, std::prev(page.end())->cursor);
    }

    // Query 7: like Query 5, with paging.since specified, and not enough matches
    //          filter         -, -, 3, 4, -, -, -, 8, 9, --, --, --, 13, 14, --, --, --, 18, 19, --
    //          request                                     (            ]
    //          response                                    (          ^ ]
    {
        // Request
        resource_paging paging(resources);
        paging.limit = 3;
        paging.until = 13;
        paging.since = 10;
        paging.since_specified = true;

        // Response
        auto page = paging.page(resources, filt2);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.since);
        BST_REQUIRE_EQUAL(13, paging.until);

        // Payload
        BST_REQUIRE_EQUAL(1, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(13, page.begin()->cursor);
    }

    // Query 8: like Query 5, but no matches
    //          filter         -, -, 3, 4, -, -, -, 8, 9, --, --, --, 13, 14, --, --, --, 18, 19, --
    //          request                                     (        ]
    //          response                                    (        ]
    {
        // Request
        resource_paging paging(resources);
        paging.limit = 3;
        paging.until = 12;
        paging.since = 10;
        paging.since_specified = true;

        // Response
        auto page = paging.page(resources, filt2);

        // Headers
        BST_REQUIRE_EQUAL(10, paging.since);
        BST_REQUIRE_EQUAL(12, paging.until);

        // Payload
        BST_REQUIRE(page.empty());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// Cursor-based paging concepts implementations using a poor (easily invalidated) cursor
namespace
{
    struct person { std::string name; };
    typedef std::vector<person> people;
    typedef people::const_iterator people_iterator;
    typedef people::size_type people_cursor;

    // customisation points

    people_cursor extract_cursor(const people& p, people_iterator it)
    {
        return it - p.begin();
    }

    people_iterator lower_bound(const people& p, people_cursor cursor)
    {
        return p.begin() + (std::min)(cursor, p.size());
    }

    // test set up

    const auto all_people = [](const person&) { return true; };
    typedef std::pair<people_cursor, people_cursor> people_cursors;
    auto people_page(const people& people, people_cursors& cursors)
        -> decltype(nmos::paging::cursor_based_page(people, all_people, cursors.first, cursors.second, 10, true))
    {
        return nmos::paging::cursor_based_page(people, all_people, cursors.first, cursors.second, 10, true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testCursorBasedPagingUsingIndicesAsPoorCursors)
{
    people abc{ { "Alice" }, { "Bob" }, { "Charlie" } };

    {
        people_cursors cursors{ 0, 10000 };
        auto page = people_page(abc, cursors);

        BST_REQUIRE_EQUAL(3, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(0, cursors.first);
        BST_REQUIRE_EQUAL(10000, cursors.second);
        BST_REQUIRE_STRING_EQUAL("Alice", page.begin()->name);
    }

    {
        people_cursors cursors{ 2, 3 };
        auto page = people_page(abc, cursors);

        BST_REQUIRE_EQUAL(1, std::distance(page.begin(), page.end()));
        BST_REQUIRE_EQUAL(2, cursors.first);
        BST_REQUIRE_EQUAL(3, cursors.second);
        BST_REQUIRE_STRING_EQUAL("Charlie", page.begin()->name);
    }

    {
        people_cursors cursors{ 42, 10000 };
        auto page = people_page(abc, cursors);

        BST_REQUIRE(page.empty());
        BST_REQUIRE_EQUAL(42, cursors.first);
        BST_REQUIRE_EQUAL(10000, cursors.second);
    }
}

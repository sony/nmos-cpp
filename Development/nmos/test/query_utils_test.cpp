// The first "test" is of course whether the header compiles standalone
#include "nmos/query_utils.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testOffsetLimitPaged)
{
    using std::begin;
    using std::end;
    const int test[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    // unpaged [0..max]
    {
        auto paged = nmos::paged<int>([](int i){ return true; });
        BST_REQUIRE_EQUAL(9, std::count_if(begin(test), end(test), paged));
        BST_REQUIRE_EQUAL(0, paged.count); // algorithms like std::count_if take the predicate by value
        BST_REQUIRE_EQUAL(9, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(9, paged.count);
    }
    // unpaged, but filtered to odd elements
    {
        auto paged = nmos::paged<int>([](int i){ return 0 != i % 2; });
        BST_REQUIRE_EQUAL(5, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
    // starting from third odd element
    {
        auto paged = nmos::paged<int>([](int i){ return 0 != i % 2; }, 2);
        BST_REQUIRE_EQUAL(3, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
    // starting from third odd element, limited to 2 at most
    {
        auto paged = nmos::paged<int>([](int i){ return 0 != i % 2; }, 2, 2);
        BST_REQUIRE_EQUAL(2, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
    // starting from fifth and last odd element, limited to 2 at most
    {
        auto paged = nmos::paged<int>([](int i){ return 0 != i % 2; }, 4, 2);
        BST_REQUIRE_EQUAL(1, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
    // starting from non-existent sixth odd element, limited to 2 at most
    {
        auto paged = nmos::paged<int>([](int i){ return 0 != i % 2; }, 5, 2);
        BST_REQUIRE_EQUAL(0, std::count_if(begin(test), end(test), std::ref(paged)));
        BST_REQUIRE_EQUAL(5, paged.count);
    }
}

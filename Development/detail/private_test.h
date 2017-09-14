#ifndef DETAIL_PRIVATE_TEST_H
#define DETAIL_PRIVATE_TEST_H

// for unit tests of private implementation
namespace detail
{
    template <typename Tag> void test_private();
}

#define DETAIL_PRIVATE_TEST_DECLARATION \
    template <typename Tag> friend void ::detail::test_private();

#endif

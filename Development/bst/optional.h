#ifndef BST_OPTIONAL_H
#define BST_OPTIONAL_H

// Provide bst::optional, etc. using either std:: or boost:: symbols
// C++17 feature test macro: __has_include(<optional>), __cpp_lib_optional

#if !defined(BST_OPTIONAL_STD) && !defined(BST_OPTIONAL_BOOST)

#if defined(__GNUC__)
// std::optional is available from GCC 7.1, with -std=c++17
//#if __GNUC__  > 7 || (__GNUC__ == 7 && __GNUC_MINOR__ >= 1)
#if __cplusplus >= 201703L
#define BST_OPTIONAL_STD
#else
#define BST_OPTIONAL_BOOST
#endif

#elif defined(_MSC_VER)

#if _MSC_VER >= 1910
// From VS2017, /std:c++17 switch is introduced, but this is only indicated in __cplusplus if /Zc:__cplusplus is also specified
#if __cplusplus >= 201703L
#define BST_OPTIONAL_STD
#else
#define BST_OPTIONAL_BOOST
#endif
#else
// Earlier
#define BST_OPTIONAL_BOOST
#endif

#else

// Default to C++17
#define BST_OPTIONAL_STD

#endif

#endif

#if defined(BST_OPTIONAL_STD)

#include <optional>
namespace bst_optional = std;

namespace bst
{
    using std::nullopt_t;
    using std::nullopt;
    using std::bad_optional_access;
}

#elif defined(BST_OPTIONAL_BOOST)

#include <boost/optional.hpp>
namespace bst_optional = boost;

namespace bst
{
    typedef boost::none_t nullopt_t;
    const boost::none_t nullopt = boost::none;
#if BOOST_VERSION >= 105600
    using boost::bad_optional_access;
#else
    // Prior to Boost 1.56.0, boost::optional provided an interface that differed in a number of ways from that ultimately defined for C++17 std::optional
    // e.g. no member function value
#endif
}

#endif

namespace bst
{
    using bst_optional::optional;
    using bst_optional::make_optional;
}

#endif

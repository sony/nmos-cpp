#ifndef BST_ANY_H
#define BST_ANY_H

// Provide bst::any, etc. using either std:: or boost:: symbols
// C++17 feature test macro: __has_include(<any>), __cpp_lib_any

#if !defined(BST_ANY_STD) && !defined(BST_ANY_BOOST)

#if defined(__GNUC__)
// std::any is available from GCC 7.1, with -std=c++17
//#if __GNUC__  > 7 || (__GNUC__ == 7 && __GNUC_MINOR__ >= 1)
#if __cplusplus >= 201703L
#define BST_ANY_STD
#else
#define BST_ANY_BOOST
#endif

#elif defined(_MSC_VER)

#if _MSC_VER >= 1910
// From VS2017, /std:c++17 switch is introduced, but this is only indicated in __cplusplus if /Zc:__cplusplus is also specified
#if __cplusplus >= 201703L
#define BST_ANY_STD
#else
#define BST_ANY_BOOST
#endif
#else
// Earlier
#define BST_ANY_BOOST
#endif

#else

// Default to C++17
#define BST_ANY_STD

#endif

#endif

#if defined(BST_ANY_STD)

#include <any>
namespace bst_any = std;

#elif defined(BST_ANY_BOOST)

#include <boost/any.hpp>
namespace bst_any = boost;

#endif

namespace bst
{
    // Note that Boost.Any does not provide make_any or the in-place any constructors
    using bst_any::any;
    using bst_any::any_cast;
}

#endif

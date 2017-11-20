#ifndef BST_TEST_TEST_H
#define BST_TEST_TEST_H

////////////////////////////////////////////////////////////////////////////////////////////
// This file introduces a shim for C++ testing frameworks.
// It can currently map to the Boost Test library, Catch or the Google Test framework.
// See http://www.boost.org/doc/libs/release/libs/test/doc/html/ (1.57.0, 1.65.1)
// See https://github.com/philsquared/Catch (v1.10.0)
// See http://code.google.com/p/googletest/ (1.8.0)

////////////////////////////////////////////////////////////////////////////////////////////
// Configuration

// Select an underlying testing framework:
// BST_TEST_BOOST - use the Boost Test library
// BST_TEST_CATCH - use Catch (the current default)
// BST_TEST_GTEST - use the Google Test library

// BST_TEST_MAIN - define this before including this header file in just one source file, e.g. main.cpp

////////////////////////////////////////////////////////////////////////////////////////////
// Defining and registering a test case

// BST_TEST_CASE(symbol) - define and register a test case
// BST_TEST_CASE_PRIVATE(symbol) - define and register a test case that requires private access
// BST_TEMPLATE_TEST_CASE_{1..8}(symbol, T, T1, ...) - define and register a templated test case

////////////////////////////////////////////////////////////////////////////////////////////
// Making test assertions

// BST_{REQUIRE,CHECK,WARN}(expr) - test Boolean expression
// BST_{REQUIRE,CHECK,WARN}_{EQUAL,NE,GE,LE}(expected, actual) - test relationship between expected and actual
// BST_{REQUIRE,CHECK,WARN}_STRING_EQUAL(expected, actual) - same, but to test strings pointed to by char*
// BST_{REQUIRE,CHECK,WARN}_THROW(expr, exception) - test expression throws the indicated exception type
// BST_{REQUIRE,CHECK,WARN}_TRY - introduce a test of an exception-throwing statement
// BST_{REQUIRE,CHECK,WARN}_CATCH(exception) - test the preceding statement throws the indicated exception type
//
// Note: Not all combinations of level/flavour {REQUIRE,CHECK,WARN} and comparison {EQUAL,NE,GE,LE} may yet be
// implemented below, but can easily be added

////////////////////////////////////////////////////////////////////////////////////////////
// Implementation

#if   !defined(BST_TEST_CATCH) && !defined(BST_TEST_GTEST) && !defined(BST_TEST_BOOST)
#define        BST_TEST_CATCH
#elif  defined(BST_TEST_CATCH)  +  defined(BST_TEST_GTEST)  +  defined(BST_TEST_BOOST) > 1
#error "Only one of BST_TEST_CATCH, BST_TEST_GTEST and BST_TEST_BOOST must be specified"
#endif

#ifdef BST_TEST_CATCH
#include "bst/test/detail/catch-1.10.0.h"
#endif

#ifdef BST_TEST_BOOST
#include <boost/version.hpp>
#if BOOST_VERSION >= 106400
#include "bst/test/detail/boost_1_65_1.h"
#else
#include "bst/test/detail/boost_1_57_0.h"
#endif
#endif

#ifdef BST_TEST_GTEST
#include "bst/test/detail/googletest-release-1.8.0.h"
#endif

#include "detail/private_access.h"

#define BST_TEST_CASE_PRIVATE(symbol) \
    namespace { struct symbol ## _private_tag; } \
    namespace detail { template <> void private_access<symbol ## _private_tag>(); } \
    BST_TEST_CASE(symbol) \
    { \
        ::detail::private_access<symbol ## _private_tag>(); \
    } \
    template <> void ::detail::private_access<symbol ## _private_tag>()

#endif

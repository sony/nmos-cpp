#ifndef BST_TEST_TEST_H
#define BST_TEST_TEST_H

////////////////////////////////////////////////////////////////////////////////////////////
// This file introduces a shim for C++ testing frameworks.
// It can currently map to the Boost Test library, Catch or the Google Test framework.
// See http://www.boost.org/doc/libs/1_57_0/libs/test/doc/html/utf/testing-tools.html
// See https://github.com/philsquared/Catch
// See http://code.google.com/p/googletest/

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

// Use Catch to emulate some of the Boost Test library (mapped to BST_*)

#define CATCH_CONFIG_PREFIX_ALL

#ifndef BST_TEST_MAIN

#include "catch/catch.hpp"

#else

#include "detail/pragma_warnings.h"
#define CATCH_CONFIG_MAIN
PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_UNREACHABLE_CODE
#include "catch/catch.hpp"
PRAGMA_WARNING_POP

#endif

//+ Break INTERNAL_CATCH_THROWS_AS in two so the statement can contain commas, etc.
#define INTERNAL_CATCH_TRY( resultDisposition, macroName ) \
    do { \
        const Catch::ResultDisposition::Flags __resultDisposition = resultDisposition; \
        Catch::ResultBuilder __catchResult( macroName, CATCH_INTERNAL_LINEINFO, "statement", __resultDisposition ); \
        if( __catchResult.allowThrows() ) \
            try {

#define INTERNAL_CATCH_CATCH_AS( exceptionType ) \
                __catchResult.captureResult( Catch::ResultWas::DidntThrowException ); \
            } \
            catch( exceptionType ) { \
                __catchResult.captureResult( Catch::ResultWas::Ok ); \
            } \
            catch( ... ) { \
                __catchResult.useActiveException( __resultDisposition ); \
            } \
        else \
            __catchResult.captureResult( Catch::ResultWas::Ok ); \
        INTERNAL_CATCH_REACT( __catchResult ) \
    } while( Catch::alwaysFalse() )

// If this config identifier is defined then all CATCH macros are prefixed with CATCH_
#ifdef CATCH_CONFIG_PREFIX_ALL

#define CATCH_REQUIRE_TRY       INTERNAL_CATCH_TRY( Catch::ResultDisposition::Normal, "CATCH_REQUIRE_THROWS_AS" )
#define CATCH_CHECK_TRY         INTERNAL_CATCH_TRY( Catch::ResultDisposition::ContinueOnFailure, "CATCH_CHECK_THROWS_AS" )
#define CATCH_CHECK_TRY_NOFAIL  INTERNAL_CATCH_TRY( Catch::ResultDisposition::ContinueOnFailure | Catch::ResultDisposition::SuppressFail, "CATCH_CHECK_THROWS_AS_NOFAIL" )

#define CATCH_REQUIRE_CATCH_AS( exceptionType )         INTERNAL_CATCH_CATCH_AS( exceptionType )
#define CATCH_CHECK_CATCH_AS( exceptionType )           INTERNAL_CATCH_CATCH_AS( exceptionType )
#define CATCH_CHECK_CATCH_AS_NOFAIL( exceptionType )    INTERNAL_CATCH_CATCH_AS( exceptionType )

#define CATCH_CHECK_THROWS_AS_NOFAIL( expr, exceptionType ) INTERNAL_CATCH_THROWS_AS( expr, exceptionType, Catch::ResultDisposition::ContinueOnFailure | Catch::ResultDisposition::SuppressFail, "CATCH_CHECK_THROWS_AS_NOFAIL" )

// If CATCH_CONFIG_PREFIX_ALL is not defined then the CATCH_ prefix is not required
#else

#define REQUIRE_TRY       INTERNAL_CATCH_TRY( Catch::ResultDisposition::Normal, "REQUIRE_THROWS_AS" )
#define CHECK_TRY         INTERNAL_CATCH_TRY( Catch::ResultDisposition::ContinueOnFailure, "CHECK_THROWS_AS" )
#define CHECK_TRY_NOFAIL  INTERNAL_CATCH_TRY( Catch::ResultDisposition::ContinueOnFailure | Catch::ResultDisposition::SuppressFail, "CHECK_THROWS_AS_NOFAIL" )

#define REQUIRE_CATCH_AS( exceptionType )         INTERNAL_CATCH_CATCH_AS( exceptionType )
#define CHECK_CATCH_AS( exceptionType )           INTERNAL_CATCH_CATCH_AS( exceptionType )
#define CHECK_CATCH_AS_NOFAIL( exceptionType )    INTERNAL_CATCH_CATCH_AS( exceptionType )

#define CHECK_THROWS_AS_NOFAIL( expr, exceptionType ) INTERNAL_CATCH_THROWS_AS( expr, exceptionType, Catch::ResultDisposition::ContinueOnFailure | Catch::ResultDisposition::SuppressFail, "CHECK_THROWS_AS_NOFAIL" )

#endif
//- Break INTERNAL_CATCH_THROWS_AS in two so the statement can contain commas, etc.

//+ Add "test templates" for Catch
// See https://github.com/philsquared/Catch/pull/382

// Type-parameterised tests via test case templates
// See https://github.com/philsquared/Catch/issues/46
// and https://github.com/philsquared/Catch/issues/357

#define INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( T ) \
    template<typename T> \
    static void INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ )();

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( Tn ) \
        INTERNAL_CATCH_SECTION( #Tn, "" ) \
        { \
            INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ )<Tn>(); \
        }

#define INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( T ) \
    template<typename T> \
    static void INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ )()

// Simple repetition, which could be accomplished more tersely with some extra preprocessor magic

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_1( name, description, T, T1 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T1 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_2( name, description, T, T1, T2 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T2 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_3( name, description, T, T1, T2, T3 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T3 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_4( name, description, T, T1, T2, T3, T4 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T4 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_5( name, description, T, T1, T2, T3, T4, T5 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T4 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T5 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_6( name, description, T, T1, T2, T3, T4, T5, T6 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T4 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T5 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T6 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_7( name, description, T, T1, T2, T3, T4, T5, T6, T7 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T4 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T5 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T6 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T7 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_8( name, description, T, T1, T2, T3, T4, T5, T6, T7, T8 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T4 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T5 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T6 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T7 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( T8 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( T )

// If this config identifier is defined then all CATCH macros are prefixed with CATCH_
#ifdef CATCH_CONFIG_PREFIX_ALL

#define CATCH_TEMPLATE_TEST_CASE_1(name, description, T, T1) INTERNAL_CATCH_TEMPLATE_TEST_CASE_1(name, description, T, T1)
#define CATCH_TEMPLATE_TEST_CASE_2(name, description, T, T1, T2) INTERNAL_CATCH_TEMPLATE_TEST_CASE_2(name, description, T, T1, T2)
#define CATCH_TEMPLATE_TEST_CASE_3(name, description, T, T1, T2, T3) INTERNAL_CATCH_TEMPLATE_TEST_CASE_3(name, description, T, T1, T2, T3)
#define CATCH_TEMPLATE_TEST_CASE_4(name, description, T, T1, T2, T3, T4) INTERNAL_CATCH_TEMPLATE_TEST_CASE_4(name, description, T, T1, T2, T3, T4)
#define CATCH_TEMPLATE_TEST_CASE_5(name, description, T, T1, T2, T3, T4, T5) INTERNAL_CATCH_TEMPLATE_TEST_CASE_5(name, description, T, T1, T2, T3, T4, T5)
#define CATCH_TEMPLATE_TEST_CASE_6(name, description, T, T1, T2, T3, T4, T5, T6) INTERNAL_CATCH_TEMPLATE_TEST_CASE_6(name, description, T, T1, T2, T3, T4, T5, T6)
#define CATCH_TEMPLATE_TEST_CASE_7(name, description, T, T1, T2, T3, T4, T5, T6, T7) INTERNAL_CATCH_TEMPLATE_TEST_CASE_7(name, description, T, T1, T2, T3, T4, T5, T6, T7)
#define CATCH_TEMPLATE_TEST_CASE_8(name, description, T, T1, T2, T3, T4, T5, T6, T7, T8) INTERNAL_CATCH_TEMPLATE_TEST_CASE_8(name, description, T, T1, T2, T3, T4, T5, T6, T7, T8)

// If CATCH_CONFIG_PREFIX_ALL is not defined then the CATCH_ prefix is not required
#else

#define TEMPLATE_TEST_CASE_1(name, description, T, T1) INTERNAL_CATCH_TEMPLATE_TEST_CASE_1(name, description, T, T1)
#define TEMPLATE_TEST_CASE_2(name, description, T, T1, T2) INTERNAL_CATCH_TEMPLATE_TEST_CASE_2(name, description, T, T1, T2)
#define TEMPLATE_TEST_CASE_3(name, description, T, T1, T2, T3) INTERNAL_CATCH_TEMPLATE_TEST_CASE_3(name, description, T, T1, T2, T3)
#define TEMPLATE_TEST_CASE_4(name, description, T, T1, T2, T3, T4) INTERNAL_CATCH_TEMPLATE_TEST_CASE_4(name, description, T, T1, T2, T3, T4)
#define TEMPLATE_TEST_CASE_5(name, description, T, T1, T2, T3, T4, T5) INTERNAL_CATCH_TEMPLATE_TEST_CASE_5(name, description, T, T1, T2, T3, T4, T5)
#define TEMPLATE_TEST_CASE_6(name, description, T, T1, T2, T3, T4, T5, T6) INTERNAL_CATCH_TEMPLATE_TEST_CASE_6(name, description, T, T1, T2, T3, T4, T5, T6)
#define TEMPLATE_TEST_CASE_7(name, description, T, T1, T2, T3, T4, T5, T6, T7) INTERNAL_CATCH_TEMPLATE_TEST_CASE_7(name, description, T, T1, T2, T3, T4, T5, T6, T7)
#define TEMPLATE_TEST_CASE_8(name, description, T, T1, T2, T3, T4, T5, T6, T7, T8) INTERNAL_CATCH_TEMPLATE_TEST_CASE_8(name, description, T, T1, T2, T3, T4, T5, T6, T7, T8)

#endif
//- Add "test templates" for Catch

#define BST_TEST_CASE(symbol) CATCH_TEST_CASE(#symbol)
#define BST_CHECK_EQUAL(expected, actual) CATCH_CHECK((expected) == (actual))
#define BST_CHECK_NE(expected, actual) CATCH_CHECK((expected) != (actual))
#define BST_CHECK_THROW(expr, exception) CATCH_CHECK_THROWS_AS(expr, exception)
#define BST_REQUIRE(expr) CATCH_REQUIRE(expr)
#define BST_REQUIRE_EQUAL(expected, actual) CATCH_REQUIRE((expected) == (actual))
#define BST_REQUIRE_NE(expected, actual) CATCH_REQUIRE((expected) != (actual))
#define BST_REQUIRE_GE(expected, actual) CATCH_REQUIRE((expected) >= (actual))
#define BST_REQUIRE_LE(expected, actual) CATCH_REQUIRE((expected) <= (actual))
#define BST_REQUIRE_THROW(expr, exception) CATCH_REQUIRE_THROWS_AS(expr, exception)
#define BST_WARN(expr) CATCH_CHECK_NOFAIL(expr)
#define BST_WARN_EQUAL(expected, actual) CATCH_CHECK_NOFAIL((expected) == (actual))
#define BST_WARN_NE(expected, actual) CATCH_CHECK_NOFAIL((expected) != (actual))

// Extension to workaround different behaviour of Catch and Boost Test library when comparing char*
#define BST_CHECK_STRING_EQUAL(expected, actual) CATCH_CHECK(std::string(expected) == std::string(actual))
#define BST_REQUIRE_STRING_EQUAL(expected, actual) CATCH_REQUIRE(std::string(expected) == std::string(actual))

#define BST_WARN_TRY CATCH_CHECK_TRY_NOFAIL
#define BST_CHECK_TRY CATCH_CHECK_TRY
#define BST_REQUIRE_TRY CATCH_REQUIRE_TRY

#define BST_WARN_CATCH(exception) CATCH_CHECK_CATCH_AS_NOFAIL(exception)
#define BST_CHECK_CATCH(exception) CATCH_CHECK_CATCH_AS(exception)
#define BST_REQUIRE_CATCH(exception) CATCH_REQUIRE_CATCH_AS(exception)

#define BST_TEMPLATE_TEST_CASE_1(symbol, T, T1) CATCH_TEMPLATE_TEST_CASE_1(#symbol, "", T, T1)
#define BST_TEMPLATE_TEST_CASE_2(symbol, T, T1, T2) CATCH_TEMPLATE_TEST_CASE_2(#symbol, "", T, T1, T2)
#define BST_TEMPLATE_TEST_CASE_3(symbol, T, T1, T2, T3) CATCH_TEMPLATE_TEST_CASE_3(#symbol, "", T, T1, T2, T3)
#define BST_TEMPLATE_TEST_CASE_4(symbol, T, T1, T2, T3, T4) CATCH_TEMPLATE_TEST_CASE_4(#symbol, "", T, T1, T2, T3, T4)
#define BST_TEMPLATE_TEST_CASE_5(symbol, T, T1, T2, T3, T4, T5) CATCH_TEMPLATE_TEST_CASE_5(#symbol, "", T, T1, T2, T3, T4, T5)
#define BST_TEMPLATE_TEST_CASE_6(symbol, T, T1, T2, T3, T4, T5, T6) CATCH_TEMPLATE_TEST_CASE_6(#symbol, "", T, T1, T2, T3, T4, T5, T6)
#define BST_TEMPLATE_TEST_CASE_7(symbol, T, T1, T2, T3, T4, T5, T6, T7) CATCH_TEMPLATE_TEST_CASE_7(#symbol, "", T, T1, T2, T3, T4, T5, T6, T7)
#define BST_TEMPLATE_TEST_CASE_8(symbol, T, T1, T2, T3, T4, T5, T6, T7, T8) CATCH_TEMPLATE_TEST_CASE_8(#symbol, "", T, T1, T2, T3, T4, T5, T6, T7, T8)

#endif

#ifdef BST_TEST_BOOST

// Use the Boost Test library (mapped to BST_*)

#ifndef BST_TEST_MAIN

#define BOOST_TEST_INCLUDED
#include <boost/test/unit_test.hpp>

#else

#include "detail/pragma_warnings.h"
#define BOOST_TEST_MAIN
PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_UNREACHABLE_CODE
#include <boost/test/included/unit_test.hpp>
PRAGMA_WARNING_POP

#endif

//+ Break BOOST_CHECK_THROW_IMPL in two so the statement can contain commas, etc.
#define BOOST_CHECK_TRY_IMPL                                                                            \
    try {                                                                                               \
        BOOST_TEST_PASSPOINT();                                                                         \
/**/

#define BOOST_CHECK_CATCH_IMPL( E, P, prefix, TL )                                                      \
        BOOST_CHECK_IMPL( false, "exception " BOOST_STRINGIZE( E ) " is expected", TL, CHECK_MSG ); }   \
    catch( E const& ex ) {                                                                              \
        ::boost::unit_test::ut_detail::ignore_unused_variable_warning( ex );                            \
        BOOST_CHECK_IMPL( P, prefix BOOST_STRINGIZE( E ) " is caught", TL, CHECK_MSG );                 \
    }                                                                                                   \
/**/

#define BOOST_WARN_TRY            BOOST_CHECK_TRY_IMPL
#define BOOST_CHECK_TRY           BOOST_CHECK_TRY_IMPL
#define BOOST_REQUIRE_TRY         BOOST_CHECK_TRY_IMPL

#define BOOST_WARN_CATCH( E )     BOOST_CHECK_CATCH_IMPL( E, true, "exception ", WARN )
#define BOOST_CHECK_CATCH( E )    BOOST_CHECK_CATCH_IMPL( E, true, "exception ", CHECK )
#define BOOST_REQUIRE_CATCH( E )  BOOST_CHECK_CATCH_IMPL( E, true, "exception ", REQUIRE )
//- Break BOOST_CHECK_THROW_IMPL in two so the statement can contain commas, etc.

#define BST_TEST_CASE(symbol) BOOST_AUTO_TEST_CASE(symbol)
#define BST_CHECK_EQUAL(expected, actual) BOOST_CHECK_EQUAL(expected, actual)
#define BST_CHECK_NE(expected, actual) BOOST_CHECK_NE(expected, actual)
#define BST_CHECK_THROW(expr, exception) BOOST_CHECK_THROW(expr, exception) 
#define BST_REQUIRE(expr) BOOST_REQUIRE(expr)
#define BST_REQUIRE_EQUAL(expected, actual) BOOST_REQUIRE_EQUAL(expected, actual)
#define BST_REQUIRE_NE(expected, actual) BOOST_REQUIRE_NE(expected, actual)
#define BST_REQUIRE_GE(expected, actual) BOOST_REQUIRE_GE(expected, actual)
#define BST_REQUIRE_LE(expected, actual) BOOST_REQUIRE_LE(expected, actual)
#define BST_REQUIRE_THROW(expr, exception) BOOST_REQUIRE_THROW(expr, exception) 
#define BST_WARN(expr) BOOST_WARN(expr)
#define BST_WARN_EQUAL(expected, actual) BOOST_WARN_EQUAL(expected, actual)
#define BST_WARN_NE(expected, actual) BOOST_WARN_NE(expected, actual)

// Extension to workaround different behaviour of Catch and Boost Test library when comparing char*
#define BST_CHECK_STRING_EQUAL(expected, actual) BOOST_CHECK_EQUAL(expected, actual)
#define BST_REQUIRE_STRING_EQUAL(expected, actual) BOOST_REQUIRE_EQUAL(expected, actual)

#define BST_WARN_TRY BOOST_WARN_TRY
#define BST_CHECK_TRY BOOST_CHECK_TRY
#define BST_REQUIRE_TRY BOOST_REQUIRE_TRY

#define BST_WARN_CATCH(exception) BOOST_WARN_CATCH(exception)
#define BST_CHECK_CATCH(exception) BOOST_CHECK_CATCH(exception)
#define BST_REQUIRE_CATCH(exception) BOOST_REQUIRE_CATCH(exception)

#include <boost/mpl/list.hpp>
#include <boost/mpl/joint_view.hpp>

#define BST_TEMPLATE_TEST_CASE_1(symbol, T, T1) \
    namespace { typedef boost::mpl::list<T1> symbol##_types; } \
    BOOST_AUTO_TEST_CASE_TEMPLATE(symbol, T, symbol##_types)

#define BST_TEMPLATE_TEST_CASE_2(symbol, T, T1, T2) \
    namespace { typedef boost::mpl::list<T1, T2> symbol##_types; } \
    BOOST_AUTO_TEST_CASE_TEMPLATE(symbol, T, symbol##_types)

#define BST_TEMPLATE_TEST_CASE_3(symbol, T, T1, T2, T3) \
    namespace { typedef boost::mpl::list<T1, T2, T3> symbol##_types; } \
    BOOST_AUTO_TEST_CASE_TEMPLATE(symbol, T, symbol##_types)

#define BST_TEMPLATE_TEST_CASE_4(symbol, T, T1, T2, T3, T4) \
    namespace { typedef boost::mpl::list<T1, T2, T3, T4> symbol##_types; } \
    BOOST_AUTO_TEST_CASE_TEMPLATE(symbol, T, symbol##_types)

#define BST_TEMPLATE_TEST_CASE_5(symbol, T, T1, T2, T3, T4, T5) \
    namespace { typedef boost::mpl::list<T1, T2, T3, T4, T5> symbol##_types; } \
    BOOST_AUTO_TEST_CASE_TEMPLATE(symbol, T, symbol##_types)

#define BST_TEMPLATE_TEST_CASE_6(symbol, T, T1, T2, T3, T4, T5, T6) \
    namespace { typedef boost::mpl::list<T1, T2, T3, T4, T5, T6> symbol##_types; } \
    BOOST_AUTO_TEST_CASE_TEMPLATE(symbol, T, symbol##_types)

#define BST_TEMPLATE_TEST_CASE_7(symbol, T, T1, T2, T3, T4, T5, T6, T7) \
    namespace { typedef boost::mpl::list<T1, T2, T3, T4, T5, T6, T7> symbol##_types; } \
    BOOST_AUTO_TEST_CASE_TEMPLATE(symbol, T, symbol##_types)

#define BST_TEMPLATE_TEST_CASE_8(symbol, T, T1, T2, T3, T4, T5, T6, T7, T8) \
    namespace { typedef boost::mpl::list<T1, T2, T3, T4, T5, T6, T7, T8> symbol##_types; } \
    BOOST_AUTO_TEST_CASE_TEMPLATE(symbol, T, symbol##_types)

#endif

#ifdef BST_TEST_GTEST

// Use Google Test to emulate some of the Boost Test library (mapped to BST_*)

#define GTEST_HAS_TR1_TUPLE 0
#define GTEST_USE_OWN_TR1_TUPLE 1

#ifndef BST_TEST_MAIN

#include "gtest/gtest.h"

#else

#include "gtest/gtest_main.cc"
#include "gtest/gtest-all.cc"

#endif

//+ Break GTEST_TEST_THROW_ in two so the statement can contain commas, etc.
#define GTEST_TEST_TRY_ \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (::testing::internal::ConstCharPtr gtest_msg = "") { \
    bool gtest_caught_expected = false; \
    try { \
      if (::testing::internal::AlwaysTrue()) {

#define GTEST_TEST_CATCH_(expected_exception, fail) \
      } \
    } \
    catch (expected_exception const&) { \
      gtest_caught_expected = true; \
    } \
    catch (...) { \
      gtest_msg.value = \
          "Expected: " "statement" " throws an exception of type " \
          #expected_exception ".\n  Actual: it throws a different type."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
    if (!gtest_caught_expected) { \
      gtest_msg.value = \
          "Expected: " "statement" " throws an exception of type " \
          #expected_exception ".\n  Actual: it throws nothing."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__): \
      fail(gtest_msg.value)

#define EXPECT_TRY GTEST_TEST_TRY_
#define ASSERT_TRY GTEST_TEST_TRY_

#define EXPECT_CATCH(expected_exception) GTEST_TEST_CATCH_(expected_exception, GTEST_NONFATAL_FAILURE_)
#define ASSERT_CATCH(expected_exception) GTEST_TEST_CATCH_(expected_exception, GTEST_FATAL_FAILURE_)
//- Break GTEST_TEST_THROW_ in two so the statement can contain commas, etc.

//+ Tweak TYPED_TEST so we can give a custom name to the type parameter
#define GTEST_STRINGIZE(x) #x

# define TYPED_TEST_NAMED_TYPE_PARAM(CaseName, TestName, TypeParam) \
  template <typename gtest_TypeParam_> \
  class GTEST_TEST_CLASS_NAME_(CaseName, TestName) \
      : public CaseName<gtest_TypeParam_> { \
   private: \
    typedef CaseName<gtest_TypeParam_> TestFixture; \
    typedef gtest_TypeParam_ TypeParam; \
    virtual void TestBody(); \
  }; \
  bool GTEST_CONCAT_TOKEN_(gtest_, CaseName) ##_##TestName##_registered_ GTEST_ATTRIBUTE_UNUSED_ = \
      ::testing::internal::TypeParameterizedTest< \
          CaseName, \
          ::testing::internal::TemplateSel< \
              GTEST_TEST_CLASS_NAME_(CaseName, TestName)>, \
          GTEST_TYPE_PARAMS_(CaseName)>::Register(\
              "", GTEST_STRINGIZE(CaseName), #TestName, 0); \
  template <typename gtest_TypeParam_> \
  void GTEST_TEST_CLASS_NAME_(CaseName, TestName)<gtest_TypeParam_>::TestBody()
//- Tweak TYPED_TEST so we can give a custom name to the type parameter

#define BST_TEST_CASE(symbol) TEST(bstTestCase, symbol)
#define BST_CHECK_EQUAL(expected, actual) EXPECT_EQ(expected, actual)
#define BST_CHECK_NE(expected, actual) EXPECT_NE(expected, actual)
#define BST_CHECK_THROW(expr, exception) EXPECT_THROW(expr, exception)
#define BST_REQUIRE(expr) ASSERT_TRUE(expr)
#define BST_REQUIRE_EQUAL(expected, actual) ASSERT_EQ(expected, actual)
#define BST_REQUIRE_NE(expected, actual) ASSERT_NE(expected, actual)
#define BST_REQUIRE_GE(expected, actual) ASSERT_GE(expected, actual)
#define BST_REQUIRE_LE(expected, actual) ASSERT_LE(expected, actual)
#define BST_REQUIRE_THROW(expr, exception) ASSERT_THROW(expr, exception)
// Hmm, Google Test doesn't have the equivalent of WARN?
#define BST_WARN(expr) EXPECT_TRUE(expr)
#define BST_WARN_EQUAL(expected, actual) EXPECT_EQ(expected, actual)
#define BST_WARN_NE(expected, actual) EXPECT_NE(expected, actual)

#define BST_CHECK_STRING_EQUAL(expected, actual) EXPECT_STREQ(expected, actual)
#define BST_REQUIRE_STRING_EQUAL(expected, actual) ASSERT_STREQ(expected, actual)

#define BST_WARN_TRY EXPECT_TRY
#define BST_CHECK_TRY EXPECT_TRY
#define BST_REQUIRE_TRY ASSERT_TRY

#define BST_WARN_CATCH(exception) EXPECT_CATCH(exception)
#define BST_CHECK_CATCH(exception) EXPECT_CATCH(exception)
#define BST_REQUIRE_CATCH(exception) ASSERT_CATCH(exception)

#define BST_GTEST_TYPED_TEST_CASE_(symbol, T) \
  namespace { \
    template <typename T> \
    class GTEST_CONCAT_TOKEN_(bstTemplateTestCase, __LINE__) : public ::testing::Test {}; \
  } \
  TYPED_TEST_CASE(GTEST_CONCAT_TOKEN_(bstTemplateTestCase, __LINE__), symbol ## Types); \
  TYPED_TEST_NAMED_TYPE_PARAM(GTEST_CONCAT_TOKEN_(bstTemplateTestCase, __LINE__), symbol, T)

#define BST_TEMPLATE_TEST_CASE_1(symbol, T, T1) \
  namespace { typedef ::testing::Types<T1> symbol ## Types; } \
  BST_GTEST_TYPED_TEST_CASE_(symbol, T)

#define BST_TEMPLATE_TEST_CASE_2(symbol, T, T1, T2) \
  namespace { typedef ::testing::Types<T1, T2> symbol ## Types; } \
  BST_GTEST_TYPED_TEST_CASE_(symbol, T)

#define BST_TEMPLATE_TEST_CASE_3(symbol, T, T1, T2, T3) \
  namespace { typedef ::testing::Types<T1, T2, T3> symbol ## Types; } \
  BST_GTEST_TYPED_TEST_CASE_(symbol, T)

#define BST_TEMPLATE_TEST_CASE_4(symbol, T, T1, T2, T3, T4) \
  namespace { typedef ::testing::Types<T1, T2, T3, T4> symbol ## Types; } \
  BST_GTEST_TYPED_TEST_CASE_(symbol, T)

#define BST_TEMPLATE_TEST_CASE_5(symbol, T, T1, T2, T3, T4, T5) \
  namespace { typedef ::testing::Types<T1, T2, T3, T4, T5> symbol ## Types; } \
  BST_GTEST_TYPED_TEST_CASE_(symbol, T)

#define BST_TEMPLATE_TEST_CASE_6(symbol, T, T1, T2, T3, T4, T5, T6) \
  namespace { typedef ::testing::Types<T1, T2, T3, T4, T5, T6> symbol ## Types; } \
  BST_GTEST_TYPED_TEST_CASE_(symbol, T)

#define BST_TEMPLATE_TEST_CASE_7(symbol, T, T1, T2, T3, T4, T5, T6, T7) \
  namespace { typedef ::testing::Types<T1, T2, T3, T4, T5, T6, T7> symbol ## Types; } \
  BST_GTEST_TYPED_TEST_CASE_(symbol, T)

#define BST_TEMPLATE_TEST_CASE_8(symbol, T, T1, T2, T3, T4, T5, T6, T7, T8) \
  namespace { typedef ::testing::Types<T1, T2, T3, T4, T5, T6, T7, T8> symbol ## Types; } \
  BST_GTEST_TYPED_TEST_CASE_(symbol, T)

#endif

#include "detail/private_test.h"

#define BST_TEST_CASE_PRIVATE(symbol) \
  namespace { struct symbol ## _private_tag; } \
  template <> void ::detail::test_private<symbol ## _private_tag>(); \
  BST_TEST_CASE(symbol) \
  { \
    ::detail::test_private<symbol ## _private_tag>(); \
  } \
  template <> void ::detail::test_private<symbol ## _private_tag>()

#endif

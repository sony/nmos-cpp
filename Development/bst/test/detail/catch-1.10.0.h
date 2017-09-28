#ifndef BST_TEST_DETAIL_CATCH_1_10_0_H
#define BST_TEST_DETAIL_CATCH_1_10_0_H

// Use Catch (v1.10.0) to emulate some of the Boost Test library (mapped to BST_*)

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
#define INTERNAL_CATCH_TRY( macroName, resultDisposition ) \
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

#define CATCH_REQUIRE_TRY       INTERNAL_CATCH_TRY( "CATCH_REQUIRE_THROWS_AS", Catch::ResultDisposition::Normal )
#define CATCH_CHECK_TRY         INTERNAL_CATCH_TRY( "CATCH_CHECK_THROWS_AS", Catch::ResultDisposition::ContinueOnFailure )
#define CATCH_CHECK_TRY_NOFAIL  INTERNAL_CATCH_TRY( "CATCH_CHECK_THROWS_AS_NOFAIL", Catch::ResultDisposition::ContinueOnFailure | Catch::ResultDisposition::SuppressFail )

#define CATCH_REQUIRE_CATCH_AS( exceptionType )         INTERNAL_CATCH_CATCH_AS( exceptionType )
#define CATCH_CHECK_CATCH_AS( exceptionType )           INTERNAL_CATCH_CATCH_AS( exceptionType )
#define CATCH_CHECK_CATCH_AS_NOFAIL( exceptionType )    INTERNAL_CATCH_CATCH_AS( exceptionType )

#define CATCH_CHECK_THROWS_AS_NOFAIL( expr, exceptionType ) INTERNAL_CATCH_THROWS_AS( "CATCH_CHECK_THROWS_AS_NOFAIL", exceptionType, Catch::ResultDisposition::ContinueOnFailure | Catch::ResultDisposition::SuppressFail, expr )

// If CATCH_CONFIG_PREFIX_ALL is not defined then the CATCH_ prefix is not required
#else

#define REQUIRE_TRY       INTERNAL_CATCH_TRY( "REQUIRE_THROWS_AS", Catch::ResultDisposition::Normal )
#define CHECK_TRY         INTERNAL_CATCH_TRY( "CHECK_THROWS_AS", Catch::ResultDisposition::ContinueOnFailure )
#define CHECK_TRY_NOFAIL  INTERNAL_CATCH_TRY( "CHECK_THROWS_AS_NOFAIL", Catch::ResultDisposition::ContinueOnFailure | Catch::ResultDisposition::SuppressFail )

#define REQUIRE_CATCH_AS( exceptionType )         INTERNAL_CATCH_CATCH_AS( exceptionType )
#define CHECK_CATCH_AS( exceptionType )           INTERNAL_CATCH_CATCH_AS( exceptionType )
#define CHECK_CATCH_AS_NOFAIL( exceptionType )    INTERNAL_CATCH_CATCH_AS( exceptionType )

#define CHECK_THROWS_AS_NOFAIL( expr, exceptionType ) INTERNAL_CATCH_THROWS_AS( "CHECK_THROWS_AS_NOFAIL", exceptionType, Catch::ResultDisposition::ContinueOnFailure | Catch::ResultDisposition::SuppressFail, expr )

#endif
//- Break INTERNAL_CATCH_THROWS_AS in two so the statement can contain commas, etc.

//+ Add "test templates" for Catch
// See https://github.com/philsquared/Catch/pull/382

// Type-parameterised tests via test case templates
// See https://github.com/philsquared/Catch/issues/46
// and https://github.com/philsquared/Catch/issues/357

#define INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( TestName, T ) \
    template<typename T> \
    static void TestName();

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, Tn ) \
        INTERNAL_CATCH_SECTION( #Tn, "" ) \
        { \
            TestName<Tn>(); \
        }

#define INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( TestName, T ) \
    template<typename T> \
    static void TestName()

// Simple repetition, which could be accomplished more tersely with some extra preprocessor magic

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE2_1( TestName, name, description, T, T1 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( TestName, T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T1 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( TestName, T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE2_2( TestName, name, description, T, T1, T2 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( TestName, T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T2 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( TestName, T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE2_3( TestName, name, description, T, T1, T2, T3 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( TestName, T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T3 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( TestName, T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE2_4( TestName, name, description, T, T1, T2, T3, T4 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( TestName, T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T4 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( TestName, T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE2_5( TestName, name, description, T, T1, T2, T3, T4, T5 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( TestName, T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T4 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T5 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( TestName, T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE2_6( TestName, name, description, T, T1, T2, T3, T4, T5, T6 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( TestName, T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T4 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T5 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T6 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( TestName, T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE2_7( TestName, name, description, T, T1, T2, T3, T4, T5, T6, T7 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( TestName, T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T4 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T5 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T6 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T7 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( TestName, T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE2_8( TestName, name, description, T, T1, T2, T3, T4, T5, T6, T7, T8 ) \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DECL( TestName, T ) \
    INTERNAL_CATCH_TESTCASE( name, description ) \
    { \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T1 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T2 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T3 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T4 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T5 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T6 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T7 ) \
        INTERNAL_CATCH_TEMPLATE_TEST_CASE_SECTION( TestName, T8 ) \
    } \
    INTERNAL_CATCH_TEST_CASE_TEMPLATE_DEFN( TestName, T )

#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_1(name, description, T, T1) INTERNAL_CATCH_TEMPLATE_TEST_CASE2_1(INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ ), name, description, T, T1)
#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_2(name, description, T, T1, T2) INTERNAL_CATCH_TEMPLATE_TEST_CASE2_2(INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ ), name, description, T, T1, T2)
#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_3(name, description, T, T1, T2, T3) INTERNAL_CATCH_TEMPLATE_TEST_CASE2_3(INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ ), name, description, T, T1, T2, T3)
#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_4(name, description, T, T1, T2, T3, T4) INTERNAL_CATCH_TEMPLATE_TEST_CASE2_4(INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ ), name, description, T, T1, T2, T3, T4)
#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_5(name, description, T, T1, T2, T3, T4, T5) INTERNAL_CATCH_TEMPLATE_TEST_CASE2_5(INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ ), name, description, T, T1, T2, T3, T4, T5)
#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_6(name, description, T, T1, T2, T3, T4, T5, T6) INTERNAL_CATCH_TEMPLATE_TEST_CASE2_6(INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ ), name, description, T, T1, T2, T3, T4, T5, T6)
#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_7(name, description, T, T1, T2, T3, T4, T5, T6, T7) INTERNAL_CATCH_TEMPLATE_TEST_CASE2_7(INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ ), name, description, T, T1, T2, T3, T4, T5, T6, T7)
#define INTERNAL_CATCH_TEMPLATE_TEST_CASE_8(name, description, T, T1, T2, T3, T4, T5, T6, T7, T8) INTERNAL_CATCH_TEMPLATE_TEST_CASE2_8(INTERNAL_CATCH_UNIQUE_NAME( ____C_A_T_C_H____T_E_M_P_L_A_TE____T_E_S_T____ ), name, description, T, T1, T2, T3, T4, T5, T6, T7, T8)

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

// Explicit STRING macros to work around the different behaviours of the frameworks when comparing two char* or wchar_t*
namespace bst_test_detail
{
    inline std::string as_string(const std::string& str) { return str; }
    inline std::wstring as_string(const std::wstring& str) { return str; }
    inline std::string as_string(const char* str) { return{ str }; }
    inline std::wstring as_string(const wchar_t* str) { return{ str }; }
}
#define BST_CHECK_STRING_EQUAL(expected, actual) CATCH_CHECK(::bst_test_detail::as_string(expected) == actual)
#define BST_CHECK_STRING_NE(expected, actual) CATCH_CHECK(::bst_test_detail::as_string(expected) != actual)
#define BST_REQUIRE_STRING_EQUAL(expected, actual) CATCH_REQUIRE(::bst_test_detail::as_string(expected) == actual)
#define BST_REQUIRE_STRING_NE(expected, actual) CATCH_REQUIRE(::bst_test_detail::as_string(expected) != actual)

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

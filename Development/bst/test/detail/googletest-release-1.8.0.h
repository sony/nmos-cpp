#ifndef BST_TEST_DETAIL_GOOGLETEST_RELEASE_1_8_0_H
#define BST_TEST_DETAIL_GOOGLETEST_RELEASE_1_8_0_H

// Use Google Test (1.8.0) to emulate some of the Boost Test library (mapped to BST_*)

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
              "", ::testing::internal::CodeLocation(__FILE__, __LINE__), \
              GTEST_STRINGIZE(CaseName), #TestName, 0); \
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

// Explicit STRING macros to work around the different behaviours of the frameworks when comparing two char* or wchar_t*
namespace bst_test_detail
{
    inline std::string as_string(const std::string& str) { return str; }
    inline std::wstring as_string(const std::wstring& str) { return str; }
    inline std::string as_string(const char* str) { return{ str }; }
    inline std::wstring as_string(const wchar_t* str) { return{ str }; }
}
#define BST_CHECK_STRING_EQUAL(expected, actual) EXPECT_EQ(::bst_test_detail::as_string(expected), actual)
#define BST_CHECK_STRING_NE(expected, actual) EXPECT_NE(::bst_test_detail::as_string(expected), actual)
#define BST_REQUIRE_STRING_EQUAL(expected, actual) ASSERT_EQ(::bst_test_detail::as_string(expected), actual)
#define BST_REQUIRE_STRING_NE(expected, actual) ASSERT_NE(::bst_test_detail::as_string(expected), actual)

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

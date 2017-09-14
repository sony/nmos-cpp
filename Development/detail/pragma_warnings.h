#ifndef DETAIL_PRAGMA_WARNINGS_H
#define DETAIL_PRAGMA_WARNINGS_H

// Use compiler-specific pragma mechanisms that, unlike #pragma, can be used inside macros

#if defined(_MSC_VER)

#define PRAGMA_WARNING_PUSH __pragma(warning(push))
#define PRAGMA_WARNING_POP __pragma(warning(pop))

#elif defined(__clang__)

#define PRAGMA_WARNING_PUSH _Pragma("clang diagnostic push")
#define PRAGMA_WARNING_POP _Pragma("clang diagnostic pop")

#elif defined(__GNUC__)

#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 406
#define PRAGMA_WARNING_PUSH _Pragma("GCC diagnostic push")
#define PRAGMA_WARNING_POP _Pragma("GCC diagnostic pop")
#else
#define PRAGMA_WARNING_PUSH
#define PRAGMA_WARNING_POP
#endif

#else

#define PRAGMA_WARNING_PUSH
#define PRAGMA_WARNING_POP

#endif

// Some specific warnings that shouldn't be disabled everywhere,
// but are likely to be worth disabling in some cases

#if defined(_MSC_VER)
#define PRAGMA_WARNING_DISABLE_FORCE_TO_BOOL \
__pragma(warning(disable:4800)) // forcing value to bool
#else
#define PRAGMA_WARNING_DISABLE_FORCE_TO_BOOL
#endif

#if defined(_MSC_VER)
#define PRAGMA_WARNING_DISABLE_DESTRUCTOR_NEVER_RETURNS \
__pragma(warning(disable:4722)) // destructor never returns
#else
#define PRAGMA_WARNING_DISABLE_DESTRUCTOR_NEVER_RETURNS
#endif

#if defined(_MSC_VER)
#define PRAGMA_WARNING_DISABLE_UNREACHABLE_CODE \
__pragma(warning(disable:4702)) // unreachable code
#else
#define PRAGMA_WARNING_DISABLE_UNREACHABLE_CODE
#endif

#if defined(_MSC_VER)
#define PRAGMA_WARNING_DISABLE_CONDITIONAL_EXPRESSION_IS_CONSTANT \
__pragma(warning(disable:4127)) // conditional expression is constant
#else
#define PRAGMA_WARNING_DISABLE_CONDITIONAL_EXPRESSION_IS_CONSTANT
#endif

#endif

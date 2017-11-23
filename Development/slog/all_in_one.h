#ifndef SLOG_ALL_IN_ONE_H
#define SLOG_ALL_IN_ONE_H
////////////////////////////////////////////////////////////////////////////////////////////
// AUTO-GENERATED AMALGAMATED HEADER
// Generated at r347; to be truly free of dependencies, define SLOG_DETAIL_PROVIDES_UNIQUE_PTR_BASED_OPTIONAL and probably SLOG_STATIC
////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/config.h"
#ifndef SLOG_CONFIG_H
#define SLOG_CONFIG_H

////////////////////////////////////////////////////////////////////////////////////////////
// Compile-time severity and logging gateway configuration

// SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT - whether the configuration is varied in this compilation unit or not
#if   !defined(SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT) && !defined(SLOG_DONT_PROVIDE_CONFIG_PER_TRANSLATION_UNIT)
#define        SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT
#elif  defined(SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT) &&  defined(SLOG_DONT_PROVIDE_CONFIG_PER_TRANSLATION_UNIT)
#error "SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT and SLOG_DONT_PROVIDE_CONFIG_PER_TRANSLATION_UNIT must not both be specified"
#endif

// SLOG_PROVIDES_SEVERITIES - whether names for typical severity levels (SLOG_FATAL, SLOG_INFO, etc.) are provided
// Note: Could be varied per compilation unit if necessary (so long as SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT is also defined)
#if   !defined(SLOG_PROVIDES_SEVERITIES) && !defined(SLOG_DONT_PROVIDE_SEVERITIES)
#define        SLOG_PROVIDES_SEVERITIES
#elif  defined(SLOG_PROVIDES_SEVERITIES) &&  defined(SLOG_DONT_PROVIDE_SEVERITIES)
#error "SLOG_PROVIDES_SEVERITIES and SLOG_DONT_PROVIDE_SEVERITIES must not both be specified"
#endif

// SLOG_LOGGING_SEVERITY - compile-time control of logging loquacity, fulfilling the same purpose as GOOGLE_STRIP_LOG in google-glog
// Use never_log_severity to strip all logging at compile-time
// Note: Could be varied per compilation unit if necessary (so long as SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT is also defined)
#ifndef SLOG_LOGGING_SEVERITY
#ifdef SLOG_PROVIDES_SEVERITIES
#define SLOG_DETAIL_INCLUDE_SEVERITIES_HEADER
#define SLOG_LOGGING_SEVERITY slog::severities::info
#else
#define SLOG_LOGGING_SEVERITY slog::nil_severity
#endif
#endif

// SLOG_TERMINATING_SEVERITY - compile-time control of which levels terminate the application
// Use never_log_severity to be certain logging won't terminate the application
// Note: Could be varied per compilation unit if necessary (so long as SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT is also defined)
#ifndef SLOG_TERMINATING_SEVERITY
#ifdef SLOG_PROVIDES_SEVERITIES
#define SLOG_DETAIL_INCLUDE_SEVERITIES_HEADER
#define SLOG_TERMINATING_SEVERITY slog::severities::fatal
#else
#define SLOG_TERMINATING_SEVERITY slog::max_severity
#endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Compile-time logging statement configuration

// SLOG_CURRENT_FILE - name of the current source file for log messages; should be a string literal
// Note: Could be varied per compilation unit if necessary
#ifndef SLOG_CURRENT_FILE
#define SLOG_CURRENT_FILE __FILE__
#endif

// SLOG_CURRENT_LINE - line number in the current source file for log messages; other than __LINE__, only a counter or 0 to disable tracking make much sense!
// Note: Could be varied per compilation unit if necessary
#ifndef SLOG_CURRENT_LINE
#define SLOG_CURRENT_LINE __LINE__
#endif

// SLOG_CURRENT_FUNCTION - name of the enclosing function as a string literal for log messages; should be a string literal
// Depending on the platform, __PRETTY_FUNCTION__ or __FUNCSIG__ might be an alternative to __FUNCTION__
// Note: Could be varied per compilation unit if necessary
#ifndef SLOG_CURRENT_FUNCTION
#define SLOG_CURRENT_FUNCTION __FUNCTION__
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Additional configuration for the macro-based API

// SLOG_PROVIDES_ABBREVIATED_LOGGING - whether LOG macros are provided as synonyms for SLOG macros, for some google-glog (and g2log/g3log, easylogging++) compatibility
// Note: Could be varied per compilation unit if necessary
#if   !defined(SLOG_DONT_PROVIDE_ABBREVIATED_LOGGING) && !defined(SLOG_PROVIDES_ABBREVIATED_LOGGING)
#define        SLOG_DONT_PROVIDE_ABBREVIATED_LOGGING
#elif  defined(SLOG_DONT_PROVIDE_ABBREVIATED_LOGGING) &&  defined(SLOG_PROVIDES_ABBREVIATED_LOGGING)
#error "SLOG_DONT_PROVIDE_ABBREVIATED_LOGGING and SLOG_PROVIDES_ABBREVIATED_LOGGING must not both be specified"
#endif

// SLOG_PROVIDES_ABBREVIATED_SEVERITIES - whether abbreviated names for severity levels are provided, fulfilling the same purpose as GLOG_NO_ABBREVIATED_SEVERITIES in google-glog
// Note: Could be varied per compilation unit if necessary
#if   !defined(SLOG_DONT_PROVIDE_ABBREVIATED_SEVERITIES) && !defined(SLOG_PROVIDES_ABBREVIATED_SEVERITIES)
// Default is to provide them if and only if typical severity levels and abbreviated logging are both enabled
#if defined(SLOG_DONT_PROVIDE_SEVERITIES) || defined(SLOG_DONT_PROVIDE_ABBREVIATED_LOGGING)
#define        SLOG_DONT_PROVIDE_ABBREVIATED_SEVERITIES
#else
#define        SLOG_PROVIDES_ABBREVIATED_SEVERITIES
#endif
#elif  defined(SLOG_DONT_PROVIDE_ABBREVIATED_SEVERITIES) &&  defined(SLOG_PROVIDES_ABBREVIATED_SEVERITIES)
#error "SLOG_DONT_PROVIDE_ABBREVIATED_SEVERITIES and SLOG_PROVIDES_ABBREVIATED_SEVERITIES must not both be specified"
#endif

// SLOG_PROVIDES_THREAD_LOCAL_LOGGING - whether SLOG macros use the thread-local API
// Note: Could be varied per compilation unit if necessary
#if   !defined(SLOG_PROVIDES_THREAD_LOCAL_LOGGING) && !defined(SLOG_DONT_PROVIDE_THREAD_LOCAL_LOGGING)
#define        SLOG_PROVIDES_THREAD_LOCAL_LOGGING
#elif  defined(SLOG_PROVIDES_THREAD_LOCAL_LOGGING) &&  defined(SLOG_DONT_PROVIDE_THREAD_LOCAL_LOGGING)
#error "SLOG_PROVIDES_THREAD_LOCAL_LOGGING and SLOG_DONT_PROVIDE_THREAD_LOCAL_LOGGING must not both be specified"
#endif

// SLOG_PROVIDES_ATOMIC_SLOG_COUNT - whether SLOG_COUNT should use atomic increment and thus guarantee to be accurate in multi-threaded apps
// Note: Could be varied per compilation unit if necessary
#if   !defined(SLOG_PROVIDES_ATOMIC_SLOG_COUNT) && !defined(SLOG_DONT_PROVIDE_ATOMIC_SLOG_COUNT)
#define        SLOG_PROVIDES_ATOMIC_SLOG_COUNT
#elif  defined(SLOG_PROVIDES_ATOMIC_SLOG_COUNT) &&  defined(SLOG_DONT_PROVIDE_ATOMIC_SLOG_COUNT)
#error "SLOG_PROVIDES_ATOMIC_SLOG_COUNT and SLOG_DONT_PROVIDE_ATOMIC_SLOG_COUNT must not both be specified"
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Other configuration and helpful preprocessor symbols

// SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG - whether the (global, for now) logging gateway functions are presented in namespace slog
// Note: Could be varied per compilation unit if necessary
#if   !defined(SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG) && !defined(SLOG_DONT_PROVIDE_LOGGING_IN_NAMESPACE_SLOG)
#define        SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG
#elif  defined(SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG) &&  defined(SLOG_DONT_PROVIDE_LOGGING_IN_NAMESPACE_SLOG)
#error "SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG and SLOG_DONT_PROVIDE_LOGGING_IN_NAMESPACE_SLOG must not both be specified"
#endif

// SLOG_LOGGING_GATEWAY - default gateway used by the macro-based API
#ifndef SLOG_LOGGING_GATEWAY
#define SLOG_DETAIL_INCLUDE_LOGGING_GATEWAY_HEADER
#ifdef SLOG_PROVIDES_THREAD_LOCAL_LOGGING
#define SLOG_LOGGING_GATEWAY slog::this_thread::gate
#else
#define SLOG_LOGGING_GATEWAY slog::global::gate
#endif
#endif

// SLOG_ID - unique integer value within the translation unit
#define SLOG_ID __COUNTER__

// SLOG_FLF - short-hand for SLOG_CURRENT_FILE, SLOG_CURRENT_LINE, SLOG_CURRENT_FUNCTION
#define SLOG_FLF SLOG_CURRENT_FILE, SLOG_CURRENT_LINE, SLOG_CURRENT_FUNCTION

////////////////////////////////////////////////////////////////////////////////////////////
// Implementation details
//
// Note: Preprocessor symbols that begin SLOG_DETAIL are private implementation detail
// which users should not rely upon. All other SLOG symbols are public, part of the API.

// ref-qualifiers aren't supported before Visual Studio 2015 (VC140, exactly speaking, the Nov 2013 CTP)
// See https://blogs.msdn.microsoft.com/vcblog/2013/12/02/c1114-core-language-features-in-vs-2013-and-the-nov-2013-ctp/
// As far as I can tell support was introduced in Clang 2.9 and GCC 4.8.1
// See https://akrzemi1.wordpress.com/2014/06/02/ref-qualifiers/
#ifndef SLOG_DETAIL_NO_REF_QUALIFIERS
#if defined(_MSC_VER) && _MSC_VER < 1900
#define SLOG_DETAIL_NO_REF_QUALIFIERS
#endif
#endif

// Conditional noexcept isn't supported before Visual Studio 2015 (VC140)
// As far as I can tell support was introduced in Clang 3.0 and GCC 4.8.1
#ifndef SLOG_DETAIL_NOEXCEPT_IF_NOEXCEPT
#if defined(_MSC_VER) && _MSC_VER < 1900
#define SLOG_DETAIL_NOEXCEPT_IF_NOEXCEPT(expr)
#else
#define SLOG_DETAIL_NOEXCEPT_IF_NOEXCEPT(expr) noexcept(noexcept(expr))
#endif
#endif

// SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API - whether SLOG macros use the template-based API internally
// Be aware, there are a small number of inconsistencies in behaviour depending on whether this macro is defined or not.
// Note: Could be varied per compilation unit if necessary
#if   !defined(SLOG_DETAIL_DONT_PROVIDE_TEMPLATE_BASED_MACRO_API) && !defined(SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API)
#define        SLOG_DETAIL_DONT_PROVIDE_TEMPLATE_BASED_MACRO_API
#elif  defined(SLOG_DETAIL_DONT_PROVIDE_TEMPLATE_BASED_MACRO_API) &&  defined(SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API)
#error "SLOG_DETAIL_DONT_PROVIDE_TEMPLATE_BASED_MACRO_API and SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API must not both be specified"
#endif

// SLOG_DETAIL_BEGIN_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT - introduce an anonymous namespace to get translation-unit specific API
#ifdef SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT
#define SLOG_DETAIL_BEGIN_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT namespace {
#define SLOG_DETAIL_END_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT }
#else
#define SLOG_DETAIL_BEGIN_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT
#define SLOG_DETAIL_END_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT
#endif

// SLOG_DETAIL_DO_TERMINATE - std::terminate
// Note: This symbol is to provide some 'indirection' for unit-testing (so long as SLOG_PROVIDES_CONFIG_PER_TRANSLATION_UNIT is also defined)
#ifndef SLOG_DETAIL_DO_TERMINATE
#define SLOG_DETAIL_DO_TERMINATE std::terminate
#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/severity_type.h"
#ifndef SLOG_SEVERITY_TYPE_H
#define SLOG_SEVERITY_TYPE_H

#include <climits>

// Severity/verbosity levels

namespace slog
{
    typedef int severity;

    // Special values outside the range of severity/verbosity levels
    const severity never_log_severity = INT_MAX;
    const severity reset_log_severity = INT_MIN;
    // Note: not using std::numeric_limits because max/min aren't constexpr with e.g. Visual Studio 2012

    // Full range of severity/verbosity levels
    // Levels in the range (nil_severity, max_severity] are severe to some degree
    // Levels in the range [max_verbosity, nil_severity) are verbose to some degree
    // The nil_severity level is neither severe nor verbose!
    const severity max_severity = never_log_severity - 1;
    const severity nil_severity = 0;
    const severity max_verbosity = reset_log_severity + 1;

    // Typical severity/verbosity levels are provided by slog::severities
    // e.g. fatal, error, warning, info
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/log_message.h"
#ifndef SLOG_LOG_MESSAGE_H
#define SLOG_LOG_MESSAGE_H

#include <sstream>
#ifdef __GLIBCXX__
#include <type_traits> // for std::is_move_constructible, etc.
#endif
// Amalgamated: #include "slog/severity_type.h"

namespace slog
{
    namespace detail
    {
      void copyfmtstate(std::ostream& target, const std::ostream& src);

#if defined(__GLIBCXX__) && defined(__GNUC__) && (__GNUC__ < 5) && !defined(__clang__)
      // workaround for gcc 4.9 bug see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64385
      const bool ostringstream_is_move_constructible = std::is_move_constructible< std::ostringstream >::value;
#endif
    }

    // To keep logging lightweight at the front end, log_message is non-copyable (but cf. copyable_log_message)
    // log_message contains a set of attributes that are widely used and can be only be captured, and captured cheaply,
    // at the log statement itself; others such as thread id and timestamp can be captured in the log handler
    class log_message
    {
    public:
        // The usual log_message constructor is the one called via the SLOG macros, which typically use
        // the well-known __FILE__, __LINE__, __FUNCTION__ macros and a severity level
        log_message(const char* file, int line, const char* function, severity level)
            : file_(file)
            , line_(line)
            , function_(function)
            , level_(level)
            , stream_(std::ios_base::ate)
        {}

        // This constructor adds an initializing string for the message stream
        log_message(const char* file, int line, const char* function, severity level, const std::string& str)
            : file_(file)
            , line_(line)
            , function_(function)
            , level_(level)
            , stream_(str, std::ios_base::ate)
        {}

        // This constructor enables the implementation of a conversion operator in copyable_log_message
        // to allow passing copyable_log_message to functions taking log_message
        log_message(const char* file, int line, const char* function, severity level, const std::string& str, const std::ostream& stream)
            : file_(file)
            , line_(line)
            , function_(function)
            , level_(level)
            , stream_(str, std::ios_base::ate)
        {
            detail::copyfmtstate(stream_, stream);
        }

        // Accessors
        const char* file() const { return file_; }
        int line() const { return line_; }
        const char* function() const { return function_; }
        severity level() const { return level_; }
        // The message stream is the only publicly mutable member is the message stream
        std::ostream& stream() { return stream_; }
        const std::ostream& stream() const { return stream_; }
        std::string str() const { return stream_.str(); }

        // Trivial destructor
        ~log_message() {}

    private:
        const char* file_;
        int line_;
        const char* function_;
        severity level_;
        std::ostringstream stream_;

        // Non-copyable
        log_message(const log_message&);
        const log_message& operator=(const log_message&);
    public:
        // Movable
#ifndef __GLIBCXX__
        log_message(log_message&& src) // = default;
            : file_(src.file_)
            , line_(src.line_)
            , function_(src.function_)
            , level_(src.level_)
            , stream_(std::move(src.stream_))
        {}
#else
    private:
        struct move_enabler {};

        template <typename LogMessage>
        log_message(LogMessage&& src, typename std::enable_if<std::is_same<log_message, LogMessage>::value && std::is_move_constructible< std::ostringstream >::value, move_enabler>::type)
            : file_(src.file_)
            , line_(src.line_)
            , function_(src.function_)
            , level_(src.level_)
            , stream_(std::move(src.stream_))
        {}

        // Workaround for missing move support for streams in libstdc++ before the version that will ship with GCC 5
        // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54316

        template <typename LogMessage>
        log_message(LogMessage&& src, typename std::enable_if<std::is_same<log_message, LogMessage>::value && !std::is_move_constructible< std::ostringstream >::value, move_enabler>::type)
            : file_(src.file_)
            , line_(src.line_)
            , function_(src.function_)
            , level_(src.level_)
            , stream_(src.stream_.str(), std::ios_base::ate)
        {
            detail::copyfmtstate(stream_, src.stream_);
        }

    public:
        log_message(log_message&& src)
            : log_message(std::move(src), move_enabler())
        {
        }
#endif
    };

    namespace detail
    {
        inline void copyfmtstate(std::ostream& target, const std::ostream& src)
        {
            // Perhaps should do target.seekp(const_cast<std::ostream&>(src).tellp());
            // Copying stream state allows use to be made of the internal extensible array
            target.copyfmt(src);
            target.clear(src.rdstate());
        }
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/base_gate.h"
#ifndef SLOG_BASE_GATE_H
#define SLOG_BASE_GATE_H

// Amalgamated: #include "slog/log_message.h" // or we could forward declare it
// Amalgamated: #include "slog/severity_type.h"

namespace slog
{
    // base_gate is an abstract interface for a logging gateway, which therefore serves
    // as a reasonable base class when polymorphism is required, e.g. to enable runtime
    // selection of the logging gateway, or to allow separate compilation of modules
    class base_gate
    {
    public:
        virtual ~base_gate() {}
        virtual bool pertinent(slog::severity level) const = 0;
        virtual void log(const slog::log_message& message) const = 0;
    };
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/copyable_log_message.h"
#ifndef SLOG_COPYABLE_LOG_MESSAGE_H
#define SLOG_COPYABLE_LOG_MESSAGE_H

// Amalgamated: #include "slog/log_message.h"

namespace slog
{
    // A copyable log_message can be passed out of the scope of the logging statement
    // e.g. sent to another thread or stored in a container
    class copyable_log_message
    {
    public:
        // Converting constructor fulfills the primary purpose of saving a log_message
        // Note: No harm in not being explicit?
        copyable_log_message(const log_message& message)
            : file_(message.file())
            , line_(message.line())
            , function_(message.function())
            , level_(message.level())
            , stream_(message.str(), std::ios_base::ate)
        {
            detail::copyfmtstate(stream_, message.stream());
        }

        // This constructor provides compatibility with log_message construction
        copyable_log_message(const std::string& file, int line, const std::string& function, severity level)
            : file_(file)
            , line_(line)
            , function_(function)
            , level_(level)
            , stream_(std::ios_base::ate)
        {}

        // This constructor provides compatibility with log_message construction
        copyable_log_message(const std::string& file, int line, const std::string& function, severity level, const std::string& str)
            : file_(file)
            , line_(line)
            , function_(function)
            , level_(level)
            , stream_(str, std::ios_base::ate)
        {}

        // This constructor provides compatibility with log_message construction
        copyable_log_message(const std::string& file, int line, const std::string& function, severity level, const std::string& str, const std::ostream& stream)
            : file_(file)
            , line_(line)
            , function_(function)
            , level_(level)
            , stream_(str, std::ios_base::ate)
        {
            detail::copyfmtstate(stream_, stream);
        }

        // A default constructor seems like a good idea, even though it constructs a barely useful instance
        copyable_log_message()
            : file_()
            , line_()
            , function_()
            , level_(nil_severity)
            , stream_(std::ios_base::ate)
        {}

        // Likewise a constructor simply taking an initializing string for the message stream
        explicit copyable_log_message(const std::string& str)
            : file_()
            , line_()
            , function_()
            , level_(nil_severity)
            , stream_(str, std::ios_base::ate)
        {}

        // Trivial destructor
        ~copyable_log_message() {}

        // Copy constructor (looks awfully like the log_message converting constructor, doesn't it?)
        copyable_log_message(const copyable_log_message& message)
            : file_(message.file())
            , line_(message.line())
            , function_(message.function())
            , level_(message.level())
            , stream_(message.str(), std::ios_base::ate)
        {
            detail::copyfmtstate(stream_, message.stream());
        }

        // The copy assignment operator is provided for consistency with the copy constructor (rule of 3 or so)
        copyable_log_message& operator=(const copyable_log_message& message)
        {
            file_ = message.file();
            line_ = message.line();
            function_ = message.function();
            level_ = message.level();
            stream_.str(message.str());
            detail::copyfmtstate(stream_, message.stream());
            return *this;
        }

        // Accessors (identical to log_message)
        const char* file() const { return file_.c_str(); }
        int line() const { return line_; }
        const char* function() const { return function_.c_str(); }
        severity level() const { return level_; }
        std::ostream& stream() { return stream_; }
        const std::ostream& stream() const { return stream_; }
        std::string str() const { return stream_.str(); }

        // Conversion operator to allow passing copyable_log_message to functions taking log_message
        // Note: The log_message is only a shallow copy, valid for the lifetime of this copyable_log_message!
        operator log_message() const
        {
            return log_message(file(), line(), function(), level(), str(), stream());
        }

    protected:
        std::string file_;
        int line_;
        std::string function_;
        severity level_;
        std::ostringstream stream_;
    };
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "bst/function.h"
#ifndef BST_FUNCTION_H
#define BST_FUNCTION_H

// Provide bst::bind, bst::placeholders::_1, etc. using either std:: or boost:: symbols

// Note: Same condition as bst/thread.h because we want consistent use of function/thread/chrono
#ifndef BST_THREAD_BOOST

#include <functional>
namespace bst_functional = std;
namespace bst_placeholders = std::placeholders;

#else

#include <boost/bind.hpp>
#include <boost/function.hpp>
namespace bst_functional = std;
namespace bst_placeholders // can't alias the global namespace!
{
    using ::_1;
    using ::_2;
    using ::_3;
    using ::_4;
    using ::_5;
    using ::_6;
    using ::_7;
    using ::_8;
    using ::_9;
}

#endif

namespace bst
{
    using bst_functional::bind;
    using bst_functional::ref;
    using bst_functional::cref;
    using bst_functional::reference_wrapper;

    namespace placeholders
    {
	    // could replace using-directive with using-declarations for symbols
		using namespace bst_placeholders;
    }

    using bst_functional::function;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/iomanip.h"
#ifndef SLOG_IOMANIP_H
#define SLOG_IOMANIP_H

#include <iosfwd>
// Amalgamated: #include "bst/function.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Utility classes and functions to provide manipulators created from e.g. lambda functions
namespace slog
{
    // Use the free functions imanip, omanip, ios_manip and base_manip to construct manipulators.

    // Instantiations of the manip_function class template are just tagged function object types,
    // but without the conversion to bool or other baggage of std::function.
    // Note: These types are not really to be used directly.
    template <typename Manipulated>
    struct manip_function
    {
        typedef Manipulated& argument_type;
        typedef Manipulated& result_type;

#if defined(_MSC_VER) && _MSC_VER < 1900
        // Visual Studio 2013 (VC140) does not implement the resolution to LWG 2420
        // See http://cplusplus.github.io/LWG/lwg-defects.html#2420
        // "function<void(ArgTypes...)> does not discard the return value of the target object"
        // And in another twist, some VS2013 bind expressions don't have a const operator() so,
        // in order that the captured copy of f can definitely be called, mark the lambda mutable
        // See http://connect.microsoft.com/VisualStudio/feedbackdetail/view/981289/std-bind-operator-is-not-const-in-some-cases-where-it-should-be
        template <typename F>
        explicit manip_function(F f)
            : function([f](argument_type arg) mutable { f(arg); })
        {}
#else
        // The target callable, f, is required to be copyable.
        template <typename F>
        explicit manip_function(F&& f)
            : function(std::forward<F>(f))
        {}
#endif

        result_type operator()(argument_type arg) const { function(arg); return arg; }

    private:
        bst::function<void(argument_type)> function;
    };

    typedef manip_function<std::ios_base> base_manip_function;
    typedef manip_function<std::ios> ios_manip_function;
    typedef manip_function<std::ostream> omanip_function;
    typedef manip_function<std::istream> imanip_function;

    typedef manip_function<std::wios> wios_manip_function;
    typedef manip_function<std::wostream> womanip_function;
    typedef manip_function<std::wistream> wimanip_function;

    // Functions for constructing manipulators

    template <typename F>
    inline base_manip_function base_manip(F&& f)
    {
        return base_manip_function(std::forward<F>(f));
    }

    template <typename F>
    inline ios_manip_function ios_manip(F&& f)
    {
        return ios_manip_function(std::forward<F>(f));
    }

    template <typename F>
    inline omanip_function omanip(F&& f)
    {
        return omanip_function(std::forward<F>(f));
    }

    template <typename F>
    inline imanip_function imanip(F&& f)
    {
        return imanip_function(std::forward<F>(f));
    }

    template <typename F>
    inline wios_manip_function wios_manip(F&& f)
    {
        return wios_manip_function(std::forward<F>(f));
    }

    template <typename F>
    inline womanip_function womanip(F&& f)
    {
        return womanip_function(std::forward<F>(f));
    }

    template <typename F>
    inline wimanip_function wimanip(F&& f)
    {
        return wimanip_function(std::forward<F>(f));
    }

    // Stream insertion operators for the relevant manipulators

    template <typename CharT, typename TraitsT>
    inline std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& s, const base_manip_function& f)
    {
        f(s); return s;
    }

    template <typename CharT, typename TraitsT>
    inline std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& s, const manip_function<std::basic_ios<CharT, TraitsT>>& f)
    {
        f(s); return s;
    }

    template <typename CharT, typename TraitsT>
    inline std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& s, const manip_function<std::basic_ostream<CharT, TraitsT>>& f)
    {
        return f(s);
    }

    // Stream extraction operators for the relevant manipulators

    template <typename CharT, typename TraitsT>
    inline std::basic_istream<CharT, TraitsT>& operator>>(std::basic_istream<CharT, TraitsT>& s, const base_manip_function& f)
    {
        f(s); return s;
    }

    template <typename CharT, typename TraitsT>
    inline std::basic_istream<CharT, TraitsT>& operator>>(std::basic_istream<CharT, TraitsT>& s, const manip_function<std::basic_ios<CharT, TraitsT>>& f)
    {
        f(s); return s;
    }

    template <typename CharT, typename TraitsT>
    inline std::basic_istream<CharT, TraitsT>& operator>>(std::basic_istream<CharT, TraitsT>& s, const manip_function<std::basic_istream<CharT, TraitsT>>& f)
    {
        return f(s);
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "detail/pragma_warnings.h"
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

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/detail/optional_log_message.h"
#ifndef SLOG_DETAIL_OPTIONAL_LOG_MESSAGE_H
#define SLOG_DETAIL_OPTIONAL_LOG_MESSAGE_H

// Amalgamated: #include "slog/config.h"

// No optional in std, no std::experimental::optional in VS2012, but somebody wanted to avoid Boost wherever possible...
// std::unique_ptr is slower because of the heap allocation, but it works...
#ifdef SLOG_DETAIL_PROVIDES_UNIQUE_PTR_BASED_OPTIONAL
#include <memory>
#else
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#endif

// Amalgamated: #include "slog/log_message.h"

namespace slog
{
    namespace detail
    {
#ifdef SLOG_DETAIL_PROVIDES_UNIQUE_PTR_BASED_OPTIONAL
        typedef std::unique_ptr<log_message> optional_log_message;
        inline optional_log_message make_optional_log_message(const char* file, int line, const char* function, severity level)
        {
            return optional_log_message(new log_message(file, line, function, level));
        }
#else
        typedef boost::optional<log_message> optional_log_message;
        // Taking arguments by const& is critical, because otherwise the returned boost::in_place_factory holds references to temporaries
        inline auto make_optional_log_message(const char* const& file, int const& line, const char* const& function, severity const& level)
            -> decltype(boost::in_place(file, line, function, level))
        {
            return boost::in_place(file, line, function, level);
        }
#endif
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/detail/uncaught_exceptions.h"
#ifndef SLOG_DETAIL_UNCAUGHT_EXCEPTIONS_H
#define SLOG_DETAIL_UNCAUGHT_EXCEPTIONS_H

// std::uncaught_exceptions has been proposed by Herb Sutter in
// http://isocpp.org/files/papers/n4259.pdf
// This implementation is derived from the code snippet by
// Andrei Alexandrescu presented in "Declarative Control Flow"
// at C++ and Beyond, Stuttgart, Germany, September 2014.

#if defined(__clang__) || defined(__GNUC__)
namespace __cxxabiv1
{
    struct __cxa_eh_globals;
    extern "C" __cxa_eh_globals* __cxa_get_globals() noexcept;
}

namespace slog
{
    namespace detail
    {
        inline int uncaught_exceptions()
        {
            // __cxa_get_globals returns a __cxa_eh_globals* (defined in unwind-cxx.h).
            // The offset below returns __cxa_eh_globals::uncaughtExceptions.
            return *reinterpret_cast<int*>(static_cast<char*>(static_cast<void*>(__cxxabiv1::__cxa_get_globals())) + sizeof(void*));
        }
    }
}
#elif defined(_MSC_VER) && _MSC_VER < 1900
// For Visual Studio 2005-2013 (VC80 to VC120) only.

struct _tiddata;
extern "C" _tiddata* _getptd();

namespace slog
{
    namespace detail
    {
        inline int uncaught_exceptions()
        {
            // _getptd() returns a _tiddata* (defined in mtdll.h).
            // The offset below returns _tiddata::_ProcessingThrow.
            return *reinterpret_cast<int*>(static_cast<char*>(static_cast<void*>(_getptd())) + sizeof(void*) * 28 + 4 * 8);
        }
    }
}
#elif defined(_MSC_VER)
// For Visual Studio 2015 (VC140) CTP 6 onwards.

extern "C" int* __processing_throw();

namespace slog
{
    namespace detail
    {
        inline int uncaught_exceptions()
        {
            return *__processing_throw();
        }
    }
}
#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/severities.h"
#ifndef SLOG_SEVERITIES_H
#define SLOG_SEVERITIES_H

// Amalgamated: #include "slog/severity_type.h"

// Typical severity/verbosity levels

namespace slog
{
    // Note: Additional or different values that meet the slog::severity criteria may also be used
    namespace severities
    {
        // Note: Which severity levels terminate the application is configured at compile-time
        const severity fatal = 40;
        const severity severe = 30;
        const severity error = 20;
        const severity warning = 10;
        const severity info = 0;
        const severity more_info = -10;
        //const severity debug = more_info;
        const severity too_much_info = -40;
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "bst/atomic.h"
#ifndef BST_ATOMIC_H
#define BST_ATOMIC_H

// Provide bst::array, using either std:: or boost:: symbols

#ifndef BST_ATOMIC_BOOST

#include <atomic>
namespace bst_atomic = std;

#else

#include <boost/atomic.hpp>
namespace bst_atomic = boost;

#endif

namespace bst
{
    using bst_atomic::atomic;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/detail/hit_count.h"
#ifndef SLOG_DETAIL_HIT_COUNT_H
#define SLOG_DETAIL_HIT_COUNT_H

// Amalgamated: #include "bst/atomic.h"

namespace slog
{
    namespace detail
    {
        template <typename tag>
        class hit_count
        {
        public:
            hit_count() : value(++static_value) {}
            const int value;

            static bst::atomic<int> static_value;

        private:
            // Non-copyable
            hit_count(const hit_count&);
            const hit_count& operator=(const hit_count&);
        public:
            // Movable
            hit_count(hit_count&& src) // = default;
                : value(src.value)
            {}
        };

        template <typename tag>
        bst::atomic<int> hit_count<tag>::static_value(0);

        // These tags may be useful beyond hit_count...
        namespace definition_tags
        {
            struct program;

            template <int id>
            struct program_c;

            namespace
            {
                struct translation_unit;

                template <int id>
                struct translation_unit_c;
            }
        }
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/log_statement_type.h"
#ifndef SLOG_LOG_STATEMENT_TYPE_H
#define SLOG_LOG_STATEMENT_TYPE_H

////////////////////////////////////////////////////////////////////////////////////////////
// Front-end API for logging with (almost) no macros in user code

#include <iosfwd>

// Amalgamated: #include "detail/pragma_warnings.h"
// Amalgamated: #include "slog/config.h"
// Amalgamated: #include "slog/severity_type.h"
// Amalgamated: #include "slog/iomanip.h"
// Amalgamated: #include "slog/detail/optional_log_message.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Logging statement interaction API - and core implementation when not disabled by compile-time filtering

namespace slog
{
    // Logging statement
    class log_statement
    {
    public:
        log_statement(severity level, const char* file, int line, const char* function, bool pert, const int& count = 0)
            : pert_(pert)
            , count_(count)
            , message_()
        {
            if (pert_) message_ = detail::make_optional_log_message(file, line, function, level);
        }

        // Insertion operators for stream-based logging

        // (1) Any value or manipulator that can be used directly with a std:::ostream
        template <typename T>
        auto operator<<(const T& value) -> decltype(std::declval<std::ostream&>() << value, std::declval<log_statement&>()) { if (pert_) message_->stream() << value; return *this; }
        // Note: Need the following for function templates like std::endl for which deduction fails
        log_statement& operator<<(std::ios& (*manip)(std::ios&)) { if (pert_) message_->stream() << manip; return *this; }
        log_statement& operator<<(std::ostream& (*manip)(std::ostream&)) { if (pert_) message_->stream() << manip; return *this; }

        // (2a) Additional manipulator support
        log_statement& operator<<(log_statement& (*manip)(log_statement&)) { if (pert_) return manip(*this); return *this; }
        // (2b) Additional manipulator function object support
        typedef slog::manip_function<log_statement> manip_function;
        log_statement& operator<<(const manip_function& manip) { if (pert_) return manip(*this); return *this; }

        // Accessors

        bool pertinent() const { return pert_; }

        int count() const { return count_; }

        log_message& message() { return *message_; }
        const log_message& message() const { return *message_; }

    protected:
        template <typename LoggingGateway>
        void log(LoggingGateway& gate) const { if (pert_) gate.log(*message_); }

    private:
        const bool pert_;
        const int& count_;
        detail::optional_log_message message_;

        // Non-copyable
        log_statement(const log_statement&);
        const log_statement& operator=(const log_statement&);
    };

    // Function for constructing manipulator function objects

    template <typename F>
    inline log_statement::manip_function log_manip(const F& f)
    {
        return log_statement::manip_function(f);
    }

    // Manipulators

    // The count manipulator inserts the hit count into the message stream.
    // Note: If the count is zero, this is not a counted logging statement!
    inline log_statement& count(log_statement& s) { return s << s.count(); }

    // The get_count manipulator just gets the hit count.
    inline log_statement::manip_function get_count(int& result)
    {
        return log_manip([&result](const log_statement& s){ result = s.count(); });
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// Logging statement core implementation when disabled by compile-time filtering

namespace slog
{
    // Disabled logging statement
    class nolog_statement
    {
    public:
        nolog_statement(severity level, const char* file, int line, const char* function, bool pert, const int& nocount = 0)
        {}

        // Insertion operators for stream-based logging

        // Any value or manipulator that can be used with a log_statement
        template <typename T>
        auto operator<<(const T& value) const -> decltype(std::declval<log_statement&>() << value, std::declval<const nolog_statement&>()) { return *this; }
        // Note: Need the following for function templates like std::endl for which deduction fails
        const nolog_statement& operator<<(std::ios& (*manip)(std::ios&)) const { return *this; }
        const nolog_statement& operator<<(std::ostream& (*manip)(std::ostream&)) const { return *this; }

        // Accessors

        bool pertinent() const { return false; }

    protected:
        template <typename LoggingGateway>
        void log(LoggingGateway& gate) const {}

    private:
        // Non-copyable
        nolog_statement(const nolog_statement&);
        const nolog_statement& operator=(const nolog_statement&);
    };
}

////////////////////////////////////////////////////////////////////////////////////////////
// Logging statement - logging gateway interaction implementation

// Amalgamated: #include "slog/detail/uncaught_exceptions.h"

namespace slog
{
    namespace detail
    {
        // Logging statement with a logging gateway
        template <typename LoggingStatement, typename LoggingGateway>
        class logw : public LoggingStatement
        {
        public:
            typedef LoggingStatement log_statement;
            typedef LoggingGateway log_gate;

            using log_statement::pertinent;
            using log_statement::log;

            logw(log_gate& gate, severity level, const char* file, int line, const char* function, bool pert = true, const int& count = 0)
                : log_statement(level, file, line, function, pert && gate.pertinent(level), count)
                , uncaught_exceptions_(pertinent() ? uncaught_exceptions() : -1)
                , gate_(gate)
            {}

            ~logw()
            {
                // Log if message preparation didn't throw
                if (pertinent() && uncaught_exceptions_ == uncaught_exceptions())
                {
                    log(gate_);
                }
            }

            // Accessors

            log_gate& gate() { return gate_; }
            const log_gate& gate() const { return gate_; }

        private:
            const int uncaught_exceptions_;
            log_gate& gate_;

            // Non-copyable
            logw(const logw&);
            const logw& operator=(const logw&);
        };
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// Compile-time filtering of impertinent (irrelevant) log messages
// and compile-time control of which severity levels terminate the application

#include <exception> // explicitly included for std::terminate, as the default for SLOG_DETAIL_DO_TERMINATE

#ifdef SLOG_DETAIL_INCLUDE_SEVERITIES_HEADER
// Amalgamated: #include "slog/severities.h" // for definition of values for SLOG_LOGGING_SEVERITY and/or SLOG_TERMINATING_SEVERITY
#endif

namespace slog
{
    namespace detail
    {
        SLOG_DETAIL_BEGIN_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT

        template <severity level, typename enable = void>
        struct select_pertinent
            : std::false_type
        {
            typedef nolog_statement log_statement;
        };

        template <severity level>
        struct select_pertinent<level, typename std::enable_if<(SLOG_LOGGING_SEVERITY) <= level>::type>
            : std::true_type
        {
            typedef slog::log_statement log_statement; // GCC 4.9 gets confused without the qualification
        };

        template <severity level, typename enable = void>
        struct select_terminate
            : std::false_type
        {};

        template <severity level>
        struct select_terminate<level, typename std::enable_if<(SLOG_TERMINATING_SEVERITY) <= level>::type>
            : std::true_type
        {
            ~select_terminate()
                SLOG_DETAIL_NOEXCEPT_IF_NOEXCEPT(SLOG_DETAIL_DO_TERMINATE())
                ;
        };
PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_DESTRUCTOR_NEVER_RETURNS
        template <severity level>
        inline select_terminate<level, typename std::enable_if<(SLOG_TERMINATING_SEVERITY) <= level>::type>::~select_terminate()
            SLOG_DETAIL_NOEXCEPT_IF_NOEXCEPT(SLOG_DETAIL_DO_TERMINATE())
        {
            SLOG_DETAIL_DO_TERMINATE();
        }
PRAGMA_WARNING_POP

        SLOG_DETAIL_END_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// Logging statement implementation almost complete...

#if defined(_MSC_VER) && _MSC_VER < 1800
#define SLOG_DETAIL_NO_UNIFORM_INITIALIZATION_SYNTAX
// In this case, declare a public, umimplemented, move constructor so the log functions,
// which deduce the LoggingGateway type, work only because the compiler does copy elision
#endif

namespace slog
{
    namespace detail
    {

        SLOG_DETAIL_BEGIN_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT

        template <typename LoggingGateway, severity level>
        class log
            : private select_terminate<level> // first base class so its destructor is last
            , public logw<typename select_pertinent<level>::log_statement, LoggingGateway>
        {
        public:
            log(LoggingGateway& gate, const char* file, int line, const char* function)
                : log::logw(gate, level, file, line, function, select_pertinent<level>::value)
            {}
        private:
            // Non-copyable
            log(const log&);
            const log& operator=(const log&);
#ifdef SLOG_DETAIL_NO_UNIFORM_INITIALIZATION_SYNTAX
        public:
            log(log&&);
#endif
        };

        SLOG_DETAIL_END_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT
    }
}

// Amalgamated: #include "slog/detail/hit_count.h"

namespace slog
{
    namespace detail
    {
        namespace
        {
            // log_every_n must always be local to translation unit to get per-logging statement count
            template <typename LoggingGateway, severity level, int n, int id>
            class log_every_n
                : private select_terminate<level> // first base class so its destructor is last
                , private hit_count<definition_tags::translation_unit_c<id>> // second base class so hit_count::value is incremented in time
                , public logw<typename select_pertinent<level>::log_statement, LoggingGateway>
            {
            public:
                log_every_n(LoggingGateway& gate, const char* file, int line, const char* function)
                    : log_every_n::logw(gate, level, file, line, function, select_pertinent<level>::value && 0 == (log_every_n::hit_count::value - 1) % n, log_every_n::hit_count::value)
                {}
            private:
                // Non-copyable
                log_every_n(const log_every_n&);
                const log_every_n& operator=(const log_every_n&);
#ifdef SLOG_DETAIL_NO_UNIFORM_INITIALIZATION_SYNTAX
            public:
                log_every_n(log_every_n&&);
#endif
            };
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// Logging statement construction API

// Right. This is it. There's only so far I can go to provide a macro-free interface!
// Use SLOG_FLF and SLOG_ID defined by slog/config.h to drive the API, like so...
// slog::log<level>(gate, SLOG_FLF) is the basic logging front-end API
// slog::log_every_n<level, n, SLOG_ID>(gate, SLOG_FLF) is the intermittent logging front-end API

namespace slog
{
    namespace detail
    {
        namespace deduce_gate
        {
            SLOG_DETAIL_BEGIN_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT

            template <severity level, typename LoggingGateway>
            inline detail::log<LoggingGateway, level> log(LoggingGateway& gate, const char* file, int line, const char* function)
            {
#ifndef SLOG_DETAIL_NO_UNIFORM_INITIALIZATION_SYNTAX
                return {gate, file, line, function};
#else
                return detail::log<LoggingGateway, level>(gate, file, line, function);
#endif
            }

            SLOG_DETAIL_END_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT

            namespace
            {
                // log_every_n must always be local to translation unit to get per-logging statement count
                template <severity level, int n, int id, typename LoggingGateway>
                inline detail::log_every_n<LoggingGateway, level, n, id> log_every_n(LoggingGateway& gate, const char* file, int line, const char* function)
                {
#ifndef SLOG_DETAIL_NO_UNIFORM_INITIALIZATION_SYNTAX
                    return {gate, file, line, function};
#else
                    return detail::log_every_n<LoggingGateway, level, n, id>(gate, file, line, function);
#endif
                }
            }
        }
    }

    using detail::deduce_gate::log;
    using detail::deduce_gate::log_every_n;
}

////////////////////////////////////////////////////////////////////////////////////////////
// This API is not quite as flexible as the macro-based one...

// It always uses int for the hit count type (not SLOG_COUNT_TYPE) and it always uses an atomic hit count
#ifdef SLOG_DONT_PROVIDE_ATOMIC_SLOG_COUNT
#pragma message("SLOG_DONT_PROVIDE_ATOMIC_SLOG_COUNT is not effective with slog::log_every_n, etc.")
#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "detail/declspec.h"
#ifndef DETAIL_DECLSPEC_H
#define DETAIL_DECLSPEC_H

// The following attributes are used to control symbol visibility, particularly for
// dynamic link libraries. Typical usage is via further module-specific macros:
//  #ifdef MYMODULE_EXPORTS
//  #define MYMODULE_API DLLEXPORT_ATTRIBUTE
//  #elif defined( MYMODULE_STATIC )
//  #define MYMODULE_API
//  #else
//  #define MYMODULE_API DLLIMPORT_ATTRIBUTE
//  #endif

#if defined(_WIN32) || defined(__CYGWIN__)

// Microsoft Visual C++, Borland C++Builder both support these attributes
// and so does gcc (in addition to its own __attribute__((dllexport)) syntax)
#define DLLEXPORT_ATTRIBUTE __declspec(dllexport)
#define DLLIMPORT_ATTRIBUTE __declspec(dllimport)
#define DLLNOEXPORT_ATTRIBUTE

#elif defined(__GNUC__) && (__GNUC__ >= 4)

// gcc has supported the following visibility attributes since gcc 3.3 (2003)
// but has only supported the command line switches that make them useful
// (i.e. -fvisibility=hidden or -fvisibility-ms-compat) since gcc 4.0 (2005)
#define DLLEXPORT_ATTRIBUTE __attribute__ ((visibility("default")))
#define DLLIMPORT_ATTRIBUTE
#define DLLNOEXPORT_ATTRIBUTE __attribute__ ((visibility("hidden")))

#else

#define DLLEXPORT_ATTRIBUTE
#define DLLIMPORT_ATTRIBUTE
#define DLLNOEXPORT_ATTRIBUTE

#endif

// The following attribute can be used when declaring functions that always throw
// to prevent "warning: control reaches end of non-void function" from gcc
// and "warning C4715: not all control paths return a value" from Microsoft VC++
// when a control path ends by calling such a function.

#if defined(__GNUC__)
// gcc has supported noreturn since gcc 2.5 (1993)
#define NORETURN_ATTRIBUTE __attribute__ ((noreturn))
#elif defined(_WIN32) && defined(__BORLANDC__)
// Borland C++Builder has supported noreturn since at least 5.0 (2000)
#define NORETURN_ATTRIBUTE __declspec(noreturn)
#elif defined(_WIN32) && defined(_MSC_VER)
// Microsoft has supported noreturn since at least VC6 (1998)
#define NORETURN_ATTRIBUTE __declspec(noreturn)
#else
#define NORETURN_ATTRIBUTE
#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/declspec.h"
#ifndef SLOG_DECLSPEC_H
#define SLOG_DECLSPEC_H

// Amalgamated: #include "detail/declspec.h"

#ifdef SLOG_EXPORTS
#define SLOG_API DLLEXPORT_ATTRIBUTE
#elif defined( SLOG_STATIC )
#define SLOG_API
#else
#define SLOG_API DLLIMPORT_ATTRIBUTE
#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "bst/array.h"
#ifndef BST_ARRAY_H
#define BST_ARRAY_H

// Provide bst::array, using either std:: or boost:: symbols

#ifndef BST_ARRAY_BOOST

#include <array>
namespace bst_array = std;

#else

#include <boost/array.hpp>
namespace bst_array = boost;

#endif

namespace bst
{
    using bst_array::array;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/stringf.h"
#ifndef SLOG_STRINGF_H
#define SLOG_STRINGF_H

#include <cstdarg>
#include <cstdio>
#include <string>
// Amalgamated: #include "bst/array.h"

namespace slog
{
    namespace detail
    {
        template <typename Buffer>
        inline std::size_t vsnprintf_truncate(Buffer& buffer, const char* format, std::va_list args)
        {
#ifdef _WIN32
            const int length_not_including_terminating_null = ::vsnprintf_s(&buffer[0], buffer.size(), _TRUNCATE, format, args);
            return -1 != length_not_including_terminating_null ? length_not_including_terminating_null : buffer.size() - 1;
#else
            const int length_not_including_terminating_null = ::vsnprintf(&buffer[0], buffer.size(), format, args);
            return length_not_including_terminating_null < (int)buffer.size() ? length_not_including_terminating_null : buffer.size() - 1;
#endif
        }
    }

    inline std::string stringf(const char* format, ...)
    {
        bst::array<char, 1024> buffer; // could use vector for C++03 but this is more efficient
        std::va_list args;
        va_start(args, format);
        const std::size_t length_not_including_terminating_null = detail::vsnprintf_truncate(buffer, format, args);
        va_end(args);
        return std::string(&buffer[0], length_not_including_terminating_null);
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/log_handler_type.h"
#ifndef SLOG_LOG_HANDLER_TYPE_H
#define SLOG_LOG_HANDLER_TYPE_H

namespace slog
{
    class log_message;

    typedef void log_signature(const log_message&);
    typedef log_signature* log_handler;

    inline void nolog(const log_message&) {}
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/log_function_type.h"
#ifndef SLOG_LOG_FUNCTION_TYPE_H
#define SLOG_LOG_FUNCTION_TYPE_H

// Amalgamated: #include "bst/function.h"
// Amalgamated: #include "slog/log_handler_type.h"

namespace slog
{
    typedef bst::function<log_signature> log_function;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/global/log.h"
#ifndef SLOG_GLOBAL_LOG_H
#define SLOG_GLOBAL_LOG_H

// Amalgamated: #include "slog/declspec.h"

namespace slog
{
    class log_message;

    namespace global
    {
        // Call the current global log handler
        SLOG_API void log(const log_message& message);
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/global/log_function.h"
#ifndef SLOG_GLOBAL_LOG_FUNCTION_H
#define SLOG_GLOBAL_LOG_FUNCTION_H

// Amalgamated: #include "slog/declspec.h"
// Amalgamated: #include "slog/log_function_type.h"

namespace slog
{
    namespace global
    {
        // Thread-safe functions for when you want to use function objects to log.
        // Notes:
        // 1. See log_handler for more details.
        // 2. A log function must neither call these functions, nor call log().
        SLOG_API log_function set_log_function(log_function log);
        SLOG_API log_function get_log_function();
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/detail/static_const.h"
#ifndef SLOG_DETAIL_STATIC_CONST_H
#define SLOG_DETAIL_STATIC_CONST_H


namespace slog
{
    namespace detail
    {
        template <typename T>
        struct static_const
        {
            static const T value;
        };

        template <typename T>
        const T static_const<T>::value = T();
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/global/severity.h"
#ifndef SLOG_GLOBAL_SEVERITY_H
#define SLOG_GLOBAL_SEVERITY_H

// Amalgamated: #include "slog/declspec.h"
// Amalgamated: #include "slog/severity_type.h"

namespace slog
{
    namespace global
    {
        // Run-time control of logging loquacity, nil by default; thread-safe
        // Note: Use never_log_severity to not log even max_severity
        SLOG_API void set_log_severity(severity level);
        SLOG_API severity get_log_severity();

        inline bool pertinent(severity level) { return get_log_severity() <= level; }
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/global/log_gate.h"
#ifndef SLOG_GLOBAL_LOG_GATE_H
#define SLOG_GLOBAL_LOG_GATE_H

// Amalgamated: #include "slog/detail/static_const.h"
// Amalgamated: #include "slog/global/log.h"
// Amalgamated: #include "slog/global/severity.h"

namespace slog
{
    namespace global
    {
        // A logging gateway provides the interface through which log messages pass from a logging statement.
        // This one is an empty class, a monostate object to wrap the global logging core.
        struct log_gate
        {
            static void log(const log_message& message) { global::log(message); }
            static bool pertinent(severity level) { return global::pertinent(level); }
        };
        // Is it still sensible to provide a singleton instance?
        namespace
        {
            const log_gate& gate = detail::static_const<log_gate>::value;
        }
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/global/log_handler.h"
#ifndef SLOG_GLOBAL_LOG_HANDLER_H
#define SLOG_GLOBAL_LOG_HANDLER_H

// Amalgamated: #include "slog/declspec.h"
// Amalgamated: #include "slog/log_handler_type.h"

namespace slog
{
    namespace global
    {
        // Two functions, to replace the current log handler and return the previous one if any,
        // and to just get the current log handler. Setting a null pointer restores the default
        // log handler; to get no logging, use a valid function that does nothing.
        // These functions are thread-safe with respect to each other and to slog::log().
        SLOG_API log_handler set_log_handler(log_handler log);
        SLOG_API log_handler get_log_handler();
        // Notes:
        // 1. Using std::function here would be nicer but would require a mutex to be locked
        // in every call to log() since we can't make an atomic std::function as C++ stands.
        // See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3877.pdf
        // and http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4058.pdf.
        // 2. But if that's what you want see log_function!
        // 3. Or use thread-local logging, for which, see slog::this_thread functions.
        // 4. In any case, it's a bad idea to call log() from a log handler.
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/global/log_statement.h"
#ifndef SLOG_GLOBAL_LOG_STATEMENT_H
#define SLOG_GLOBAL_LOG_STATEMENT_H

// Amalgamated: #include "slog/log_statement_type.h"
// Amalgamated: #include "slog/global/log_gate.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Logging statement construction API

// Right. This is it. There's only so far I can go to provide a macro-free interface!
// Use SLOG_FLF and SLOG_ID defined by slog/config.h to drive the API, like so...
// slog::global::log<level>(SLOG_FLF) is the basic logging front-end API
// slog::global::log_every_n<level, n, SLOG_ID>(SLOG_FLF) is the intermittent logging front-end API

namespace slog
{
    namespace detail
    {
        namespace global_gate
        {
            SLOG_DETAIL_BEGIN_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT

            template <severity level>
            inline detail::log<const global::log_gate, level> log(const char* file, int line, const char* function)
            {
#ifndef SLOG_DETAIL_NO_UNIFORM_INITIALIZATION_SYNTAX
                return {global::gate, file, line, function};
#else
                return detail::log<const global::log_gate, level>(global::gate, file, line, function);
#endif
            }

            SLOG_DETAIL_END_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT

            namespace
            {
                // log_every_n must always be local to translation unit to get per-logging statement count
                template <severity level, int n, int id>
                inline detail::log_every_n<const global::log_gate, level, n, id> log_every_n(const char* file, int line, const char* function)
                {
#ifndef SLOG_DETAIL_NO_UNIFORM_INITIALIZATION_SYNTAX
                    return {global::gate, file, line, function};
#else
                    return detail::log_every_n<const global::log_gate, level, n, id>(global::gate, file, line, function);
#endif
                }
            }
        }
    }

    namespace global
    {
        using detail::global_gate::log;
        using detail::global_gate::log_every_n;
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/this_thread/log.h"
#ifndef SLOG_THIS_THREAD_LOG_H
#define SLOG_THIS_THREAD_LOG_H

// Amalgamated: #include "slog/declspec.h"

namespace slog
{
    class log_message;

    namespace this_thread
    {
        // Call the current log function for this thread if present,
        // or the global log handler otherwise
        SLOG_API void log(const log_message& message);
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/this_thread/log_function.h"
#ifndef SLOG_THIS_THREAD_LOG_FUNCTION_H
#define SLOG_THIS_THREAD_LOG_FUNCTION_H

// Amalgamated: #include "slog/declspec.h"
// Amalgamated: #include "slog/log_function_type.h"

namespace slog
{
    namespace this_thread
    {
        // Access to the log function for this thread, used by slog::this_thread::log().
        // Setting an invalid function restores the global log handler; to get no logging for
        // just this thread, use a valid function that does nothing.
        SLOG_API log_function set_log_function(log_function log);
        SLOG_API log_function get_log_function();

        // This is a better way to install a thread-local log function
        class SLOG_API scoped_log_function
        {
        public:
            explicit scoped_log_function(log_function log);
            ~scoped_log_function();

        private:
            log_function old;

            // Non-copyable
            scoped_log_function(const scoped_log_function&);
            const scoped_log_function& operator=(const scoped_log_function&);
        };
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/this_thread/severity.h"
#ifndef SLOG_THIS_THREAD_SEVERITY_H
#define SLOG_THIS_THREAD_SEVERITY_H

// Amalgamated: #include "slog/declspec.h"
// Amalgamated: #include "slog/severity_type.h"

namespace slog
{
    namespace this_thread
    {
        // Run-time control of logging loquacity for this thread
        // Note: Use reset_log_severity to restore global control
        SLOG_API void set_log_severity(severity level);
        SLOG_API severity get_log_severity();

        inline bool pertinent(severity level) { return get_log_severity() <= level; }

        // This is a better way to set a thread-local severity level
        class SLOG_API scoped_log_severity
        {
        public:
            explicit scoped_log_severity(severity level);
            ~scoped_log_severity();

        private:
            severity old;

            // Non-copyable
            scoped_log_severity(const scoped_log_severity&);
            const scoped_log_severity& operator=(const scoped_log_severity&);
        };
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/this_thread/log_gate.h"
#ifndef SLOG_THIS_THREAD_LOG_GATE_H
#define SLOG_THIS_THREAD_LOG_GATE_H

// Amalgamated: #include "slog/detail/static_const.h"
// Amalgamated: #include "slog/this_thread/log.h"
// Amalgamated: #include "slog/this_thread/severity.h"

namespace slog
{
    namespace this_thread
    {
        // A logging gateway provides the interface through which log messages pass from a logging statement.
        // This one is an empty class, a monostate object to wrap the thread-local logging core.
        struct log_gate
        {
            static void log(const log_message& message) { this_thread::log(message); }
            static bool pertinent(severity level) { return this_thread::pertinent(level); }
        };
        // Is it still sensible to provide a singleton instance?
        namespace
        {
            const log_gate& gate = detail::static_const<log_gate>::value;
        }
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/this_thread/log_statement.h"
#ifndef SLOG_THIS_THREAD_LOG_STATEMENT_H
#define SLOG_THIS_THREAD_LOG_STATEMENT_H

// Amalgamated: #include "slog/log_statement_type.h"
// Amalgamated: #include "slog/this_thread/log_gate.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Logging statement construction API

// Right. This is it. There's only so far I can go to provide a macro-free interface!
// Use SLOG_FLF and SLOG_ID defined by slog/config.h to drive the API, like so...
// slog::this_thread::log<level>(SLOG_FLF) is the basic logging front-end API
// slog::this_thread::log_every_n<level, n, SLOG_ID>(SLOG_FLF) is the intermittent logging front-end API

namespace slog
{
    namespace detail
    {
        namespace this_thread_gate
        {
            SLOG_DETAIL_BEGIN_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT

            template <severity level>
            inline detail::log<const this_thread::log_gate, level> log(const char* file, int line, const char* function)
            {
#ifndef SLOG_DETAIL_NO_UNIFORM_INITIALIZATION_SYNTAX
                return {this_thread::gate, file, line, function};
#else
                return detail::log<const this_thread::log_gate, level>(this_thread::gate, file, line, function);
#endif
            }

            SLOG_DETAIL_END_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT

            namespace
            {
                // log_every_n must always be local to translation unit to get per-logging statement count
                template <severity level, int n, int id>
                inline detail::log_every_n<const this_thread::log_gate, level, n, id> log_every_n(const char* file, int line, const char* function)
                {
#ifndef SLOG_DETAIL_NO_UNIFORM_INITIALIZATION_SYNTAX
                    return {this_thread::gate, file, line, function};
#else
                    return detail::log_every_n<const this_thread::log_gate, level, n, id>(this_thread::gate, file, line, function);
#endif
                }
            }
        }
    }

    namespace this_thread
    {
        using detail::this_thread_gate::log;
        using detail::this_thread_gate::log_every_n;
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/log.h"
#ifndef SLOG_LOG_H
#define SLOG_LOG_H

// Amalgamated: #include "slog/config.h"

#ifdef SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG

// Amalgamated: #include "slog/global/log.h"

namespace slog
{
    using global::log;
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/log_function.h"
#ifndef SLOG_LOG_FUNCTION_H
#define SLOG_LOG_FUNCTION_H

// Amalgamated: #include "slog/config.h"

// Amalgamated: #include "slog/log_function_type.h"

#ifdef SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG

// Amalgamated: #include "slog/global/log_function.h"

namespace slog
{
    using global::set_log_function;
    using global::get_log_function;
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/log_handler.h"
#ifndef SLOG_LOG_HANDLER_H
#define SLOG_LOG_HANDLER_H

// Amalgamated: #include "slog/config.h"

// Amalgamated: #include "slog/log_handler_type.h"

#ifdef SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG

// Amalgamated: #include "slog/global/log_handler.h"

namespace slog
{
    using global::set_log_handler;
    using global::get_log_handler;
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/log_statement.h"
#ifndef SLOG_LOG_STATEMENT_H
#define SLOG_LOG_STATEMENT_H

// Amalgamated: #include "slog/config.h"

// Amalgamated: #include "slog/log_statement_type.h"

#ifdef SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG

// Amalgamated: #include "slog/global/log_statement.h"

namespace slog
{
    using global::log;
    using global::log_every_n;
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/severity.h"
#ifndef SLOG_SEVERITY_H
#define SLOG_SEVERITY_H

// Amalgamated: #include "slog/config.h"

// Amalgamated: #include "slog/severity_type.h"

#ifdef SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG

// Amalgamated: #include "slog/global/severity.h"

namespace slog
{
    using global::set_log_severity;
    using global::get_log_severity;
    using global::pertinent;
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/detail/impertinent.h"
#ifndef SLOG_DETAIL_IMPERTINENT_H
#define SLOG_DETAIL_IMPERTINENT_H

#include <exception> // for std::terminate
// Amalgamated: #include "slog/config.h"

namespace slog
{
    namespace detail
    {
        // An impertinent imp is used in logging statements to ensure that fatal errors terminate the application
        // Notes:
        // 1. An alternative design would have this logic in log_message, but this allows more flexibility and optimization, I think
        // 2. As it stands, the implementation is only using this with compile-time values, which suggests they could be template arguments
        // but last I checked the compiler seems to be doing a good enough job optimizing even without that change
        template <typename TerminatorFunction = decltype(&std::terminate), TerminatorFunction Terminator = &std::terminate>
        class impertinent
        {
        public:
            explicit impertinent(bool pertinent = false, bool terminate = false) : pertinent(pertinent), terminate(terminate) {}
            operator bool() const { return !pertinent; }
            ~impertinent()
                SLOG_DETAIL_NOEXCEPT_IF_NOEXCEPT(Terminator()) // if terminate were a compile-time constant this could be tighter
            {
                if (terminate) Terminator();
            }
        private:
            // Non-copyable
            impertinent(const impertinent&);
            const impertinent& operator=(const impertinent&);

            const bool pertinent;
            const bool terminate;
        };
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/slog.h"
#ifndef SLOG_SLOG_H
#define SLOG_SLOG_H

// Amalgamated: #include "slog/config.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Logging statements
//
// This file provides the front-end API for logging statements.
// There are four kinds of logging statement.
//
// 1. Basic - the statement is enabled or disabled based only on its severity level, e.g.
// SLOG(level) << ...;
//
// 2. Conditional - there is an additional condition to enable the statement or not, e.g.
// SLOG_IF(level, condition) << ...;
//
// 3. Intermittent - log every n-th time, e.g.
// SLOG_EVERY_N(level, n) << ...;
//
// 4. Intermittent conditional - log every n-th time the condition is satisfied, e.g.
// SLOG_IF_EVERY_N(level, condition, n) << ...;
//
// There are also logged assertions:
// SLOG_ASSERT(condition);
//
// And an abbreviated API to provide some front-end compatibility with other logging libraries:
// LOG(level) << ...;

////////////////////////////////////////////////////////////////////////////////////////////
// Logging styles
//
// Each kind of logging statement macro comes in four styles.
//
// The simplest one is for stream-based logging through the default gateway, e.g.
// SLOG(level) << ...;
//
// To log with a different gateway, add the 'W' suffix, e.g.
// SLOGW(gateway, level) << ...;
//
// To use the printf-style logging, add the 'F' suffix, e.g.
// SLOGF(level, format, ...);
// SLOGWF(gateway, level, format, ...);
//
// The longest logging macro is therefore for printf-style intermittent conditional logging
// with a custom logging gateway, i.e.
// SLOGWF_IF_EVERY_N(gateway, level, condition, n, format, ...);

////////////////////////////////////////////////////////////////////////////////////////////
// Basic logging API

#ifdef SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API
#define SLOGW(gateway, level) slog::log<level>(gateway, SLOG_FLF)
#else
#define SLOGW(gateway, level) SLOG_DETAIL_IF_PERTINENT(level) SLOG_DETAIL_IF(SLOG_DETAIL_DO_PERTINENT(gateway)(level)) SLOG_DETAIL_STREAM(gateway, level)
// SLOG_MESSAGE allows access to the log_message object from within the logging statement
#define SLOG_MESSAGE slog_message_
// Note: Should const access be enforced?
#endif

// Typical severity/verbosity levels

#ifdef SLOG_PROVIDES_SEVERITIES
// Amalgamated: #include "slog/severities.h"

// SLOG_FATAL will, by default, terminate the application after the fatal error is logged
#define SLOG_FATAL slog::severities::fatal
#define SLOG_SEVERE slog::severities::severe
#define SLOG_ERROR slog::severities::error
#define SLOG_WARNING slog::severities::warning
#define SLOG_INFO slog::severities::info
#define SLOG_MORE slog::severities::more_info
//#define SLOG_DEBUG slog::severities::debug // nothing to do with Debug builds, so use MORE as a synonym by preference
#define SLOG_TOO_MUCH slog::severities::too_much_info // TMI isn't usually appropriate; TRACE might be a synonym

#endif

// Compile-time filtering of impertinent (irrelevant) log messages (run-time filtering comes later)
// and compile-time control of which severity levels terminate the application
// Note: SLOG_LOGGING_SEVERITY and SLOG_TERMINATING_SEVERITY are configuration macros that allows straightforward modification of SLOG_PERTINENT and SLOG_TERMINATES

#define SLOG_PERTINENT(level) ((SLOG_LOGGING_SEVERITY) <= (level))
#define SLOG_TERMINATES(level) ((SLOG_TERMINATING_SEVERITY) <= (level))

////////////////////////////////////////////////////////////////////////////////////////////
// Conditional logging API

#define SLOGW_IF(gateway, level, condition) SLOG_DETAIL_IF(condition) SLOGW(gateway, level)

////////////////////////////////////////////////////////////////////////////////////////////
// Intermittent logging API

// Log every n-th time
#ifdef SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API
#define SLOGW_EVERY_N(gateway, level, n) slog::log_every_n<level, n, SLOG_ID>(gateway, SLOG_FLF)
#else
#define SLOGW_EVERY_N(gateway, level, n) SLOG_DETAIL_IF_PERTINENT(level) SLOG_DETAIL_WITH(static SLOG_DETAIL_STATIC_SLOG_COUNT_TYPE static_slog_count_(0)) SLOG_DETAIL_WITH_IF_THEN(SLOG_COUNT_TYPE slog_count_ = 0, 0 == ((slog_count_ = ++static_slog_count_) - 1) % (n), (void)0) SLOG_DETAIL_IF(SLOG_DETAIL_DO_PERTINENT(gateway)(level)) SLOG_DETAIL_STREAM(gateway, level)
// Notes:
// 1. SLOG_EVERY_N(level, 10) will log on the 1st, 11th, 21st, ... times through the statement
// 2. It's necessary to copy the static_slog_count_ to the local slog_count_ to ensure that SLOG_COUNT
// has a consistent value for the logging statement in case it can be hit in multiple threads
// 3. Under certain conditions, it's proven necessary for static_slog_count_ to be atomic even though
// most of the time you won't witness e.g. duplicated increment using just a plain int...
#endif

// SLOG_FIRST_N(level, n), etc. could be provided similarly (cf. google-glog, and LOG_N_TIMES in easylogging++)
// SLOG_AFTER_N(level, n), etc. could be provided similarly (cf. easylogging++)

// SLOG_COUNT allows the hit count to be inserted into the message stream, fulfilling the same purpose as google::COUNTER in google-glog
// SLOG_GET_COUNT allows the value of the hit count to be used after the logging statement
#ifdef SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API
#define SLOG_COUNT slog::count
#define SLOG_GET_COUNT(result) slog::get_count(result)
#else
#define SLOG_COUNT slog_count_
// Note: Should const access be enforced?
#define SLOG_GET_COUNT(result) (result = slog_count_, "")
#endif

// SLOG_COUNT_TYPE must be an integral type for the hit count for intermittent logging statements
#ifndef SLOG_COUNT_TYPE
#define SLOG_COUNT_TYPE int
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Intermittent conditional logging API

#define SLOGW_IF_EVERY_N(gateway, level, condition, n) SLOG_DETAIL_IF(condition) SLOGW_EVERY_N(gateway, level, n)
// Note: These macros only increment the hit count if the condition is satisfied.

////////////////////////////////////////////////////////////////////////////////////////////
// Verbose logging API

// Separate macros are not required because slog uses a single severity/verbosity hierarchy

////////////////////////////////////////////////////////////////////////////////////////////
// Logged assertion API

// If the assertion fails then, by default, terminate the application after the fatal error is logged
#define SLOGW_ASSERT(gateway, condition) SLOG_DETAIL_IF(!(condition)) SLOGW(gateway, SLOG_TERMINATING_SEVERITY) << "Assert failed: " #condition
// Note: It would be possible to put the stringized condition into the log_message like file/function instead.
// Or the stringized condition could be injected into the statement scope like SLOG_COUNT. Not necessary now.

////////////////////////////////////////////////////////////////////////////////////////////
// API for printf-style logging

// Amalgamated: #include "slog/stringf.h"
// Note: In all these variadic macros, the first unspecified argument is the printf-style format string.
// (Defining them using an explicit format arg and the ##__VA_ARGS__ extension results in compiler warnings.)
#define SLOGWF(gateway, level, ...) SLOGW(gateway, level) << slog::stringf(__VA_ARGS__)
#define SLOGWF_IF(gateway, level, condition, ...) SLOGW_IF(gateway, level, condition) << slog::stringf(__VA_ARGS__)
#define SLOGWF_EVERY_N(gateway, level, n, ...) SLOGW_EVERY_N(gateway, level, n) << slog::stringf(__VA_ARGS__)
#define SLOGWF_IF_EVERY_N(gateway, level, condition, n, ...) SLOGW_IF_EVERY_N(gateway, level, condition, n) << slog::stringf(__VA_ARGS__)
#define SLOGWF_ASSERT(gateway, condition, ...) SLOGW_ASSERT(gateway, condition) << slog::stringf(__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////////////
// Default logging gateway API

#define SLOG(level) SLOGW(SLOG_LOGGING_GATEWAY, level)
#define SLOGF(level, ...) SLOGWF(SLOG_LOGGING_GATEWAY, level, __VA_ARGS__)
#define SLOG_IF(level, condition) SLOGW_IF(SLOG_LOGGING_GATEWAY, level, condition)
#define SLOGF_IF(level, condition, ...) SLOGWF_IF(SLOG_LOGGING_GATEWAY, level, condition, __VA_ARGS__)
#define SLOG_EVERY_N(level, n) SLOGW_EVERY_N(SLOG_LOGGING_GATEWAY, level, n)
#define SLOGF_EVERY_N(level, n, ...) SLOGWF_EVERY_N(SLOG_LOGGING_GATEWAY, level, n, __VA_ARGS__)
#define SLOG_IF_EVERY_N(level, condition, n) SLOGW_IF_EVERY_N(SLOG_LOGGING_GATEWAY, level, condition, n)
#define SLOGF_IF_EVERY_N(level, condition, n, ...) SLOGWF_IF_EVERY_N(SLOG_LOGGING_GATEWAY, level, condition, n, __VA_ARGS__)
#define SLOG_ASSERT(condition) SLOGW_ASSERT(SLOG_LOGGING_GATEWAY, condition)
#define SLOGF_ASSERT(condition, ...) SLOGWF_ASSERT(SLOG_LOGGING_GATEWAY, condition, __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////////////
// Abbreviated API

// google-glog (and g2log/g3log, easylogging++) style macros
// Note: This is intended to provide some front-end compatibility, but not configuration compatibility, etc.

#ifdef SLOG_PROVIDES_ABBREVIATED_SEVERITIES

//#define DEBUG SLOG_MORE
#define INFO SLOG_INFO
#define WARNING SLOG_WARNING
#define ERROR SLOG_ERROR
#define FATAL SLOG_FATAL

#endif

#ifdef SLOG_PROVIDES_ABBREVIATED_LOGGING

#define LOG(level) SLOG(level)
#define LOG_IF(level, condition) SLOG_IF(level, condition)

#define LOG_COUNT SLOG_COUNT // performs same function as google::COUNTER
#define LOG_EVERY_N(level, n) SLOG_EVERY_N(level, n)
#define LOG_IF_EVERY_N(level, condition, n) SLOG_IF_EVERY_N(level, condition, n)

#define LOG_ASSERT(condition) SLOG_ASSERT(condition)

// google-glog verboselevel is usually a small positive integer, and in such cases we can support them
// reasonably well simply by negating them
#define VLOG(verboselevel) SLOG(-(verboselevel))
#define VLOG_IF(verboselevel, condition) SLOG_IF(-(verboselevel), condition)
#define VLOG_EVERY_N(verboselevel, n) SLOG_EVERY_N(-(verboselevel), n)
#define VLOG_IF_EVERY_N(verboselevel, condition, n)  SLOG_IF_EVERY_N(-(verboselevel), condition, n)

// g2log/g3log style macros

#define LOGF(level, ...) SLOGF(level, __VA_ARGS__)
#define LOGF_IF(level, condition, ...) SLOGF_IF(level, condition, __VA_ARGS__)
#define LOGF_EVERY_N(level, n, ...) SLOGF_EVERY_N(level, n, __VA_ARGS__)
#define LOGF_IF_EVERY_N(level, condition, n, ...) SLOGF_IF_EVERY_N(level, condition, n, __VA_ARGS__)
#define LOGF_ASSERT(condition) SLOGF_ASSERT(condition)

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Implementation details
//
// Note: Preprocessor symbols that begin SLOG_DETAIL are private implementation detail
// which users should not rely upon. All other SLOG symbols are public, part of the API.

// With-if-then uses a nested for-statement to get an (optional) init-statement scoped to the
// conditionally-executed appended statement (and an optional post-statement expression as well)
#define SLOG_DETAIL_IF(condition) SLOG_DETAIL_WITH_IF_THEN((void)0, condition, (void)0)
#define SLOG_DETAIL_WITH(init_statement) SLOG_DETAIL_WITH_IF_THEN(init_statement, true, (void)0)
#define SLOG_DETAIL_WITH_IF_THEN(init_statement, condition, post_statement_expression) for (bool slog_with_if_then_done_ = false; !slog_with_if_then_done_; slog_with_if_then_done_ = true) for (init_statement; !slog_with_if_then_done_ && (condition); slog_with_if_then_done_ = true, post_statement_expression)
// Note: Other formulations using if-false-condition-else can ensure no dangling-else errors but
// still result in dangling-else warnings in code like: if (condition) SLOG(...) << "...";

#ifdef SLOG_DETAIL_INCLUDE_LOGGING_GATEWAY_HEADER
#ifdef SLOG_PROVIDES_THREAD_LOCAL_LOGGING
// Amalgamated: #include "slog/this_thread/log_gate.h"
#else
// Amalgamated: #include "slog/global/log_gate.h"
#endif
#endif

#ifdef SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API

// Amalgamated: #include "slog/log_statement.h"

#else

////////////////////////////////////////////////////////////////////////////////////////////
// Basic logging API - implementation detail

#define SLOG_DETAIL_DO_LOG(gateway) (gateway).log

// Construct a temporary log message with the location metadata and process it after the complete statement
// Amalgamated: #include "slog/log_message.h"
#define SLOG_DETAIL_MAKE_MESSAGE(gateway, level) SLOG_DETAIL_WITH_IF_THEN(slog::log_message slog_message_(SLOG_FLF, level), true, SLOG_DETAIL_DO_LOG(gateway)(slog_message_)) slog_message_
// Note: A simpler implementation technique in which the log_message destructor calls log(), is flawed
// if log() may throw, since an exception from the user's message preparation could already be active
//#define SLOG_DETAIL_MAKE_MESSAGE(level) slog::log_message(SLOG_FLF, level)

// Construct the log message and access its stream to prepare the message
#define SLOG_DETAIL_STREAM(gateway, level) SLOG_DETAIL_MAKE_MESSAGE(gateway, level).stream()

// An impertinent imp is used to ensure that fatal errors terminate the application
// Amalgamated: #include "slog/detail/impertinent.h"
#define SLOG_DETAIL_IF_PERTINENT(level) SLOG_DETAIL_WITH_IF_THEN(const SLOG_DETAIL_IMP& slog_imp_ = SLOG_DETAIL_IMP(SLOG_PERTINENT(level), SLOG_TERMINATES(level)), !slog_imp_, (void)0)
#define SLOG_DETAIL_IMP slog::detail::impertinent<decltype(&SLOG_DETAIL_DO_TERMINATE), &SLOG_DETAIL_DO_TERMINATE>
// Note: Bind the (non-copyable) temporary to const& on the stack to get the block scope required
// See http://herbsutter.com/2008/01/01/gotw-88-a-candidate-for-the-most-important-const/

#define SLOG_DETAIL_DO_PERTINENT(gateway) (gateway).pertinent

////////////////////////////////////////////////////////////////////////////////////////////
// Intermittent logging API - implementation detail

// Some implementation detail to provide the guarantee to be accurate in multi-threaded apps
#ifdef SLOG_PROVIDES_ATOMIC_SLOG_COUNT
// Amalgamated: #include "bst/atomic.h"
#define SLOG_DETAIL_STATIC_SLOG_COUNT_TYPE bst::atomic<SLOG_COUNT_TYPE>
#else
#define SLOG_DETAIL_STATIC_SLOG_COUNT_TYPE SLOG_COUNT_TYPE
#endif

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "bst/chrono.h"
#ifndef BST_CHRONO_H
#define BST_CHRONO_H

// Provide bst::chrono::duration, etc. using either std:: or boost:: symbols

// Note: Same condition as bst/thread.h because we want consistent use of function/thread/chrono
#ifndef BST_THREAD_BOOST

#include <chrono>
namespace bst_chrono = std::chrono;

#else

#include <boost/chrono/chrono.hpp>
namespace bst_chrono = boost::chrono;

#endif

namespace bst
{
    namespace chrono
    {
	    // should replace using-directive with using-declarations for compatible symbols
        using namespace bst_chrono;
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "bst/thread.h"
#ifndef BST_THREAD_H
#define BST_THREAD_H

// Provide bst::thread, etc. using either std:: or boost:: symbols

#ifndef BST_THREAD_BOOST

#include <thread>
namespace bst_thread = std;
namespace bst_this_thread = std::this_thread;

#else

#include <boost/thread/thread.hpp>
namespace bst_thread = boost;
namespace bst_this_thread = boost::this_thread;

#endif

namespace bst
{
    namespace this_thread
    {
        // should replace using-directive with using-declarations for compatible symbols
        using namespace bst_this_thread;
    }
    using bst_thread::thread;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/async_log_message.h"
#ifndef SLOG_ASYNC_LOG_MESSAGE_H
#define SLOG_ASYNC_LOG_MESSAGE_H

// Amalgamated: #include "bst/chrono.h"
// Amalgamated: #include "bst/thread.h"
// Amalgamated: #include "slog/copyable_log_message.h"

namespace slog
{
    // A copyable log message for logging in multi-threaded application, which captures a timestamp and the thread id
    class async_log_message : public copyable_log_message
    {
    public:
        typedef bst::chrono::system_clock clock;
        typedef clock::time_point time_point;

        // Converting constructor fulfills the primary purpose of saving a log_message
        // Note: No harm in not being explicit?
        async_log_message(const log_message& message)
            : copyable_log_message(message)
            , thread_id_(bst::this_thread::get_id())
            , timestamp_(clock::now())
        {}

        // This constructor provides compatibility with log_message construction
        async_log_message(const std::string& file, int line, const std::string& function, severity level)
            : copyable_log_message(file, line, function, level)
            , thread_id_(bst::this_thread::get_id())
            , timestamp_(clock::now())
        {}

        // This constructor provides compatibility with log_message construction
        async_log_message(const std::string& file, int line, const std::string& function, severity level, const std::string& str)
            : copyable_log_message(file, line, function, level, str)
            , thread_id_(bst::this_thread::get_id())
            , timestamp_(clock::now())
        {}

        // A default constructor seems like a good idea, even though it constructs a barely useful instance
        // Note: Need to decide whether this is even "barely useful" or whether default meaning invalid would be better
        // and use default-constructed thread id and timestamp if so
        async_log_message()
            : copyable_log_message()
            , thread_id_(bst::this_thread::get_id())
            , timestamp_(clock::now())
        {}

        // Likewise a constructor simply taking an initializing string for the message stream
        explicit async_log_message(const std::string& str)
            : copyable_log_message(str)
            , thread_id_(bst::this_thread::get_id())
            , timestamp_(clock::now())
        {}

        // Trivial destructor
        ~async_log_message() {}

        // Copy constructor (unlike the log_message converting constructor, copy timestamp etc., don't generate anew!)
        async_log_message(const async_log_message& message)
            : copyable_log_message(message)
            , thread_id_(message.thread_id())
            , timestamp_(message.timestamp())
        {}

        // The copy assignment operator is provided for consistency with the copy constructor (rule of 3 or so)
        async_log_message& operator=(async_log_message message)
        {
            swap(*this, message);
            return *this;
        }

        // The swap function can only be found via ADL; it's used to implement assignment via copy-swap
        friend void swap(async_log_message& a, async_log_message& b)
        {
            using std::swap;
            swap((copyable_log_message&)a, (copyable_log_message&)b);
            swap(a.thread_id_, b.thread_id_);
            swap(a.timestamp_, b.timestamp_);
        }

        // Accessors (in addition to those in copyable_log_message)
        bst::thread::id thread_id() const { return thread_id_; }
        time_point timestamp() const { return timestamp_; }

    protected:
        bst::thread::id thread_id_;
        time_point timestamp_;
    };
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "bst/condition_variable.h"
#ifndef BST_CONDITION_VARIABLE_H
#define BST_CONDITION_VARIABLE_H

// Provide bst::condition_variable, etc. using either std:: or boost:: symbols

// Amalgamated: #include "bst/chrono.h"

// Note: Same condition as bst/thread.h because we want consistent use of function/thread/chrono
#ifndef BST_THREAD_BOOST

#include <condition_variable>
namespace bst_thread = std;

#else

#include <boost/thread/condition_variable.hpp>
namespace bst_thread = boost;

#endif

namespace bst
{
    using bst_thread::condition_variable;
    using bst_thread::lock_guard;
    using bst_thread::mutex;
    using bst_thread::unique_lock;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "util/concurrent_queue.h"
#ifndef UTIL_CONCURRENT_QUEUE_H
#define UTIL_CONCURRENT_QUEUE_H

////////////////////////////////////////////////////////////////////////////////////////////
// This concurrent queue supports thread-safe push/pop via condition variable notification.
// Initial design influenced by: 
// http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
// http://www.justsoftwaresolutions.co.uk/threading/condition-variable-spurious-wakes.html
// Support for closing the queue is added, to enable uses where it must be possible to wake
// threads waiting on an empty queue; the mechanism ended up close to that proposed in the
// WG21 paper, "C++ Concurrent Queues":
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3533.html

#include <deque>
// Amalgamated: #include "bst/condition_variable.h"

namespace util
{
  class concurrent_queue_closed_exception;

  template < typename Value >
  class concurrent_queue;

  template < typename Value >
  class concurrent_queue_front;

  template < typename Value >
  class concurrent_queue_back;
}

////////////////////////////////////////////////////////////////////////////////////////////
class util::concurrent_queue_closed_exception {};

////////////////////////////////////////////////////////////////////////////////////////////
template < typename Value >
class util::concurrent_queue
{
  typedef std::deque< Value > underlying_queue_t;
  typedef bst::mutex mutex_t;
  typedef bst::unique_lock< mutex_t > lock_t;
public:
  typedef typename underlying_queue_t::value_type value_type;
  typedef typename underlying_queue_t::size_type size_type;

  void push( const value_type& value )
  {
    lock_t lock( mutex );

    if ( closed )
    {
      throw concurrent_queue_closed_exception();
    }

    queue.push_back( value );
    lock.unlock();
    condition_variable.notify_one();
  }

  // ephemeral result
  bool empty() const
  {
    lock_t lock( mutex );
    return queue.empty();
  }

  // ephemeral result
  size_type size() const
  {
    lock_t lock( mutex );
    return queue.size();
  }

  // ephemeral result
  bool try_front( value_type& front_value )
  {
    lock_t lock( mutex );

    if ( queue.empty() )
    {
      return false;
    }

    front_value = queue.front();

    return true;
  }

  bool try_pop( value_type& popped_value )
  {
    lock_t lock( mutex );

    if ( queue.empty() )
    {
      return false;
    }

    popped_value = queue.front();
    queue.pop_front();

    return true;
  }

  void pop( value_type& popped_value )
  {
    lock_t lock( mutex );

    condition_variable.wait( lock, [&]{ return closed || !queue.empty(); } );

    if ( queue.empty() )
    {
      // must be closed
      throw concurrent_queue_closed_exception();
    }

    popped_value = queue.front();
    queue.pop_front();
  }

  template < typename TimePoint >
  bool pop_or_wait_until( value_type& popped_value, const TimePoint& wait_time )
  {
    lock_t lock( mutex );

    if ( !condition_variable.wait_until( lock, wait_time, [&]{ return closed || !queue.empty(); } ) )
    {
      return false;
    }

    if ( queue.empty() )
    {
      // must be closed
      throw concurrent_queue_closed_exception();
    }

    popped_value = queue.front();
    queue.pop_front();

    return true;
  }

  template < typename Duration >
  bool pop_or_wait_for( value_type& popped_value, const Duration& wait_duration )
  {
    return pop_or_wait_until( popped_value, bst::chrono::steady_clock::now() + wait_duration );
  }

  // ephemeral result
  bool is_closed() const
  {
    lock_t lock( mutex );
    return closed;
  }

  void close()
  {
    lock_t lock( mutex );
    closed = true;
    lock.unlock();
    condition_variable.notify_all();
  }

  void open()
  {
    lock_t lock( mutex );
    closed = false;
  }

  concurrent_queue() : closed( false ) {}
private:
  concurrent_queue( const concurrent_queue& );
  const concurrent_queue& operator=( const concurrent_queue& );

private:
  underlying_queue_t queue;
  bool closed;
  mutable bst::mutex mutex;
  bst::condition_variable condition_variable;
};

////////////////////////////////////////////////////////////////////////////////////////////
template < typename Value >
class util::concurrent_queue_front
{
public:
  typedef concurrent_queue< Value > concurrent_queue_type;
  typedef typename concurrent_queue_type::value_type value_type;

  // ephemeral result
  bool empty() const
  {
    return queue.empty();
  }

  // ephemeral result
  bool try_front( value_type& front_value )
  {
    return queue.try_front( front_value );
  }

  bool try_pop( value_type& popped_value )
  {
    return queue.try_pop( popped_value );
  }

  void pop( value_type& popped_value )
  {
    queue.pop( popped_value );
  }

  template < typename TimePoint >
  bool pop_or_wait_until( value_type& popped_value, const TimePoint& wait_time )
  {
    return queue.pop_or_wait_until( popped_value, wait_time );
  }

  template < typename Duration >
  bool pop_or_wait_for( value_type& popped_value, const Duration& wait_duration )
  {
    return queue.pop_or_wait_for( popped_value, wait_duration );
  }

  concurrent_queue_front( concurrent_queue_type& queue ) : queue( queue ) {}
private:
  concurrent_queue_front( const concurrent_queue_front& );
  const concurrent_queue_front& operator=( const concurrent_queue_front& );

private:
  concurrent_queue_type& queue;
};

////////////////////////////////////////////////////////////////////////////////////////////
template < typename Value >
class util::concurrent_queue_back
{
public:
  typedef concurrent_queue< Value > concurrent_queue_type;
  typedef typename concurrent_queue_type::value_type value_type;

  void push( const value_type& value )
  {
    queue.push( value );
  }

  // ephemeral result
  bool is_closed() const
  {
    return queue.is_closed();
  }

  void close()
  {
    queue.close();
  }

  void open()
  {
    queue.open();
  }

  concurrent_queue_back( concurrent_queue_type& queue ) : queue( queue ) {}
private:
  concurrent_queue_back( const concurrent_queue_back& );
  const concurrent_queue_back& operator=( const concurrent_queue_back& );

private:
  concurrent_queue_type& queue;
};

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "util/message_service.h"
#ifndef UTIL_MESSAGE_SERVICE_H
#define UTIL_MESSAGE_SERVICE_H

// Amalgamated: #include "util/concurrent_queue.h"
#include <map>
namespace util
{
    // This message_service provides very simple asynchronous handling of messages
    // Notes:
    // 1. It isn't based on C++11 std::packaged_task, etc. and provides no means of determining whether the messages have been serviced or not
    // 2. It's something like an active object, but can utilise multiple threads concurrently, a bit like boost::asio::io_service
    template <typename MessageType>
    class message_service
    {
    public:
        typedef concurrent_queue_closed_exception stopped_exception;
        typedef MessageType message_type;

        message_service() : message_queue() {}

        // Utilise the current thread to run, returning normally when the service is stopped
        template <typename ServiceFunction>
        void run(const ServiceFunction& fn = ServiceFunction())
        {
            try
            {
                // Inefficient and unnecessary to test !stopped() all the time, and need to handle stopped_exception anyway
                for (;;)
                {
                    message_type message;
                    message_queue.pop(message);
                    fn(message);
                }
            }
            catch (const stopped_exception&)
            {
                // Must have been stopped (though might have been reset by now!)
            }
        }

        // Signal all threads that are waiting in run() to stop
        void stop() { message_queue.close(); }

        // Ephemeral state
        bool idle() const { return message_queue.empty(); }
        bool stopping() const { return message_queue.is_closed(); }
        bool stopped() const { return stopping() && idle(); }

        // Queue message and return immediately; message will be serviced on a thread calling run()
        // Notes:
        // 1. Will throw stopped_exception if the service is stopped
        // 2. It is the responsibility of the service function to synchronize if necessary e.g. with other queued messages
        void post(const message_type& message) { message_queue.push(message); }

        // Clear stopped state, cf. io_service
        void reset() { message_queue.open(); }

        // Trivial destructor
        // Note: It is the caller's responsibility to ensure all threads have left run() before destroying this!
        ~message_service() {}

    private:
        // Non-copyable
        message_service(const message_service&);
        const message_service& operator=(const message_service&);

        concurrent_queue<message_type> message_queue;
    };
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/async_log_service.h"
#ifndef SLOG_ASYNC_LOG_SERVICE_H
#define SLOG_ASYNC_LOG_SERVICE_H

// Amalgamated: #include "bst/thread.h"
// Amalgamated: #include "util/message_service.h"
// Amalgamated: #include "slog/async_log_message.h"
// Amalgamated: #include "slog/log_handler_type.h"

namespace slog
{
    // Forward declare default parameter helper for async_log_service
    namespace detail { template <typename ServiceFunction> struct async_log_service_message_default; }

    // An async_log_service is used to move logging to a separate worker thread.
    // It can be used as part of a custom logging gateway, or by reference, as a
    // log_function with the global or thread-local logging gateway.
    template <typename ServiceFunction, typename ServiceMessage = typename detail::async_log_service_message_default<ServiceFunction>::type>
    // ServiceMessage must be default constructible, copyable and support explicit
    // construction from a log_message, e.g. copyable_log_message or async_log_message.
    // A ServiceFunction must be callable with a ServiceMessage parameter. It could
    // be a function object or function pointer. With copyable_log_message as the
    // message type, that means it could be log_handler or log_function.
    class async_log_service
    {
    public:
        typedef ServiceMessage service_message;
        typedef ServiceFunction service_function;

        explicit async_log_service(service_function service_fun = service_function())
            : worker([=](){ service.run(service_fun); })
        {}

        ~async_log_service()
        {
            service.stop();
            worker.join();
            // assert service.stopped()?
        }

        void operator()(const log_message& message)
        {
            service.post(service_message(message));
        }

    private:
        // Non-copyable
        async_log_service(const async_log_service&);
        const async_log_service& operator=(const async_log_service&);

        util::message_service<service_message> service;
        bst::thread worker;
    };

    namespace detail
    {
        template <typename ServiceFunction>
        struct async_log_service_message_default
        {
            typedef typename std::remove_const<typename std::remove_reference<typename ServiceFunction::argument_type>::type>::type arg_type;
            typedef typename std::conditional<std::is_same<log_message, arg_type>::value, copyable_log_message, arg_type>::type type;
        };
        template <>
        struct async_log_service_message_default<log_handler> { typedef copyable_log_message type; };
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "detail/private_access.h"
#ifndef DETAIL_PRIVATE_ACCESS_H
#define DETAIL_PRIVATE_ACCESS_H

//+ https://gist.github.com/dabrahams/1528856

namespace detail
{
    // This is a rewrite and analysis of the technique in this article:
    // http://bloglitb.blogspot.com/2010/07/access-to-private-members-thats-easy.html

    // ------- Framework -------
    // The little library required to work this magic

    // Generate a static data member of type Tag::type in which to store
    // the address of a private member.  It is crucial that Tag does not
    // depend on the /value/ of the the stored address in any way so that
    // we can access it from ordinary code without directly touching
    // private data.
    template <class Tag>
    struct stowed
    {
        static typename Tag::type value;
    };

    template <class Tag>
    typename Tag::type stowed<Tag>::value;

    // Generate a static data member whose constructor initializes
    // stowed<Tag>::value.  This type will only be named in an explicit
    // instantiation, where it is legal to pass the address of a private
    // member.
    template <class Tag, typename Tag::type x>
    struct stow_private
    {
        stow_private() { stowed<Tag>::value = x; }
        static stow_private instance;
    };
    template <class Tag, typename Tag::type x>
    stow_private<Tag, x> stow_private<Tag, x>::instance;
}

#ifdef DETAIL_PRIVATE_ACCESS_DEMO
#include <ostream>

namespace detail
{
    namespace
    {
        // ------- Usage -------
        // A demonstration of how to use the library, with explanation

        struct A
        {
            A() : x("proof!") {}
            bool has_value() const { return 0 != x; }

        private:
            char const* x;
        };

        // A tag type for A::x.  Each distinct private member you need to
        // access should have its own tag.  Each tag should contain a
        // nested ::type that is the corresponding pointer-to-member type.
        struct A_x { typedef char const*(A::*type); };
    }

    // Explicit instantiation; the only place where it is legal to pass
    // the address of a private member.  Generates the static ::instance
    // that in turn initializes stowed<Tag>::value.
    template struct stow_private<A_x, &A::x>;

    namespace
    {
        void demo_A(std::ostream& os)
        {
            A a;

            // Use the stowed private member pointer
            os << a.*stowed<A_x>::value;
        }

        // This technique can also be used to access private static member
        // variables.

        struct B
        {
        private:
            static char const* x;
        };

        char const* B::x = "proof!";

        // A tag type for B::x.
        struct B_x { typedef char const** type; };
    }

    // Explicit instantiation.
    template struct stow_private<B_x, &B::x>;

    namespace
    {
        void demo_B(std::ostream& os)
        {
            // Use the stowed private static member pointer
            os << *stowed<B_x>::value;
        }
    }
}
#endif

//- https://gist.github.com/dabrahams/1528856

// when that's not enough, e.g. for unit tests of private implementation
namespace detail
{
    template <typename Context> void private_access(Context);
    template <typename Tag> void private_access();
}

#define DETAIL_PRIVATE_ACCESS_DECLARATION \
    template <typename Context> friend void ::detail::private_access(Context); \
    template <typename Tag> friend void ::detail::private_access();

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/ios_stasher.h"
#ifndef SLOG_IOS_STASHER_H
#define SLOG_IOS_STASHER_H

#include <ios> // for ios_base::event
#include <type_traits> // for enable_if, is_base_of
#include <typeinfo> // for bad_cast
// Amalgamated: #include "detail/pragma_warnings.h"
#ifdef _LIBCPP_VERSION
// Amalgamated: #include "detail/private_access.h" // for stow_private
#endif
// Amalgamated: #include "slog/config.h" // for SLOG_DETAIL_NO_REF_QUALIFIERS

////////////////////////////////////////////////////////////////////////////////////////////
// Utility class and functions to provide type-safe heterogeneous storage in a stream
namespace slog
{
    // Use the free functions set_stash, get_stash, has_stash and clear_stash
    // and/or the manipulators stash, stashed and nostash to access the values.
    // Use unique stash_tag types to distinguish multiple values of an underlying type.

    // Note: The ios_stasher class is not to be used directly.
    template <typename StashTag>
    class ios_stasher;

    // Define unique tagged types with e.g. struct my_int : stash_tag<int> {};

    struct stash_tag_marker {};

    template <typename T>
    struct is_stash_tag : std::is_base_of<stash_tag_marker, T> {};

    template <typename ValueType>
    struct stash_tag : stash_tag_marker { typedef ValueType type; };

    template <typename ValueType, typename = void>
    struct make_stash_tag { typedef stash_tag<ValueType> type; };

    template <typename StashTag>
    struct make_stash_tag<StashTag, typename std::enable_if<is_stash_tag<StashTag>::value, void>::type> { typedef StashTag type; };

    // Free functions

    template <typename StashTag>
    inline typename std::enable_if<is_stash_tag<StashTag>::value, void>::type set_stash(std::ios_base& ios, const typename StashTag::type& value)
    {
        (ios_stasher<StashTag>(value))(ios);
    }

    template <typename StashTag>
    inline typename std::enable_if<is_stash_tag<StashTag>::value, void>::type set_stash(std::ios_base& ios, typename StashTag::type&& value)
    {
        (ios_stasher<StashTag>(std::move(value)))(ios);
    }

    template <typename ValueType>
    inline void set_stash(std::ios_base& ios, const ValueType& value)
    {
        set_stash<stash_tag<ValueType>>(ios, value);
    }

    template <typename ArgType, typename ValueType = typename std::remove_const<typename std::remove_reference<ArgType>::type>::type>
    inline void set_stash(std::ios_base& ios, ArgType&& value)
    {
        set_stash<stash_tag<ValueType>>(ios, std::forward<ArgType>(value));
    }

    template <typename StashTagOrValueType>
    inline bool has_stash(const std::ios_base& ios)
    {
        return ios_stasher<typename make_stash_tag<StashTagOrValueType>::type>::has(ios);
    }

    template <typename StashTagOrValueType>
    inline void clear_stash(std::ios_base& ios)
    {
        ios_stasher<typename make_stash_tag<StashTagOrValueType>::type>::clear(ios);
    }

    template <typename StashTagOrValueType>
    inline typename make_stash_tag<StashTagOrValueType>::type::type get_stash(const std::ios_base& ios)
    {
        return ios_stasher<typename make_stash_tag<StashTagOrValueType>::type>::get(ios);
    }

    template <typename StashTag>
    inline typename std::enable_if<is_stash_tag<StashTag>::value, typename StashTag::type>::type get_stash(const std::ios_base& ios, const typename StashTag::type& default_value)
    {
        return ios_stasher<StashTag>::get(ios, default_value);
    }

    template <typename StashTag>
    inline typename std::enable_if<is_stash_tag<StashTag>::value, typename StashTag::type>::type get_stash(const std::ios_base& ios, typename StashTag::type&& default_value)
    {
        return ios_stasher<StashTag>::get(ios, std::move(default_value));
    }

    template <typename ValueType>
    inline ValueType get_stash(const std::ios_base& ios, const ValueType& default_value)
    {
        return get_stash<stash_tag<ValueType>>(ios, default_value);
    }

    template <typename ArgType, typename ValueType = typename std::remove_const<typename std::remove_reference<ArgType>::type>::type>
    inline ValueType get_stash(const std::ios_base& ios, ArgType&& default_value)
    {
        return get_stash<stash_tag<ValueType>>(ios, std::forward<ArgType>(default_value));
    }

    // Manipulators

    template <typename StashTag>
    inline ios_stasher<typename std::enable_if<is_stash_tag<StashTag>::value, StashTag>::type> stash(const typename StashTag::type& value)
    {
        return ios_stasher<StashTag>(value);
    }

    template <typename StashTag>
    inline ios_stasher<typename std::enable_if<is_stash_tag<StashTag>::value, StashTag>::type> stash(typename StashTag::type&& value)
    {
        return ios_stasher<StashTag>(std::move(value));
    }

    template <typename ValueType>
    inline ios_stasher<stash_tag<ValueType>> stash(const ValueType& value)
    {
        return stash<stash_tag<ValueType>>(value);
    }

    template <typename ArgType, typename ValueType = typename std::remove_const<typename std::remove_reference<ArgType>::type>::type>
    inline ios_stasher<stash_tag<ValueType>> stash(ArgType&& value)
    {
        return stash<stash_tag<ValueType>>(std::forward<ArgType>(value));
    }

    template <typename StashTagOrValueType>
    inline std::ios_base& nostash(std::ios_base& ios)
    {
        clear_stash<typename make_stash_tag<StashTagOrValueType>::type>(ios);
        return ios;
    }

    template <typename StashTagOrValueType, typename CharT, typename TraitsT>
    inline std::basic_ostream<CharT, TraitsT>& stashed(std::basic_ostream<CharT, TraitsT>& os)
    {
        return ios_stasher<typename make_stash_tag<StashTagOrValueType>::type>::insert(os);
    }

    // Implementation

    namespace detail
    {
#ifdef _LIBCPP_VERSION
        // Workaround for libc++ bug: https://llvm.org/bugs/show_bug.cgi?id=21321#c4
        // pword and iword elements are not initialised to 0 when the capacity of the array is increased by more than one.
        struct ios_base_xindex { typedef std::atomic<int>* type; };
        template struct ::detail::stow_private<ios_base_xindex, &std::ios_base::__xindex_>;

        inline void init_stash(std::ios_base& ios)
        {
            const int xindex = *::detail::stowed<ios_base_xindex>::value;
            for (int i = 0; i < xindex; ++i)
            {
                ios.pword(i);
                ios.iword(i);
            }
        }
#else
        inline void init_stash(std::ios_base& ios)
        {
        }
#endif

        template <typename StashTag>
        class ios_pword_stasher
        {
        public:
            static const bool is_iword = false;
            typedef typename StashTag::type value_type;

            explicit ios_pword_stasher(const value_type& value) : value(value) {}
            explicit ios_pword_stasher(value_type&& value) : value(std::move(value)) {}

            static bool has(const std::ios_base& ios)
            {
                return 0 != pword(ios);
            }

            static void clear(std::ios_base& ios)
            {
                void*& p = pword(ios);
                if (0 != p)
                {
                    delete static_cast<value_type*>(p);
                    p = 0;
                }
            }

            static value_type get(const std::ios_base& ios)
            {
                const void* p = pword(ios);
                if (0 != p)
                {
                    return *static_cast<const value_type*>(p);
                }
                throw std::bad_cast();
            }

            static value_type get(const std::ios_base& ios, const value_type& default_value)
            {
                const void* p = pword(ios);
                if (0 != p)
                {
                    return *static_cast<const value_type*>(p);
                }
                return default_value;
            }

            static value_type get(const std::ios_base& ios, value_type&& default_value)
            {
                const void* p = pword(ios);
                if (0 != p)
                {
                    return *static_cast<const value_type*>(p);
                }
                return std::move(default_value);
            }

            template <typename CharT, typename TraitsT>
            static std::basic_ostream<CharT, TraitsT>& insert(std::basic_ostream<CharT, TraitsT>& os)
            {
                const void* p = pword(os);
                if (0 != p)
                {
                    return os << *static_cast<const value_type*>(p);
                }
                // set failbit?
                return os;
            }

            static void*& pword(std::ios_base& ios)
            {
                init_stash(ios);
                return ios.pword(index);
            }

            static const void* pword(const std::ios_base& ios)
            {
                init_stash(const_cast<std::ios_base&>(ios));
                return const_cast<std::ios_base&>(ios).pword(index);
            }

#ifndef SLOG_DETAIL_NO_REF_QUALIFIERS
            void operator()(std::ios_base& ios) const &
            {
                registered_pword(ios) = new value_type(value);
            }

            void operator()(std::ios_base& ios) &&
            {
                registered_pword(ios) = new value_type(std::move(value));
            }
#else
            void operator()(std::ios_base& ios) const
            {
                registered_pword(ios) = new value_type(value);
            }
#endif

        private:
            static void*& registered_pword(std::ios_base& ios)
            {
                void*& p = pword(ios);
                if (0 != p)
                {
                    delete static_cast<value_type*>(p);
                    p = 0;
                }
                else
                {
                    ios.register_callback(&ios_pword_stasher::event_callback, index);
                }
                return p;
            }

            static void event_callback(std::ios_base::event event, std::ios_base& ios, int index)
            {
                // assert ios_pword_stasher::index == index?
                void*& p = pword(ios);
                if (0 != p)
                {
                    if (std::ios_base::copyfmt_event == event)
                    {
                        p = new value_type(*static_cast<value_type*>(p));
                    }
                    else if (std::ios_base::erase_event == event)
                    {
                        delete static_cast<value_type*>(p);
                        p = 0;
                    }
                }
            }

            value_type value;

            static const int index;
        };

        template <typename StashTag>
        const int ios_pword_stasher<StashTag>::index = std::ios_base::xalloc();

        template <typename StashTag>
        class ios_iword_stasher
        {
        public:
            static const bool is_iword = true;
            typedef typename StashTag::type value_type;

            explicit ios_iword_stasher(const value_type& value) : value(value) {}

            static bool has(const std::ios_base& ios)
            {
                // use pword to indicate presence
                return 0 != pword(ios);
            }

            static void clear(std::ios_base& ios)
            {
                pword(ios) = 0;
                iword(ios) = 0;
            }

            static value_type get(const std::ios_base& ios)
            {
                if (0 != pword(ios))
                {
PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_FORCE_TO_BOOL
                    return static_cast<value_type>(iword(ios));
PRAGMA_WARNING_POP
                }
                throw std::bad_cast();
            }

            static value_type get(const std::ios_base& ios, const value_type& default_value)
            {
                if (0 != pword(ios))
                {
PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_FORCE_TO_BOOL
                    return static_cast<value_type>(iword(ios));
PRAGMA_WARNING_POP
                }
                return default_value;
            }

            template <typename CharT, typename TraitsT>
            static std::basic_ostream<CharT, TraitsT>& insert(std::basic_ostream<CharT, TraitsT>& os)
            {
                if (0 != pword(os))
                {
PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_FORCE_TO_BOOL
                    return os << static_cast<value_type>(iword(os));
PRAGMA_WARNING_POP
                }
                // set failbit?
                return os;
            }

            static void*& pword(std::ios_base& ios)
            {
                init_stash(ios);
                return ios.pword(index);
            }

            static const void* pword(const std::ios_base& ios)
            {
                init_stash(const_cast<std::ios_base&>(ios));
                return const_cast<std::ios_base&>(ios).pword(index);
            }

            static long& iword(std::ios_base& ios)
            {
                return ios.iword(index);
            }

            static long iword(const std::ios_base& ios)
            {
                return const_cast<std::ios_base&>(ios).iword(index);
            }

            void operator()(std::ios_base& ios) const
            {
                iword(ios) = static_cast<long>(value);
                // use pword to indicate presence
                pword(ios) = reinterpret_cast<void*>(-1);
            }

        private:
            value_type value;

            static const int index;
        };

        template <typename StashTag>
        const int ios_iword_stasher<StashTag>::index = std::ios_base::xalloc();

        template <typename ValueType>
        struct is_iword_type : std::integral_constant<bool, sizeof(long) >= sizeof(ValueType)
            && (std::is_integral<ValueType>::value || std::is_enum<ValueType>::value)>
        {};

        template <typename StashTag>
        struct ios_stasher_base : std::conditional<
            is_iword_type<typename StashTag::type>::value,
                ios_iword_stasher<StashTag>,
                ios_pword_stasher<StashTag>>
        {};
    }

    template <typename StashTag>
    class ios_stasher : public detail::ios_stasher_base<StashTag>::type
    {
    public:
        typedef typename detail::ios_stasher_base<StashTag>::type base_type;
        typedef typename base_type::value_type value_type;

        explicit ios_stasher(const value_type& value) : base_type(value) {}
        explicit ios_stasher(value_type&& value) : base_type(std::move(value)) {}

    private:
        template <typename CharT, typename TraitsT>
        friend std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& os, const ios_stasher& stasher)
        {
            stasher(os);
            return os;
        }

        template <typename CharT, typename TraitsT>
        friend std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& os, ios_stasher&& stasher)
        {
            std::move(stasher)(os);
            return os;
        }

        template <typename CharT, typename TraitsT>
        friend std::basic_istream<CharT, TraitsT>& operator>>(std::basic_istream<CharT, TraitsT>& is, const ios_stasher& stasher)
        {
            stasher(is);
            return is;
        }

        template <typename CharT, typename TraitsT>
        friend std::basic_istream<CharT, TraitsT>& operator>>(std::basic_istream<CharT, TraitsT>& is, ios_stasher&& stasher)
        {
            std::move(stasher)(is);
            return is;
        }
    };
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/scopes.h"
#ifndef SLOG_SCOPES_H
#define SLOG_SCOPES_H

#include <algorithm> // for std::transform
#include <deque>
// Amalgamated: #include "slog/config.h"
// Amalgamated: #include "slog/copyable_log_message.h"
// Amalgamated: #include "slog/declspec.h"
// Amalgamated: #include "slog/ios_stasher.h"

namespace slog
{
    namespace scopes
    {
        typedef copyable_log_message scope_message;
        typedef std::deque<scope_message> scope_message_stack;
        // Note: Are these definitions a good idea or would unique user-defined types be better?

        // Thread-specific stack of scopes, similar to a call stack, with both pros and cons!
        SLOG_API scope_message_stack& scope_stack();
        inline scope_message innermost_scope() { return !scope_stack().empty() ? scope_stack().back() : scope_message("no scope"); }
        inline scope_message outermost_scope() { return !scope_stack().empty() ? scope_stack().front() : scope_message("no scope"); }
        // Note: What's the right way of handling out-of-scope?
        // Is returning by value from inner/outermost_scope() the right approach? Is there a better implementation?

        class scope
        {
        public:
            explicit scope(const scope_message& message)
            {
                scope_stack().push_back(message);
            }
            ~scope()
            {
                scope_stack().pop_back();
            }
        private:
            // Non-copyable
            scope(const scope&);
            const scope& operator=(const scope&);
        };

        // The indent manipulator can be used to indent log messages according to the scope
        template <typename CharT, typename TraitsT>
        std::basic_ostream<CharT, TraitsT>& indent(std::basic_ostream<CharT, TraitsT>& os);

        // The setindent manipulator is used to change the indent string
        namespace detail { struct indent_tag : slog::stash_tag<std::string> {}; }
        inline ios_stasher<detail::indent_tag> setindent(const std::string& single_indent)
        {
            return ios_stasher<detail::indent_tag>(single_indent);
        }

        // Note: Could the indent be a string type of the same char_type as the stream, rather than widened?
        // Would a template tag or some sort of tag rebind mechanism help?
        namespace detail
        {
            template <typename CharT, typename TraitsT>
            inline std::basic_string<CharT, TraitsT> widen(std::basic_ostream<CharT, TraitsT>& os, const std::string& s)
            {
                std::basic_string<CharT, TraitsT> ws;
                ws.reserve(s.size());
                std::transform(s.begin(), s.end(), std::back_inserter(ws), [&os](char c){ return os.widen(c); });
                return ws;
            }
            template <>
            inline std::string widen(std::ostream& os, const std::string& s)
            {
                return s;
            }
        }

        template <typename CharT, typename TraitsT>
        inline std::basic_ostream<CharT, TraitsT>& indent(std::basic_ostream<CharT, TraitsT>& os)
        {
            const std::basic_string<CharT, TraitsT> single_indent = detail::widen(os, get_stash<detail::indent_tag>(os, "\t"));
            for (std::size_t i = scope_stack().size(); 0 < i; --i)
            {
                os << single_indent;
            }
            return os;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// Scope construction

// Implementation detail - access the current scope_message
#define SLOG_DETAIL_SCOPE_STREAM() slog::scopes::scope_stack().back().stream()

// Implementation detail - make a per-source file unique symbol based on the line number (so don't use it more than once on a line!)
#define SLOG_DETAIL_SYMBOL_FOR_LINE(symbol, line) SLOG_DETAIL_CONCAT(symbol, line)
#define SLOG_DETAIL_CONCAT(a, b) a##b

// Simple construction with a name for the message
#define SLOG_SCOPE(name) const slog::scopes::scope SLOG_DETAIL_SYMBOL_FOR_LINE(slog_scope_, __LINE__)(slog::scopes::scope_message(SLOG_CURRENT_FILE, SLOG_CURRENT_LINE, SLOG_CURRENT_FUNCTION, slog::nil_severity, name))

// Construction from the current function name
#define SLOG_FUNCTION_SCOPE() SLOG_SCOPE(SLOG_CURRENT_FUNCTION)

// Construction with additional message
#define SLOG_SCOPE_MESSAGE(name) SLOG_SCOPE(name); SLOG_DETAIL_SCOPE_STREAM()
#define SLOG_FUNCTION_SCOPE_MESSAGE() SLOG_FUNCTION_SCOPE(); SLOG_DETAIL_SCOPE_STREAM()

// Notes:
// 1. Is there a way of writing SLOG_SCOPE(name) to allow you to stream more stuff into the scope_message directly?
// 2. Are the SLOG_FUNCTION_SCOPE macros actually useful? The function name is already captured of course...
// 3. What about some additional macros?
//#define SLOG_OUTERMOST_SCOPE slog::scopes::outermost_scope().str()
//#define SLOG_INNERMOST_SCOPE slog::scopes::innermost_scope().str()
//#define SLOG_SCOPE_STACK ... hmm, custom insertion operator for scope_message_stack?
// See proposed Boost.RangeIO or https://github.com/DarkerStar/cpp-range-streaming.

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog/log_manip.h"
#ifndef SLOG_LOG_MANIP_H
#define SLOG_LOG_MANIP_H

#include <iomanip>
// Amalgamated: #include "slog/async_log_message.h"
// Amalgamated: #include "slog/severities.h"
// Amalgamated: #include "slog/stringf.h"

namespace slog
{
    // These manipulators may serve as inspiration for log message formatting
    // even when they don't provide the required format directly

    struct put_severity_name
    {
        explicit put_severity_name(severity level) : level(level) {}
        friend std::ostream& operator<<(std::ostream& os, const put_severity_name& manip)
        {
            switch (manip.level)
            {
            case severities::fatal: return os << "fatal error";
            case severities::severe: return os << "severe error";
            case severities::error: return os << "error";
            case severities::warning: return os << "warning";
            case severities::info: return os << "info";
            case severities::more_info: return os << "more info";
            case severities::too_much_info: return os << "too much info";
            default: return os << "severity level " << manip.level;
            }
        }
        const severity level;
    };

    // This is an example of formatting a log message (used by the default global log handler)
    // that integrates well with the build process of many IDEs, including Visual Studio
    struct put_build_log_message
    {
        explicit put_build_log_message(const log_message& message) : message(message) {}

        friend std::ostream& operator<<(std::ostream& os, const put_build_log_message& manip)
        {
            return os
#ifdef _MSC_VER
                << manip.message.file() << "(" << manip.message.line() << "): "
#else // GCC, Clang
                << manip.message.file() << ":" << manip.message.line() << ": "
#ifdef __APPLE__
                // Xcode recognizes severities of "error", "warning" and "note", everything else is treated as an error
                << (manip.message.level() <= severities::info ? "note: " : "")
#endif
#endif
                << put_severity_name(manip.message.level()) << ": "
                << manip.message.function() << " : "
                << manip.message.str();
        }

        const log_message& message;
    };

    namespace detail
    {
        // Find the next printf-style conversion specification in format starting at or after off
        // * if no conversion specification is found, the result is { size, 0 }
        // * if an incomplete specification is found, the result is { offset of incomplete specification, 0 }
        // * otherwise, the result is { offset of specification, size of specification }
        // Notes:
        // * width or precision of '*' is not supported, neither are length modifiers
        // * original motivation was to avoid bst::regex dependency
        inline std::pair<std::string::size_type, std::string::size_type> find_conversion_specification(const std::string& format, std::string::size_type off = 0u)
        {
            enum state_index { intro, flag, width, point, precision, specifier };

            struct transition
            {
                bool consume; // if false, transition to next state but with current input
                state_index next;
            };

            struct state_transitions
            {
                state_index current; // redundant
                std::string chars_to_find; // select transition depending on whether current input is found in these chars
                transition found;
                transition not_found;
            };

            const state_transitions state_table[] =
            {
                { intro,        "%",            { true, flag },         { true, intro } },
                { flag,         "-+0 #",        { true, flag },         { false, width } },
                { width,        "0123456789",   { true, width },        { false, point } },
                { point,        ".",            { true, precision },    { false, specifier } },
                { precision,    "0123456789",   { true, precision },    { false, specifier } },
                { specifier,    "",             { true, intro },        { true, intro } }
            };

            state_index current = intro;

            const std::string::size_type size = format.size();
            std::string::size_type coff = off;

            while (off != size)
            {
                const state_transitions& current_state = state_table[current];
                const bool found = std::string::npos != current_state.chars_to_find.find(format[off]);
                const transition& found_or_not = found ? current_state.found : current_state.not_found;

                if (found_or_not.consume) ++off;
                if (specifier == current) return { coff, off - coff };
                current = found_or_not.next;
                if (intro == current) coff = off;
            }
            return { coff, 0u };
        }
    }

    struct put_timestamp
    {
        explicit put_timestamp(const async_log_message::time_point& timestamp, const char* format = "%Y-%m-%d %H:%M:%06.3S")
            : timestamp(timestamp)
            , format(format)
        {}

        friend std::ostream& operator<<(std::ostream& os, const put_timestamp& manip)
        {
            // Truncate to whole seconds and convert to UTC
            const auto secs = bst::chrono::time_point_cast<bst::chrono::seconds>(manip.timestamp);
            const std::time_t secs_time = bst::chrono::system_clock::to_time_t(secs);
            std::tm secs_tm;
#ifdef _WIN32
            gmtime_s(&secs_tm, &secs_time);
#else
            gmtime_r(&secs_time, &secs_tm);
#endif
            // Get fractional seconds component
            const bst::chrono::duration<double> secs_fract = manip.timestamp - secs;

            // Look for (just one...) custom seconds format specifier with %f compatible padding/width/precision
            // and use std::put_time for everything else
            const std::string format(manip.format);

            std::pair<std::string::size_type, std::string::size_type> spec = detail::find_conversion_specification(format);
            while (spec.second != 0 && 'S' != format[spec.first + spec.second - 1])
            {
                spec = detail::find_conversion_specification(format, spec.first + spec.second);
            }

            if (spec.second > 2) // not just "%S"
            {
                const std::string prefix_format = format.substr(0, spec.first);
                const std::string secs_format = format.substr(spec.first, spec.second - 1) + 'f';
                const std::string suffix_format = format.substr(spec.first + spec.second);
                put_time(os, secs_tm, prefix_format);
                os << stringf(secs_format.c_str(), secs_tm.tm_sec + secs_fract.count());
                return put_time(os, secs_tm, suffix_format);
            }
            else
            {
                return put_time(os, secs_tm, manip.format);
            }
        }

        const async_log_message::time_point& timestamp;
        const char* format;

    private:
        // libstdc++ has std::time_put, but not std::put_time until GCC 5
        // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54354
        // and Visual Studio 2012 doesn't like const std::tm*
        // See https://connect.microsoft.com/VisualStudio/feedback/details/773846
        static std::ostream& put_time(std::ostream& os, std::tm& t, const std::string& format)
        {
#ifndef __GLIBCXX__
            return os << std::put_time(&t, format.c_str());
#else
            return put_time(os, t, format.c_str(), format.length());
#endif
        }

        static std::ostream& put_time(std::ostream& os, std::tm& t, const char* format)
        {
#ifndef __GLIBCXX__
            return os << std::put_time(&t, format);
#else
            return put_time(os, t, format, std::char_traits<char>::length(format));
#endif
        }

#ifdef __GLIBCXX__
        static std::ostream& put_time(std::ostream& os, std::tm& t, const char* format, std::size_t format_length)
        {
            bool bad = false;
            try
            {
                bad = std::use_facet<std::time_put<char>>(os.getloc())
                    .put(std::time_put<char>::iter_type(os.rdbuf()), os, os.fill(), &t, format, format + format_length)
                    .failed();
            }
            catch (...) { os.setstate(std::ios_base::badbit); throw; }
            if (bad) os.setstate(std::ios_base::badbit);
            return os;
        }
#endif
    };

    // Another example log line format, from google-glog LogSink::ToString
    // ("Lmmdd hh:mm:ss.uuuuuu threadid file:line] msg...")

    struct put_glog_severity_code
    {
        explicit put_glog_severity_code(severity level) : level(level) {}
        friend std::ostream& operator<<(std::ostream& os, const put_glog_severity_code& manip)
        {
            if (severities::fatal <= manip.level) return os << 'F';
            else if (severities::error <= manip.level) return os << 'E';
            else if (severities::warning <= manip.level) return os << 'W';
            else return os << 'I';
        }
        const severity level;
    };

    struct put_glog_message
    {
        explicit put_glog_message(const async_log_message& message) : message(message) {}

        friend std::ostream& operator<<(std::ostream& os, const put_glog_message& manip)
        {
            return os
                << put_glog_severity_code(manip.message.level())
                << put_timestamp(manip.message.timestamp(), "%m%d %H:%M:%09.6S")
                << ' ' << std::setfill(' ') << std::setw(5) << manip.message.thread_id() << std::setfill('0')
                << ' ' << manip.message.file()
                << ':' << manip.message.line() << ']'
                << ' ' << manip.message.str();
        }

        const async_log_message& message;
    };
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Amalgamating: #include "slog\all.h"
#ifndef SLOG_ALL_H
#define SLOG_ALL_H

////////////////////////////////////////////////////////////////////////////////////////////
// Core

// Configuration
// Amalgamated: #include "slog/config.h"

// Fundamentals (depends on configuration)
// Amalgamated: #include "slog/log_message.h"
// Amalgamated: #include "slog/severity_type.h"

// Polymorphic logging gateway (depends on fundamentals) 
// Amalgamated: #include "slog/base_gate.h"

// Copyable log message (depends on fundamentals)
// Amalgamated: #include "slog/copyable_log_message.h"

// Manipulators created from e.g. lambda functions
// Amalgamated: #include "slog/iomanip.h"

// Template-based API (depends on fundamentals, manipulators)
#ifdef SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API
//#include "slog/detail/hit_count.h"
//#include "slog/detail/optional_log_message.h"
//#include "slog/detail/uncaught_exceptions.h"
// Amalgamated: #include "slog/log_statement_type.h"
#endif

// Typical severity/verbosity levels (depends on fundamentals)
#ifdef SLOG_PROVIDES_SEVERITIES
// Amalgamated: #include "slog/severities.h"
#endif

// Export configuration for non-header-only library code
// Amalgamated: #include "slog/declspec.h"

// Support for printf-style logging
// Amalgamated: #include "slog/stringf.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Built-in logging gateways

// Logging gateways core (depends on fundamentals)
//#include "slog/detail/static_const.h"
// Amalgamated: #include "slog/log_function_type.h"
// Amalgamated: #include "slog/log_handler_type.h"

// Global logging gateway (depends on logging gateways core, export configuration)
// Amalgamated: #include "slog/global/log.h"
// Amalgamated: #include "slog/global/log_function.h"
// Amalgamated: #include "slog/global/log_gate.h"
// Amalgamated: #include "slog/global/log_handler.h"
#ifdef SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API
// Amalgamated: #include "slog/global/log_statement.h" // (also depends on template-based API)
#endif
// Amalgamated: #include "slog/global/severity.h"

// Thread-local logging gateway (depends on logging gateways core, export configuration)
// Amalgamated: #include "slog/this_thread/log.h"
// Amalgamated: #include "slog/this_thread/log_function.h"
// Amalgamated: #include "slog/this_thread/log_gate.h"
#ifdef SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API
// Amalgamated: #include "slog/this_thread/log_statement.h" // (also depends on template-based API)
#endif
// Amalgamated: #include "slog/this_thread/severity.h"

// Logging in namespace slog (depends on global logging gateway)
#ifdef SLOG_PROVIDES_LOGGING_IN_NAMESPACE_SLOG
// Amalgamated: #include "slog/log.h"
// Amalgamated: #include "slog/log_function.h"
// Amalgamated: #include "slog/log_handler.h"
#ifdef SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API
// Amalgamated: #include "slog/log_statement.h" // (also depends on template-based API)
#endif
// Amalgamated: #include "slog/severity.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Front-end API for logging statements (depends on core, built-in logging gateways)
#ifndef SLOG_DETAIL_PROVIDES_TEMPLATE_BASED_MACRO_API
//#include "slog/detail/impertinent.h" // for macro-based API
#endif
// Amalgamated: #include "slog/slog.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Asynchronous logging (depends on fundamentals, copyable log message)

// Amalgamated: #include "slog/async_log_message.h"
// Amalgamated: #include "slog/async_log_service.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Other extensions

//#include "slog/detail/function_service.h"

// Type-safe heterogeneous storage in a stream
// Amalgamated: #include "slog/ios_stasher.h"

// Logging scopes (depends on copyable log message, export configuration, asynchronous logging, ios_stasher)
// Amalgamated: #include "slog/scopes.h"

// Manipulators for log message formatting (depends on printf-style logging, typical severity/verbosity levels, asynchronous logging)
// Amalgamated: #include "slog/log_manip.h"

#endif

#endif

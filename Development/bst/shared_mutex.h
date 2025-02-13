#ifndef BST_SHARED_MUTEX_H
#define BST_SHARED_MUTEX_H

// Provide bst::shared_mutex, bst::shared_lock etc. using either std:: or boost:: symbols

#if !defined(BST_SHARED_MUTEX_STD) && !defined(BST_SHARED_MUTEX_BOOST)
// To do: Detect whether std::shared_mutex is available using __cplusplus (and compiler/library-specific preprocessor definitions)
// Note: If this isn't defined under the same condition as BST_THREAD_BOOST, adding shared_timed_mutex is problematic
// because its timeout functionality won't be consistent with bst::chrono
#define BST_SHARED_MUTEX_BOOST
#endif

#ifndef BST_SHARED_MUTEX_BOOST

#include <condition_variable>
#include <shared_mutex>
namespace bst_shared_mutex = std;
namespace bst_shared_mutex_condition_variable_any = std;

#else

#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
namespace bst_shared_mutex = boost;

#if defined(_WIN32)
// note, Windows boost::condition_variable_any::wait would throw a lock expectation when boost::shared_mutex has reached the
// maximum number of exclusive_waiting locks. Unfortunately, the standard boost::condition_variable_any's relocker
// destructor could also throw a lock exception. This could cause program termination due to unhandled exceptions by the
// boost::condition_variable_any's do_wait_until(...)
#include "boost/thread/win32_condition_variable.hpp"
namespace bst_shared_mutex_condition_variable_any = boost::experimental;
#else
#include <boost/thread/condition_variable.hpp>
namespace bst_shared_mutex_condition_variable_any = boost;
#endif

#endif

namespace bst
{
    using bst_shared_mutex::shared_mutex;
    using bst_shared_mutex::shared_lock;
    using bst_shared_mutex_condition_variable_any::condition_variable_any;
    using bst_shared_mutex::nano;
    using bst_shared_mutex::cv_status;
}

#endif

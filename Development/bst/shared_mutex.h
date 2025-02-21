#ifndef BST_SHARED_MUTEX_H
#define BST_SHARED_MUTEX_H

// Provide bst::shared_mutex, bst::shared_lock etc. using either std:: or boost:: symbols

#ifndef BST_THREAD_BOOST

#include <condition_variable>
#include <shared_mutex>
namespace bst_shared_mutex = std;
namespace bst_shared_mutex_condition_variable_any = std;

#else

#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
namespace bst_shared_mutex = boost;

#ifdef _WIN32
// Note:: Windows boost::condition_variable_any::wait would throw nested lock exceptions when boost::shared_mutex has reached the
// maximum number of exclusive_waiting locks.A modified version of the boost::condition_variable_any was created to address this issue.
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

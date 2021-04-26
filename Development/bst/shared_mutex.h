#ifndef BST_SHARED_MUTEX_H
#define BST_SHARED_MUTEX_H

// Provide bst::shared_mutex, bst::shared_lock etc. using either std:: or boost:: symbols
// To do: Detect whether std::shared_mutex is available using __cplusplus (and compiler/library-specific preprocessor definitions)

// Note: If this isn't defined under the same condition as BST_THREAD_BOOST, adding shared_timed_mutex is problematic
// because its timeout functionality won't be consistent with bst::chrono
#ifndef BST_SHARED_MUTEX_BOOST

#include <shared_mutex>
namespace bst_shared_mutex = std;

#else

#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
namespace bst_shared_mutex = boost;

#endif

namespace bst
{
    using bst_shared_mutex::shared_mutex;
    using bst_shared_mutex::shared_lock;
}

#endif

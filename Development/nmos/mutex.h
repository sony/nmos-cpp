#ifndef NMOS_MUTEX_H
#define NMOS_MUTEX_H

#include <condition_variable>
#include "bst/shared_mutex.h"

namespace nmos
{
    typedef bst::shared_mutex mutex;

    typedef bst::shared_lock<mutex> read_lock;
    typedef std::unique_lock<mutex> write_lock;

    // locking strategy tag structs must be usable for both read_lock and write_lock
#ifndef BST_SHARED_MUTEX_BOOST
    typedef std::adopt_lock_t adopt_lock_t;
    typedef std::defer_lock_t defer_lock_t;
    typedef std::try_to_lock_t try_to_lock_t;
    using std::adopt_lock;
    using std::defer_lock;
    using std::try_to_lock;
#else
    struct adopt_lock_t : boost::adopt_lock_t, std::adopt_lock_t {};
    struct defer_lock_t : boost::defer_lock_t, std::defer_lock_t {};
    struct try_to_lock_t : boost::try_to_lock_t, std::try_to_lock_t {};
    const adopt_lock_t adopt_lock;
    const defer_lock_t defer_lock;
    const try_to_lock_t try_to_lock;
#endif

    typedef std::condition_variable_any condition_variable;
    typedef std::cv_status cv_status;

    template <typename Func>
    auto with_read_lock(nmos::mutex& mutex, Func&& func) -> decltype(func())
    {
        nmos::read_lock lock(mutex);
        return func();
    }

    template <typename Func>
    auto with_write_lock(nmos::mutex& mutex, Func&& func) -> decltype(func())
    {
        nmos::write_lock lock(mutex);
        return func();
    }
}

#endif

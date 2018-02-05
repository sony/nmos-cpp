#ifndef NMOS_MUTEX_H
#define NMOS_MUTEX_H

#include <condition_variable>
#include "bst/shared_mutex.h"

namespace nmos
{
    typedef bst::shared_mutex mutex;

    typedef bst::shared_lock<mutex> read_lock;
    typedef std::unique_lock<mutex> write_lock;

    typedef std::condition_variable_any condition_variable;
}

#endif

#ifndef NMOS_MUTEX_H
#define NMOS_MUTEX_H

#include <condition_variable>

namespace nmos
{
    typedef std::mutex mutex;

    typedef std::unique_lock<mutex> read_lock;
    typedef std::unique_lock<mutex> write_lock;

    typedef std::condition_variable condition_variable;
}

#endif

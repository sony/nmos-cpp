#ifndef NMOS_COPYABLE_ATOMIC_H
#define NMOS_COPYABLE_ATOMIC_H

#include <atomic>

namespace nmos
{
    namespace details
    {
        // Before using this, understand why std::atomic<T> *isn't* copyable or movable
        // See e.g. https://stackoverflow.com/questions/15249998/why-are-stdatomic-objects-not-copyable
        template <typename T>
        struct copyable_atomic : public std::atomic<T>
        {
            copyable_atomic() {}
            copyable_atomic(T value) : std::atomic<T>(value) {}
            copyable_atomic(const copyable_atomic& other) : std::atomic<T>(other.load()) {}
            copyable_atomic& operator=(const copyable_atomic& other) { if (this != &other) this->store(other.load()); return *this; }
        };
    }
}

#endif

#ifndef NMOS_THREAD_UTILS_H
#define NMOS_THREAD_UTILS_H

#include <thread>

namespace nmos
{
    // Utility types, constants and functions extending std functionality (could be extracted from the nmos module)
    namespace details
    {
        // temporarily unlock a mutex
        template <typename BasicLockable>
        class reverse_lock_guard
        {
        public:
            typedef BasicLockable mutex_type;
            explicit reverse_lock_guard(mutex_type& m) : m(m) { m.unlock(); }
            ~reverse_lock_guard() { m.lock(); }
            reverse_lock_guard(const reverse_lock_guard&) = delete;
            reverse_lock_guard& operator=(const reverse_lock_guard&) = delete;
        private:
            mutex_type& m;
        };

        // std::condition_variable::wait_until when max time_point is specified seems unreliable
        // note, all parameters are template parameters in order to also handle e.g. boost::condition_variable_any
        template <typename ConditionVariable, typename Lock, typename TimePoint, typename Predicate>
        inline bool wait_until(ConditionVariable& condition, Lock& lock, const TimePoint& tp, Predicate predicate)
        {
            for (;;)
            {
                // Note: the try-catch block is here because Windows boost::condition_variable_any::wait can throw
                // an exception once boost::shared_mutex has reached the maximum number of exclusive_waiting locks
                try
                {
                    if ((TimePoint::max)() == tp)
                    {
                        condition.wait(lock, predicate);
                        return true;
                    }
                    else
                    {
                        return condition.wait_until(lock, tp, predicate);
                    }
                }
                catch (...)
                {
                    // try the wait again
                }
            }
        }

        template <typename ConditionVariable, typename Lock, typename Rep, typename Period, typename Predicate>
        inline bool wait_for(ConditionVariable& condition, Lock& lock, const bst::chrono::duration<Rep, Period>& duration, Predicate predicate)
        {
            if ((bst::chrono::duration<Rep, Period>::max)() == duration)
            {
                condition.wait(lock, predicate);
                return true;
            }
            else
            {
                // using wait_until as a workaround for bug in VS2015, resolved in VS2017
                // see https://developercommunity.visualstudio.com/content/problem/274532/bug-in-visual-studio-2015-implementation-of-stdcon.html
                return condition.wait_until(lock, bst::chrono::steady_clock::now() + duration, predicate);
            }
        }

        template <typename ConditionVariable, typename Lock, typename TimePoint>
        inline auto wait_until(ConditionVariable& condition, Lock& lock, const TimePoint& tp) -> decltype(condition.wait_until(lock, tp))
        {
            if ((TimePoint::max)() == tp)
            {
                condition.wait(lock);
                typedef decltype(condition.wait_until(lock, tp)) cv_status;
                return cv_status::no_timeout;
            }
            else
            {
                return condition.wait_until(lock, tp);
            }
        }

        template <typename ConditionVariable, typename Lock, typename Rep, typename Period>
        inline auto wait_for(ConditionVariable& condition, Lock& lock, const bst::chrono::duration<Rep, Period>& duration) -> decltype(condition.wait_for(lock, duration))
        {
            if ((bst::chrono::duration<Rep, Period>::max)() == duration)
            {
                condition.wait(lock);
                typedef decltype(condition.wait_for(lock, duration)) cv_status;
                return cv_status::no_timeout;
            }
            else
            {
                return condition.wait_until(lock, bst::chrono::steady_clock::now() + duration);
            }
        }

        // RAII helper for starting and later joining threads which may require unblocking before join
        template <typename Function, typename PreJoin>
        class thread_guard
        {
        public:
            thread_guard(Function f, PreJoin pre_join) : thread(f), pre_join(pre_join) {}
            thread_guard(thread_guard&& rhs) : thread(std::move(rhs.thread)), pre_join(std::move(rhs.pre_join)) {}
            ~thread_guard() { pre_join(); if (thread.joinable()) thread.join(); }
            thread_guard(const thread_guard&) = delete;
            thread_guard& operator=(const thread_guard&) = delete;
            thread_guard& operator=(thread_guard&&) = delete;
        private:
            std::thread thread;
            PreJoin pre_join;
        };

        template <typename Function, typename PreJoin>
        inline thread_guard<Function, PreJoin> make_thread_guard(Function f, PreJoin pj)
        {
            return{ f, pj };
        }
    }
}

#endif

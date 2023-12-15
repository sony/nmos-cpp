#ifndef PPLX_PPLX_UTILS_H
#define PPLX_PPLX_UTILS_H

#include <chrono>
#include <boost/range/adaptor/transformed.hpp>
#include "pplx/pplxtasks.h"

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) && !CPPREST_FORCE_PPLX
namespace Concurrency // since namespace pplx = Concurrency
#else
namespace pplx
#endif
{
    // Creates a task that completes after a specified amount of time.
    // milliseconds: The number of milliseconds after which the task should complete.
    // token: Cancellation token for cancellation of this operation.
    // Because the scheduler is cooperative in nature, the delay before the task completes could be longer than the specified amount of time.
    pplx::task<void> complete_after(unsigned int milliseconds, const pplx::cancellation_token& token = pplx::cancellation_token::none());

    // Creates a task that completes after a specified amount of time.
    // duration: The amount of time (milliseconds and up) after which the task should complete.
    // token: Cancellation token for cancellation of this operation.
    // Because the scheduler is cooperative in nature, the delay before the task completes could be longer than the specified amount of time.
    template <typename Rep, typename Period>
    inline pplx::task<void> complete_after(const std::chrono::duration<Rep, Period>& duration, const pplx::cancellation_token& token = pplx::cancellation_token::none())
    {
        return duration > std::chrono::duration<Rep, Period>::zero()
            ? complete_after((unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), token)
            : pplx::task_from_result();
    }

    // Creates a task that completes at a specified time.
    // time: The time point at which the task should complete.
    // token: Cancellation token for cancellation of this operation.
    // Because the scheduler is cooperative in nature, the time at which the task completes could be after the specified time.
    template <typename Clock, typename Duration = typename Clock::duration>
    inline pplx::task<void> complete_at(const std::chrono::time_point<Clock, Duration>& time, const pplx::cancellation_token& token = pplx::cancellation_token::none())
    {
        return complete_after(time - Clock::now(), token);
    }

    // Creates a task for an asynchronous do-while loop. Executes a task repeatedly, until the returned condition value becomes false.
    // create_iteration_task: This function should create a task that performs the loop iteration and returns the Boolean value of the loop condition.
    // token: Cancellation token for cancellation of the do-while loop.
    pplx::task<void> do_while(const std::function<pplx::task<bool>()>& create_iteration_task, const pplx::cancellation_token& token = pplx::cancellation_token::none());

    // Returns true if the task is default constructed.
    // A default constructed task cannot be used until you assign a valid task to it. Methods such as <c>get</c>, <c>wait</c> or <c>then</c>
    // will throw an <see cref="invalid_argument Class">invalid_argument</see> exception when called on a default constructed task.
    template <typename ReturnType>
    bool empty(const pplx::task<ReturnType>& task)
    {
        return pplx::task<ReturnType>() == task;
    }

    namespace details
    {
        template <typename ReturnType>
        void wait_nothrow(const pplx::task<ReturnType>& task)
        {
            try
            {
                task.wait();
            }
            catch (...) {}
        }
    }

    struct exception_observer
    {
        template <typename ContinuationReturnType>
        void operator()(pplx::task<ContinuationReturnType> finally) const
        {
            details::wait_nothrow(finally);
        }
    };

    // Silently 'observe' any exception thrown from a task.
    // Exceptions that are unobserved when a task is destructed will terminate the process.
    // Add this as a continuation to silently swallow all exceptions.
    inline exception_observer observe_exception()
    {
        return exception_observer();
    }

    namespace details
    {
        // see http://ericniebler.com/2013/08/07/universal-references-and-the-copy-constructo/
        template<typename A, typename B>
        using disable_if_same_or_derived =
            typename std::enable_if<
                !std::is_base_of<A,typename
                    std::remove_reference<B>::type
                >::value
            >::type;
    }

    template <typename ReturnType>
    struct exceptions_observer
    {
        template <typename InputRange, typename X = pplx::details::disable_if_same_or_derived<exceptions_observer, InputRange>>
        explicit exceptions_observer(InputRange&& tasks) : tasks(tasks.begin(), tasks.end()) {}

        template <typename InputIterator>
        exceptions_observer(InputIterator&& first, InputIterator&& last) : tasks(std::forward<InputIterator>(first), std::forward<InputIterator>(last)) {}

        template <typename ContinuationReturnType>
        void operator()(pplx::task<ContinuationReturnType> finally) const
        {
            for (auto& task : tasks) details::wait_nothrow(task);
            details::wait_nothrow(finally);
        }

    private:
        std::vector<pplx::task<ReturnType>> tasks;
    };

    // Silently 'observe' all exceptions thrown from a range of tasks.
    // Exceptions that are unobserved when a task is destructed will terminate the process.
    // Add this as a continuation to silently swallow all exceptions.
    template <typename InputRange, typename ReturnType = typename std::iterator_traits<decltype(std::declval<InputRange>().begin())>::value_type::result_type>
    inline exceptions_observer<ReturnType> observe_exceptions(InputRange&& tasks)
    {
        return exceptions_observer<ReturnType>(std::forward<InputRange>(tasks));
    }

    // Silently 'observe' all exceptions thrown from a range of tasks.
    // Exceptions that are unobserved when a task is destructed will terminate the process.
    // Add this as a continuation to silently swallow all exceptions.
    template <typename InputIterator, typename ReturnType = typename std::iterator_traits<InputIterator>::value_type::result_type>
    inline exceptions_observer<ReturnType> observe_exceptions(InputIterator&& first, InputIterator&& last)
    {
        return exceptions_observer<ReturnType>(std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    namespace details
    {
        template <typename ReturnType>
        struct workaround_default_task
        {
            pplx::task<ReturnType> operator()(pplx::task<ReturnType> task) const
            {
                if (!pplx::empty(task)) return task;
                // convert default constructed tasks into ones that pplx::when_{all,any} can handle
                // see https://github.com/microsoft/cpprestsdk/issues/1701
                try
                {
                    task.wait();
                    // unreachable code
                    return task;
                }
                catch (const pplx::invalid_operation& e)
                {
                    auto workaround = pplx::task_from_exception<ReturnType>(e);
                    details::wait_nothrow(workaround);
                    return workaround;
                }
            }
        };
    }

    namespace ranges
    {
        namespace details
        {
            template <typename InputRange>
            auto when_all(InputRange&& tasks, const pplx::task_options& options = pplx::task_options())
                -> decltype(pplx::when_all(tasks.begin(), tasks.end(), options))
            {
                return pplx::when_all(tasks.begin(), tasks.end(), options);
            }
        }

        template <typename InputRange>
        auto when_all(InputRange&& tasks, const pplx::task_options& options = pplx::task_options())
            -> decltype(pplx::when_all(tasks.begin(), tasks.end(), options))
        {
            using ReturnType = typename std::iterator_traits<decltype(tasks.begin())>::value_type::result_type;
            return pplx::ranges::details::when_all(tasks | boost::adaptors::transformed(pplx::details::workaround_default_task<ReturnType>()));
        }

        namespace details
        {
            template <typename InputRange>
            auto when_any(InputRange&& tasks, const pplx::task_options& options = pplx::task_options())
                -> decltype(pplx::when_any(tasks.begin(), tasks.end(), options))
            {
                return pplx::when_any(tasks.begin(), tasks.end(), options);
            }
        }

        template <typename InputRange>
        auto when_any(InputRange&& tasks, const pplx::task_options& options = pplx::task_options())
            -> decltype(pplx::when_any(tasks.begin(), tasks.end(), options))
        {
            using ReturnType = typename std::iterator_traits<decltype(tasks.begin())>::value_type::result_type;
            return pplx::ranges::details::when_any(tasks | boost::adaptors::transformed(pplx::details::workaround_default_task<ReturnType>()));
        }
    }

    // RAII helper for classes that have asynchronous open/close member functions.
    template <typename T>
    struct open_close_guard
    {
        typedef T guarded_t;
        open_close_guard() : guarded() {}
        open_close_guard(T& t) : guarded(&t) { guarded->open().wait(); }
        ~open_close_guard() { if (0 != guarded) guarded->close().wait(); }
        open_close_guard(open_close_guard&& other) : guarded(other.guarded) { other.guarded = 0; }
        open_close_guard& operator=(open_close_guard&& other) { if (this != &other) { if (0 != guarded) guarded->close().wait(); guarded = other.guarded; other.guarded = 0; } return *this; }
        open_close_guard(const open_close_guard&) = delete;
        open_close_guard& operator=(const open_close_guard&) = delete;
        guarded_t* guarded;
    };
}

#endif

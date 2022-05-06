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
    /// <summary>
    ///     Creates a task that completes after a specified amount of time.
    /// </summary>
    /// <param name="milliseconds">
    ///     The number of milliseconds after which the task should complete.
    /// </param>
    /// <param name="token">
    ///     Cancellation token for cancellation of this operation.
    /// </param>
    /// <remarks>
    ///     Because the scheduler is cooperative in nature, the delay before the task completes could be longer than the specified amount of time.
    /// </remarks>
    pplx::task<void> complete_after(unsigned int milliseconds, const pplx::cancellation_token& token = pplx::cancellation_token::none());

    /// <summary>
    ///     Creates a task that completes after a specified amount of time.
    /// </summary>
    /// <param name="duration">
    ///     The amount of time (milliseconds and up) after which the task should complete.
    /// </param>
    /// <param name="token">
    ///     Cancellation token for cancellation of this operation.
    /// </param>
    /// <remarks>
    ///     Because the scheduler is cooperative in nature, the delay before the task completes could be longer than the specified amount of time.
    /// </remarks>
    template <typename Rep, typename Period>
    inline pplx::task<void> complete_after(const std::chrono::duration<Rep, Period>& duration, const pplx::cancellation_token& token = pplx::cancellation_token::none())
    {
        return duration > std::chrono::duration<Rep, Period>::zero()
            ? complete_after((unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), token)
            : pplx::task_from_result();
    }

    /// <summary>
    ///     Creates a task that completes at a specified time.
    /// </summary>
    /// <param name="time">
    ///     The time point at which the task should complete.
    /// </param>
    /// <param name="token">
    ///     Cancellation token for cancellation of this operation.
    /// </param>
    /// <remarks>
    ///     Because the scheduler is cooperative in nature, the time at which the task completes could be after the specified time.
    /// </remarks>
    template <typename Clock, typename Duration = typename Clock::duration>
    inline pplx::task<void> complete_at(const std::chrono::time_point<Clock, Duration>& time, const pplx::cancellation_token& token = pplx::cancellation_token::none())
    {
        return complete_after(time - Clock::now(), token);
    }

    /// <summary>
    ///     Creates a task for an asynchronous do-while loop. Executes a task repeatedly, until the returned condition value becomes false.
    /// </summary>
    /// <param name="create_iteration_task">
    ///     This function should create a task that performs the loop iteration and returns the Boolean value of the loop condition.
    /// </param>
    /// <param name="token">
    ///     Cancellation token for cancellation of the do-while loop.
    /// </param>
    pplx::task<void> do_while(const std::function<pplx::task<bool>()>& create_iteration_task, const pplx::cancellation_token& token = pplx::cancellation_token::none());

    /// <summary>
    ///     Returns true if the task is default constructed.
    /// </summary>
    /// <remarks>
    ///     A default constructed task cannot be used until you assign a valid task to it. Methods such as <c>get</c>, <c>wait</c> or <c>then</c>
    ///     will throw an <see cref="invalid_argument Class">invalid_argument</see> exception when called on a default constructed task.
    /// </remarks>
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
        
    /// <summary>
    ///     Silently 'observe' any exception thrown from a task.
    /// </summary>
    /// <remarks>
    ///     Exceptions that are unobserved when a task is destructed will terminate the process.
    ///     Add this as a continuation to silently swallow all exceptions.
    /// </remarks>
    template <typename ReturnType>
    struct observe_exception
    {
        void operator()(pplx::task<ReturnType> finally) const
        {
            details::wait_nothrow(finally);
        }
    };

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

    /// <summary>
    ///     Silently 'observe' all exceptions thrown from a range of tasks.
    /// </summary>
    /// <remarks>
    ///     Exceptions that are unobserved when a task is destructed will terminate the process.
    ///     Add this as a continuation to silently swallow all exceptions.
    /// </remarks>
    template <typename ReturnType>
    struct observe_exceptions
    {
        template <typename InputRange, typename X = pplx::details::disable_if_same_or_derived<observe_exceptions, InputRange>>
        explicit observe_exceptions(InputRange&& tasks) : tasks(tasks.begin(), tasks.end()) {}

        template <typename InputIterator>
        observe_exceptions(InputIterator&& first, InputIterator&& last) : tasks(first, last) {}

        void operator()(pplx::task<ReturnType> finally) const
        {
            for (auto& task : tasks) details::wait_nothrow(task);
            details::wait_nothrow(finally);
        }

    private:
        std::vector<pplx::task<ReturnType>> tasks;
    };

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

    /// <summary>
    ///     RAII helper for classes that have asynchronous open/close member functions.
    /// </summary>
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

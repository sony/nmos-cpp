#ifndef PPLX_PPLX_UTILS_H
#define PPLX_PPLX_UTILS_H

#include <chrono>
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
    /// <remarks>
    ///     Because the scheduler is cooperative in nature, the delay before the task completes could be longer than the specified amount of time.
    /// </remarks>
    pplx::task<void> complete_after(unsigned int milliseconds);

    /// <summary>
    ///     Creates a task that completes after a specified amount of time.
    /// </summary>
    /// <param name="duration">
    ///     The amount of time (milliseconds and up) after which the task should complete.
    /// </param>
    /// <remarks>
    ///     Because the scheduler is cooperative in nature, the delay before the task completes could be longer than the specified amount of time.
    /// </remarks>
    template <typename Rep, typename Period>
    inline pplx::task<void> complete_after(const std::chrono::duration<Rep, Period>& duration)
    {
        return complete_after((unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }
}

#endif

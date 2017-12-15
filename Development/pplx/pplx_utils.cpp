#include "pplx/pplx_utils.h"

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) && !CPPREST_FORCE_PPLX

#include <agents.h>

namespace Concurrency // since namespace pplx = Concurrency
{
    pplx::task<void> complete_after(unsigned int milliseconds)
    {
        // just yielding the current thread for the duration by pplx::wait(milliseconds) probably isn't good enough, so instead
        // construct a task that completes after an asynchronous wait on a timer

        task_completion_event<void> tce;

        // namespace concurrency = Concurrency too...
        auto timer = std::make_shared<concurrency::timer<int>>(milliseconds, 0);
        auto callback = std::make_shared<concurrency::call<int>>([tce](int)
        {
            tce.set();
        });
        timer->link_target(callback.get());
        timer->start();

        // capture the timer and callback so they don't get deleted too soon
        return pplx::create_task(tce).then([timer, callback]{});
    }
}

#else

#include <boost/asio/basic_waitable_timer.hpp>
#include "pplx/threadpool.h"

namespace pplx
{
    pplx::task<void> complete_after(unsigned int milliseconds)
    {
        // construct a task that completes after an asynchronous wait on a timer
        pplx::task_completion_event<void> tce;

        typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> steady_timer;
        // when the current <see cref="set_ambient_scheduler Function">ambient scheduler</see> has been changed from the default
        // using the shared threadpool perhaps isn't appropriate, but given the scheduler_interface, the alternative is unclear...
        auto timer = std::make_shared<steady_timer>(crossplat::threadpool::shared_instance().service());
        timer->expires_from_now(std::chrono::duration_cast<steady_timer::duration>(std::chrono::milliseconds(milliseconds)));
        timer->async_wait([tce](const boost::system::error_code& ec)
        {
            if (ec == boost::asio::error::operation_aborted)
            {
                pplx::cancel_current_task();
            }
            else
            {
                tce.set();
            }
        });

        // capture the timer so it doesn't get deleted too soon
        return pplx::create_task(tce).then([timer]{});
    }
}

#endif

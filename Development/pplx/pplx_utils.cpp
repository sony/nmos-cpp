#include "pplx/pplx_utils.h"

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) && !CPPREST_FORCE_PPLX

#include <agents.h>

namespace Concurrency // since namespace pplx = Concurrency
{
    pplx::task<void> complete_after(unsigned int milliseconds, const pplx::cancellation_token& token)
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

        auto result = pplx::create_task(tce, token);

        // when the token is canceled, cancel the timer
        if (token.is_cancelable())
        {
            auto registration = token.register_callback([timer, tce]
            {
                timer->stop();
                // calling tce.set_exception(pplx::task_canceled()) does not have the right effect, it results in a call
                // to wait on the task throwing rather than returning pplx::canceled
                if (!tce._IsTriggered())
                {
                    tce._Cancel();
                }
            });

            result.then([token, registration](pplx::task<void>)
            {
                token.deregister_callback(registration);
            });
        }

        // capture the timer and callback so they definitely don't get deleted too soon
        result.then([timer, callback]{});

        return result;
    }
}

#else

#include <boost/asio/basic_waitable_timer.hpp>
#include "pplx/threadpool.h"

namespace pplx
{
    pplx::task<void> complete_after(unsigned int milliseconds, const pplx::cancellation_token& token)
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
                // calling tce.set_exception(pplx::task_canceled()) does not have the right effect, it results in a call
                // to wait on the task throwing rather than returning pplx::canceled
                if (!tce._IsTriggered())
                {
                    tce._Cancel();
                }
            }
            else
            {
                tce.set();
            }
        });

        auto result = pplx::create_task(tce, token);

        // when the token is canceled, cancel the timer
        if (token.is_cancelable())
        {
            auto registration = token.register_callback([timer]
            {
                boost::system::error_code ignored;
                timer->cancel(ignored);
            });

            result.then([token, registration](pplx::task<void>)
            {
                token.deregister_callback(registration);
            });
        }

        // capture the timer so it definitely doesn't get deleted too soon
        result.then([timer]{});

        return result;
    }
}

#endif

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) && !CPPREST_FORCE_PPLX
namespace Concurrency // since namespace pplx = Concurrency
#else
namespace pplx
#endif
{
    namespace details
    {
        void propagate_exception(pplx::task_completion_event<void> event, const pplx::task<void>& task)
        {
            task.then([event](pplx::task<void> completed_or_canceled)
            {
                try
                {
                    completed_or_canceled.get();
                }
                catch (const pplx::task_canceled&)
                {
                    if (!event._IsTriggered())
                    {
                        event._Cancel();
                    }
                }
                catch (...)
                {
                    if (!event._IsTriggered())
                    {
                        event.set_exception(std::current_exception());
                    }
                }
            });
        }

        void do_while_iteration(pplx::task_completion_event<void> event, const std::function<pplx::task<bool>()>& create_iteration_task, const pplx::cancellation_token& token)
        {
            propagate_exception(event, create_iteration_task().then([event, create_iteration_task, token](pplx::task<bool> condition)
            {
                if (condition.get())
                {
                    do_while_iteration(event, create_iteration_task, token);
                }
                else
                {
                    event.set();
                }
           }, token));
        }
    }

    pplx::task<void> do_while(const std::function<pplx::task<bool>()>& create_iteration_task, const pplx::cancellation_token& token)
    {
        pplx::task_completion_event<void> event;

        // create the result task before starting the 'loop', as a workaround for a bug in pplx::task_completion_event::_RegisterTask
        // that a task created from an event cancelled without a user exception (using _Cancel as above) isn't marked as cancelled
        auto result = pplx::create_task(event, token);

        details::propagate_exception(event, pplx::create_task([event, create_iteration_task, token]
        {
            details::do_while_iteration(event, create_iteration_task, token);
        }, token));

        return result;
    }
}

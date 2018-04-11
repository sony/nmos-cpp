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
                tce.set_exception(pplx::task_canceled());
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
                // perhaps better done directly in the token callback?
                tce.set_exception(pplx::task_canceled());
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

namespace pplx
{
    namespace details
    {
        void do_while_iteration(pplx::task_completion_event<void> event, const std::function<pplx::task<bool>()>& create_iteration_task, const pplx::cancellation_token& token)
        {
            create_iteration_task().then([=](pplx::task<bool> condition)
            {
                try
                {
                    if (condition.get())
                    {
                        do_while_iteration(event, create_iteration_task, token);
                    }
                    else
                    {
                        event.set();
                    }
                }
                catch (...)
                {
                    event.set_exception(std::current_exception());
                }
            }, token);
        }
    }

    pplx::task<void> do_while(const std::function<pplx::task<bool>()>& create_iteration_task, const pplx::cancellation_token& token)
    {
        pplx::task_completion_event<void> event;

        pplx::create_task([=]
        {
            // this try-catch is for exceptions from the first call to create_iteration_task as opposed to the task it creates
            try
            {
                details::do_while_iteration(event, create_iteration_task, token);
            }
            catch (...)
            {
                event.set_exception(std::current_exception());
            }
        }, token);

        return pplx::create_task(event, token);
    }
}

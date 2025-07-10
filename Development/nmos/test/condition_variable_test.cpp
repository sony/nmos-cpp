// The first "test" is of course whether the header compiles standalone
#include <iostream>
#include "nmos/mutex.h"
#include "nmos/tai.h"
#include "nmos/thread_utils.h" // for wait_until

#include "bst/test/test.h"

namespace
{
    struct test_model
    {
        // mutex to be used to protect the members of the model from simultaneous access by multiple threads
        mutable nmos::mutex mutex;

        // condition to be used to wait for, and notify other threads about, changes to any member of the model
        // including the shutdown flag
        mutable nmos::condition_variable condition;

        bool shutdown{ false };

        nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
        nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }
        void notify() const { return condition.notify_all(); }
    };

    test_model model;

    void lock_then_unlock()
    {
        try
        {
            auto lock = model.write_lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        catch (...)
        {
            // ignore lock failure
        }
        std::cerr << "*";
    }

    void wait()
    {
        int waken_up_count{ 0 };

        auto lock = model.write_lock();
        auto& condition = model.condition;
        auto& shutdown = model.shutdown;

        auto earliest_necessary_update = (nmos::tai_clock::time_point::max)();

        for (;;)
        {
            try
            {
                nmos::details::wait_until(condition, lock, earliest_necessary_update, [&] { return shutdown; });

                ++waken_up_count;
            }
            catch (const std::exception& e)
            {
                std::stringstream ss;
                ss << "wait_until exception: " << e.what() << std::endl;
                std::cerr << ss.str();
            }

            if (shutdown) break;
        }

        BST_REQUIRE_EQUAL(1, waken_up_count);
    }

    void shutdown(const std::chrono::seconds& pause_for)
    {
        for (;;)
        {
            std::this_thread::sleep_for(pause_for);

            try
            {
                auto lock = model.write_lock();
                model.shutdown = true;
                std::cerr << "shutdown\n";
                break;
            }
            catch (const std::exception& e)
            {
                // ignore lock failure
                std::stringstream ss;
                ss << "shutdown: " << e.what() << std::endl;
                std::cerr << ss.str();
            }
        }

        BST_CHECK(true);

        model.notify();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testConditionVariableWait)
{
    for (int i = 0; i < 100; ++i)
    {
        {
            std::stringstream ss;
            ss << "Iteration: " << i << std::endl;
            std::cerr << ss.str();
        }

        const int max_threads{ 500 };

        // start a wait thread
        std::thread wait_thread(wait);

        // start a large number of lock_then_unlock threads to exhaust the lock limit
        std::vector<std::thread> threads;
        for (auto idx = 0; idx < max_threads; idx++)
        {
            threads.push_back(std::thread(lock_then_unlock));
        }

        // send a lot of notifications to wake up the wait thread
        // to wake the wait thread when lock limit is exhausted
        for (auto idx = 0; idx < 100; idx++)
        {
            model.notify();
        }

        // start the thread to pause 1 second before signal a shutdown
        std::thread shutdown_thread(shutdown, std::chrono::seconds{ 1 });

        shutdown_thread.join();
        wait_thread.join();

        for (auto idx = 0; idx < max_threads; idx++)
        {
            threads[idx].join();
        }

        std::cerr << "\n";

        BST_CHECK(true);
    }
}
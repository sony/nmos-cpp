#include "nmos/server.h"
#include "nmos/model.h"

#include "boost/range/adaptors.hpp"

namespace nmos
{
    server::server(nmos::base_model& model)
        : model(model)
    {}

    namespace details
    {
        void wait_nothrow(pplx::task<void> t) { try { t.wait(); } catch (...) {} }
    }

    pplx::task<void> server::open()
    {
        return pplx::create_task([&]
        {
            start_threads();
            return open_listeners();
        }).then([&](pplx::task<void> finally)
        {
            try
            {
                return finally.get();
            }
            catch (...)
            {
                stop_threads();
                throw;
            }
        });
    }

    pplx::task<void> server::close()
    {
        return close_listeners().then([&](pplx::task<void> finally)
        {
            stop_threads();
            return finally;
        });
    }

    void server::start_threads()
    {
        // Start up threads

        for (auto& thread_function : thread_functions)
        {
            // hm, this is intended to be one-shot, so could move thread_function into the thread?
            threads.push_back(std::thread(thread_function));
        }
    }

    pplx::task<void> server::open_listeners()
    {
        return pplx::create_task([&]
        {
            // Open the API ports

            std::vector<pplx::task<void>> tasks;

            for (auto& http_listener : http_listeners)
            {
                if (0 <= http_listener.uri().port()) tasks.push_back(http_listener.open());
            }

            for (auto& ws_listener : ws_listeners)
            {
                if (0 <= ws_listener.uri().port()) tasks.push_back(ws_listener.open());
            }

            return pplx::when_all(tasks.begin(), tasks.end()).then([tasks](pplx::task<void> finally)
            {
                for (auto& task : tasks) details::wait_nothrow(task);
                finally.wait();
            });
        });
    }

    pplx::task<void> server::close_listeners()
    {
        return pplx::create_task([&]
        {
            // Close the API ports

            std::vector<pplx::task<void>> tasks;

            for (auto& http_listener : http_listeners)
            {
                if (0 <= http_listener.uri().port()) tasks.push_back(http_listener.close());
            }
            for (auto& ws_listener : ws_listeners)
            {
                if (0 <= ws_listener.uri().port()) tasks.push_back(ws_listener.close());
            }


            // Call range::when_all to filter default constructed tasks which may occur when already closed http_listeners are closed again.
            // See open issues microsoft/cpprestsdk#1700 and microsoft/cpprestsdk#1701. 
            // TODO: Use standard pplx::when_all if cpprest issues are resolved and released.
            return pplx::range::when_all( tasks | boost::adaptors::transformed( []( pplx::task<void> task )
            {
                try { task.is_done(); return task; }
                catch( const pplx::invalid_operation& ) { return pplx::task_from_result(); }
            } ) ).then( [tasks]( pplx::task<void> finally )
            {
                for( auto task : tasks ) details::wait_nothrow( task );
                finally.wait();
            } );
        });
    }

    void server::stop_threads()
    {
        // Signal shutdown
        model.controlled_shutdown();

        // Join all threads

        for (auto& thread : threads)
        {
            if (thread.joinable())
            {
                // hm, ignoring possibility of exceptions?
                thread.join();
            }
        }

        threads.clear();
    }
}

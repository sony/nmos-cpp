#include "nmos/server.h"
#include "nmos/model.h"

namespace nmos
{
    server::server(nmos::base_model& model)
        : model(model)
        , ocsp_settings(std::make_shared<nmos::experimental::ocsp_settings>())
    {}

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

            return pplx::ranges::when_all(tasks).then(pplx::observe_exceptions(tasks));
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

            return pplx::ranges::when_all(tasks).then(pplx::observe_exceptions(tasks));
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

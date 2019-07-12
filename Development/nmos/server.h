#ifndef NMOS_SERVER_H
#define NMOS_SERVER_H

#include "cpprest/http_listener.h"
#include "nmos/model.h"
#include "nmos/websockets.h"

namespace nmos
{
    typedef std::pair<utility::string_t, int> host_port;

    // A server consists of the APIs and behaviours required to implement one or more NMOS Specifications
    // on top of an NMOS data model
    struct server
    {
        // server is constructed with a model only so that it can signal controlled shutdown on close
        server(nmos::base_model& model) : model(model) {}

        nmos::base_model& model;

        // HTTP APIs
        std::map<host_port, web::http::experimental::listener::api_router> api_routers;
        std::vector<web::http::experimental::listener::http_listener> http_listeners;

        // WebSocket APIs
        std::map<host_port, std::pair<web::websockets::experimental::listener::websocket_listener_handlers, nmos::websockets>> ws_handlers;
        std::vector<web::websockets::experimental::listener::websocket_listener> ws_listeners;

        // Server behaviours
        std::vector<std::function<void()>> thread_functions;

        pplx::task<void> open();
        pplx::task<void> close();

    private:
        // Threads are started and stopped (created and joined) on open and close respectively
        std::vector<std::thread> threads;

        static void wait_nothrow(pplx::task<void> t) { try { t.wait(); } catch (...) {} }

        void start_threads();
        void stop_threads();

        pplx::task<void> open_listeners();
        pplx::task<void> close_listeners();
    };

    inline pplx::task<void> server::open()
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

    inline pplx::task<void> server::close()
    {
        return close_listeners().then([&](pplx::task<void> finally)
        {
            stop_threads();
            return finally;
        });
    }

    inline void server::start_threads()
    {
        // Start up threads

        for (auto& thread_function : thread_functions)
        {
            // hm, or std::move(thread_function), making this clearly one-shot?
            threads.push_back(std::thread(thread_function));
        }
    }

    inline pplx::task<void> server::open_listeners()
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
                for (auto& task : tasks) wait_nothrow(task);
                finally.wait();
            });
        });
    }

    inline pplx::task<void> server::close_listeners()
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

            return pplx::when_all(tasks.begin(), tasks.end()).then([tasks](pplx::task<void> finally)
            {
                for (auto& task : tasks) wait_nothrow(task);
                finally.wait();
            });
        });
    }

    inline void server::stop_threads()
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

    // RAII helper for server sessions
    typedef pplx::open_close_guard<server> server_guard;
}

#endif

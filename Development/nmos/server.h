#ifndef NMOS_SERVER_H
#define NMOS_SERVER_H

#include <thread>
#include "cpprest/api_router.h"
#include "cpprest/http_listener.h"
#include "nmos/websockets.h"

namespace nmos
{
    struct base_model;

    typedef std::pair<utility::string_t, int> host_port;

    // A server consists of the APIs and behaviours required to implement one or more NMOS Specifications
    // on top of an NMOS data model
    struct server
    {
        // server is constructed with a model only so that it can signal controlled shutdown on close
        server(nmos::base_model& model);

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

        void start_threads();
        void stop_threads();

        pplx::task<void> open_listeners();
        pplx::task<void> close_listeners();
    };

    // RAII helper for server sessions
    typedef pplx::open_close_guard<server> server_guard;
}

#endif

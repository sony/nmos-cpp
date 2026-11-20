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
        // each port may be shared by multiple APIs, each identified by a path prefix (e.g. "/x-nmos/events/")
        // in order to maintain a separate set of connections (nmos::websockets) per API, even when a port is shared
        typedef std::map<utility::string_t, std::pair<web::websockets::experimental::listener::websocket_listener_handlers, nmos::websockets>> ws_api_handlers;
        std::map<host_port, ws_api_handlers> ws_handlers;
        std::deque<web::websockets::experimental::listener::websocket_listener_handlers> combined_ws_handlers; // for APIs sharing a port, the handlers are combined into one set of handlers
        std::map<host_port, web::websockets::experimental::listener::websocket_listener> ws_listeners;

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

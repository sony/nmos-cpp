#ifndef CPPREST_WS_LISTENER_H
#define CPPREST_WS_LISTENER_H

#include <functional>
#include <memory>

#include "pplx/pplxtasks.h"
#include "cpprest/logging_utils.h" // for web::logging::experimental::callback_function
#include "cpprest/ws_client.h" // for web::websockets::client::websocket_outgoing_message and web::websockets::client::websocket_exception

// websocket_listener is an experimental server-side implementation of WebSockets
namespace web
{
    namespace websockets
    {
        namespace experimental
        {
            namespace listener
            {
                // websocket server implementation
                namespace details
                {
                    class websocket_listener_impl;
                }

                // ultimately, these classes would seem to belong in web::websockets, but until that time...
                using web::websockets::client::websocket_exception;
                using web::websockets::client::websocket_outgoing_message;

                // opaque websocket connection id
                // usable as a key in ordered associated containers (but not in unordered ones since although owner-based equivalence
                // could easily be added, a constant hash, or any representation, can't - see e.g. http://stackoverflow.com/a/30922307)
                class connection_id
                {
                public:
                    connection_id() {} // = default
                    friend bool operator<(const connection_id& lhs, const connection_id& rhs) { return std::owner_less<std::weak_ptr<void>>{}(lhs.hdl, rhs.hdl); }

                private:
                    connection_id(const std::weak_ptr<void>& hdl) : hdl(hdl) {}
                    std::weak_ptr<void> hdl;
                    friend class details::websocket_listener_impl;
                };

                inline bool operator>=(const connection_id& lhs, const connection_id& rhs) { return !(lhs < rhs); }
                inline bool operator> (const connection_id& lhs, const connection_id& rhs) { return  (rhs < lhs); }
                inline bool operator<=(const connection_id& lhs, const connection_id& rhs) { return !(rhs < lhs); }

                // a validate handler gets the resource path and returns a flag indicating whether to accept the connection or not
                // the default validate handler accepts all connections
                typedef std::function<bool(const utility::string_t&)> validate_handler;
                // an open handler gets the resource path and the connection id
                typedef std::function<void(const utility::string_t&, const connection_id&)> open_handler;
                // a close handler gets the resource path and the connection id
                typedef std::function<void(const utility::string_t&, const connection_id&)> close_handler;

                class websocket_listener
                {
                public:
                    explicit websocket_listener(int port = 80, web::logging::experimental::callback_function log = {});
                    ~websocket_listener();

                    void set_validate_handler(validate_handler handler);
                    void set_open_handler(open_handler handler);
                    void set_close_handler(close_handler handler);

                    pplx::task<void> open();
                    pplx::task<void> close();

                    pplx::task<void> send(const connection_id& connection, websocket_outgoing_message message);
                    //pplx::task<websocket_incoming_message> receive(const connection_id& connection);
                    //or void set_message_handler(const connection_id& connection, message_handler handler);

                    websocket_listener(websocket_listener&& other);
                    websocket_listener& operator=(websocket_listener&& other);

                private:
                    websocket_listener(const websocket_listener& other);
                    websocket_listener& operator=(const websocket_listener& other);

                    std::unique_ptr<details::websocket_listener_impl> impl;
                    int port;
                };

                // RAII helper for websocket_listener sessions (could be extracted to another header)
                struct websocket_listener_guard
                {
                    websocket_listener_guard(web::websockets::experimental::listener::websocket_listener& listener) : listener(listener) { listener.open().wait(); }
                    ~websocket_listener_guard() { listener.close().wait(); }
                    web::websockets::experimental::listener::websocket_listener& listener;
                };
            }
        }
    }
}

#endif

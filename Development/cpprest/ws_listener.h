#ifndef CPPREST_WS_LISTENER_H
#define CPPREST_WS_LISTENER_H

#if !defined(CPPREST_EXCLUDE_WEBSOCKETS)

#include <functional>
#include <memory>

#if !defined(_WIN32) || !defined(__cplusplus_winrt)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif
#include "boost/asio/ssl.hpp"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif

#include "cpprest/logging_utils.h" // for web::logging::experimental::log_handler
#include "cpprest/ws_msg.h"
#include "cpprest/ws_client.h" // only for websocket_exception which ought to be in cpprest/ws_msg.h

// websocket_listener is an experimental server-side implementation of WebSockets
namespace web
{
    namespace websockets
    {
        // ultimately, it would be nice to switch the namespaces of the definitions and the using-declarations... 
        using web::websockets::client::websocket_exception;
        using web::websockets::client::websocket_incoming_message;
        using web::websockets::client::websocket_outgoing_message;

        namespace experimental
        {
            namespace listener
            {
                // websocket server implementation
                namespace details
                {
                    class websocket_listener_impl;
                }

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

#if !defined(_WIN32) || !defined(__cplusplus_winrt)
                // ultimately, this would seem to belong in web, in order to also be adopted by web::http, but until that time...
                typedef std::function<void(boost::asio::ssl::context&)> ssl_context_callback;
#endif

                class websocket_listener_config
                {
                public:
                    websocket_listener_config() : m_backlog(0) {}

                    const web::logging::experimental::log_handler& get_log_callback() const
                    {
                        return m_log_callback;
                    }

                    void set_log_callback(const web::logging::experimental::log_handler& log_callback)
                    {
                        m_log_callback = log_callback;
                    }

                    int backlog() const
                    {
                        return m_backlog;
                    }

                    void set_backlog(int backlog)
                    {
                        m_backlog = backlog;
                    }

#if !defined(_WIN32) || !defined(__cplusplus_winrt)
                    const ssl_context_callback& get_ssl_context_callback() const
                    {
                        return m_ssl_context_callback;
                    }

                    void set_ssl_context_callback(const ssl_context_callback& ssl_context_callback)
                    {
                        m_ssl_context_callback = ssl_context_callback;
                    }
#endif

                private:
                    web::logging::experimental::log_handler m_log_callback;
                    int m_backlog;
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
                    ssl_context_callback m_ssl_context_callback;
#endif
                };

                class websocket_listener
                {
                public:
                    explicit websocket_listener(web::uri address, websocket_listener_config = {});
                    websocket_listener();
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

                    const web::uri& uri() const;

                    const websocket_listener_config& configuration() const;

                private:
                    websocket_listener(const websocket_listener& other);
                    websocket_listener& operator=(const websocket_listener& other);

                    std::unique_ptr<details::websocket_listener_impl> impl;
                };
            }
        }
    }
}

#endif

#endif

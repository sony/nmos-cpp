#include "cpprest/ws_listener.h"

#include <mutex>
#include <set>
#include "detail/pragma_warnings.h"
#include "detail/private_access.h"

// use of websocketpp should be an implementation detail, i.e. not in a public header file
PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_CONDITIONAL_EXPRESSION_IS_CONSTANT
#ifdef _MSC_VER
__pragma(warning(disable:4267)) // e.g. conversion from 'size_t' to 'uint32_t', possible loss of data
__pragma(warning(disable:4701)) // e.g. potentially uninitialized local variable 'key' used
#endif
// it seems impossible to get rid of the dependencies of Boost.Asio on Boost.System and Boost.Date_Time
#define BOOST_ASIO_DISABLE_BOOST_REGEX
#include "websocketpp/config/boost_config.hpp"
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/logger/levels.hpp"
#include "websocketpp/server.hpp"
PRAGMA_WARNING_POP

#include "cpprest/asyncrt_utils.h" // for utility::conversions

// websocket_listener is an experimental server-side implementation of WebSockets
namespace web
{
    namespace websockets
    {
        namespace experimental
        {
            namespace listener
            {
                namespace details
                {
                    // mapping from websocketpp error levels and access log channels to a single logging level value
                    // may need adjustment in the future
                    web::logging::experimental::level level_from_elevel(websocketpp::log::level channel)
                    {
                        using namespace web::logging::experimental;
                        switch (channel)
                        {
                        case websocketpp::log::elevel::devel:   return levels::devel;
                        case websocketpp::log::elevel::library: return levels::verbose;
                        case websocketpp::log::elevel::info:    return levels::info;
                        case websocketpp::log::elevel::warn:    return levels::warning;
                        case websocketpp::log::elevel::rerror:  return levels::error;
                        case websocketpp::log::elevel::fatal:   return levels::severe;
                        default:                                return levels::error;
                        }
                    }

                    web::logging::experimental::level level_from_alevel(websocketpp::log::level channel)
                    {
                        using namespace web::logging::experimental;
                        switch (channel)
                        {
                        case websocketpp::log::alevel::connect:         return levels::info;
                        case websocketpp::log::alevel::disconnect:      return levels::info;
                        case websocketpp::log::alevel::control:         return levels::verbose;
                        case websocketpp::log::alevel::frame_header:    return levels::verbose;
                        case websocketpp::log::alevel::frame_payload:   return levels::devel;
                        case websocketpp::log::alevel::debug_handshake: return levels::devel;
                        case websocketpp::log::alevel::debug_close:     return levels::devel;
                        case websocketpp::log::alevel::devel:           return levels::devel;
                        case websocketpp::log::alevel::app:             return levels::info;
                        case websocketpp::log::alevel::fail:            return levels::warning;
                        default:                                        return levels::verbose;
                        }
                    }

                    // implementation of websocketpp logger concept
                    // currently ignores any channels configuration and adapts access/error logging to single callback
                    class websocketpp_log
                    {
                    public:
                        websocketpp_log() : hint(websocketpp::log::channel_type_hint::access) {}
                        explicit websocketpp_log(websocketpp::log::channel_type_hint::value hint) : hint(hint) {}
                        websocketpp_log(websocketpp::log::level, websocketpp::log::channel_type_hint::value hint) : hint(hint) {}

                        void set_channels(websocketpp::log::level) {}
                        void clear_channels(websocketpp::log::level) {}
                        void write(websocketpp::log::level channel, const std::string& message);
                        void write(websocketpp::log::level channel, char const *message) { write(channel, std::string{ message }); }
                        bool static_test(websocketpp::log::level) const { return true; }
                        bool dynamic_test(websocketpp::log::level) { return true; }

                        // only custom interface
                        void set_callback(web::logging::experimental::callback_function callback) { this->callback = callback; }

                    private:
                        websocketpp::log::channel_type_hint::value hint;
                        web::logging::experimental::callback_function callback;
                    };
                    
                    void websocketpp_log::write(websocketpp::log::level channel, const std::string& message)
                    {
                        if (callback)
                        {
                            if (websocketpp::log::channel_type_hint::access == hint)
                            {
                                callback(level_from_alevel(channel), message, websocketpp::log::alevel::channel_name(channel));
                            }
                            else // if (websocketpp::log::channel_type_hint::error == hint)
                            {
                                callback(level_from_elevel(channel), message, {});
                            }
                        }
                    }

                    // websocketpp config that just overrides the two log types
                    struct websocketpp_config : websocketpp::config::asio
                    {
                        typedef websocketpp_config type;
                        typedef websocketpp::config::asio base;

                        typedef base::concurrency_type concurrency_type;

                        typedef base::request_type request_type;
                        typedef base::response_type response_type;

                        typedef base::message_type message_type;
                        typedef base::con_msg_manager_type con_msg_manager_type;
                        typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

                        typedef websocketpp_log alog_type;
                        typedef websocketpp_log elog_type;

                        typedef base::rng_type rng_type;

                        struct transport_config : base::transport_config
                        {
                            typedef base::transport_config::concurrency_type concurrency_type;
                            typedef type::alog_type alog_type;
                            typedef type::elog_type elog_type;
                            typedef base::transport_config::request_type request_type;
                            typedef base::transport_config::response_type response_type;
                            typedef base::transport_config::socket_type socket_type;
                        };

                        typedef websocketpp::transport::asio::endpoint<transport_config> transport_type;

                        // reminder: these compile-time filters can be adjusted
                        static const websocketpp::log::level elog_level = base::elog_level;
                        static const websocketpp::log::level alog_level = base::alog_level;
                    };

                    struct websocket_outgoing_message_body { typedef concurrency::streams::streambuf<uint8_t>(websocket_outgoing_message::*type); };

                    concurrency::streams::streambuf<uint8_t>& get_message_body(websocket_outgoing_message& message)
                    {
                        return message.*detail::stowed<websocket_outgoing_message_body>::value;
                    }

                    static std::string build_error_msg(const std::error_code& ec, const std::string& location)
                    {
                        std::stringstream ss;
                        ss.imbue(std::locale::classic());
                        ss << location
                            << ": " << ec.value()
                            << ": " << ec.message();
                        return ss.str();
                    }

                    // websocket server implementation that uses websocketpp
                    class websocket_listener_impl
                    {
                    public:
                        explicit websocket_listener_impl(web::logging::experimental::callback_function callback)
                        {
                            // since we cannot set the callback function before the server constructor we can't get log message from that
                            server.get_alog().set_callback(callback);
                            server.get_elog().set_callback(callback);
                        }

                        ~websocket_listener_impl()
                        {
                            try
                            {
                                close().wait();
                            }
                            catch (...)
                            {
                            }
                        }

                        void set_validate_handler(validate_handler handler)
                        {
                            user_validate = handler;
                        }

                        void set_open_handler(open_handler handler)
                        {
                            user_open = handler;
                        }

                        void set_close_handler(close_handler handler)
                        {
                            user_close = handler;
                        }

                        pplx::task<void> open(int port)
                        {
                            try
                            {
                                server.init_asio();
                                server.start_perpetual();
                                // hmm, is one thread enough?
                                thread = std::thread(&server_t::run, &server);

                                using websocketpp::lib::bind;
                                using websocketpp::lib::placeholders::_1;

                                server.set_validate_handler(bind(&websocket_listener_impl::handle_validate, this, _1));
                                server.set_open_handler(bind(&websocket_listener_impl::handle_open, this, _1));
                                server.set_close_handler(bind(&websocket_listener_impl::handle_close, this, _1));

                                websocketpp::lib::error_code ec;
                                server.listen((uint16_t)port, ec);
                                // if the error is "Underlying Transport Error" (pass_through), this might be a platform that doesn't support IPv6
                                // (and we can't detect boost::asio::error::address_family_not_supported directly)
                                if (websocketpp::transport::asio::error::make_error_code(websocketpp::transport::asio::error::pass_through) == ec)
                                {
                                    // retry, limiting ourselves to IPv4
                                    server.get_alog().write(websocketpp::log::alevel::app, "listening with IPv6 failed; retrying with IPv4 only");
                                    server.listen(boost::asio::ip::tcp::v4(), (uint16_t)port);
                                }
                                // otherwise treat any error as usual
                                else if (ec)
                                {
                                    throw websocketpp::exception(ec);
                                }

                                server.start_accept();
                            }
                            catch (const websocketpp::exception& e)
                            {
                                return pplx::task_from_exception<void>(websocket_exception(e.code(), build_error_msg(e.code(), "open")));
                            }
                            catch (const std::system_error& e)
                            {
                                return pplx::task_from_exception<void>(websocket_exception(e.code(), build_error_msg(e.code(), "open")));
                            }

                            return pplx::task_from_result();
                        }

                        pplx::task<void> close()
                        {
                            try
                            {
                                // This seems to be the recommended technique for stopping a websocketpp::server
                                // although it results in quite a bit of noise in the error and access logs, for example:
                                // info: Error getting remote endpoint: system:10009 (The file handle supplied is not valid)
                                // warning: WebSocket Connection Unknown - "" - 0 websocketpp:26 Operation canceled
                                // info: asio async_shutdown error: system:10009 (The file handle supplied is not valid)
                                // info: handle_accept error: Operation canceled
                                // info: Stopping acceptance of new connections because the underlying transport is no longer listening.
                                // The author of WebSocket++ says:
                                // "Is the error actually causing problems? or are you just worried about the messages in the log?
                                // Unless there is specific problem I believe that this is the currently expected behavior."
                                // See https://github.com/zaphoyd/websocketpp/issues/556#issuecomment-221563661

                                if (server.is_listening())
                                {
                                    server.stop_listening();
                                }

                                {
                                    std::lock_guard<std::mutex> lock(mutex);
                                    for (auto& hdl : connections)
                                    {
                                        server.close(hdl, websocketpp::close::status::going_away, "server going down");
                                    }
                                    connections.clear();
                                }

                                server.stop_perpetual();
                            }
                            catch (const websocketpp::exception& e)
                            {
                                if (thread.joinable())
                                {
                                    thread.join();
                                }
                                return pplx::task_from_exception<void>(websocket_exception(e.code(), build_error_msg(e.code(), "close")));
                            }

                            if (thread.joinable())
                            {
                                thread.join();
                            }
                            return pplx::task_from_result();
                        }

                        pplx::task<void> send(const connection_id& connection, websocket_outgoing_message message)
                        {
                            // right now, this implementation is only tested to work with simple UTF-8 text messages
                            // message.set_utf8_message("body");

                            auto body = get_message_body(message);
                            uint8_t* ptr = nullptr;
                            size_t count = 0;
                            bool acquired = body.acquire(ptr, count);
                            if (!acquired || nullptr == ptr || 0 == count)
                            {
                                return pplx::task_from_exception<void>(websocket_exception("Invalid message body"));
                            }

                            try
                            {
                                // send will throw if the connection_hdl isn't valid
                                server.send(hdl_from_id(connection), ptr, count, websocketpp::frame::opcode::text);
                            }
                            catch (const websocketpp::exception& e)
                            {
                                body.release(ptr, count);
                                return pplx::task_from_exception<void>(websocket_exception(e.code(), build_error_msg(e.code(), "send")));
                            }

                            body.release(ptr, count);
                            return pplx::task_from_result();
                        }

                    private:
                        typedef websocketpp::server<websocketpp_config> server_t;
                        typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_t;

                        utility::string_t resource_from_hdl(websocketpp::connection_hdl hdl)
                        {
                            return utility::conversions::to_string_t(server.get_con_from_hdl(hdl)->get_resource());
                        }

                        static connection_id id_from_hdl(websocketpp::connection_hdl hdl)
                        {
                            return{ hdl };
                        }

                        static websocketpp::connection_hdl hdl_from_id(connection_id connection)
                        {
                            return connection.hdl;
                        }

                        bool handle_validate(websocketpp::connection_hdl hdl)
                        {
                            return user_validate ? user_validate(resource_from_hdl(hdl)) : true;
                        }

                        void handle_open(websocketpp::connection_hdl hdl)
                        {
                            {
                                std::lock_guard<std::mutex> lock(mutex);
                                connections.insert(hdl);
                            }

                            if (user_open)
                            {
                                user_open(resource_from_hdl(hdl), id_from_hdl(hdl));
                            }
                        }

                        void handle_close(websocketpp::connection_hdl hdl)
                        {
                            if (user_close)
                            {
                                user_close(resource_from_hdl(hdl), id_from_hdl(hdl));
                            }

                            {
                                std::lock_guard<std::mutex> lock(mutex);
                                connections.erase(hdl);
                            }
                        }

                        std::thread thread;
                        server_t server;
                        connections_t connections;
                        std::mutex mutex;

                        validate_handler user_validate;
                        open_handler user_open;
                        close_handler user_close;
                    };
                }

                websocket_listener::websocket_listener(int port, web::logging::experimental::callback_function log)
                    : impl(std::unique_ptr<details::websocket_listener_impl>(new details::websocket_listener_impl(log)))
                    , port(port)
                {
                }

                websocket_listener::websocket_listener(websocket_listener&& other)
                    : impl(std::move(other.impl))
                    , port(other.port)
                {
                }

                websocket_listener& websocket_listener::operator=(websocket_listener&& other)
                {
                    if (this != &other)
                    {
                        impl = std::move(other.impl);
                        port = other.port;
                    }
                    return *this;
                }

                websocket_listener::~websocket_listener()
                {
                }

                void websocket_listener::set_validate_handler(validate_handler handler)
                {
                    impl->set_validate_handler(handler);
                }

                void websocket_listener::set_open_handler(open_handler handler)
                {
                    impl->set_open_handler(handler);
                }

                void websocket_listener::set_close_handler(close_handler handler)
                {
                    impl->set_close_handler(handler);
                }

                pplx::task<void> websocket_listener::open()
                {
                    return impl->open(port);
                }

                pplx::task<void> websocket_listener::close()
                {
                    return impl->close();
                }

                pplx::task<void> websocket_listener::send(const connection_id& connection, websocket_outgoing_message message)
                {
                    return impl->send(connection, message);
                }
            }
        }
    }
}

// Sigh. "An explicit instantiation shall appear in an enclosing namespace of its template."
template struct detail::stow_private<web::websockets::experimental::listener::details::websocket_outgoing_message_body, &web::websockets::experimental::listener::websocket_outgoing_message::m_body>;

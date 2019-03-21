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
                        void set_log_handler(web::logging::experimental::log_handler log) { this->log = log; }

                    private:
                        websocketpp::log::channel_type_hint::value hint;
                        web::logging::experimental::log_handler log;
                    };

                    void websocketpp_log::write(websocketpp::log::level channel, const std::string& message)
                    {
                        if (log)
                        {
                            if (websocketpp::log::channel_type_hint::access == hint)
                            {
                                log(level_from_alevel(channel), message, websocketpp::log::alevel::channel_name(channel));
                            }
                            else // if (websocketpp::log::channel_type_hint::error == hint)
                            {
                                log(level_from_elevel(channel), message, {});
                            }
                        }
                    }

                    // websocketpp config that just overrides the two log types
                    template <typename Base>
                    struct websocketpp_config : Base
                    {
                        typedef websocketpp_config type;
                        typedef Base base;

                        typedef typename base::concurrency_type concurrency_type;

                        typedef typename base::request_type request_type;
                        typedef typename base::response_type response_type;

                        typedef typename base::message_type message_type;
                        typedef typename base::con_msg_manager_type con_msg_manager_type;
                        typedef typename base::endpoint_msg_manager_type endpoint_msg_manager_type;

                        typedef websocketpp_log alog_type;
                        typedef websocketpp_log elog_type;

                        typedef typename base::rng_type rng_type;

                        struct transport_config : base::transport_config
                        {
                            typedef typename base::transport_config::concurrency_type concurrency_type;
                            typedef typename type::alog_type alog_type;
                            typedef typename type::elog_type elog_type;
                            typedef typename base::transport_config::request_type request_type;
                            typedef typename base::transport_config::response_type response_type;
                            typedef typename base::transport_config::socket_type socket_type;
                        };

                        typedef websocketpp::transport::asio::endpoint<transport_config> transport_type;

                        // reminder: these compile-time filters can be adjusted
                        static const websocketpp::log::level elog_level = base::elog_level;
                        static const websocketpp::log::level alog_level = base::alog_level;
                    };

                    typedef websocketpp_config<websocketpp::config::asio> ws_config;

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

                    // websocket server implementation base class for type erasure
                    class websocket_listener_impl
                    {
                    public:
                        websocket_listener_impl(web::uri address, websocket_listener_config config)
                            : address(std::move(address))
                            , config(std::move(config))
                        {
                            check_uri(uri());
                        }

                        virtual ~websocket_listener_impl() {}

                        const web::uri& uri() const
                        {
                            return address;
                        }

                        const websocket_listener_config& configuration() const
                        {
                            return config;
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

                        virtual pplx::task<void> open() = 0;
                        virtual pplx::task<void> close() = 0;
                        virtual pplx::task<void> send(const connection_id& connection, websocket_outgoing_message message) = 0;

                    protected:
                        // extend friendship with connection_id to derived classes
                        static connection_id id_from_hdl(std::weak_ptr<void> hdl)
                        {
                            return{ hdl };
                        }
                        static std::weak_ptr<void> hdl_from_id(connection_id connection)
                        {
                            return connection.hdl;
                        }

                        static void check_uri(const web::uri& address)
                        {
                            if (address.scheme() != _XPLATSTR("ws"))
                            {
                                throw std::invalid_argument("URI scheme must be 'ws'");
                            }
                            if (!address.user_info().empty())
                            {
                                throw std::invalid_argument("URI can't contain user information");
                            }
                            if (address.host().empty())
                            {
                                throw std::invalid_argument("URI must contain a host");
                            }
                            if (!address.is_path_empty())
                            {
                                throw std::invalid_argument("URI can't contain a path");
                            }
                            if (!address.query().empty())
                            {
                                throw std::invalid_argument("URI can't contain a query");
                            }
                            if (!address.fragment().empty())
                            {
                                throw std::invalid_argument("URI can't contain a fragment");
                            }
                        }

                        static int get_port(const web::uri& address)
                        {
                            return address.port() > 0 ? address.port() : 80;
                        }

                        web::uri address;
                        websocket_listener_config config;

                        validate_handler user_validate;
                        open_handler user_open;
                        close_handler user_close;
                    };

                    // websocket server implementation that uses websocketpp
                    template <typename WsppConfig>
                    class websocket_listener_wspp : public websocket_listener_impl
                    {
                    public:
                        explicit websocket_listener_wspp(web::uri address, websocket_listener_config config)
                            : websocket_listener_impl(std::move(address), std::move(config))
                        {
                            // since we cannot set the callback function before the server constructor we can't get log message from that
                            server.get_alog().set_log_handler(configuration().get_log_callback());
                            server.get_elog().set_log_handler(configuration().get_log_callback());
                        }

                        pplx::task<void> open()
                        {
                            const std::string host = utility::conversions::to_utf8string(address.host());

                            const int port = get_port(address);
                            // this check could be done earlier, in check_uri
                            if ((uint16_t)port != port) return pplx::task_from_exception<void>(websocket_exception("Invalid port"));
                            // hmm, port 0 could be used to indicate a free port should be assigned rather than meaning default; one might want to know which one...
                            // since WebSocket++ 0.7.0, server.get_local_endpoint().port() can be used when the server is listening
                            const std::string service = std::to_string(port);

                            try
                            {
                                server.init_asio();
                                server.start_perpetual();
                                // hmm, is one thread enough?
                                thread = std::thread(&server_t::run, &server);

                                using websocketpp::lib::bind;
                                using websocketpp::lib::placeholders::_1;

                                server.set_validate_handler(bind(&websocket_listener_wspp::handle_validate, this, _1));
                                server.set_open_handler(bind(&websocket_listener_wspp::handle_open, this, _1));
                                server.set_close_handler(bind(&websocket_listener_wspp::handle_close, this, _1));

                                server.set_listen_backlog(configuration().backlog());
                                websocketpp::lib::asio::ip::tcp::resolver resolver(server.get_io_service());
                                websocketpp::lib::asio::ip::tcp::resolver::query query(host, service);
                                websocketpp::lib::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
                                websocketpp::lib::error_code ec;
                                server.listen(endpoint, ec);
                                // if the error is "Underlying Transport Error" (pass_through), this might be a platform that doesn't support IPv6
                                // (and depending on configuration, one can't detect boost::asio::error::address_family_not_supported directly)
                                // since WebSocket++ 0.8.0, the error category and code used in this case seem to have changed
                                if (make_error_code(boost::asio::error::address_family_not_supported) == ec
                                    || make_error_code(websocketpp::transport::asio::error::pass_through) == ec
                                    || make_error_code(websocketpp::transport::error::pass_through) == ec)
                                {
                                    // retry, limiting ourselves to IPv4
                                    server.get_alog().write(websocketpp::log::alevel::app, "listening with IPv6 failed; retrying with IPv4 only");
                                    websocketpp::lib::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), host, service);
                                    websocketpp::lib::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
                                    server.listen(endpoint);
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
                            }
                            catch (const websocketpp::exception& e)
                            {
                                server.stop_perpetual();
                                if (thread.joinable())
                                {
                                    thread.join();
                                }
                                return pplx::task_from_exception<void>(websocket_exception(e.code(), build_error_msg(e.code(), "close")));
                            }

                            server.stop_perpetual();
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
                        typedef websocketpp::server<WsppConfig> server_t;
                        typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_t;

                        utility::string_t resource_from_hdl(websocketpp::connection_hdl hdl)
                        {
                            return utility::conversions::to_string_t(server.get_con_from_hdl(hdl)->get_resource());
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
                    };
                }

                websocket_listener::websocket_listener()
                {
                }

                websocket_listener::websocket_listener(web::uri address, websocket_listener_config config)
                    : impl(new details::websocket_listener_wspp<details::ws_config>(std::move(address), std::move(config)))
                {
                }

                websocket_listener::websocket_listener(websocket_listener&& other)
                    : impl(std::move(other.impl))
                {
                }

                websocket_listener& websocket_listener::operator=(websocket_listener&& other)
                {
                    if (this != &other)
                    {
                        impl = std::move(other.impl);
                    }
                    return *this;
                }

                websocket_listener::~websocket_listener()
                {
                    if (impl)
                    {
                        try
                        {
                            close().wait();
                        }
                        catch (...)
                        {
                        }
                    }
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
                    return impl->open();
                }

                pplx::task<void> websocket_listener::close()
                {
                    return impl->close();
                }

                pplx::task<void> websocket_listener::send(const connection_id& connection, websocket_outgoing_message message)
                {
                    return impl->send(connection, message);
                }

                const web::uri& websocket_listener::uri() const
                {
                    return impl->uri();
                }

                const websocket_listener_config& websocket_listener::configuration() const
                {
                    return impl->configuration();
                }
            }
        }
    }
}

// Sigh. "An explicit instantiation shall appear in an enclosing namespace of its template."
template struct detail::stow_private<web::websockets::experimental::listener::details::websocket_outgoing_message_body, &web::websockets::websocket_outgoing_message::m_body>;

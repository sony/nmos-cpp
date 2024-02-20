#include "nmos/client_utils.h"

// cf. preprocessor conditions in nmos::details::make_client_ssl_context_callback and nmos::details::make_client_nativehandle_options
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
#if defined(__linux__)
#include <boost/asio/ip/tcp.hpp>
#endif
#include "boost/asio/ssl/set_cipher_list.hpp"
#include "cpprest/host_utils.h"
#endif
#include "cpprest/basic_utils.h"
#include "cpprest/details/system_error.h"
#include "cpprest/http_utils.h"
#include "cpprest/response_type.h"
#include "cpprest/ws_client.h"
#include "nmos/certificate_settings.h"
#include "nmos/json_fields.h"
#include "nmos/slog.h"
#include "nmos/ssl_context_options.h"

// Utility types, constants and functions for implementing NMOS REST API clients
namespace nmos
{
    web::uri proxy_uri(const nmos::settings& settings)
    {
        const auto& proxy_address = nmos::experimental::fields::proxy_address(settings);
        if (proxy_address.empty()) return{};
        return web::uri_builder()
            .set_scheme(U("http"))
            .set_host(proxy_address)
            .set_port(nmos::experimental::fields::proxy_port(settings))
            .to_uri();
    }

    namespace details
    {
// cf. preprocessor conditions in nmos::make_http_client_config and nmos::make_websocket_client_config
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        template <typename ExceptionType>
        inline std::function<void(boost::asio::ssl::context&)> make_client_ssl_context_callback(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
        {
            if (!load_ca_certificates)
            {
                load_ca_certificates = make_load_ca_certificates_handler(settings, gate);
            }

            return [load_ca_certificates](boost::asio::ssl::context& ctx)
            {
                try
                {
                    ctx.set_options(nmos::details::ssl_context_options);

                    const auto cacerts = utility::us2s(load_ca_certificates());
                    ctx.add_certificate_authority(boost::asio::buffer(cacerts.data(), cacerts.size()));

                    set_cipher_list(ctx, nmos::details::ssl_cipher_list);
                }
                catch (const boost::system::system_error& e)
                {
                    throw web::details::from_boost_system_system_error<ExceptionType>(e);
                }
            };
        }
#endif

#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        // bind socket to a specific network interface
        // for now, only supporting Linux because SO_BINDTODEVICE is not defined on Windows and Mac
        inline void bind_to_device(const utility::string_t& interface_name, bool secure, void* native_handle)
        {
#if defined(__linux__)
            int socket_fd;
            // hmm, frustrating that native_handle type has been erased so we need secure flag
            if (secure)
            {
                auto socket = (boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>*)native_handle;
                if (!socket->lowest_layer().is_open())
                {
                    // for now, limited to IPv4
                    socket->lowest_layer().open(boost::asio::ip::tcp::v4());
                }
                socket_fd = socket->lowest_layer().native_handle();
            }
            else
            {
                auto socket = (boost::asio::ip::tcp::socket*)native_handle;
                if (!socket->is_open())
                {
                    // for now, limited to IPv4
                    socket->open(boost::asio::ip::tcp::v4());
                }
                socket_fd = socket->lowest_layer().native_handle();
            }
            const auto interface_name_ = utility::us2s(interface_name);
            if (0 != setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, interface_name_.data(), interface_name_.length()))
            {
                char error[1024];
                throw std::runtime_error(strerror_r(errno, error, sizeof(error)));
            }
#else
            throw std::logic_error("unsupported");
#endif
        }

        inline std::function<void(web::http::client::native_handle)> make_client_nativehandle_options(bool secure, const utility::string_t& client_address, slog::base_gate& gate)
        {
            if (client_address.empty()) return {};
            // get the associated network interface name from IP address
            const auto interface_name = web::hosts::experimental::get_interface_name(client_address);
            if (interface_name.empty())
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "No network interface found for " << client_address << " to bind for the HTTP client connection";
                return {};
            }

            return [interface_name, secure, &gate](web::http::client::native_handle native_handle)
            {
                try
                {
                    bind_to_device(interface_name, secure, native_handle);
                }
                catch (const std::exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unable to bind HTTP client connection to " << interface_name << ": " << e.what();
                }
            };
        }

#ifdef CPPRESTSDK_ENABLE_BIND_WEBSOCKET_CLIENT
        // The current version of the C++ REST SDK 2.10.19 does not provide the callback to enable the custom websocket setting
        inline std::function<void(web::websockets::client::native_handle)> make_ws_client_nativehandle_options(bool secure, const utility::string_t& client_address, slog::base_gate& gate)
        {
            if (client_address.empty()) return {};
            // get the associated network interface name from IP address
            const auto interface_name = web::hosts::experimental::get_interface_name(client_address);
            if (interface_name.empty())
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "No network interface found for " << client_address << " to bind for the websocket client connection";
                return {};
            }

            return [interface_name, secure, &gate](web::websockets::client::native_handle native_handle)
            {
                try
                {
                    bind_to_device(interface_name, secure, native_handle);
                }
                catch (const std::exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unable to bind websocket client connection to " << interface_name << ": " << e.what();
                }
            };
        }
#endif

#endif
    }

    // construct client config based on specified secure flag and settings, e.g. using the specified proxy and OCSP config
    // with the remaining options defaulted, e.g. request timeout
    web::http::client::http_client_config make_http_client_config(bool secure, const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
    {
        web::http::client::http_client_config config;
        const auto proxy = proxy_uri(settings);
        if (!proxy.is_empty()) config.set_proxy(proxy);
        if (secure) config.set_validate_certificates(nmos::experimental::fields::validate_certificates(settings));
#if !defined(_WIN32) && !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        if (secure) config.set_ssl_context_callback(details::make_client_ssl_context_callback<web::http::http_exception>(settings, load_ca_certificates, gate));
        config.set_nativehandle_options(details::make_client_nativehandle_options(secure, nmos::experimental::fields::client_address(settings), gate));
#endif

        return config;
    }

    // construct client config based on settings, e.g. using the specified proxy and OCSP config
    // with the remaining options defaulted, e.g. request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
    {
        return make_http_client_config(nmos::experimental::fields::client_secure(settings), settings, load_ca_certificates, gate);
    }

    // construct oauth2 config with the bearer token
    web::http::oauth2::experimental::oauth2_config make_oauth2_config(const web::http::oauth2::experimental::oauth2_token& bearer_token)
    {
        web::http::oauth2::experimental::oauth2_config config(U(""), U(""), U(""), U(""), U(""), U(""));
        config.set_token(bearer_token);

        return config;
    }

    // construct client config including OAuth 2.0 config based on settings, e.g. using the specified proxy and OCSP config
    // with the remaining options defaulted, e.g. authorization request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, const web::http::oauth2::experimental::oauth2_token& bearer_token, slog::base_gate& gate)
    {
        auto config = make_http_client_config(settings, load_ca_certificates, gate);

        if (bearer_token.is_valid_access_token())
        {
            config.set_oauth2(make_oauth2_config(bearer_token));
        }

        return config;
    }

    // construct client config including OAuth 2.0 config based on settings, e.g. using the specified proxy and OCSP config
    // with the remaining options defaulted, e.g. authorization request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, nmos::experimental::get_authorization_bearer_token_handler get_authorization_bearer_token, slog::base_gate& gate)
    {
        web::http::oauth2::experimental::oauth2_token bearer_token;

        if (get_authorization_bearer_token)
        {
            bearer_token = get_authorization_bearer_token();
        }

        return make_http_client_config(settings, load_ca_certificates, bearer_token, gate);
    }

    // construct client config based on specified secure flag and settings, e.g. using the specified proxy
    // with the remaining options defaulted
    web::websockets::client::websocket_client_config make_websocket_client_config(bool secure, const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
    {
        web::websockets::client::websocket_client_config config;
        const auto proxy = proxy_uri(settings);
        if (!proxy.is_empty()) config.set_proxy(proxy);
        if (secure) config.set_validate_certificates(nmos::experimental::fields::validate_certificates(settings));
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
        if (secure) config.set_ssl_context_callback(details::make_client_ssl_context_callback<web::websockets::client::websocket_exception>(settings, load_ca_certificates, gate));
#ifdef CPPRESTSDK_ENABLE_BIND_WEBSOCKET_CLIENT
        config.set_nativehandle_options(details::make_ws_client_nativehandle_options(secure, nmos::experimental::fields::client_address(settings), gate));
#endif
#endif

        return config;
    }

    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted
    web::websockets::client::websocket_client_config make_websocket_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
    {
        return make_websocket_client_config(nmos::experimental::fields::client_secure(settings), settings, load_ca_certificates, gate);
    }

    // construct client config based on settings and access token, e.g. using the specified proxy
    // with the remaining options defaulted
    web::websockets::client::websocket_client_config make_websocket_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, nmos::experimental::get_authorization_bearer_token_handler get_authorization_bearer_token, slog::base_gate& gate)
    {
        auto config = make_websocket_client_config(settings, std::move(load_ca_certificates), gate);

        if (get_authorization_bearer_token)
        {
            const auto bearer_token = get_authorization_bearer_token();
            config.headers().add(web::http::header_names::authorization, U("Bearer ") + bearer_token.access_token());
        }

        return config;
    }

    namespace details
    {
        // make a client for the specified base_uri and config, with Host header sneakily stashed in user info
        std::unique_ptr<web::http::client::http_client> make_http_client(const web::uri& base_uri, const web::http::client::http_client_config& client_config)
        {
            // unstash the Host header
            // cf. nmos::details::resolve_service
            const auto& host_name = base_uri.user_info();
            std::unique_ptr<web::http::client::http_client> client(new web::http::client::http_client(web::uri_builder(base_uri).set_user_info({}).to_uri(), client_config));
            if (!host_name.empty())
            {
                client->add_handler([host_name](web::http::http_request request, std::shared_ptr<web::http::http_pipeline_stage> next_stage) -> pplx::task<web::http::http_response>
                {
                    request.headers().add(web::http::header_names::host, host_name);
                    return next_stage->propagate(request);
                });
            }
            return client;
        }
    }

    // make a request with logging
    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, web::http::http_request request, slog::base_gate& gate, const pplx::cancellation_token& token)
    {
        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Sending request";
        // see https://developer.mozilla.org/en-US/docs/Web/API/Resource_Timing_API#Resource_loading_timestamps
        const auto start_time = std::chrono::system_clock::now();
        return client.request(request, token).then([start_time, &gate](web::http::http_response res)
        {
            const auto response_start = std::chrono::system_clock::now();
            const auto request_dur = std::chrono::duration_cast<std::chrono::microseconds>(response_start - start_time).count() / 1000.0;

            // see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Server-Timing
            const auto server_timing = res.headers().find(web::http::experimental::header_names::server_timing);

            if (res.headers().end() != server_timing)
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Response received after " << request_dur << "ms (server-timing: " << server_timing->second << ")";
            }
            else
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Response received after " << request_dur << "ms";
            }

            return res;
        });
    }

    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, const web::http::method& mtd, slog::base_gate& gate, const pplx::cancellation_token& token)
    {
        web::http::http_request msg(mtd);
        return api_request(client, msg, gate, token);
    }

    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, const web::http::method& mtd, const utility::string_t& path_query_fragment, slog::base_gate& gate, const pplx::cancellation_token& token)
    {
        web::http::http_request msg(mtd);
        msg.set_request_uri(path_query_fragment);
        return api_request(client, msg, gate, token);
    }

    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, const web::http::method& mtd, const utility::string_t& path_query_fragment, const web::json::value& body_data, slog::base_gate& gate, const pplx::cancellation_token& token)
    {
        web::http::http_request msg(mtd);
        msg.set_request_uri(path_query_fragment);
        msg.set_body(body_data);
        return api_request(client, msg, gate, token);
    }
}

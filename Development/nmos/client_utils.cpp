#include "nmos/client_utils.h"

#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
#include "boost/asio/ssl/set_cipher_list.hpp"
#endif
#include "cpprest/basic_utils.h"
#include "cpprest/details/system_error.h"
#include "cpprest/http_client.h"
#include "cpprest/ws_client.h"
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
#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        template <typename ExceptionType>
        inline std::function<void(boost::asio::ssl::context&)> make_client_ssl_context_callback(const nmos::settings& settings)
        {
            const auto ca_certificate_file = utility::us2s(nmos::experimental::fields::ca_certificate_file(settings));
            return [ca_certificate_file](boost::asio::ssl::context& ctx)
            {
                try
                {
                    ctx.set_options(nmos::details::ssl_context_options);
                    ctx.load_verify_file(ca_certificate_file);
                    set_cipher_list(ctx, nmos::details::ssl_cipher_list);
                }
                catch (const boost::system::system_error& e)
                {
                    throw web::details::from_boost_system_system_error<ExceptionType>(e);
                }
            };
        }
#endif
    }

    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted, e.g. request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings)
    {
        web::http::client::http_client_config config;
        const auto proxy = proxy_uri(settings);
        if (!proxy.is_empty()) config.set_proxy(proxy);
        config.set_validate_certificates(nmos::experimental::fields::validate_certificates(settings));
#if !defined(_WIN32) && !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        config.set_ssl_context_callback(details::make_client_ssl_context_callback<web::http::http_exception>(settings));
#endif

        return config;
    }

    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted
    web::websockets::client::websocket_client_config make_websocket_client_config(const nmos::settings& settings)
    {
        web::websockets::client::websocket_client_config config;
        const auto proxy = proxy_uri(settings);
        if (!proxy.is_empty()) config.set_proxy(proxy);
        config.set_validate_certificates(nmos::experimental::fields::validate_certificates(settings));
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
        config.set_ssl_context_callback(details::make_client_ssl_context_callback<web::websockets::client::websocket_exception>(settings));
#endif

        return config;
    }
}

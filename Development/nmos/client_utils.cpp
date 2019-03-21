#include "nmos/client_utils.h"

#include "cpprest/basic_utils.h"
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

    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted, e.g. request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings)
    {
        web::http::client::http_client_config config;
        const auto proxy = proxy_uri(settings);
        if (!proxy.is_empty()) config.set_proxy(proxy);
#if !defined(_WIN32) && !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        const auto ca_certificate_filename = utility::us2s(nmos::experimental::fields::ca_certificate_file(settings));
        config.set_ssl_context_callback([ca_certificate_filename](boost::asio::ssl::context& ctx)
        {
            try
            {
                ctx.set_options(details::ssl_context_options);
                ctx.load_verify_file(ca_certificate_filename);
            }
            catch (const boost::system::system_error& e)
            {
                throw web::http::http_exception(e.code(), e.what());
            }
        });
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
#if !defined(_WIN32) || !defined(__cplusplus_winrt)
        const auto ca_certificate_filename = utility::us2s(nmos::experimental::fields::ca_certificate_file(settings));
        config.set_ssl_context_callback([ca_certificate_filename](boost::asio::ssl::context& ctx)
        {
            try
            {
                ctx.set_options(details::ssl_context_options);
                ctx.load_verify_file(ca_certificate_filename);
            }
            catch (const boost::system::system_error& e)
            {
                throw web::websockets::client::websocket_exception(e.code(), e.what());
            }
        });
#endif

        return config;
    }
}

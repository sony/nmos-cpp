#include "nmos/client_utils.h"

#include "cpprest/basic_utils.h"
#include "cpprest/http_client.h"
#include "cpprest/ws_client.h"

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

#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
    namespace details
    {
        // AMWA BCP-003-01 states that "implementations SHOULD support TLS 1.3 and SHALL support TLS 1.2."
        // "Implementations SHALL NOT use TLS 1.0 or 1.1. These are deprecated."
        // "Implementations SHALL NOT use SSL. Although the SSL protocol has previously,
        // been used to secure HTTP traffic no version of SSL is now considered secure."
        // See https://github.com/AMWA-TV/nmos-api-security/blob/master/best-practice-secure-comms.md#tls
        const auto ssl_context_options =
            ( boost::asio::ssl::context::no_sslv2
            | boost::asio::ssl::context::no_sslv3
            | boost::asio::ssl::context::no_tlsv1
// TLS v1.2 support was added in Boost 1.54.0
// but the option to disable TLS v1.1 didn't arrive until Boost 1.58.0
#if BOOST_VERSION >= 105800
            | boost::asio::ssl::context::no_tlsv1_1
#endif
            );
    }
#endif

    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted, e.g. request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings)
    {
        web::http::client::http_client_config config;
        const auto proxy = proxy_uri(settings);
        if (!proxy.is_empty()) config.set_proxy(proxy);
#if !defined(_WIN32) && !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        const auto ca_certificate_filename = utility::us2s(nmos::experimental::fields::ca_certificate_file(settings));
        config.set_ssl_context_callback([ca_certificate_filename](boost::asio::ssl::context &ctx)
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
        config.set_ssl_context_callback([ca_certificate_filename](boost::asio::ssl::context &ctx)
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

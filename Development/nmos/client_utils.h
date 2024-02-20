#ifndef NMOS_CLIENT_UTILS_H
#define NMOS_CLIENT_UTILS_H

#include "cpprest/http_client.h" // for http_client, http_client_config, http_response, etc.
#include "nmos/authorization_handlers.h"
#include "nmos/certificate_handlers.h"
#include "nmos/settings.h"

namespace web { namespace websockets { namespace client { class websocket_client_config; } } }
namespace slog { class base_gate; }

// Utility types, constants and functions for implementing NMOS REST API clients
namespace nmos
{
    // construct client config based on specified secure flag and settings, e.g. using the specified proxy and OCSP config
    // with the remaining options defaulted, e.g. request timeout
    web::http::client::http_client_config make_http_client_config(bool secure, const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);
    // construct client config based on settings, e.g. using the specified proxy and OCSP config
    // with the remaining options defaulted, e.g. request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);
    // construct client config including OAuth 2.0 config based on settings, e.g. using the specified proxy and OCSP config
    // with the remaining options defaulted, e.g. authorization request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, const web::http::oauth2::experimental::oauth2_token& bearer_token, slog::base_gate& gate);
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, nmos::experimental::get_authorization_bearer_token_handler get_authorization_bearer_token, slog::base_gate& gate);

    // construct client config based on specified secure flag and settings, e.g. using the specified proxy
    // with the remaining options defaulted
    web::websockets::client::websocket_client_config make_websocket_client_config(bool secure, const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);
    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted
    web::websockets::client::websocket_client_config make_websocket_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, nmos::experimental::get_authorization_bearer_token_handler get_authorization_bearer_token, slog::base_gate& gate);
    web::websockets::client::websocket_client_config make_websocket_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);

    namespace details
    {
        // make a client for the specified base_uri and config, with Host header sneakily stashed in user info
        std::unique_ptr<web::http::client::http_client> make_http_client(const web::uri& base_uri_with_host_name_in_user_info, const web::http::client::http_client_config& client_config);
    }

    // make an API request with logging
    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, web::http::http_request request, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, const web::http::method& mtd, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, const web::http::method& mtd, const utility::string_t& path_query_fragment, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, const web::http::method& mtd, const utility::string_t& path_query_fragment, const web::json::value& body_data, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
}

#endif

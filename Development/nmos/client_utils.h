#ifndef NMOS_CLIENT_UTILS_H
#define NMOS_CLIENT_UTILS_H

#include "cpprest/http_client.h" // for http_client, http_client_config, http_response, etc.
#include "nmos/certificate_handlers.h"
#include "nmos/settings.h"

namespace web { namespace websockets { namespace client { class websocket_client_config; } } }
namespace slog { class base_gate; }

// Utility types, constants and functions for implementing NMOS REST API clients
namespace nmos
{
    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted, e.g. request timeout
    web::http::client::http_client_config make_http_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);

    // construct client config based on settings, e.g. using the specified proxy
    // with the remaining options defaulted
    web::websockets::client::websocket_client_config make_websocket_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);

    // make an API request with logging
    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, web::http::http_request request, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, const web::http::method& mtd, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, const web::http::method& mtd, const utility::string_t& path_query_fragment, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
    pplx::task<web::http::http_response> api_request(web::http::client::http_client client, const web::http::method& mtd, const utility::string_t& path_query_fragment, const web::json::value& body_data, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
}

#endif

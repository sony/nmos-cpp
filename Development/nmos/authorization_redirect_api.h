#ifndef NMOS_AUTHORIZATION_REDIRECT_API_H
#define NMOS_AUTHORIZATION_REDIRECT_API_H

#include "cpprest/api_router.h"
#include "nmos/certificate_handlers.h"

namespace slog
{
    class base_gate;
}

// This is an experimental extension to support authorization code via a REST API
namespace nmos
{
    namespace experimental
    {
        struct authorization_state;
    }

    struct base_model;

    namespace experimental
    {
        web::http::experimental::listener::api_router make_authorization_redirect_api(nmos::base_model& model, authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::load_rsa_private_keys_handler load_rsa_private_keys, slog::base_gate& gate);
    }
}

#endif

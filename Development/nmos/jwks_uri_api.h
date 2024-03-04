#ifndef NMOS_JWK_URI_API_H
#define NMOS_JWK_URI_API_H

#include "cpprest/api_router.h"
#include "nmos/certificate_handlers.h"

namespace slog
{
    class base_gate;
}

// This is an experimental extension to support authorization code via a REST API
namespace nmos
{
    struct base_model;

    namespace experimental
    {
        web::http::experimental::listener::api_router make_jwk_uri_api(nmos::base_model& model, load_rsa_private_keys_handler load_rsa_private_keys, slog::base_gate& gate);
    }
}

#endif

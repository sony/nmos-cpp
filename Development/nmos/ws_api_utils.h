#ifndef NMOS_WS_API_UTILS_H
#define NMOS_WS_API_UTILS_H

#include <functional>
#include "cpprest/http_msg.h"
#include "nmos/scope.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct base_model;

    namespace experimental
    {
        struct authorization_state;

        typedef std::function<bool(web::http::http_request& request, const nmos::experimental::scope& scope)> ws_validate_authorization_handler;
        ws_validate_authorization_handler make_ws_validate_authorization_handler(nmos::base_model& model, authorization_state& authorization_state, slog::base_gate& gate);
    }
}

#endif

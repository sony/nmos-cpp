#include "nmos/ws_api_utils.h"

#include "cpprest/http_utils.h"
#include "nmos/api_utils.h"
#include "nmos/authorization.h"
#include "nmos/authorization_state.h"
#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        ws_validate_authorization_handler make_ws_validate_authorization_handler(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, slog::base_gate& gate)
        {
            return [&model, &authorization_state, &gate](web::http::http_request& request, const nmos::experimental::scope& scope)
            {
                if (web::http::methods::OPTIONS != request.method() && nmos::experimental::fields::server_authorization(model.settings))
                {
                    const auto& settings = model.settings;

                    authorization_state.write_lock();
                    const auto error = ws_validate_authorization(authorization_state.issuers, request, scope, nmos::get_host_name(settings), authorization_state.token_issuer, gate);
                    if (error)
                    {
                        // set error repsonse
                        auto realm = web::http::get_host_port(request).first;
                        if (realm.empty()) { realm = nmos::get_host(settings); }
                        web::http::http_response res;
                        nmos::experimental::details::set_error_reply(res, realm, error);
                        request.reply(res);

                        // if error was deal to no matching keys, trigger authorization_token_issuer_thread to fetch public keys from the token issuer
                        if (error.value == authorization_error::no_matching_keys)
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Invalid websocket connection to: " << request.request_uri().path() << ": " << error.message;
                            authorization_state.fetch_token_issuer_pubkeys = true;
                            model.notify();
                        }
                        else
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Invalid websocket connection to: " << request.request_uri().path() << ": " << error.message;
                        }
                        return false;
                    }
                }
                return true;
            };
        }
	}
}

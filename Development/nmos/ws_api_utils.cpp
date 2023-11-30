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
        ws_validate_authorization_handler make_ws_validate_authorization_handler(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, validate_authorization_token_handler access_token_validation, slog::base_gate& gate)
        {
            return [&model, &authorization_state, access_token_validation, &gate](web::http::http_request& request, const nmos::experimental::scope& scope)
            {
                if (web::http::methods::OPTIONS != request.method() && nmos::experimental::fields::server_authorization(model.settings))
                {
                    const auto& settings = model.settings;

                    web::uri token_issuer;
                    // note: the ws_validate_authorization returns the token_issuer via function parameter
                    const auto result = ws_validate_authorization(request, scope, nmos::get_host_name(settings), token_issuer, access_token_validation, gate);
                    if (!result)
                    {
                        // set error repsonse
                        auto realm = web::http::get_host_port(request).first;
                        if (realm.empty()) { realm = nmos::get_host(settings); }
                        web::http::http_response res;
                        const auto retry_after = nmos::experimental::fields::service_unavailable_retry_after(settings);
                        nmos::experimental::details::set_error_reply(res, realm, retry_after, result);
                        request.reply(res);

                        // if no matching public keys caused the error, trigger a re-fetch to obtain public keys from the token issuer (authorization_state.token_issuer)
                        if (result.value == authorization_error::no_matching_keys)
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Invalid websocket connection to: " << request.request_uri().path() << ": " << result.message;

                            with_write_lock(authorization_state.mutex, [&authorization_state, token_issuer]
                            {
                                authorization_state.fetch_token_issuer_pubkeys = true;
                                authorization_state.token_issuer = token_issuer;
                            });

                            model.notify();
                        }
                        else
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Invalid websocket connection to: " << request.request_uri().path() << ": " << result.message;
                        }
                        return false;
                    }
                }
                return true;
            };
        }
	}
}

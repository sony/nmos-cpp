#include "nmos/jwks_uri_api.h"

#include "cpprest/response_type.h"
#include "nmos/api_utils.h"
#include "nmos/authorization_utils.h"
#include "nmos/jwk_utils.h"
#include "nmos/model.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_jwk_uri_api(nmos::base_model& model, load_rsa_private_keys_handler load_rsa_private_keys, slog::base_gate& /*gate_*/)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router jwks_api;

            jwks_api.support(U("/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("x-authorization/") }, req, res));
                return pplx::task_from_result(true);
            });

            jwks_api.support(U("/x-authorization/?"), methods::GET, [](http_request req, http_response res, const string_t&, const route_parameters&)
            {
                set_reply(res, status_codes::OK, nmos::make_sub_routes_body({ U("callback/"), U("jwks/") }, req, res));
                return pplx::task_from_result(true);
            });

            jwks_api.support(U("/x-authorization/jwks/?"), methods::GET, [&model, load_rsa_private_keys](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                using web::json::array;

                // hmm, for now new "kid" key ID is used for every request, may be it should only be updated after key has changed

                auto keys = value::array();
                std::vector<string_t> rsa_private_keys;
                with_read_lock(model.mutex, [&model, &rsa_private_keys, load_rsa_private_keys]
                {
                    rsa_private_keys = load_rsa_private_keys();
                });

                for (const auto rsa_private_key : rsa_private_keys)
                {
                    const auto jwk = details::private_key_to_jwk(rsa_private_key, make_id());
                    web::json::push_back(keys, jwk);
                }

                const auto jwks = value_of({
                    { nmos::experimental::fields::keys, keys }
                });

                set_reply(res, status_codes::OK, jwks);
                return pplx::task_from_result(true);
            });

            return jwks_api;
        }
    }
}

#ifndef NMOS_AUTHORIZATION_BEHAVIOUR_H
#define NMOS_AUTHORIZATION_BEHAVIOUR_H

#include <functional>
#include "cpprest/http_client.h"
#include "nmos/authorization_handlers.h"
#include "nmos/certificate_handlers.h"
#include "nmos/mdns.h"
#include "nmos/settings.h" // just a forward declaration of nmos::settings

namespace slog
{
    class base_gate;
}

namespace mdns
{
    class service_discovery;
}

namespace nmos
{
    struct base_model;

    namespace experimental
    {
        struct authorization_state;

        // uses the default DNS-SD implementation
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        void authorization_behaviour_thread(nmos::base_model& model, authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::load_rsa_private_keys_handler load_rsa_private_keys, load_authorization_clients_handler load_authorization_clients, save_authorization_client_handler save_authorization_client, request_authorization_code_handler request_authorization_code, slog::base_gate& gate);

        // uses the specified DNS-SD implementation
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        void authorization_behaviour_thread(nmos::base_model& model, authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::load_rsa_private_keys_handler load_rsa_private_keys, load_authorization_clients_handler load_authorization_clients, save_authorization_client_handler save_authorization_client, request_authorization_code_handler request_authorization_code, mdns::service_discovery& discovery, slog::base_gate& gate);

        // callbacks from this function are called with the model locked, and may read or write directly to the model and the authorization settings
        void authorization_token_issuer_thread(nmos::base_model& model, authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);

        namespace details
        {
            // services functions which are used by authorization operation
            bool empty_authorization_services(const nmos::settings& settings);
            resolved_service top_authorization_service(const nmos::settings& settings);
            void pop_authorization_service(nmos::settings& settings);
        }
    }
}

#endif

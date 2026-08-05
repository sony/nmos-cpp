#ifndef NMOS_REGISTRY_SERVER_H
#define NMOS_REGISTRY_SERVER_H

#include "nmos/authorization_handlers.h"
#include "nmos/certificate_handlers.h"
#include "nmos/est_certificate_handlers.h"
#include "nmos/ocsp_response_handler.h"
#include "nmos/ws_api_utils.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct registry_model;

    struct server;

    namespace experimental
    {
        struct log_model;

        // a type to simplify passing around the application callbacks necessary to integrate the device-specific
        // underlying implementation into the server instance for the NMOS Registry
        struct registry_implementation
        {
            registry_implementation(nmos::load_server_certificates_handler load_server_certificates, nmos::load_dh_param_handler load_dh_param, nmos::load_ca_certificates_handler load_ca_certificates, nmos::load_client_certificate_handler load_client_certificate, nmos::experimental::receive_ca_certificate_handler ca_certificate_received, nmos::experimental::receive_server_certificate_handler rsa_server_certificate_received, nmos::experimental::receive_server_certificate_handler ecdsa_server_certificate_received, nmos::ocsp_response_handler get_ocsp_response, validate_authorization_handler validate_authorization, ws_validate_authorization_handler ws_validate_authorization)
                : load_server_certificates(std::move(load_server_certificates))
                , load_dh_param(std::move(load_dh_param))
                , load_ca_certificates(std::move(load_ca_certificates))
                , load_client_certificate(std::move(load_client_certificate))
                , ca_certificate_received(std::move(ca_certificate_received))
                , rsa_server_certificate_received(std::move(rsa_server_certificate_received))
                , ecdsa_server_certificate_received(std::move(ecdsa_server_certificate_received))
                , get_ocsp_response(std::move(get_ocsp_response))
                , validate_authorization(std::move(validate_authorization))
                , ws_validate_authorization(std::move(ws_validate_authorization))
            {}

            // use the default constructor and chaining member functions for fluent initialization
            // (by itself, the default constructor does not construct a valid instance)
            registry_implementation()
            {}

            registry_implementation& on_load_server_certificates(nmos::load_server_certificates_handler load_server_certificates) { this->load_server_certificates = std::move(load_server_certificates); return *this; }
            registry_implementation& on_load_dh_param(nmos::load_dh_param_handler load_dh_param) { this->load_dh_param = std::move(load_dh_param); return *this; }
            registry_implementation& on_load_ca_certificates(nmos::load_ca_certificates_handler load_ca_certificates) { this->load_ca_certificates = std::move(load_ca_certificates); return *this; }
            registry_implementation& on_get_ocsp_response(nmos::ocsp_response_handler get_ocsp_response) { this->get_ocsp_response = std::move(get_ocsp_response); return *this; }
            registry_implementation& on_validate_authorization(validate_authorization_handler validate_authorization) { this->validate_authorization = std::move(validate_authorization); return* this; }
            registry_implementation& on_ws_validate_authorization(ws_validate_authorization_handler ws_validate_authorization) { this->ws_validate_authorization = std::move(ws_validate_authorization); return *this; }
            registry_implementation& on_load_client_certificate(nmos::load_client_certificate_handler load_client_certificate) { this->load_client_certificate = std::move(load_client_certificate); return *this; }
            registry_implementation& on_ca_certificate_received(nmos::experimental::receive_ca_certificate_handler ca_certificate_received) { this->ca_certificate_received = std::move(ca_certificate_received); return *this; }
            registry_implementation& on_rsa_server_certificate_received(nmos::experimental::receive_server_certificate_handler rsa_server_certificate_received) { this->rsa_server_certificate_received = std::move(rsa_server_certificate_received); return *this; }
            registry_implementation& on_ecdsa_server_certificate_received(nmos::experimental::receive_server_certificate_handler ecdsa_server_certificate_received) { this->ecdsa_server_certificate_received = std::move(ecdsa_server_certificate_received); return *this; }

            // determine if the required callbacks have been specified
            bool valid() const
            {
                return true;
            }

            nmos::load_server_certificates_handler load_server_certificates;
            nmos::load_dh_param_handler load_dh_param;
            nmos::load_ca_certificates_handler load_ca_certificates;
            nmos::load_client_certificate_handler load_client_certificate;

            nmos::experimental::receive_ca_certificate_handler ca_certificate_received;
            nmos::experimental::receive_server_certificate_handler rsa_server_certificate_received;
            nmos::experimental::receive_server_certificate_handler ecdsa_server_certificate_received;

            nmos::ocsp_response_handler get_ocsp_response;

            validate_authorization_handler validate_authorization;
            ws_validate_authorization_handler ws_validate_authorization;
        };

        // Construct a server instance for an NMOS Registry instance, implementing the IS-04 Registration and Query APIs, the Node API, the IS-09 System API, the IS-10 Authorization API
        // and the experimental DNS-SD Browsing API, Logging API and Settings API, according to the specified data models
        nmos::server make_registry_server(nmos::registry_model& registry_model, nmos::experimental::registry_implementation registry_implementation, nmos::experimental::log_model& log_model, slog::base_gate& gate);

        // deprecated
        nmos::server make_registry_server(nmos::registry_model& registry_model, nmos::experimental::log_model& log_model, slog::base_gate& gate);
    }
}

#endif

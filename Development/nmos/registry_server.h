#ifndef NMOS_REGISTRY_SERVER_H
#define NMOS_REGISTRY_SERVER_H

#include "nmos/certificate_handlers.h"

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
            registry_implementation(nmos::load_cert_handler load_rsa, nmos::load_cert_handler load_ecdsa, nmos::load_dh_param_handler load_dh_param, nmos::load_cacert_handler load_cacert)
                : load_rsa(std::move(load_rsa))
                , load_ecdsa(std::move(load_ecdsa))
                , load_dh_param(std::move(load_dh_param))
                , load_cacert(std::move(load_cacert))
            {}

            // use the default constructor and chaining member functions for fluent initialization
            // (by itself, the default constructor does not construct a valid instance)
            registry_implementation()
            {}

            registry_implementation& on_load_rsa(nmos::load_cert_handler load_rsa) { this->load_rsa = std::move(load_rsa); return *this; }
            registry_implementation& on_load_ecdsa(nmos::load_cert_handler load_ecdsa) { this->load_ecdsa = std::move(load_ecdsa); return *this; }
            registry_implementation& on_load_dh_param(nmos::load_dh_param_handler load_dh_param) { this->load_dh_param = std::move(load_dh_param); return *this; }
            registry_implementation& on_load_cacert(nmos::load_cacert_handler load_cacert) { this->load_cacert = std::move(load_cacert); return *this; }

            // determine if the required callbacks have been specified
            bool valid() const
            {
                return true;
            }

            nmos::load_cert_handler load_rsa;
            nmos::load_cert_handler load_ecdsa;
            nmos::load_dh_param_handler load_dh_param;
            nmos::load_cacert_handler load_cacert;
        };

        // Construct a server instance for an NMOS Registry instance, implementing the IS-04 Registration and Query APIs, the Node API, the IS-09 System API, the IS-10 Authorization API
        // and the experimental DNS-SD Browsing API, Logging API and Settings API, according to the specified data models
        nmos::server make_registry_server(nmos::registry_model& registry_model, nmos::experimental::registry_implementation registry_implementation, nmos::experimental::log_model& log_model, slog::base_gate& gate);

        nmos::server make_registry_server(nmos::registry_model& registry_model, nmos::load_cert_handler load_rsa, nmos::load_cert_handler load_ecdsa_cert, nmos::load_dh_param_handler load_dh_param, nmos::load_cacert_handler load_cacert, nmos::experimental::log_model& log_model, slog::base_gate& gate);
    }
}

#endif

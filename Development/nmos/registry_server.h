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
            registry_implementation(nmos::load_tls_handler load_tls, nmos::load_dh_param_handler load_dh_param, nmos::load_cacerts_handler load_cacerts)
                : load_tls(std::move(load_tls))
                , load_dh_param(std::move(load_dh_param))
                , load_cacerts(std::move(load_cacerts))
            {}

            // use the default constructor and chaining member functions for fluent initialization
            // (by itself, the default constructor does not construct a valid instance)
            registry_implementation()
            {}

            registry_implementation& on_load_tls(nmos::load_tls_handler load_tls) { this->load_tls = std::move(load_tls); return *this; }
            registry_implementation& on_load_dh_param(nmos::load_dh_param_handler load_dh_param) { this->load_dh_param = std::move(load_dh_param); return *this; }
            registry_implementation& on_load_cacerts(nmos::load_cacerts_handler load_cacerts) { this->load_cacerts = std::move(load_cacerts); return *this; }

            // determine if the required callbacks have been specified
            bool valid() const
            {
                return true;
            }

            nmos::load_tls_handler load_tls;
            nmos::load_dh_param_handler load_dh_param;
            nmos::load_cacerts_handler load_cacerts;
        };

        // Construct a server instance for an NMOS Registry instance, implementing the IS-04 Registration and Query APIs, the Node API, the IS-09 System API, the IS-10 Authorization API
        // and the experimental DNS-SD Browsing API, Logging API and Settings API, according to the specified data models
        nmos::server make_registry_server(nmos::registry_model& registry_model, nmos::experimental::registry_implementation registry_implementation, nmos::experimental::log_model& log_model, slog::base_gate& gate);

        // deprecated
        nmos::server make_registry_server(nmos::registry_model& registry_model, nmos::experimental::log_model& log_model, slog::base_gate& gate);
    }
}

#endif

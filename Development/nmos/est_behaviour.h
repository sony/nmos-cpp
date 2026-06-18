#ifndef NMOS_EST_BEHAVIOUR_H
#define NMOS_EST_BEHAVIOUR_H

#include "nmos/certificate_handlers.h"
#include "nmos/est_certificate_handlers.h"

namespace slog
{
    class base_gate;
}

namespace mdns
{
    class service_discovery;
}

// EST behaviour including fetch CA certificates, request server certificates and certificate revocation operation
// See https://specs.amwa.tv/is-10/
namespace nmos
{
    struct model;

    namespace experimental
    {
        // uses the default DNS-SD implementation
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        void est_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, load_client_certificate_handler load_client_certificate, receive_ca_certificate_handler ca_certificate_received, receive_server_certificate_handler rsa_server_certificate_received, receive_server_certificate_handler ecdsa_server_certificate_received, slog::base_gate& gate);

        // uses the specified DNS-SD implementation
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        void est_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, load_client_certificate_handler load_client_certificate, receive_ca_certificate_handler ca_certificate_received, receive_server_certificate_handler rsa_server_certificate_received, receive_server_certificate_handler ecdsa_server_certificate_received, mdns::service_discovery& discovery, slog::base_gate& gate);
    }
}

#endif

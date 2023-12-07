#ifndef NMOS_NODE_BEHAVIOUR_H
#define NMOS_NODE_BEHAVIOUR_H

#include <functional>
#include "nmos/authorization_handlers.h"
#include "nmos/certificate_handlers.h"

namespace web
{
    class uri;
}

namespace slog
{
    class base_gate;
}

namespace mdns
{
    class service_advertiser;
    class service_discovery;
}

// Node behaviour including both registered operation and peer to peer operation
// See https://specs.amwa.tv/is-04/releases/v1.2.0/docs/4.1._Behaviour_-_Registration.html
// and https://specs.amwa.tv/is-04/releases/v1.2.0/docs/3.1._Discovery_-_Registered_Operation.html
namespace nmos
{
    struct model;

    // a registration_handler is a notification that the current Registration API has changed
    // registration uri should be like http://api.example.com/x-nmos/registration/{version}
    // or empty if errors have been encountered when interacting with all discoverable Registration APIs
    // this callback should not throw exceptions
    typedef std::function<void(const web::uri& registration_uri)> registration_handler;

    // uses the default DNS-SD implementation
    // callbacks from this function are called with the model locked, and may read or write directly to the model
    void node_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, registration_handler registration_changed, nmos::experimental::get_authorization_bearer_token_handler get_authorization_bearer_token, slog::base_gate& gate);
    void node_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, registration_handler registration_changed, slog::base_gate& gate);

    // uses the specified DNS-SD implementation
    // callbacks from this function are called with the model locked, and may read or write directly to the model
    void node_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, registration_handler registration_changed, nmos::experimental::get_authorization_bearer_token_handler get_authorization_bearer_token, mdns::service_advertiser& advertiser, mdns::service_discovery& discovery, slog::base_gate& gate);
    void node_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, registration_handler registration_changed, mdns::service_advertiser& advertiser, mdns::service_discovery& discovery, slog::base_gate& gate);

    // uses the default DNS-SD implementation
    void node_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);

    // uses the specified DNS-SD implementation
    void node_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, mdns::service_advertiser& advertiser, mdns::service_discovery& discovery, slog::base_gate& gate);
}

#endif
